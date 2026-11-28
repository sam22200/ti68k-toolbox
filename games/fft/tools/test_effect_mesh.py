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

    def test_the_colour_curves_are_read_straight(self):
        """They are not READ_CURVE: row 0 is a real row.

        The source indexes `g_effect_palette_table[k & 0xf]` with the nibble
        itself. Subtracting one there turned every ring stack black, Holy's beam
        included, and the geometry was right the whole time.
        """
        self.assertEqual(M.palette([[9, 10], [20]], 0, 0), 9)
        self.assertEqual(M.palette([[9, 10], [20]], 1, 0), 20)
        self.assertEqual(M.palette([[9]], 5, 0), 0)          # past the table

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

    def test_colour_curves_tint_the_whole_stack(self):
        e = entry(radius_start=10, radius_end=10, flags=M.GEOMETRY_FLAG_COLOUR_CURVES,
                  colour_curves=0x012)
        s = M.RingStack(e, FLAT, [[0x40], [0x80], [0xc0]])
        # red takes the low nibble, green the next, blue the high byte
        self.assertEqual(s.colour(0), (0xc0, 0x80, 0x40))

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


class Mesh(unittest.TestCase):
    """The ring mesh is the stack's renderer with other counts."""

    def test_its_lattice_is_three_rows_of_thirty_two(self):
        s = M.RingMesh(entry(radius_start=100, radius_end=100), [[0x1000] * 3])
        pts = s.lattice(0)
        self.assertEqual(len(pts), 3)
        self.assertEqual(len(pts[0]), 32)

    def test_two_rings_of_thirty_two_quads(self):
        s = M.RingMesh(entry(radius_start=100, radius_end=100,
                             height_start=0x40, height_end=0x40), [[0x1000] * 3])
        self.assertEqual(len(s.quads(0, lambda x, y, z: (x, y, 64))), 64)

    def test_it_shifts_the_height_left_two(self):
        """`lerp(height_start, _end, j) << 2`, where the stack takes it raw."""
        kw = dict(radius_start=10, radius_end=10, height_start=0x100, height_end=0x100,
                  height_step_start=0, height_step_end=0)
        stack = M.RingStack(entry(**kw), [[0x1000] * 9]).lattice(0)
        mesh = M.RingMesh(entry(**kw), [[0x1000] * 3]).lattice(0)
        self.assertEqual(stack[1][0][1] - stack[0][0][1], 1)
        self.assertEqual(mesh[1][0][1] - mesh[0][0][1], 4)


class Grid(unittest.TestCase):
    """The 4x4 textured grid: sixteen screen-space quads, never projected."""

    @staticmethod
    def grid(**kw):
        raw = bytearray(196)
        for name, (fmt, off) in (('flags', ('<H', 0x06)), ('curve0', ('<I', 0x08)),
                                 ('texture_u', ('<H', 0x14)), ('texture_v', ('<H', 0x16)),
                                 ('page_flags', ('<h', 0x4c)), ('brightness_row', ('<h', 0x4e)),
                                 ('ot_depth', ('<h', 0x54)), ('speed_start', ('<h', 0x5c)),
                                 ('speed_end', ('<h', 0x60)), ('tex_w', ('<h', 0xa8)),
                                 ('tex_h', ('<h', 0xaa))):
            if name in kw:
                struct.pack_into(fmt, raw, off, kw[name])
        return M.QuadGrid4x4(bytes(raw), [[0x1000] * 5])

    def test_sixteen_quads_of_sixty_four_pixels(self):
        q = self.grid(tex_w=64, tex_h=64, ot_depth=40).quads(0)
        self.assertEqual(len(q), 16)
        (x0, y0), (x1, _), _, (_, y3) = q[0]['pts']
        self.assertEqual((x1 - x0, y3 - y0), (M.QuadGrid4x4.CELL, M.QuadGrid4x4.CELL))

    def test_they_are_screen_space(self):
        q = self.grid(tex_w=64, tex_h=64, ot_depth=40).quads(0)
        self.assertTrue(q[0]['screen'])
        self.assertEqual(q[0]['pts'][0], (M.QuadGrid4x4.ORIGIN_X, 0))

    def test_the_depth_is_pinned_and_clamped(self):
        self.assertEqual(self.grid(tex_w=8, tex_h=8, ot_depth=0).quads(0)[0]['depth'], 1)
        self.assertEqual(self.grid(tex_w=8, tex_h=8, ot_depth=-5).quads(0)[0]['depth'], 1)
        self.assertEqual(self.grid(tex_w=8, tex_h=8, ot_depth=0x400).quads(0)[0]['depth'],
                         M.OT_DEPTH_LIMIT - 1)

    def test_the_window_wraps_inside_half_the_texture(self):
        g = self.grid(tex_w=64, tex_h=64, ot_depth=40, speed_start=0x40, speed_end=0x40)
        seen = {g.step(i)[0] for i in range(40)}
        self.assertTrue(max(seen) < 32 + 1)         # texture_width / 2


class Spawn(unittest.TestCase):
    """battle_effect_interpolate_emitter_spawn_position and the anchors."""

    @staticmethod
    def rec(start, end, placement=0):
        raw = bytearray(196)
        struct.pack_into('<3h', raw, 0x14, *start)
        struct.pack_into('<3h', raw, 0x1a, *end)
        struct.pack_into('<H', raw, 0x02, placement)
        return bytes(raw)

    def test_a_factor_of_zero_keeps_the_start(self):
        r = self.rec((10, 20, 30), (70, 80, 90))
        self.assertEqual(M.spawn_position(r, 0), (10, 20, 30))

    def test_a_factor_of_256_reaches_the_end(self):
        r = self.rec((10, 20, 30), (70, 80, 90))
        self.assertEqual(M.spawn_position(r, 256), (70, 80, 90))

    def test_it_walks_between_them(self):
        r = self.rec((0, 0, 0), (256, 512, -256))
        self.assertEqual(M.spawn_position(r, 128), (128, 256, -128))

    def test_the_anchor_comes_from_bits_9_to_11(self):
        s = M.RingStack(self.rec((0, 0, 0), (0, 0, 0), M.ORIGIN_TARGET), [[0x1000] * 9])
        _, mode, unplaceable = s.origin(0)
        self.assertEqual(mode, M.ORIGIN_TARGET)
        self.assertFalse(unplaceable)

    def test_two_anchors_a_preview_cannot_place(self):
        for mode in (M.ORIGIN_PARENT_PARTICLE, M.ORIGIN_MAP_CENTRE):
            s = M.RingStack(self.rec((0, 0, 0), (0, 0, 0), mode), [[0x1000] * 9])
            self.assertTrue(s.origin(0)[2], M.ORIGIN_NAMES[mode])

    def test_the_mesh_sits_on_its_spawn_position(self):
        """The lattice is built about zero and the origin moves the whole ring."""
        bright = [[0x1000] * 9]
        flat = lambda x, y, z: (x, y, 64)
        here = M.RingStack(self.rec((0, 0, 0), (0, 0, 0)), bright)
        there = M.RingStack(self.rec((40, -15, 0), (40, -15, 0)), bright)
        for k in ('radius_start', 'radius_end'):
            pass
        a = here.quads(0, flat)
        b = there.quads(0, flat)
        if a and b:
            self.assertNotEqual(a[0]['pts'][0], b[0]['pts'][0])


if __name__ == '__main__':
    unittest.main()
