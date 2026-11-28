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
        """0x81 is LOOP, not an end: a sequence repeats until the timeline stops it."""
        stream = (bytes([0x82]) + struct.pack('<2h', 0, -276)
                  + bytes([2, 2, 0]) + bytes([0x83, 0, 32])
                  + bytes([3, 30, 0]) + bytes([0x83]) + struct.pack('<2b', -8, -2)
                  + bytes([0x81]))
        ops = A.script(section1([stream]))[0]
        self.assertEqual(ops, [('moveto', 0, -276), ('frame', 2, 2, 0), ('moveby', 0, 32),
                               ('frame', 3, 30, 0), ('moveby', -8, -2), ('loop',)])

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


class EffectScript(unittest.TestCase):          # section 2, not the sprite sequences
    def test_widths_come_from_the_handlers(self):
        """A jump is 4 bytes, a counter test 6, a camera move 8, a bare op 2."""
        code = (struct.pack('<2H', 0x27, 0)[:2]          # init_particle_physics
                + struct.pack('<2H', 0x1f, 0x22)         # jump_if_targets_eq -> 0x22
                + struct.pack('<3H', 0x11, 3, 0x0e)      # jump_if_counter_eq 3 -> 0x0e
                + struct.pack('<4H', 0x08, 1, 2, 3)      # camera_position
                + struct.pack('<2H', 0x04, 0))           # end
        got = [(o, n, ops) for o, _, n, _, ops in A.program(code)]
        self.assertEqual([g[0] for g in got], [0, 2, 6, 12, 20])
        self.assertEqual([g[1] for g in got],
                         ['init_particle_physics', 'jump_if_targets_eq',
                          'jump_if_counter_eq', 'camera_position', 'end'])
        self.assertEqual(got[2][2], [3, 14])

    def test_it_stops_at_end(self):
        code = struct.pack('<2H', 0x04, 0) + b'\xff' * 8
        self.assertEqual(len(A.program(code)), 1)


class Timeline(unittest.TestCase):
    def build(self, phase1, child, phase2, delay, keyframes):
        buf = bytearray(6278)
        struct.pack_into('<4H', buf, 0, 0, phase2, phase1, delay)
        struct.pack_into('<H', buf, 0x00a, child)
        for k, (start, end, emitter, action) in enumerate(keyframes, 1):
            struct.pack_into('<H', buf, 0x00c + 2 * (k - 1), start)
            struct.pack_into('<H', buf, 0x00c + 2 * k, end)
            buf[0x00c + 0x32 + k - 1] = emitter + 1
            struct.pack_into('<H', buf, 0x00c + 0x4a + 2 * k, action)
        struct.pack_into('<h', buf, 0x00c + 0x7e, len(keyframes))
        return bytes(buf)

    def test_durations_and_the_spawn_delay(self):
        t = A.phases(self.build(11, 55, 20, 50, [(2, 4, 2, 0)]))
        self.assertEqual((t['phase1'], t['child'], t['phase2']), (11, 55, 20))
        self.assertEqual(t['spawn_delay'], 50)
        self.assertEqual(t['total'], 86)

    def test_a_channel_gives_its_emitters_and_frames(self):
        t = A.phases(self.build(0, 10, 0, 0, [(2, 4, 2, 0), (4, 6, 3, 0x40)]))
        c = t['child_channels'][0]
        self.assertEqual([(k['frm'], k['to'], k['emitter']) for k in c],
                         [(2, 4, 2), (4, 6, 3)])
        self.assertEqual(A.action_text(c[1]['action']), 'target animation')

    def test_a_short_section_is_refused(self):
        self.assertIsNone(A.phases(b'\0' * 100))


class Scheduled(unittest.TestCase):
    """The timeline drives the playback: when an emitter starts, and for how long."""

    def emitter(self, seq, count=1, interval=1, life=-1):
        r = bytearray(A.REC_SIZE)
        r[1] = seq
        struct.pack_into('<h', r, 0x94, life)
        struct.pack_into('<h', r, 0xb0, count)
        struct.pack_into('<h', r, 0xb4, interval)
        return bytes(r)

    def setUp(self):
        rec = struct.pack('<2H', 0x02a0, 1) + poly(0, 0, 8, 8, 0, 0)
        self.fr = A.frames(section0([rec, rec, rec]))
        self.seqs = A.script(section1([
            bytes([1, 2, 0]) + bytes([0x81]),
            bytes([2, 2, 0]) + bytes([0x81])]))

    def timeline(self, phase1, child, keyframes):
        buf = bytearray(6278)
        struct.pack_into('<4H', buf, 0, 0, 0, phase1, 0)
        struct.pack_into('<H', buf, 0x00a, child)
        for k, (start, end, em) in enumerate(keyframes, 1):
            struct.pack_into('<H', buf, 0x00c + 2 * (k - 1), start)
            struct.pack_into('<H', buf, 0x00c + 2 * k, end)
            buf[0x00c + 0x32 + k - 1] = em + 1
        struct.pack_into('<h', buf, 0x00c + 0x7e, len(keyframes))
        return A.phases(bytes(buf))

    def test_an_emitter_starts_at_its_keyframe_after_phase_one(self):
        cs = [dict(seq=0, raw=self.emitter(0)), dict(seq=1, raw=self.emitter(1))]
        tl = self.timeline(5, 10, [(2, 3, 1)])
        got = A.schedule(self.fr, self.seqs, cs, tl)
        # phase 1 is 5 frames, the keyframe starts at child frame 2: tick (5+2)*2
        self.assertEqual(got[0]['t'], 14)
        self.assertEqual(got[0]['frame'], 2)        # emitter 1 plays sequence 1

    def test_it_loops_until_the_lifetime_runs_out(self):
        cs = [dict(seq=0, raw=self.emitter(0, life=4))]
        tl = self.timeline(0, 20, [(0, 1, 0)])
        got = A.schedule(self.fr, self.seqs, cs, tl)
        # a 2-tick sequence looping over 4 frames = 8 ticks: four draws
        self.assertEqual([d['t'] for d in got], [0, 2, 4, 6])

    def test_an_endless_particle_stops_with_the_child_timeline(self):
        cs = [dict(seq=0, raw=self.emitter(0, life=-1))]
        tl = self.timeline(0, 3, [(0, 1, 0)])
        got = A.schedule(self.fr, self.seqs, cs, tl)
        self.assertEqual([d['t'] for d in got], [0, 2, 4])

    def test_no_timeline_means_no_schedule(self):
        self.assertIsNone(A.schedule(self.fr, self.seqs, [], None))


if __name__ == '__main__':
    unittest.main()
