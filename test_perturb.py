"""Requires the GPU container. Compare perturbation and repaired pixels to GMP."""
from decimal import Decimal, localcontext
import base64
import json
import re
import urllib.request
from test_gpu import render
from server import png


def zoom_path(real, imag, depth):
    with localcontext() as context:
        context.prec = 256
        cx, cy = Decimal(real), Decimal(imag)
        left, top, span = Decimal('-2.5'), Decimal('1.3125'), Decimal('3.5')
        path = []
        for _ in range(depth):
            height = span * Decimal('.75')
            x = max(0, min(900000, int(((cx-left)/span-Decimal('.05'))*1000000)))
            y = max(0, min(900000, int(((top-cy)/height-Decimal('.05'))*1000000)))
            path.append([x, y, 100000])
            left += span*x/1000000
            top -= height*y/1000000
            span *= Decimal('.1')
        return path


if __name__ == '__main__':
    cases = [
        ('start', [], 250),
        ('boundary', [[400000, 150000, 250000]], 500),
        ('deep-boundary', zoom_path('-.743643887037151', '.131825904205330', 6), 1000),
        ('deeper-boundary', zoom_path('-.743643887037151', '.131825904205330', 10), 1000),
        ('escaping-reference', zoom_path('-.75', '.1', 8), 250),
    ]
    for name, path, iterations in cases:
        args = [str(iterations)] + [str(value) for rect in path for value in rect]
        print(name, flush=True)
        actual, log = render(args, 'perturb')
        assert b'backend=opencl-perturb' in log
        expected, _ = render(args, 'gmp')
        differences = sum(actual[i:i+3] != expected[i:i+3] for i in range(0, len(actual), 3))
        print('Pixelabweichungen:', differences, flush=True)
        assert differences == 0
        if name == 'start':
            repaired = int(re.search(rb'GMP repairs=(\d+)', log)[1])
            assert 0 < repaired < 307200
        if name == 'deep-boundary':
            automatic, auto_log = render(args, 'auto')
            assert b'backend=gmp' in auto_log and automatic == expected
            deep_view, deep_image = {'path': path, 'iterations': iterations, 'backend': 'perturb'}, expected
    args = ['50'] + ['400000', '150000', '250000']*60
    actual, log = render(args, 'perturb')
    assert b'backend=gmp' in log and actual == render(args, 'gmp')[0]
    missing, log = render(['100'], 'perturb', extra='OCL_ICD_VENDORS=/nonexistent')
    assert b'backend=gmp' in log and missing == render(['100'], 'gmp')[0]
    for route in ['/render', '/render/stream']:
        request = urllib.request.Request('http://127.0.0.1:8080'+route,
            data=json.dumps(deep_view).encode(), headers={'Content-Type': 'application/json'})
        with urllib.request.urlopen(request, timeout=55) as response:
            if route == '/render':
                assert response.headers['X-Render-Backend'].startswith('opencl-perturb')
                assert response.read() == png(deep_image)
            else:
                events = [json.loads(line) for line in response]
                rows = [event for event in events if event['type'] == 'row']
                assert len(rows) == 480
                reconstructed = bytearray(640*480*3)
                for row in rows:
                    reconstructed[row['y']*1920:(row['y']+1)*1920] = base64.b64decode(row['rgb'])
                assert reconstructed == deep_image
                assert events[-1]['backend'].startswith('opencl-perturb')
                assert base64.b64decode(events[-1]['png']) == png(deep_image)
    print('Reference orbit, repaired pixels, deep zoom, streaming and fallbacks passed.')
