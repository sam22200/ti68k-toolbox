"""Guard source parts that the old human-only compositor cannot represent."""
import struct
import unittest

import numpy as np

import monster_ko_review as review


class SourceParts(unittest.TestCase):
    def sheet(self):
        raw = bytearray(512 + 128*256)
        # Two asymmetric adjacent colours; nibble order is part of the format.
        raw[512] = 0x32
        return raw

    def test_parts_flips_and_order(self):
        spr = self.sheet()
        part = (0, -8, 0, 0, 8, 8, False, False)
        normal = review.compose(spr, [part])
        self.assertEqual(normal[88, 64:66].tolist(), [2, 3])
        flipped = review.compose(spr, [(*part[:6], True, True)])
        self.assertEqual(flipped[95, 70:72].tolist(), [3, 2])
        # First SHP part overlays the later one. Transparent pixels don't erase.
        result = review.compose(spr, [(-1, -8, *part[2:]), part])
        self.assertEqual(result[88, 63:66].tolist(), [2, 3, 3])

    def test_transparent_off_canvas_dummy(self):
        part = (0, -8, 0, 0, 8, 8, False, False)
        dummy = (124, 124, 240, 200, 8, 8, False, False)
        spr = self.sheet()
        self.assertTrue(np.array_equal(review.compose(spr, [part]),
                                       review.compose(spr, [dummy, part])))
        spr[512 + 200*128 + 120] = 1
        with self.assertRaisesRegex(ValueError, 'opaque'):
            review.compose(spr, [dummy, part])

    def test_shape_payload_bounds(self):
        data = bytearray(0x40a + 6)
        struct.pack_into('<H', data, 0x408, 6)
        struct.pack_into('<bbH', data, 0x40c, -4, -8, 0xc000)
        self.assertEqual(review.shape(data, 0), [(-4, -8, 0, 0, 8, 8, True, True)])
        # Offset zero is valid, even for a nonzero frame index.
        self.assertEqual(review.shape(data, 1), review.shape(data, 0))
        for bad in (data[:-1], data[:0x400]):
            with self.assertRaises(ValueError):
                review.shape(bad, 0)
        struct.pack_into('<I', data, 8, 0xffffffff)
        with self.assertRaises(ValueError):
            review.shape(data, 0)

    def test_sequence_rejects_missing_and_control(self):
        data = bytearray(0x408)
        struct.pack_into('<H', data, 0x404, 2)
        data[0x406:] = bytes((26, 0))
        self.assertEqual(review.first_frame(data, 52), (26, 0))
        data[0x406] = 255
        with self.assertRaisesRegex(ValueError, 'control'):
            review.first_frame(data, 52)
        struct.pack_into('<I', data, 4 + 4*52, 2)
        with self.assertRaisesRegex(ValueError, 'missing'):
            review.first_frame(data, 52)


if __name__ == '__main__':
    unittest.main()
