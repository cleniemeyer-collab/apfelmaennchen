import copy
from decimal import Decimal
from fractions import Fraction
import json
import unittest
from server import make_config, import_config, validate


class ConfigTests(unittest.TestCase):
    def config(self, path=None):
        return make_config({'view': {'path': path or [], 'iterations': 777, 'backend': 'perturb'},
                            'palette': {'offset': 256, 'direction': -1}})

    def test_start_bounds(self):
        config = self.config()
        self.assertEqual({k: Decimal(v) for k, v in config['bounds'].items()},
                         {'left': Decimal('-2.5'), 'right': Decimal('1'),
                          'top': Decimal('1.3125'), 'bottom': Decimal('-1.3125')})
        self.assertEqual(import_config(json.loads(json.dumps(config))), config)
        validate(config['view'])

    def test_deep_exact_roundtrip(self):
        path = [[123457, 234567, 1001]] * 80
        config = self.config(path)
        left, top, span = Fraction('-2.5'), Fraction('1.3125'), Fraction('3.5')
        for x, y, size in path:
            left += span*x/1000000
            top -= span*Fraction(3, 4)*y/1000000
            span *= Fraction(size, 1000000)
        self.assertEqual(Fraction(config['bounds']['left']), left)
        self.assertEqual(Fraction(config['bounds']['right']), left+span)
        self.assertEqual(Fraction(config['bounds']['top']), top)
        self.assertEqual(Fraction(config['bounds']['bottom']), top-span*Fraction(3, 4))
        self.assertLess(len(json.dumps(config).encode()), 16384)
        self.assertEqual(import_config(config), config)

    def test_reject_invalid(self):
        for field, value in [('version', 2), ('version', True), ('format', 'other'),
                             ('view', None), ('palette', []), ('image_size', {}),
                             ('bounds', {'left': 'NaN'})]:
            config = self.config()
            config[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                import_config(config)
        for palette in [{'offset': True, 'direction': 1}, {'offset': 768, 'direction': 1},
                        {'offset': 0, 'direction': 0}]:
            with self.assertRaises(ValueError):
                make_config({'view': {}, 'palette': palette})

    def test_tampered_bounds(self):
        config = self.config([[200000, 300000, 250000]])
        altered = copy.deepcopy(config)
        altered['bounds']['left'] = '-2.5'
        with self.assertRaises(ValueError):
            import_config(altered)
