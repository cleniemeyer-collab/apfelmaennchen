"""Integration checks for the Intel/OpenCL Docker variant."""
import json
import subprocess
import time
import urllib.request


def render(args, backend='auto', extra=None):
    command = ['docker', 'compose', 'exec', '-T', '-e', f'RENDER_BACKEND={backend}']
    if extra:
        command += ['-e', extra]
    started = time.monotonic()
    result = subprocess.run(command + ['apfelmaennchen', './render', *args], capture_output=True, check=True, timeout=55)
    assert len(result.stdout) == 640*480*3
    print(backend, result.stderr.decode().strip(), f'{time.monotonic()-started:.3f}s', flush=True)
    return result.stdout, result.stderr


if __name__ == '__main__':
    gpu, log = render(['250'])
    assert b'backend=opencl-' in log, 'No working GPU detected'
    cpu, log = render(['250'], 'gmp')
    assert b'backend=gmp' in log
    mismatches = sum(gpu[i:i+3] != cpu[i:i+3] for i in range(0, len(gpu), 3))
    print(f'FP32/GMP: {mismatches} abweichende Pixel von 307200')
    assert mismatches < 307200 * 0.02
    # Stable exterior and interior points must agree, unlike boundary pixels.
    for x, y in [(0, 0), (639, 479), (450, 240), (275, 240)]:
        pos = (y*640+x)*3
        assert gpu[pos:pos+3] == cpu[pos:pos+3]
    args = ['100'] + ['400000', '150000', '250000']*20
    deep, log = render(args)
    assert b'backend=gmp' in log
    assert deep == render(args, 'gmp')[0]
    fallback, log = render(['250'], extra='OCL_ICD_VENDORS=/nonexistent')
    assert b'backend=gmp' in log and fallback == cpu
    for backend, expected in [('auto', 'opencl-'), ('gmp', 'gmp')]:
        request = urllib.request.Request('http://127.0.0.1:8080/render',
            data=json.dumps({'iterations': 100, 'path': [], 'backend': backend}).encode(),
            headers={'Content-Type': 'application/json'})
        with urllib.request.urlopen(request, timeout=55) as response:
            assert response.headers['X-Render-Backend'].startswith(expected)
            assert response.read().startswith(b'\x89PNG')
    print('GPU, CPU-Umschaltung, fehlende GPU und HTTP-Auswahl erfolgreich geprüft.')
