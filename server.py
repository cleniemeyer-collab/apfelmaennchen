import base64
import selectors
import json
import os
import struct
import subprocess
import threading
import time
import zlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from decimal import Decimal, localcontext
from collections import deque

ROOT = Path(__file__).resolve().parent
MAX_RENDER_JOBS = int(os.environ.get('MAX_RENDER_JOBS', '4'))
if not 1 <= MAX_RENDER_JOBS <= 16:
    raise ValueError('MAX_RENDER_JOBS muss zwischen 1 und 16 liegen.')
SLOT = threading.BoundedSemaphore(MAX_RENDER_JOBS)
WIDTH, HEIGHT = 640, 480
RENDER_TIMEOUT = 300
TIMEOUT_MESSAGE = 'Zeitlimit von 5 Minuten erreicht. Weniger Iterationen wählen.'
BACKEND_LINES = {'backend=gmp', 'backend=opencl-fp32', 'backend=opencl-fp64',
                 'backend=opencl-perturb', 'backend=opencl-perturb-gmp', 'backend=cpu-long-double'}


class RenderScheduler:
    """One work unit at a time; ready jobs receive turns in FIFO order."""
    def __init__(self):
        self.lock = threading.Lock()
        self.waiting = deque()
        self.owner = None

    def request(self, job):
        with self.lock:
            if self.owner is job:
                self.owner = None
            if job in self.waiting:
                raise ValueError('Doppelte Rechenfreigabe angefordert.')
            self.waiting.append(job)

    def acquire(self, job):
        with self.lock:
            if self.owner is None and self.waiting and self.waiting[0] is job:
                self.owner = self.waiting.popleft()
                return True
            return False

    def remove(self, job):
        with self.lock:
            if self.owner is job:
                self.owner = None
            if job in self.waiting:
                self.waiting.remove(job)


SCHEDULER = RenderScheduler()


def render_environment(data):
    env = os.environ.copy()
    if env.get('RENDER_BACKEND') != 'gmp':
        env['RENDER_BACKEND'] = data.get('backend', 'long-double')
    return env


def validate(data):
    if not isinstance(data, dict) or set(data) - {'iterations', 'path', 'backend'}:
        raise ValueError('Ungültiger Auftrag.')
    if data.get('backend', 'long-double') not in ('auto', 'gmp', 'perturb', 'long-double'):
        raise ValueError('Ungültiger Rechenweg.')
    iterations, path = data.get('iterations', 250), data.get('path', [])
    if type(iterations) is not int or not 50 <= iterations <= 100000:
        raise ValueError('Erlaubt sind 50 bis 100000 Iterationen.')
    if not isinstance(path, list) or len(path) > 80:
        raise ValueError('Maximal 80 Zoom-Schritte sind erlaubt.')
    for rect in path:
        if not isinstance(rect, list) or len(rect) != 3 or any(type(v) is not int for v in rect):
            raise ValueError('Ungültiges Auswahlrechteck.')
        x, y, size = rect
        if not 1000 <= size <= 1000000 or not 0 <= x <= 1000000-size or not 0 <= y <= 1000000-size:
            raise ValueError('Auswahl außerhalb des Bildes.')
    return [str(iterations)] + [str(v) for rect in path for v in rect]


def make_config(data):
    if not isinstance(data, dict) or set(data) != {'view', 'palette'}:
        raise ValueError('Ansicht und Palette fehlen in der Konfiguration.')
    view, palette = data['view'], data['palette']
    validate(view)
    if not isinstance(palette, dict) or set(palette) != {'offset', 'direction'}:
        raise ValueError('Ungültige Palettenkonfiguration.')
    if type(palette['offset']) is not int or not 0 <= palette['offset'] < 768:
        raise ValueError('Palettenversatz muss zwischen 0 und 767 liegen.')
    if type(palette['direction']) is not int or palette['direction'] not in (-1, 1):
        raise ValueError('Ungültige Palettenrichtung.')
    view = {'iterations': view.get('iterations', 250), 'backend': view.get('backend', 'long-double'),
            'path': [list(rect) for rect in view.get('path', [])]}
    # Every zoom denominator is 10^6. 1024 digits cover all 80 permitted steps
    # exactly, including bounds separated by much less than a double ULP.
    with localcontext() as context:
        context.prec = 1024
        left, top, span = Decimal('-2.5'), Decimal('1.3125'), Decimal('3.5')
        unit, aspect = Decimal(1000000), Decimal('0.75')
        for x, y, size in view['path']:
            left += span * x / unit
            top -= span * aspect * y / unit
            span *= Decimal(size) / unit
        bounds = {name: format(value, 'f') for name, value in
                  [('left', left), ('right', left+span), ('top', top), ('bottom', top-span*aspect)]}
    return {'format': 'apfelmaennchen', 'version': 1, 'view': view, 'bounds': bounds,
            'image_size': {'width': WIDTH, 'height': HEIGHT}, 'palette': dict(palette)}


