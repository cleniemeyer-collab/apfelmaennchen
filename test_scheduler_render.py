"""Real renderer checks; run inside a built image with this directory on sys.path."""
import base64
import json
import os
import subprocess
import threading
import unittest
import urllib.request
from concurrent.futures import ThreadPoolExecutor
import server


@unittest.skipUnless((server.ROOT / "render").exists(), "Built renderer required")
class RendererTests(unittest.TestCase):
    def test_scheduled_pixels_match_standalone(self):
        for backend in ("long-double", "gmp", "auto", "perturb"):
            with self.subTest(backend=backend):
                env = dict(os.environ, RENDER_BACKEND=backend, RENDER_STREAM="0", RENDER_SCHEDULED="0")
                expected = subprocess.run([str(server.ROOT / "render"), "50"], env=env,
                                          capture_output=True, check=True, timeout=60)
                for threads in ("1", "4"):
                    events = list(server.render_events(["50"], dict(env, OMP_NUM_THREADS=threads), timeout=60))
                    rows = [event for event in events if event["type"] == "row"]
                    self.assertEqual(len(rows), 480)
                    image = bytearray(640*480*3)
                    for row in rows:
                        image[row["y"]*1920:(row["y"]+1)*1920] = base64.b64decode(row["rgb"])
                    self.assertEqual(image, expected.stdout)
                    self.assertEqual(base64.b64decode(events[-1]["png"]), server.png(expected.stdout))
                    self.assertIn(("backend="+events[-1]["backend"]).encode(), expected.stderr)
                    print(backend, threads, events[-1]["backend"], events[-1]["seconds"], flush=True)

    def test_deep_fpu_fallback(self):
        args = ["50"] + ["400000", "150000", "250000"]*30
        events = list(server.render_events(args, dict(os.environ, RENDER_BACKEND="long-double"), timeout=60))
        self.assertEqual(events[0]["reason"], "long-double-precision")
        self.assertEqual(events[-1]["backend"], "gmp")
        reference = list(server.render_events(args, dict(os.environ, RENDER_BACKEND="gmp"), timeout=60))
        self.assertEqual(events[-1]["png"], reference[-1]["png"])

    def test_four_clients_make_progress(self):
        barrier = threading.Barrier(4)
        lock, progress = threading.Lock(), []
        def render(job):
            client, backend = job
            barrier.wait(timeout=5)
            events = []
            for event in server.render_events(["50"], dict(os.environ, RENDER_BACKEND=backend), timeout=60):
                events.append(event)
                if event["type"] in ("row", "done"):
                    with lock:
                        progress.append((client, event["type"]))
            self.assertEqual(events[-1]["type"], "done")
            self.assertEqual(len([event for event in events if event["type"] == "row"]), 480)
        for backends in (("gmp", "long-double", "auto", "perturb"), ("auto",)*4, ("perturb",)*4):
            with self.subTest(backends=backends):
                progress.clear()
                with ThreadPoolExecutor(4) as pool:
                    list(pool.map(render, enumerate(backends)))
                first_done = next(i for i, (_, kind) in enumerate(progress) if kind == "done")
                self.assertEqual(len({client for client, _ in progress[:first_done]}), 4)
                self.assertIsNone(server.SCHEDULER.owner)
                self.assertFalse(server.SCHEDULER.waiting)

    def test_http_stream_png_and_cancel(self):
        http = server.ThreadingHTTPServer(("127.0.0.1", 0), server.Handler)
        thread = threading.Thread(target=http.serve_forever, daemon=True)
        thread.start()
        def post(route, view):
            request = urllib.request.Request(f"http://127.0.0.1:{http.server_port}{route}",
                data=json.dumps(view).encode(), headers={"Content-Type": "application/json"})
            return urllib.request.urlopen(request, timeout=15)
        try:
            cheap = {"iterations": 50, "path": [[0, 0, 100000]]}
            expensive = {"iterations": 100000, "backend": "gmp", "path": [[700000, 499500, 1000]]}
            first = post("/render/stream", expensive)
            try:
                other = post("/render/stream", cheap)
                first.close()
                with other:
                    events = [json.loads(line) for line in other]
                self.assertEqual(events[-1]["type"], "done")
                self.assertEqual(events[-1]["backend"], "cpu-long-double")
                with post("/render", cheap) as response:
                    self.assertEqual(response.headers["X-Render-Backend"], "cpu-long-double")
                    self.assertEqual(response.read(), base64.b64decode(events[-1]["png"]))
            finally:
                first.close()
        finally:
            http.shutdown()
            http.server_close()
            thread.join()
