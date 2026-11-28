#!/usr/bin/env python3
"""The ring stack transcription, against the C it came from; the disc is not needed."""
import struct
import unittest

import effect_mesh as M


def entry(**kw):
    """A 196-byte section-3 record with the named fields set."""
    raw = bytearray(196)
    for name, value in kw.items():
        fmt, off = M.RingMeshView._F[name]
        struct.pack_into(fmt, raw, off, value)
    return bytes(raw)


FLAT = [[0x1000] * 9]          # one brightness row, 1.0 in 12-bit fixed


class Helpers(unittest.TestCase):
    def test_lerp_is_the_source_formula(self):
        self.assertEqual(M.lerp(10, 20, 0), 10)      # factor 0 keeps the start
        self.assertEqual(M.lerp(10, 20, 256), 20)    # 256 is 1.0
        self.assertEqual(M.lerp(0, 256, 128), 128)

    def test_curve_nibble_zero_is_no_curve(self):
        self.assertEqual(M.curve([[7, 8]], 0, 0), 0)
        self.assertEqual(M.curve([[7, 8]], 1, 1), 8)

    def test_a_whole_turn_is_4096(self):
        self.assertEqual(M.rcos(0), 4096)
        self.assertEqual(M.rsin(1024), 4096)         # a quarter turn
        self.assertEqual(M.rcos(4096), 4096)


class Lattice(unittest.TestCase):
    def stack(self, **kw):
        return M.RingStack(entry(**kw), FLAT)

    def test_nine_rows_of_eight_points(self):
        pts = self.stack(radius_start=100, radius_end=100).lattice(0)
        self.assertEqual(len(pts), M.RING_ROWS)
        self.assertTrue(all(len(r) == M.RING_SEGMENTS for r in pts))

    def test_points_sit_on_the_ring_45_degrees_apart(self):
        pts = self.stack(radius_start=100, radius_end=100).lattice(0)
        # spin 0: point 0 is (r, y, 0) and the even points walk the quadrants.
        self.assertEqual(pts[0][0][0], 100)
        self.assertEqual(pts[0][0][2], 0)
        self.assertEqual(pts[0][2], (0, 0, 100))
        self.assertEqual(pts[0][4], (-100, 0, 0))
        self.assertEqual(pts[0][6], (0, 0, -100))
        # the odd ones are the same ring at +0x200, which is 45 degrees
        self.assertEqual(pts[0][1][0], pts[0][1][2])

    def test_rows_climb_by_their_own_step(self):
        """y is n >> 8, and n gathers height + height_step once a row."""
        pts = self.stack(radius_start=10, radius_end=10,
                         height_start=0x100, height_end=0x100,
                         height_step_start=0, height_step_end=0).lattice(0)
        ys = [r[0][1] for r in pts]
        self.assertEqual(ys[0], 0)
        self.assertEqual(ys[1] - ys[0], 1)        # 0x100 >> 8
        self.assertEqual(ys[8], 8)

    def test_the_spin_wraps_at_a_whole_turn(self):
        s = self.stack(radius_start=10, radius_end=10,
                       spin_speed_start=0x900, spin_speed_end=0x900)
        s.lattice(0)
        self.assertEqual(s.spin, 0x900)
        s.lattice(0)
        self.assertEqual(s.spin, (0x900 * 2) & 0xfff)   # 0x1200 -> 0x200


class Quads(unittest.TestCase):
    @staticmethod
    def flat(x, y, z):
        return x, y, 64        # a camera that keeps everything at one depth

    def test_sixty_four_quads_and_the_ring_closes(self):
        s = M.RingStack(entry(radius_start=100, radius_end=100,
                              height_start=0x100, height_end=0x100), FLAT)
        q = s.quads(0, self.flat)
        self.assertEqual(len(q), M.RING_SEGMENTS * M.RING_SEGMENTS)
        # the last segment of ring 0 joins point 7 back to point 0
        last = q[M.RING_SEGMENTS - 1]
        self.assertEqual(len(last['pts']), 4)

    def test_neutral_colour_without_the_curve_flag(self):
        s = M.RingStack(entry(radius_start=10, radius_end=10), FLAT)
        self.assertEqual(s.colour(0), (0x80, 0x80, 0x80))
        q = s.quads(0, self.flat)
        self.assertEqual(q[0]['rgb'][0], (0x80, 0x80, 0x80))   # 0x1000 * 0x80 >> 12

    def test_a_dim_brightness_row_dims_every_vertex(self):
        s = M.RingStack(entry(radius_start=10, radius_end=10), [[0x800] * 9])
        q = s.quads(0, self.flat)
        self.assertEqual(q[0]['rgb'][0], (0x40, 0x40, 0x40))

    def test_abe_follows_the_texture_page_flags(self):
        self.assertTrue(M.RingStack(entry(texture_page_flags=1), FLAT).e.texture_page_flags & 3)
        s = M.RingStack(entry(radius_start=10, radius_end=10, texture_page_flags=4), FLAT)
        self.assertFalse(s.quads(0, self.flat)[0]['abe'])      # bit 2 clears ABE

    def test_quads_come_back_farthest_first(self):
        def wedge(x, y, z):
            return x, y, 64 + z
        s = M.RingStack(entry(radius_start=100, radius_end=100,
                              height_start=0x100, height_end=0x100), FLAT)
        d = [q['depth'] for q in s.quads(0, wedge)]
        self.assertEqual(d, sorted(d, reverse=True))


if __name__ == '__main__':
    unittest.main()
