import struct
import unittest
import zlib
from server import validate, png, WIDTH, HEIGHT

class Tests(unittest.TestCase):
    def test_valid(self):
        self.assertEqual(validate({'iterations': 250, 'path': [[0, 0, 500000]]}), ['250', '0', '0', '500000'])
    def test_high_iterations(self):
        for iterations in (2001, 10000, 100000):
            self.assertEqual(validate({'iterations': iterations}), [str(iterations)])

    def test_invalid(self):
        for value in [None, [], {'iterations': True}, {'iterations': 100001}, {'path': [[0, 0, 0]]},
                      {'path': [[900000, 0, 500000]]}, {'path': [[0.0, 0, 1000]]},
                      {'path': [[0, 0, 1000]] * 81}, {'unknown': 1}, {'backend': 'invalid'}, {'backend': []}]:
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate(value)
    def test_png(self):
        data = png(bytes(WIDTH * HEIGHT * 3))
        self.assertEqual(data[:8], b'\x89PNG\r\n\x1a\n')
        self.assertEqual(struct.unpack('!II', data[16:24]), (WIDTH, HEIGHT))
        offset, compressed = 8, b''
        while offset < len(data):
            size = struct.unpack('!I', data[offset:offset+4])[0]
            kind, payload = data[offset+4:offset+8], data[offset+8:offset+8+size]
            crc = struct.unpack('!I', data[offset+8+size:offset+12+size])[0]
            self.assertEqual(crc, zlib.crc32(kind+payload))
            if kind == b'IDAT': compressed += payload
            offset += size+12
        self.assertEqual(len(zlib.decompress(compressed)), (WIDTH*3+1)*HEIGHT)
    def test_short_image(self):
        with self.assertRaises(ValueError): png(b'')
