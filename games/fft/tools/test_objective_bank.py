"""FGO1 ownership, bounded spans and unsupported name/layout rejection."""
import struct
import unittest
from unittest.mock import patch
import objective_bank as goals


class Objectives(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.entry = dict(route='g', target=dict(slot=1, name='Keane'), protect=dict(slot=0, name='Delita'))
        cls.bank = goals.build([cls.entry])

    def test_valid(self):
        self.assertEqual(goals.validate(self.bank, {'g'}), {'g'})

    def test_bad_spans(self):
        for offset, value in [(4, 0), (6, 255), (10, 8192), (12, 2), (14, 1), (16, 65534), (20, 1)]:
            bad = bytearray(self.bank); struct.pack_into('>H', bad, offset, value)
            with self.subTest(offset=offset), self.assertRaises(ValueError): goals.validate(bad)
        for length in (0, 7, 23, len(self.bank)-1):
            with self.assertRaises(ValueError): goals.validate(self.bank[:length])

    def test_bad_strip(self):
        at = 8+struct.unpack_from('>H', self.bank, 14)[0]
        for field, value in enumerate((1, 255, 255, 255)):
            bad = bytearray(self.bank); bad[at+field] = value
            with self.assertRaises(ValueError): goals.validate(bad)

    def test_route_not_installed(self):
        with self.assertRaises(ValueError): goals.validate(self.bank, {'o'})

    def test_invalid_identities(self):
        with patch.object(goals.banner, '_font_outlines', side_effect=AssertionError('must reject before rendering')):
            for entries in ([], [self.entry, self.entry], [dict(route='c', target=dict(slot=0, name='A'))],
                            [dict(route='g', target=dict(slot=13, name='A'))],
                            [dict(route='g', target=dict(slot=1, name='A'), protect=dict(slot=1, name='B'))]):
                with self.assertRaises(ValueError): goals.build(entries)


if __name__ == '__main__': unittest.main()
