#!/usr/bin/env python3
"""extract.terrain(): which walk-2 tile is decoration, on synthetic terrain.

FFT's walk-2 flag says "you cannot stop here", not "this is decoration": you cannot
stop on a wall either, and reading the flag alone took the Chapel of Orbonne's whole
perimeter for props. A prop must also rise above the ground around it and stand alone
among the rising flagged tiles. A tile with no second terrain level carries eight zero
bytes and must not be read as an ordinary tile of height 0.

These fixtures carry no commercial data.
"""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import extract   # noqa: E402

TERR = 0x68
HEAD = 0xC0
ORDINARY, BLOCKED, FLAGGED = 0x00, 0x01, 0x02


def mesh(nx, nz, level0, level1=None):
    """A mesh carrying only a terrain chunk: {(x, z): (height, flags)} per level."""
    head = bytearray(HEAD)
    struct.pack_into('<I', head, TERR, HEAD)
    body = bytearray([nx, nz])
    for level in (level0, level1):
        for i in range(nx * nz):
            spec = None if level is None else level.get((i % nx, i // nx))
            if spec is None:
                body += bytes(8)            # this tile has no such level
                continue
            h, flags = spec
            e = bytearray(8)
            e[2], e[6] = h, flags
            body += e
    return bytes(head) + bytes(body)


def ground(nx, nz, h=3):
    return {(x, z): (h, ORDINARY) for x in range(nx) for z in range(nz)}


class PropRule(unittest.TestCase):
    def props(self, m, nx, nz):
        return sorted((x, z) for x, z, _ in extract.terrain(m, nx, nz)[2])

    def test_a_lone_rising_flagged_tile_is_a_prop(self):
        """Sweegy's tree: one tile of height 18 over ground near 3."""
        g = ground(5, 5)
        g[(2, 2)] = (18, FLAGGED)
        self.assertEqual(self.props(mesh(5, 5, g), 5, 5), [(2, 2)])

    def test_a_run_of_flagged_tiles_is_a_wall(self):
        """Chapel of Orbonne: 46 flagged tiles in one 4-connected component."""
        g = ground(5, 5)
        for z in range(5):
            g[(0, z)] = (19, FLAGGED)
        self.assertEqual(self.props(mesh(5, 5, g), 5, 5), [])

    def test_a_wall_corner_is_a_wall_on_both_arms(self):
        g = ground(5, 5)
        for z in range(3):
            g[(0, z)] = (19, FLAGGED)
        for x in range(3):
            g[(x, 0)] = (19, FLAGGED)
        self.assertEqual(self.props(mesh(5, 5, g), 5, 5), [])

    def test_a_short_run_in_the_middle_of_the_room_is_furniture(self):
        """Book Storage: three 2x1 bookcases away from the walls."""
        g = ground(6, 6)
        g[(2, 2)] = (18, FLAGGED)
        g[(3, 2)] = (18, FLAGGED)
        self.assertEqual(self.props(mesh(6, 6, g), 6, 6), [(2, 2), (3, 2)])

    def test_a_lone_tile_at_the_map_edge_is_still_a_prop(self):
        """Araguay: three of its five trees grow at the very edge of the map."""
        g = ground(5, 5)
        g[(0, 0)] = (17, FLAGGED)
        self.assertEqual(self.props(mesh(5, 5, g), 5, 5), [(0, 0)])

    def test_a_run_reaching_the_edge_is_the_enclosure(self):
        """Book Storage's back wall spans the full width of the map."""
        g = ground(6, 6)
        for x in range(6):
            g[(x, 0)] = (18, FLAGGED)
        self.assertEqual(self.props(mesh(6, 6, g), 6, 6), [])

    def test_two_props_apart_are_both_props(self):
        g = ground(5, 5)
        g[(1, 1)] = (18, FLAGGED)
        g[(3, 3)] = (17, FLAGGED)
        self.assertEqual(self.props(mesh(5, 5, g), 5, 5), [(1, 1), (3, 3)])

    def test_a_flagged_tile_level_with_the_ground_carries_nothing(self):
        g = ground(5, 5)
        g[(2, 2)] = (3, FLAGGED)
        self.assertEqual(self.props(mesh(5, 5, g), 5, 5), [])

    def test_a_flat_flagged_neighbour_does_not_hide_a_trunk(self):
        """Araguay: two flagged tiles level with the ground sit beside a trunk."""
        g = ground(5, 5)
        g[(2, 2)] = (17, FLAGGED)
        g[(3, 2)] = (3, FLAGGED)
        g[(2, 3)] = (3, FLAGGED)
        self.assertEqual(self.props(mesh(5, 5, g), 5, 5), [(2, 2)])

    def test_an_absent_second_level_is_not_ground_at_height_zero(self):
        """Map 3 carries 117 all-zero level-1 entries of 117."""
        g = ground(5, 5, h=6)
        g[(2, 2)] = (18, FLAGGED)
        walk, top, props = extract.terrain(mesh(5, 5, g, None), 5, 5)
        self.assertEqual([t for t in top if t is not None], [6] * 24)
        self.assertEqual(sorted((x, z) for x, z, _ in props), [(2, 2)])

    def test_a_real_second_level_still_raises_the_standable_top(self):
        g = ground(5, 5, h=3)
        up = {(1, 1): (9, ORDINARY)}
        walk, top, props = extract.terrain(mesh(5, 5, g, up), 5, 5)
        self.assertEqual(top[1 * 5 + 1], 9)

    def test_a_blocked_tile_is_neither_ground_nor_prop(self):
        g = ground(5, 5)
        g[(2, 2)] = (19, BLOCKED | FLAGGED)
        walk, top, props = extract.terrain(mesh(5, 5, g), 5, 5)
        self.assertEqual(props, [])
        self.assertIsNone(top[2 * 5 + 2])


class ApprovedDrops(unittest.TestCase):
    """extract.PROPS_DROPPED: the authoring decision, applied by extract.views()."""

    def test_a_map_with_no_approved_drop_is_untouched(self):
        g = ground(5, 5)
        g[(2, 2)] = (18, FLAGGED)
        self.assertEqual(extract.dropped_props(mesh(5, 5, g), 5, 5, m=22), [])

    def test_an_approved_tile_drops_the_polygons_over_it(self):
        g = ground(5, 5)
        g[(2, 2)] = (18, FLAGGED)
        m = mesh(5, 5, g)
        _, top, props = extract.terrain(m, 5, 5)
        want = extract.prop_polygons(extract.polygons(m), top, props, 5, 5)
        try:
            extract.PROPS_DROPPED[999] = [(2, 2)]
            self.assertEqual(extract.dropped_props(m, 5, 5, m=999), want)
        finally:
            del extract.PROPS_DROPPED[999]

    def test_a_tile_that_carries_no_prop_is_refused(self):
        """A drop that silently matched nothing would ship the map over budget."""
        g = ground(5, 5)
        g[(2, 2)] = (18, FLAGGED)
        try:
            extract.PROPS_DROPPED[999] = [(4, 4)]
            with self.assertRaises(ValueError):
                extract.dropped_props(mesh(5, 5, g), 5, 5, m=999)
        finally:
            del extract.PROPS_DROPPED[999]

    def test_every_approved_map_is_one_the_census_knows(self):
        self.assertEqual(sorted(extract.PROPS_DROPPED), [5, 50, 74, 80])


class ReviewTool(unittest.TestCase):
    """cosmetic_prop_review calls into the rule; moving it must not break it silently."""

    def test_shrink_props_scales_a_prop_about_its_tile(self):
        import cosmetic_prop_review as review
        g = ground(5, 5)
        g[(2, 2)] = (18, FLAGGED)
        m = mesh(5, 5, g)
        polys = extract.polygons(m)
        _, top, props = extract.terrain(m, 5, 5)
        self.assertEqual(len(review.shrink_props(polys, top, props, 5, 5, 0.5)), len(polys))


if __name__ == '__main__':
    unittest.main()
