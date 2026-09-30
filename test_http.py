import json
import urllib.request
import urllib.error

BASE = 'http://127.0.0.1:8080'

def render(path, iterations=100):
    request = urllib.request.Request(BASE+'/render', data=json.dumps({'path': path, 'iterations': iterations}).encode(), headers={'Content-Type': 'application/json'})
    with urllib.request.urlopen(request, timeout=55) as response:
        data = response.read()
        assert response.headers['Content-Type'] == 'image/png'
        assert data.startswith(b'\x89PNG\r\n\x1a\n')
        print('Bild:', len(data), 'Bytes,', response.headers['X-Render-Seconds'], 'Sekunden', flush=True)
        return data

if __name__ == '__main__':
    for route in ['/', '/app.js', '/style.css', '/health']:
        with urllib.request.urlopen(BASE+route, timeout=5) as response:
            assert response.status == 200
    original = render([], 250)
    zoom = render([[400000, 150000, 250000]])
    assert zoom != original
    assert render([], 250) == original
    try:
        render([[900000, 0, 500000]])
        raise AssertionError('Ungültiger Zoom akzeptiert')
    except urllib.error.HTTPError as error:
        assert error.code == 400
    print('HTTP-Tests erfolgreich.')
