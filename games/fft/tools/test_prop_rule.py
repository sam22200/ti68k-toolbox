#!/usr/bin/env python3
"""depth_budget.terrain(): which walk-2 tile is decoration, on synthetic terrain.

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
import depth_budget   # noqa: E402

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
        return sorted((x, z) for x, z, _ in depth_budget.terrain(m, nx, nz)[2])

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
        walk, top, props = depth_budget.terrain(mesh(5, 5, g, None), 5, 5)
        self.assertEqual([t for t in top if t is not None], [6] * 24)
        self.assertEqual(sorted((x, z) for x, z, _ in props), [(2, 2)])

    def test_a_real_second_level_still_raises_the_standable_top(self):
        g = ground(5, 5, h=3)
        up = {(1, 1): (9, ORDINARY)}
        walk, top, props = depth_budget.terrain(mesh(5, 5, g, up), 5, 5)
        self.assertEqual(top[1 * 5 + 1], 9)

    def test_a_blocked_tile_is_neither_ground_nor_prop(self):
        g = ground(5, 5)
        g[(2, 2)] = (19, BLOCKED | FLAGGED)
        walk, top, props = depth_budget.terrain(mesh(5, 5, g), 5, 5)
        self.assertEqual(props, [])
        self.assertIsNone(top[2 * 5 + 2])


if __name__ == '__main__':
    unittest.main()
