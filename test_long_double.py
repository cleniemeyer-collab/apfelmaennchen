"""Integration checks for native long double, including streaming and config files."""
import base64
import json
import re
import urllib.request
from test_gpu import render
from server import png


def post(route, data):
    return urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:8080'+route,
        data=json.dumps(data).encode(), headers={'Content-Type': 'application/json'}), timeout=55)


if __name__ == '__main__':
    for name, args in [('start', ['250']), ('1000 iterations', ['1000']),
                       ('zoom', ['500', '400000', '150000', '250000'])]:
        print(name, flush=True)
        actual, log = render(args, 'long-double')
        assert b'backend=cpu-long-double' in log
        precision = re.search(rb'long double: (\d+) mantissa bits, (\d+) storage bytes', log)
        assert precision and int(precision[1]) >= 64
        expected, _ = render(args, 'gmp')
        differences = sum(actual[i:i+3] != expected[i:i+3] for i in range(0, len(actual), 3))
        print(f'{differences}/307200 pixels differ from GMP', flush=True)
        # Native floating point may diverge near the boundary, but not broadly.
        assert differences < 307200 * 0.001
        if name == 'start': start_image = actual
    single, _ = render(['250'], 'long-double', extra='OMP_NUM_THREADS=1')
    assert single == start_image
    deep = ['100'] + ['400000', '150000', '250000']*30
    actual, log = render(deep, 'long-double')
    assert b'backend=gmp' in log and b'insufficient coordinate resolution' in log
    assert actual == render(deep, 'gmp')[0]
    view = {'path': [], 'iterations': 250, 'backend': 'long-double'}
    with post('/render', view) as response:
        assert response.headers['X-Render-Backend'] == 'cpu-long-double'
        assert response.read() == png(start_image)
    with post('/render/stream', view) as response:
        events = [json.loads(line) for line in response]
    rows = [event for event in events if event['type'] == 'row']
    assert len(rows) == 480
    reconstructed = bytearray(len(start_image))
    for row in rows:
        reconstructed[row['y']*1920:(row['y']+1)*1920] = base64.b64decode(row['rgb'])
    assert reconstructed == start_image
    assert events[-1]['backend'] == 'cpu-long-double'
    assert base64.b64decode(events[-1]['png']) == png(start_image)
    with post('/config/export', {'view': view, 'palette': {'offset': 256, 'direction': -1}}) as response:
        config = json.load(response)
    with post('/config/import', config) as response:
        restored = json.load(response)
    assert config == restored and restored['view']['backend'] == 'long-double'
    print('long double: precision, threads, GMP fallback, HTTP, streaming and config roundtrip passed.')