def import_config(data):
    if (not isinstance(data, dict) or data.get('format') != 'apfelmaennchen'
            or type(data.get('version')) is not int or data['version'] != 1):
        raise ValueError('Unbekanntes Konfigurationsformat oder nicht unterstützte Version.')
    expected = make_config({'view': data.get('view'), 'palette': data.get('palette')})
    if data != expected:
        raise ValueError('Konfiguration unvollständig oder Eckpunkte passen nicht zum Zoompfad.')
    return expected


def png(rgb):
    if len(rgb) != WIDTH * HEIGHT * 3:
        raise ValueError('Unvollständiges Bild.')
    def chunk(kind, value):
        return struct.pack('!I', len(value)) + kind + value + struct.pack('!I', zlib.crc32(kind + value))
    rows = b''.join(b'\0' + rgb[y*WIDTH*3:(y+1)*WIDTH*3] for y in range(HEIGHT))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('!2I5B', WIDTH, HEIGHT, 8, 2, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(rows, 3)) + chunk(b'IEND', b''))


def render_events(args, env, timeout=RENDER_TIMEOUT, connection=None):
    """Drain both pipes, enforce a deadline, and reap the renderer on disconnect."""
    start = time.monotonic()
    env = dict(env, RENDER_STREAM='1', RENDER_SCHEDULED='1', OMP_WAIT_POLICY='PASSIVE')
    job, waiting = object(), False
    process = subprocess.Popen([str(ROOT / 'render'), *args], stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, stdin=subprocess.PIPE, env=env)
    try:
        image = bytearray(WIDTH * HEIGHT * 3)
        seen, pending, diagnostics = set(), bytearray(), bytearray()
        frame_size = 4 + WIDTH * 3
        status_tail, fallback_sent = b'', False
        schedule_tail = b''
        with selectors.DefaultSelector() as selector:
            selector.register(process.stdout, selectors.EVENT_READ)
            selector.register(process.stderr, selectors.EVENT_READ)
            if connection is not None:
                selector.register(connection, selectors.EVENT_READ)
            open_pipes = 2
            while open_pipes or process.poll() is None:
                remaining = timeout - (time.monotonic() - start)
                if remaining <= 0:
                    raise subprocess.TimeoutExpired(process.args, timeout)
                if waiting and SCHEDULER.acquire(job):
                    process.stdin.write(b'1')
                    process.stdin.flush()
                    waiting = False
                for key, _ in selector.select(min(remaining, 0.001 if waiting else 0.2)):
                    # The request body is consumed and this response closes the
                    # connection. EOF/reset (or unexpected pipelined input) means
                    # this render should stop, even if no row has finished yet.
                    if key.fileobj is connection:
                        raise ConnectionAbortedError('Bildübertragung vom Client abgebrochen.')
                    block = os.read(key.fileobj.fileno(), 65536)
                    if not block:
                        selector.unregister(key.fileobj)
                        open_pipes -= 1
                        continue
                    if key.fileobj is process.stderr:
                        schedule_lines = (schedule_tail + block).split(b'\n')
                        schedule_tail = schedule_lines.pop()[-16384:]
                        for line in schedule_lines:
                            if line == b'schedule=next':
                                SCHEDULER.request(job)
                                waiting = True
                        status = status_tail + block
                        status_tail = status[-64:]
                        if not fallback_sent and b'fallback=long-double-precision\n' in status:
                            fallback_sent = True
                            yield {'type': 'notice', 'reason': 'long-double-precision',
                                   'message': 'FPU → GMP: Für diesen Zoom ist höhere Präzision nötig. Rechenzeitlimit: 5 Minuten.'}
                        diagnostics.extend(block)
                        del diagnostics[:-16384]
                        continue
                    pending.extend(block)
                    while len(pending) >= frame_size:
                        if time.monotonic() - start >= timeout:
                            raise subprocess.TimeoutExpired(process.args, timeout)
                        y = struct.unpack('!I', pending[:4])[0]
                        if y >= HEIGHT:
                            raise ValueError('Ungültige Bildzeile.')
                        row = bytes(pending[4:frame_size])
                        del pending[:frame_size]
                        image[y*WIDTH*3:(y+1)*WIDTH*3] = row
                        seen.add(y)
                        yield {'type': 'row', 'y': y, 'rgb': base64.b64encode(row).decode()}
            remaining = timeout - (time.monotonic() - start)
            if remaining <= 0:
                raise subprocess.TimeoutExpired(process.args, timeout)
            if process.wait(timeout=remaining) != 0 or pending or len(seen) != HEIGHT:
                raise ValueError('Unvollständiges Bild vom Renderer.')
        SCHEDULER.remove(job)
        backend = 'gmp'
        for line in diagnostics.decode(errors='replace').splitlines():
            if line in BACKEND_LINES:
                backend = line.split('=', 1)[1]
            elif line != 'schedule=next':
                print(line, flush=True)
        yield {'type': 'done', 'backend': backend, 'seconds': round(time.monotonic()-start, 2),
               'png': base64.b64encode(png(image)).decode()}
    finally:
        if process.poll() is None:
            process.kill()
        process.wait()
        SCHEDULER.remove(job)
        process.stdin.close()
        process.stdout.close()
        process.stderr.close()


