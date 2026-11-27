#!/usr/bin/env python3
"""The animation decoder's grammar, on bytes built here; the disc is not needed."""
import struct
import unittest

import effect_anim as A


def poly(u, v, w, h, x, y):
    """One 24-byte polygon: page, palette, cell, then the four corners."""
    return (struct.pack('<2H', 0x02a0, 0x00a6) + struct.pack('<2B2b', u, v, w, h)
            + struct.pack('<8h', x, y, x + abs(w), y, x, y + abs(h),
                          x + abs(w), y + abs(h)))


def section0(records):
    """An offset table that points four bytes short of each record, as the disc does."""
    head = 4 + 2 * (len(records) + 1)        # two head words, then a blank and the rest
    body, offsets, at = b'', [2], head
    for r in records:
        offsets.append(at - 4)
        body += r
        at += len(r)
    return struct.pack('<2H', 1, 0) + struct.pack('<%dH' % len(offsets), *offsets) + body


def section1(streams):
    tbl_end = 4 + 2 * len(streams)
    offsets, body, at = [], b'', tbl_end
    for s in streams:
        offsets.append(at - 4)
        body += s
        at += len(s)
    return (struct.pack('<2H', len(streams), 0)
            + struct.pack('<%dH' % len(offsets), *offsets) + body)


def section3(seqs):
    out = struct.pack('<2H', 2, len(seqs)) + bytes(16)
    for s in seqs:
        out += bytes([0, s]) + bytes(A.REC_SIZE - 2)
    return out


class Frames(unittest.TestCase):
    def test_one_polygon_a_frame(self):
        fr = A.frames(section0([struct.pack('<2H', 0x02a0, 1) + poly(56, 8, 48, 104, -22, -7)]))
        self.assertEqual(fr[0], [])                       # index 0 is the blank
        self.assertEqual(len(fr[1]), 1)
        p = fr[1][0]
        self.assertEqual((p['u'], p['v'], p['w'], p['h']), (56, 8, 48, 104))
        self.assertEqual(p['pts'], [(-22, -7), (26, -7), (-22, 97), (26, 97)])

    def test_four_corners_not_three(self):
        """A record is 4 + 24n. Reading three corners drops the fourth and the cell."""
        rec = struct.pack('<2H', 0x02a0, 3) + b''.join(
            poly(8, 8, 24, 32, 0, 0) for _ in range(3))
        self.assertEqual(len(rec), 4 + 24 * 3)
        fr = A.frames(section0([rec]))
        self.assertEqual(len(fr[1]), 3)

    def test_a_negative_size_mirrors_the_cell(self):
        fr = A.frames(section0([struct.pack('<2H', 0x02a0, 1) + poly(39, 112, -32, 112, 0, 0)]))
        self.assertEqual(fr[1][0]['w'], -32)              # not 224, which is off the atlas


class Script(unittest.TestCase):
    def test_the_four_opcodes(self):
        stream = (bytes([0x82]) + struct.pack('<2h', 0, -276)
                  + bytes([2, 2, 0]) + bytes([0x83, 0, 32])
                  + bytes([3, 30, 0]) + bytes([0x83]) + struct.pack('<2b', -8, -2)
                  + bytes([0x81]))
        ops = A.script(section1([stream]))[0]
        self.assertEqual(ops, [('moveto', 0, -276), ('frame', 2, 2, 0), ('moveby', 0, 32),
                               ('frame', 3, 30, 0), ('moveby', -8, -2), ('end',)])

    def test_play_accumulates_the_moves_and_the_clock(self):
        stream = (bytes([0x82]) + struct.pack('<2h', 10, -100)
                  + bytes([1, 2, 0]) + bytes([0x83, 0, 32])
                  + bytes([1, 4, 0]) + bytes([0x83, 0, 32])
                  + bytes([1, 0, 0]) + bytes([0x81]))
        draws = A.play(A.script(section1([stream]))[0], 8)
        self.assertEqual([(d['t'], d['y']) for d in draws],
                         [(0, -100), (2, -68), (6, -36)])
        self.assertEqual({d['x'] for d in draws}, {10})

    def test_a_frame_past_the_table_is_not_drawn(self):
        stream = bytes([9, 2, 0]) + bytes([0x81])
        self.assertEqual(A.play(A.script(section1([stream]))[0], 4), [])


class Cast(unittest.TestCase):
    def test_a_sequence_can_be_cast_twice(self):
        cs = A.cast(section3([0, 3, 3]))
        self.assertEqual([c['seq'] for c in cs], [0, 3, 3])
        self.assertEqual(len(cs[0]['raw']), A.REC_SIZE)

    def test_a_wrong_size_is_refused_rather_than_guessed(self):
        self.assertEqual(A.cast(section3([0]) + b'\0'), [])

    def test_the_timeline_merges_the_cast_in_tick_order(self):
        rec = struct.pack('<2H', 0x02a0, 1) + poly(0, 0, 8, 8, 0, 0)
        fr = A.frames(section0([rec, rec]))
        seqs = A.script(section1([
            bytes([1, 4, 0]) + bytes([2, 2, 0]) + bytes([0x81]),
            bytes([2, 2, 0]) + bytes([0x81])]))
        tl = A.timeline(fr, seqs, A.cast(section3([0, 1])))
        self.assertEqual([(d['t'], d['inst'], d['frame']) for d in tl],
                         [(0, 0, 1), (0, 1, 2), (4, 0, 2)])


if __name__ == '__main__':
    unittest.main()
