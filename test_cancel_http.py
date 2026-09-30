"""Check cancellation before the first row and immediate reuse of the renderer."""
import json
import time
import urllib.error
import urllib.request

BASE = 'http://127.0.0.1:8080'


def post(route, view):
    return urllib.request.urlopen(urllib.request.Request(BASE+route,
        data=json.dumps(view).encode(), headers={'Content-Type': 'application/json'}), timeout=5)


if __name__ == '__main__':
    # All pixels are inside the main cardioid: the old implementation would
    # wait for an expensive row before noticing the closed connection.
    expensive = {'path': [[700000,499500,1000]], 'iterations': 100000, 'backend': 'gmp'}
    cheap = {'path': [[0,0,100000]], 'iterations': 50, 'backend': 'long-double'}
    stream = post('/render/stream', expensive)
    other = post('/render/stream', cheap)
    stream.close()
    try:
        events = [json.loads(line) for line in other]
        assert events[-1]['type'] == 'done', events[-1]
    finally:
        other.close()
    start = time.monotonic()
    while True:
        try:
            with post('/render', cheap) as response:
                assert response.read().startswith(b'\x89PNG')
            break
        except urllib.error.HTTPError as error:
            assert error.code == 503 and time.monotonic()-start < 3
            time.sleep(0.05)
    print(f'Cancel before first row: next render finished after {time.monotonic()-start:.3f}s')
