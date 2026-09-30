"""Pixel regression against the original GMP loop, using the running container."""
import hashlib
import statistics
import subprocess
import time
import unittest


class OptimizationTests(unittest.TestCase):
    def test_reference_pixels(self):
        cases = [
            ('start', ['1000'], '2e79f368227f6eb398d289a9818af945630ab7addada9f8f7c1d93b962d943d0'),
            ('zoom', ['500', '400000', '150000', '250000'],
             '6c5761633def66ca434c7cd43e5bbb101aa6512c5201265ffadfbfcba3ee90ef'),
        ]
        for name, args, expected in cases:
            durations = []
            for _ in range(3):
                start = time.monotonic()
                result = subprocess.run(
                    ['docker', 'compose', 'exec', '-T', '-e', 'RENDER_BACKEND=gmp',
                     'apfelmaennchen', './render', *args],
                    capture_output=True, check=True, timeout=55)
                durations.append(time.monotonic()-start)
                with self.subTest(view=name):
                    self.assertEqual(hashlib.sha256(result.stdout).hexdigest(), expected)
            print(f'{name}: Median {statistics.median(durations):.3f}s, '
                  f'pixelgleich zum ursprünglichen Renderer', flush=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
