import os
import itertools
import subprocess
import sys
import socket
import unittest
import threading
import time
import json
import urllib.request
import urllib.error
from concurrent.futures import ThreadPoolExecutor
from unittest.mock import patch
from server import render_events, RENDER_TIMEOUT, RenderScheduler, Handler, ThreadingHTTPServer
import server


class SchedulerTests(unittest.TestCase):
    def test_round_robin_and_removal(self):
        scheduler = RenderScheduler()
        a, b, c = object(), object(), object()
        scheduler.request(a)
        self.assertTrue(scheduler.acquire(a))
        scheduler.request(b)
        scheduler.request(c)
        self.assertFalse(scheduler.acquire(b))
        scheduler.request(a)
        self.assertFalse(scheduler.acquire(a))
        self.assertTrue(scheduler.acquire(b))
        scheduler.remove(c)
        scheduler.remove(b)
        self.assertTrue(scheduler.acquire(a))
        scheduler.remove(a)
        self.assertIsNone(scheduler.owner)
        self.assertFalse(scheduler.waiting)

    def test_backend_defaults_and_override(self):
        with patch.dict(os.environ, {'RENDER_BACKEND': 'auto'}):
            self.assertEqual(server.render_environment({})['RENDER_BACKEND'], 'long-double')
            for backend in ('auto', 'long-double', 'gmp', 'perturb'):
                self.assertEqual(server.render_environment({'backend': backend})['RENDER_BACKEND'], backend)
        with patch.dict(os.environ, {'RENDER_BACKEND': 'gmp'}):
            self.assertEqual(server.render_environment({'backend': 'long-double'})['RENDER_BACKEND'], 'gmp')
        self.assertEqual(server.make_config({'view': {}, 'palette': {'offset': 0, 'direction': 1}})['view']['backend'], 'long-double')

    def test_http_capacity_and_release(self):
        entered, release = threading.Event(), threading.Event()
        lock, active = threading.Lock(), []
        def events(*args, **kwargs):
            with lock:
                active.append(1)
                if len(active) == 2:
                    entered.set()
            if not release.wait(3):
                raise TimeoutError('Test did not release jobs')
            yield {'type': 'done', 'backend': 'gmp', 'seconds': 0,
                   'png': 'iVBORw0KGgo='}
        http = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=http.serve_forever, daemon=True)
        thread.start()
        def post(route):
            request = urllib.request.Request(f'http://127.0.0.1:{http.server_port}{route}',
                                             data=b'{}', headers={'Content-Type': 'application/json'})
            with urllib.request.urlopen(request, timeout=5) as response:
                return response.read()
        try:
            with patch('server.SLOT', threading.BoundedSemaphore(2)), patch('server.render_events', events):
                with ThreadPoolExecutor(2) as pool:
                    first = pool.submit(post, '/render')
                    second = pool.submit(post, '/render/stream')
                    try:
                        self.assertTrue(entered.wait(2), 'Both endpoints must enter concurrently')
                        with self.assertRaises(urllib.error.HTTPError) as error:
                            post('/render')
                        self.assertEqual(error.exception.code, 503)
                    finally:
                        release.set()
                    self.assertTrue(first.result().startswith(b'\x89PNG'))
                    self.assertEqual(json.loads(second.result())['type'], 'done')
                self.assertTrue(post('/render').startswith(b'\x89PNG'))
        finally:
            release.set()
            http.shutdown()
            http.server_close()
            thread.join()