class Handler(BaseHTTPRequestHandler):
    server_version = 'Apfelmaennchen'

    def setup(self):
        super().setup()
        self.connection.settimeout(10)

    def reply(self, code, body, content_type='text/plain; charset=utf-8', elapsed=None, backend=None):
        if isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Content-Security-Policy', "default-src 'self'; img-src 'self' blob:; style-src 'self'; script-src 'self'; frame-ancestors 'none'")
        if elapsed is not None:
            self.send_header('X-Render-Seconds', f'{elapsed:.2f}')
        if backend is not None:
            self.send_header('X-Render-Backend', backend)
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError, TimeoutError):
            pass

    def do_GET(self):
        if self.path == '/health':
            return self.reply(200, 'ok')
        files = {'/': ('index.html', 'text/html; charset=utf-8'),
                 '/app.js': ('app.js', 'text/javascript; charset=utf-8'),
                 '/style.css': ('style.css', 'text/css; charset=utf-8')}
        if self.path not in files:
            return self.reply(404, 'Nicht gefunden.')
        name, mime = files[self.path]
        self.reply(200, (ROOT / name).read_bytes(), mime)

    def stream(self, args, env):
        self.send_response(200)
        self.send_header('Content-Type', 'application/x-ndjson; charset=utf-8')
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('X-Accel-Buffering', 'no')
        self.send_header('Connection', 'close')
        self.end_headers()
        self.close_connection = True
        def send(event):
            self.wfile.write(json.dumps(event, separators=(',', ':')).encode() + b'\n')
            self.wfile.flush()
        events = render_events(args, env, connection=self.connection)
        try:
            try:
                for event in events:
                    send(event)
            except ConnectionAbortedError:
                pass
            except subprocess.TimeoutExpired:
                send({'type': 'error', 'message': TIMEOUT_MESSAGE})
            except (OSError, ValueError):
                send({'type': 'error', 'message': 'Bild konnte nicht vollständig berechnet werden.'})
        except (BrokenPipeError, ConnectionResetError, TimeoutError):
            pass
        finally:
            events.close()

    def do_POST(self):
        if self.path not in ('/render', '/render/stream', '/config/export', '/config/import'):
            return self.reply(404, 'Nicht gefunden.')
        try:
            length = int(self.headers.get('Content-Length', '0'))
            if not 0 < length <= 16384:
                raise ValueError('Auftrag fehlt oder ist zu groß.')
            data = json.loads(self.rfile.read(length))
            if self.path in ('/config/export', '/config/import'):
                config = make_config(data) if self.path == '/config/export' else import_config(data)
                return self.reply(200, json.dumps(config, ensure_ascii=False, indent=2), 'application/json; charset=utf-8')
            args = validate(data)
        except (ValueError, UnicodeError) as error:
            return self.reply(400, str(error))
        if not SLOT.acquire(blocking=False):
            return self.reply(503, 'Alle Rechenplätze sind belegt. Bitte gleich erneut versuchen.')
        try:
            start = time.monotonic()
            env = render_environment(data)
            if self.path == '/render/stream':
                return self.stream(args, env)
            events = render_events(args, env, connection=self.connection)
            try:
                for event in events:
                    if event['type'] == 'done':
                        self.reply(200, base64.b64decode(event['png']), 'image/png',
                                   time.monotonic()-start, event['backend'])
            finally:
                events.close()
        except ConnectionAbortedError:
            pass
        except subprocess.TimeoutExpired:
            self.reply(504, TIMEOUT_MESSAGE)
        except (subprocess.CalledProcessError, OSError, ValueError):
            self.reply(500, 'Bild konnte nicht berechnet werden.')
        finally:
            SLOT.release()


if __name__ == '__main__':
    print('Apfelmännchen auf Port 8080', flush=True)
    ThreadingHTTPServer(('0.0.0.0', 8080), Handler).serve_forever()
