"""Integration test: requires the running OpenMP-enabled Docker container."""
import hashlib
import subprocess
import time
import unittest


class ParallelTests(unittest.TestCase):
    def test_identical_pixels(self):
        cases = [('start', ['250']),
                 ('zoom', ['100', '400000', '150000', '250000']),
                 ('deep', ['50'] + ['400000', '150000', '250000'] * 12)]
        for name, args in cases:
            images, durations = [], []
            for threads in (1, 4):
                start = time.monotonic()
                image = subprocess.check_output(
                    ['docker', 'compose', 'exec', '-T', '-e',
                     f'OMP_NUM_THREADS={threads}', '-e', 'RENDER_BACKEND=gmp',
                                          'apfelmaennchen', './render', *args],
                    timeout=55)
                durations.append(time.monotonic() - start)
                self.assertEqual(len(image), 640 * 480 * 3)
                images.append(image)
            with self.subTest(view=name):
                self.assertEqual(images[0], images[1])
                if name == 'start':
                    # Reference from the original, sequential GMP renderer.
                    self.assertEqual(hashlib.sha256(images[0]).hexdigest(),
                                     'f8eca1866dca4dc41402e891d8a6b2475a15c77bae7d70ee7a420eebe7a00022')
            print(f'{name}: 1 Thread {durations[0]:.2f}s; 4 Threads {durations[1]:.2f}s', flush=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
