"""Real HTTP streaming tests against the running container."""
import base64
import json
import time
import urllib.request
import urllib.error
from server import png

BASE = 'http://127.0.0.1:8080'


def request(route, backend='gmp', iterations=1000):
    return urllib.request.urlopen(urllib.request.Request(BASE+route,
        data=json.dumps({'path': [], 'iterations': iterations, 'backend': backend}).encode(),
        headers={'Content-Type': 'application/json'}), timeout=55)


if __name__ == '__main__':
    for backend in ('gmp', 'auto'):
        start = time.monotonic()
        seen, image, first, completed = set(), bytearray(640*480*3), None, None
        with request('/render/stream', backend) as response:
            assert response.headers['Content-Type'].startswith('application/x-ndjson')
            for line in response:
                event = json.loads(line)
                if event['type'] == 'row':
                    if first is None: first = time.monotonic()-start
                    y = event['y']
                    row = base64.b64decode(event['rgb'])
                    assert 0 <= y < 480 and len(row) == 1920
                    image[y*1920:(y+1)*1920] = row
                    seen.add(y)
                else:
                    assert event['type'] == 'done', event
                    completed = event
        elapsed = time.monotonic()-start
        assert len(seen) == 480 and completed
        assert base64.b64decode(completed['png']) == png(image)
        with request('/render', backend) as response:
            assert response.read() == png(image), 'Stream changed final pixels'
        assert first < elapsed
        if backend == 'gmp': assert elapsed-first > 0.1
        print(f'{backend}: erste Zeile {first:.3f}s, Bild fertig {elapsed:.3f}s; Pixel identisch', flush=True)

    response = request('/render/stream', 'gmp', 2000)
    assert json.loads(response.readline())['type'] == 'row'
    try:
        with request('/render/stream', 'long-double', 50) as other:
            assert json.loads(other.readline())['type'] == 'row'
            response.close()
            events = [json.loads(line) for line in other]
            assert events[-1]['type'] == 'done', events[-1]
    finally:
        response.close()
    deadline = time.monotonic()+4
    while True:
        try:
            with request('/render', 'gmp', 50) as response:
                assert response.status == 200
            break
        except urllib.error.HTTPError as error:
            assert error.code == 503 and time.monotonic() < deadline
            time.sleep(0.1)
    print('Zwei Clients liefern Zeilen; Abbruch beendet nur den eigenen Auftrag.')
