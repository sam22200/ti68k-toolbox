#!/usr/bin/env python3
"""The animation decoder's grammar, on bytes built here; the disc is not needed."""
import struct
import unittest

import effect_anim as A
import effect_rip as R


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



class EmbeddedData(unittest.TestCase):
    """110 effect files are MIPS overlays that carry their data behind the code."""

    @staticmethod
    def built(prefix):
        """`prefix` bytes, then a ten-word header and enough palette and pixels."""
        body = bytes(0x400) + b'\x00\x00\x00\x01' + bytes(R.PALETTE_BYTES - 0x404 + 128)
        head = struct.pack('<10I', 40, 44, 48, 52, 56, 60, 64, 68, 72, 76)
        pad = bytes(76 - 40)                       # sections 0..8, one word each
        return prefix + head + pad + body

    def test_plain_file_starts_at_zero(self):
        self.assertEqual(R.data_base(self.built(b'')), 0)

    def test_header_found_behind_code(self):
        code = struct.pack('<I', 0x27bdf980) * 64   # addiu $sp, $sp, -0x680, over and over
        self.assertEqual(R.data_base(self.built(code)), len(code))

    def test_code_without_data_is_none(self):
        self.assertIsNone(R.data_base(struct.pack('<I', 0x27bdf980) * 64))

    def test_a_bare_forty_is_not_a_header(self):
        """The word 40 alone does not open a file: the nine offsets must hold up."""
        bad = struct.pack('<10I', 40, 9, 8, 7, 6, 5, 4, 3, 2, 1) + bytes(4096)
        self.assertIsNone(R.data_base(bad))


class Blend(unittest.TestCase):
    """The GPU's own compositing: 5 bits a channel, saturated per primitive."""

    @staticmethod
    def one(back, front, abe, mode):
        import numpy as np
        fb = np.full((1, 1, 3), float(back))
        prim = np.full((1, 1, 3), float(front))
        A._blend(fb, prim, np.ones((1, 1)), abe, mode)
        return fb[0, 0, 0]

    def test_opaque_replaces(self):
        self.assertEqual(self.one(200, 64, False, 1), 64)
        self.assertEqual(self.one(200, 64, False, 3), 64)   # the mode is not consulted

    def test_add_saturates_at_five_bits(self):
        self.assertEqual(self.one(128, 64, True, 1), 192)
        self.assertEqual(self.one(200, 200, True, 1), 248)  # 25 + 25 -> 31, not 50

    def test_half_and_quarter(self):
        self.assertEqual(self.one(128, 64, True, 0), 96)    # (16 + 8) / 2 -> 12
        self.assertEqual(self.one(128, 64, True, 3), 144)   # 16 + 8/4 -> 18

    def test_subtract_floors_at_zero(self):
        self.assertEqual(self.one(64, 200, True, 2), 0)

    def test_nothing_drawn_where_the_primitive_is_not_covered(self):
        import numpy as np
        fb = np.full((1, 1, 3), 72.0)
        A._blend(fb, np.full((1, 1, 3), 255.0), np.zeros((1, 1)), True, 1)
        self.assertEqual(fb[0, 0, 0], 72.0)


class Depth(unittest.TestCase):
    """The ordering-table index of a sprite, and the modes that override it."""

    def test_the_default_is_the_projected_z_over_four(self):
        self.assertEqual(A.sprite_depth(400, 0), 100)

    def test_the_two_nearer_modes_pull_it_forward(self):
        self.assertEqual(A.sprite_depth(400, 1), 92)
        self.assertEqual(A.sprite_depth(400, 5), 84)

    def test_the_fixed_modes_ignore_the_position(self):
        self.assertEqual(A.sprite_depth(400, 2), 8)
        self.assertEqual(A.sprite_depth(400, 4), 0x10)
        self.assertEqual(A.sprite_depth(4, 3), A.SPRITE_DEPTH_LIMIT)

    def test_a_negative_index_drops_the_sprite(self):
        """`if (depth < 0) return;` -- it is not clamped, it is not drawn."""
        self.assertIsNone(A.sprite_depth(4, 1))
        self.assertIsNone(A.sprite_depth(-40, 0))

    def test_it_clamps_at_the_far_end(self):
        self.assertEqual(A.sprite_depth(0x10000, 0), A.SPRITE_DEPTH_LIMIT)


class Rotation(unittest.TestCase):
    """The 2D matrix submit_sprite builds from the node's screen angle."""

    def test_no_angle_leaves_the_corners_alone(self):
        pts = [(1, 2), (3, 4)]
        self.assertIs(A.rotate2(pts, 0.0), pts)

    def test_a_quarter_turn(self):
        import math
        (x, y), = A.rotate2([(10, 0)], math.pi / 2)
        self.assertAlmostEqual(x, 0, places=6)
        self.assertAlmostEqual(y, 10, places=6)

    def test_it_turns_every_corner_about_the_sprite_origin(self):
        import math
        out = A.rotate2([(1, 0), (0, 1), (-1, 0), (0, -1)], math.pi)
        for (x, y), (ex, ey) in zip(out, [(-1, 0), (0, -1), (1, 0), (0, 1)]):
            self.assertAlmostEqual(x, ex, places=6)
            self.assertAlmostEqual(y, ey, places=6)


if __name__ == '__main__':
    unittest.main()