class StreamTests(unittest.TestCase):
    def launch(self, script):
        original = subprocess.Popen
        self.process = None
        def create(_args, **kwargs):
            self.process = original([sys.executable, '-u', '-c', script], **kwargs)
            return self.process
        return patch('server.subprocess.Popen', side_effect=create)

    def test_scheduled_workers_alternate(self):
        class RecordingScheduler(RenderScheduler):
            def __init__(self):
                super().__init__()
                self.jobs, self.order = set(), []
                self.both = threading.Event()

            def request(self, job):
                super().request(job)
                with self.lock:
                    self.jobs.add(job)
                    if len(self.jobs) == 2:
                        self.both.set()

            def acquire(self, job):
                if not self.both.is_set():
                    return False
                granted = super().acquire(job)
                if granted:
                    self.order.append(job)
                return granted

        scheduler = RecordingScheduler()
        script = '''import sys,struct,time
for y in range(480):
 sys.stderr.write('schedule=next\\n'); sys.stderr.flush()
 assert sys.stdin.read(1)=='1'
 time.sleep(.001)
 sys.stdout.buffer.write(struct.pack('!I',y)+bytes(1920)); sys.stdout.flush()
sys.stderr.write('backend=gmp\\n')
'''
        with self.launch(script), patch('server.SCHEDULER', scheduler):
            with ThreadPoolExecutor(2) as pool:
                jobs = [pool.submit(lambda: list(render_events(['50'], os.environ, timeout=10))) for _ in range(2)]
                for job in jobs:
                    result = job.result(timeout=12)
                    self.assertEqual(len(result), 481)
                    self.assertEqual(result[-1]['type'], 'done')
        self.assertEqual(len(scheduler.order), 960)
        # Both workers receive turns while the other still has unfinished rows.
        self.assertEqual(len(set(scheduler.order[:4])), 2)
        self.assertGreater(sum(a is not b for a, b in zip(scheduler.order, scheduler.order[1:])), 900)
        self.assertIsNone(scheduler.owner)
        self.assertFalse(scheduler.waiting)

    def test_waiting_disconnect_preserves_owner(self):
        scheduler = RenderScheduler()
        owner = object()
        scheduler.request(owner)
        self.assertTrue(scheduler.acquire(owner))
        connection, client = socket.socketpair()
        script = "import sys,time; sys.stderr.write('schedule=next\\n'); sys.stderr.flush(); sys.stdin.read(1)"
        try:
            with self.launch(script), patch('server.SCHEDULER', scheduler):
                with ThreadPoolExecutor(1) as pool:
                    future = pool.submit(lambda: list(render_events(['50'], os.environ, timeout=2, connection=connection)))
                    deadline = time.monotonic()+1
                    while not scheduler.waiting and time.monotonic() < deadline:
                        time.sleep(.001)
                    self.assertTrue(scheduler.waiting)
                    client.close()
                    with self.assertRaises(ConnectionAbortedError):
                        future.result(timeout=3)
            self.assertIs(scheduler.owner, owner)
            self.assertFalse(scheduler.waiting)
            self.assertIsNotNone(self.process.poll())
        finally:
            connection.close()
            client.close()

    def test_timeout_reaps_process(self):
        scheduler = RenderScheduler()
        script = "import sys,time; sys.stderr.write('schedule=next\\n'); sys.stderr.flush(); sys.stdin.read(1); time.sleep(10)"
        with self.launch(script), patch('server.SCHEDULER', scheduler):
            with self.assertRaises(subprocess.TimeoutExpired):
                list(render_events(['50'], os.environ, timeout=0.1))
        self.assertIsNotNone(self.process.poll())
        self.assertIsNone(scheduler.owner)
        self.assertFalse(scheduler.waiting)

    def test_waiting_timeout_removes_only_waiter(self):
        scheduler = RenderScheduler()
        owner = object()
        scheduler.request(owner)
        scheduler.acquire(owner)
        script = "import sys; sys.stderr.write('schedule=next\\n'); sys.stderr.flush(); sys.stdin.read(1)"
        with self.launch(script), patch('server.SCHEDULER', scheduler):
            with self.assertRaises(subprocess.TimeoutExpired):
                list(render_events(['50'], os.environ, timeout=0.1))
        self.assertIs(scheduler.owner, owner)
        self.assertFalse(scheduler.waiting)
        self.assertIsNotNone(self.process.poll())

    def test_five_minute_default_allows_more_than_45_seconds(self):
        self.assertEqual(RENDER_TIMEOUT, 300)
        script = "import sys,struct; sys.stdout.buffer.write(b''.join(struct.pack('!I',y)+bytes(1920) for y in range(480)))"
        clock = itertools.chain([0], itertools.repeat(60))
        with self.launch(script), patch('server.time.monotonic', side_effect=lambda: next(clock)):
            events = list(render_events(['50'], os.environ))
        self.assertEqual(events[-1]['type'], 'done')
        self.assertEqual(events[-1]['seconds'], 60)

    def test_fallback_notice_precedes_image(self):
        script = "import sys,time; sys.stderr.write('fallback=long-double-'); sys.stderr.flush(); time.sleep(.02); sys.stderr.write('precision\\n'); sys.stderr.flush(); time.sleep(10)"
        with self.launch(script):
            events = render_events(['50'], os.environ)
            event = next(events)
            self.assertEqual(event['type'], 'notice')
            self.assertEqual(event['reason'], 'long-double-precision')
            self.assertIn('5 Minuten', event['message'])
            events.close()
        self.assertIsNotNone(self.process.poll())

    def test_partial_frame_is_rejected(self):
        with self.launch('import sys; sys.stdout.buffer.write(bytes(17))'):
            with self.assertRaises(ValueError):
                list(render_events(['50'], os.environ))
        self.assertIsNotNone(self.process.poll())

    def test_disconnect_reaps_process(self):
        script = 'import sys,time; sys.stdout.buffer.write(bytes(1924)); sys.stdout.flush(); time.sleep(10)'
        with self.launch(script):
            events = render_events(['50'], os.environ)
            self.assertEqual(next(events)['type'], 'row')
            events.close()
        self.assertIsNotNone(self.process.poll())

    def test_socket_disconnect_without_rows_reaps_process(self):
        server, client = socket.socketpair()
        try:
            client.close()
            with self.launch('import time; time.sleep(10)'):
                with self.assertRaises(ConnectionAbortedError):
                    list(render_events(['50'], os.environ, timeout=1, connection=server))
            self.assertIsNotNone(self.process.poll())
        finally:
            server.close()
            client.close()

    def test_socket_disconnect_between_rows_reaps_process(self):
        server, client = socket.socketpair()
        script = 'import sys,time; sys.stdout.buffer.write(bytes(1924)); sys.stdout.flush(); time.sleep(10)'
        try:
            with self.launch(script):
                events = render_events(['50'], os.environ, timeout=1, connection=server)
                self.assertEqual(next(events)['type'], 'row')
                client.close()
                with self.assertRaises(ConnectionAbortedError):
                    next(events)
            self.assertIsNotNone(self.process.poll())
        finally:
            server.close()
            client.close()

    def test_connected_socket_does_not_prevent_completion(self):
        server, client = socket.socketpair()
        script = "import sys,struct; sys.stdout.buffer.write(b''.join(struct.pack('!I',y)+bytes(1920) for y in range(480)))"
        try:
            with self.launch(script):
                events = list(render_events(['50'], os.environ, timeout=1, connection=server))
            self.assertEqual(events[-1]['type'], 'done')
        finally:
            server.close()
            client.close()

    def test_out_of_order_rows_and_stderr(self):
        script = '''import sys,struct
sys.stderr.write('x'*70000)
for y in reversed(range(480)):
 sys.stdout.buffer.write(struct.pack('!I',y)+bytes([y%256])*1920)
sys.stderr.write('\\nbackend=gmp\\n')
'''
        with self.launch(script), patch('builtins.print'):
            events = list(render_events(['50'], os.environ))
        self.assertEqual(len(events), 481)
        self.assertEqual(events[0]['y'], 479)
        self.assertEqual(events[-1]['type'], 'done')
        self.assertEqual(events[-1]['backend'], 'gmp')
