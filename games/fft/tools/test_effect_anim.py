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
        # The cell table starts at `fd + *(s16*)fd`, so the first record IS cell
        # 0: there is no blank in front of it.
        self.assertEqual(len(fr), 1)
        self.assertEqual(len(fr[0]), 1)
        p = fr[0][0]
        self.assertEqual((p['u'], p['v'], p['w'], p['h']), (56, 8, 48, 104))
        self.assertEqual(p['pts'], [(-22, -7), (26, -7), (-22, 97), (26, 97)])

    def test_four_corners_not_three(self):
        """A record is 4 + 24n. Reading three corners drops the fourth and the cell."""
        rec = struct.pack('<2H', 0x02a0, 3) + b''.join(
            poly(8, 8, 24, 32, 0, 0) for _ in range(3))
        self.assertEqual(len(rec), 4 + 24 * 3)
        fr = A.frames(section0([rec]))
        self.assertEqual(len(fr[0]), 3)

    def test_the_size_is_unsigned_and_wraps_to_mirror(self):
        """`quad->u1 = quad->u0 + frame->width` is written into a u8, so a width
        that carries past 255 gives u1 < u0 and the cell interpolates backwards.
        That wrap is the mirroring -- the field itself is never negative."""
        fr = A.frames(section0([struct.pack('<2H', 0x02a0, 1) + poly(39, 112, -32, 112, 0, 0)]))
        self.assertEqual(fr[0][0]['w'], 224)
        u, w = fr[0][0]['u'], fr[0][0]['w']
        self.assertEqual((u + w) & 0xff, 7)               # 39 + 224 wraps to 7
        self.assertLess((u + w) & 0xff, u)                # so it runs backwards


class Physics(unittest.TestCase):
    """The section 3 header, which opcode 0x27 copies into the integrator."""

    def test_the_header_carries_gravity_and_the_threshold(self):
        sec3 = (struct.pack('<2H', 2, 0) + struct.pack('<3i', 0, 4096, 0)
                + struct.pack('<i', 64))
        self.assertEqual(A.physics(sec3), ((0, 4096, 0), 64))

    def test_the_damping_slows_a_falling_particle(self):
        """`(mass - threshold) / mass` a frame. Meteor's 64 against a mass of
        4096 takes 111 frames of fall from 3,030 units down to 1,310."""
        r = bytearray(A.REC_SIZE)
        struct.pack_into('<2h', r, 0x5c, 13991, 13991)   # the rock's radial speed
        struct.pack_into('<h', r, 0x44, 4096)            # its mass
        struct.pack_into('<h', r, 0x54, -128)            # its weight
        rec = bytes(r)
        free = A._motion(rec, (0, 0, 0), 111)[1]
        damped = A._motion(rec, (0, 0, 0), 111, physics=((0, 4096, 0), 64))[1]
        self.assertAlmostEqual(free, 111 * 13991 / 512.0, delta=20)
        self.assertLess(damped, free / 2)
        self.assertAlmostEqual(damped, 1310, delta=120)


class CellIndexing(unittest.TestCase):
    """`battle_effect_render_particle_sprite`'s two-step lookup."""

    def test_the_group_offset_picks_the_table(self):
        """fd = section 0 + 4; the halfword there is the group's offset, and the
        cell table starts at fd + it. Reading from `words[2]` put every cell one
        slot early -- which is what hid Meteor's rock behind an 8x8 spark."""
        rec = struct.pack('<2H', 0x02a0, 1) + poly(0, 0, 8, 8, 0, 0)
        sec = section0([rec])
        self.assertEqual(struct.unpack_from('<h', sec, 4)[0], 2)
        self.assertEqual(len(A.frames(sec)), 1)
        self.assertEqual(len(A.frames(sec)[0]), 1)


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
            bytes([0, 4, 0]) + bytes([1, 2, 0]) + bytes([0x81]),
            bytes([1, 2, 0]) + bytes([0x81])]))
        tl = A.timeline(fr, seqs, A.cast(section3([0, 1])))
        self.assertEqual([(d['t'], d['inst'], d['frame']) for d in tl],
                         [(0, 0, 0), (0, 1, 1), (4, 0, 1)])


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
                         ['set_gravity', 'jump_if_targets_eq',
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

    def test_a_particle_outlives_its_phase(self):
        """The script's tail keeps calling `update_all_particles` until the last
        particle is gone, so a lifetime runs its full count even once the
        timeline's own frames are spent."""
        cs = [dict(seq=0, raw=self.emitter(0, life=6))]
        tl = self.timeline(0, 2, [(0, 1, 0)])       # a two-frame child timeline
        got = A.schedule(self.fr, self.seqs, cs, tl)
        self.assertEqual([d['t'] for d in got], [0, 2, 4, 6, 8, 10])

    def test_an_animation_driven_particle_stops_with_its_phase(self):
        """Its real bound is its sequence's frozen frame; a sequence that loops
        before reaching one would make it immortal, so the phase stays the
        bound. Holy's sequence 0 is exactly that case."""
        cs = [dict(seq=0, raw=self.emitter(0, life=-1))]
        tl = self.timeline(0, 3, [(0, 1, 0)])
        got = A.schedule(self.fr, self.seqs, cs, tl)
        self.assertEqual([d['t'] for d in got], [0, 2, 4])

    def test_an_animation_driven_particle_dies_on_its_frozen_frame(self):
        """A hold of 0 freezes the cursor, and `frame_timer == 0` frees it."""
        seqs = A.script(section1([bytes([1, 2, 0]) + bytes([2, 0, 0])]))
        cs = [dict(seq=0, raw=self.emitter(0, life=-1))]
        tl = self.timeline(0, 3, [(0, 1, 0)])
        got = A.schedule(self.fr, seqs, cs, tl)
        self.assertEqual([d['t'] for d in got], [0, 2])

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


class Curves(unittest.TestCase):
    """Section 4 is rows of 160 bytes whose first four are a header."""

    @staticmethod
    def section(rows):
        out = bytearray()
        for r in rows:
            out += bytes(4) + bytes((r + [0] * 156)[:156])
        return bytes(out)

    def test_the_ramp_starts_four_bytes_into_its_row(self):
        c = A.curves(self.section([[1, 2, 3], [9, 8, 7]]))
        self.assertEqual(c[0][:3], [1, 2, 3])
        self.assertEqual(c[1][:3], [9, 8, 7])

    def test_the_row_count_comes_from_the_section_not_a_header_word(self):
        self.assertEqual(len(A.curves(self.section([[1]] * 3))), 3)

    def test_a_nibble_can_never_name_more_than_sixteen(self):
        self.assertEqual(len(A.curves(self.section([[1]] * 20))), 16)

    def test_a_section_too_short_for_a_row_has_none(self):
        self.assertEqual(A.curves(b'\x0f\x00\x00\x00'), [])


class Tracks(unittest.TestCase):
    """The child timeline's four colour tracks."""

    @staticmethod
    def section(dur, colour, preset, count, second=None):
        """A section 7 holding one palette or background track at its place."""
        n = A.TRACK_KEYFRAMES
        background = second is not None
        size = A.BACKGROUND_TRACK if background else A.PALETTE_TRACK
        t = bytearray(size)
        struct.pack_into('<%dh' % n, t, 0, *((dur + [0] * n)[:n]))
        flat = [c for k in range(n) for c in (colour[k] if k < len(colour) else (0, 0, 0))]
        struct.pack_into('<%db' % (3 * n), t, 2 * n, *flat)
        at = 5 * n
        if background:
            flat2 = [c for k in range(n) for c in (second[k] if k < len(second) else (0, 0, 0))]
            struct.pack_into('<%db' % (3 * n), t, at, *flat2)
            at = 8 * n
        struct.pack_into('<%dB' % n, t, at, *((preset + [0] * n)[:n]))
        struct.pack_into('<h', t, size - 2, count)
        base = 8 + 0x326 + (A.PALETTE_TRACK * 3 if background else 0)
        return bytes(base) + bytes(t) + bytes(8)

    def test_a_keyframe_lasts_its_duration_times_eight(self):
        t = A.tracks(self.section([2, 0], [(1, 2, 3), (4, 5, 6)], [0x80, 0x80], 2))
        self.assertEqual([e['frames'] for e in t['affected units']], [16, 1])

    def test_bit_seven_makes_the_colour_an_offset(self):
        t = A.tracks(self.section([1, 1], [(1, 2, 3), (4, 5, 6)], [0x85, 0x05], 2))
        a, b = t['affected units']
        self.assertEqual((a['offset'], a['mode']), (True, 5))
        self.assertEqual((b['offset'], b['mode']), (False, 5))

    def test_the_count_bounds_the_keyframes(self):
        t = A.tracks(self.section([1] * 10, [(1, 1, 1)] * 10, [0] * 10, 3))
        self.assertEqual(len(t['affected units']), 3)

    def test_the_screen_track_carries_a_second_colour(self):
        sec = self.section([1], [(7, 8, 9)], [0], 1, second=[(1, 2, 3)])
        self.assertEqual(t0 := A.tracks(sec)['screen'][0]['second'], (1, 2, 3))
        self.assertEqual(t0, (1, 2, 3))

    def test_the_screen_colour_is_walked_by_frame(self):
        sec = self.section([1, 1], [(10, 10, 10), (20, 20, 20)], [0x80, 0x80], 2,
                           second=[(0, 0, 0), (0, 0, 0)])
        self.assertEqual(A.screen_colour(A.tracks(sec), 0)[0], (20, 20, 20))
        self.assertEqual(A.screen_colour(A.tracks(sec), 8)[0], (40, 40, 40))

    def test_the_parent_keeps_one_set_of_tracks_per_phase(self):
        """`effect_timeline_t` puts them at 0xdde, 0x384 apart."""
        t = self.section([3], [(9, 9, 9)], [0x80], 1)[8 + 0x326:]
        for phase, at in ((0, A.PARENT_TRACKS), (1, A.PARENT_TRACKS + A.PARENT_TRACKS_SIZE)):
            sec = bytes(at) + t + bytes(8)
            got = A.parent_tracks(sec, phase)['affected units']
            self.assertEqual([(e['frames'], e['colour']) for e in got], [(24, (9, 9, 9))])
            self.assertFalse(A.parent_tracks(sec, 1 - phase)['affected units'])

    def test_the_phases_run_one_after_the_other(self):
        """Phase 1 fades, the child's window starts where it stopped."""
        one = A.tracks(self.section([1], [(40, 0, 0)], [0], 1))
        two = A.tracks(self.section([1], [(80, 0, 0)], [0], 1))
        got = A.stage_offsets([(0, 4, one), (4, 8, two), (8, 1 << 30, None)], 'affected units', 8)
        self.assertEqual([round(c[0]) for c in got], [5, 10, 15, 20, 28, 35, 42, 50])

    def test_a_stage_writes_only_its_own_frames(self):
        """A keyframe longer than its phase stops at the phase's last frame."""
        one = A.tracks(self.section([4], [(32, 0, 0)], [0], 1))
        got = A.stage_offsets([(0, 3, one), (3, 1 << 30, None)], 'affected units', 6)
        self.assertEqual([round(c[0]) for c in got], [1, 2, 3, 0, 0, 0])


class EmitterMotion(unittest.TestCase):
    """`battle_effect_spawn_emitter_particles`: speed and angles follow curves."""

    def record(self, **kw):
        r = bytearray(0xc4)
        for at, fmt, val in kw.values():
            struct.pack_into(fmt, r, at, *val)
        return bytes(r)

    def entry(self, curves=0, speed=(0, 0), speed_end=(0, 0),
              angle=(0, 0, 0), angle_end=(0, 0, 0)):
        r = bytearray(0xc4)
        struct.pack_into('<I', r, 0x08, curves)
        struct.pack_into('<2h', r, 0x5c, *speed)
        struct.pack_into('<2h', r, 0x60, *speed_end)
        struct.pack_into('<3h', r, 0x2c, *angle)
        struct.pack_into('<3h', r, 0x32, *angle_end)
        return bytes(r)

    def test_a_still_emitter_stays_still(self):
        self.assertEqual(A._velocity(self.entry(), (0, 0, 0)), (0.0, 0.0, 0.0))

    def test_the_radial_speed_is_lerped_toward_its_end(self):
        """Haste stores 0 -> 1280 behind nibble 7; the start alone never moves."""
        rec = self.entry(curves=0x50000000, speed=(0, 0), speed_end=(1280, 1280))
        table = [[0] * 160, [0] * 160, [0] * 160, [0] * 160, [0, 128, 256] + [256] * 157]
        self.assertEqual(A._velocity(rec, (0, 0, 0), table, 0), (0.0, 0.0, 0.0))
        half = A._velocity(rec, (0, 0, 0), table, 1)
        full = A._velocity(rec, (0, 0, 0), table, 2)
        self.assertAlmostEqual(half[1], 1280 / 2 / 512.0, places=5)
        self.assertAlmostEqual(full[1], 1280 / 512.0, places=5)

    def test_the_third_base_angle_turns_the_velocity(self):
        """`RotMatrix`'s Z, applied last: a quarter turn sends +y to -x."""
        rec = self.entry(speed=(512, 512), angle=(0, 0, 1024))
        vx, vy, _ = A._velocity(rec, (0, 0, 0))
        self.assertAlmostEqual(vx, -1.0, places=5)
        self.assertAlmostEqual(vy, 0.0, places=5)

    def test_the_base_angle_is_lerped_toward_its_end(self):
        """Haste's two helices are one turn of that angle, half a turn apart."""
        rec = self.entry(curves=0x100, speed=(512, 512),
                         angle=(0, 0, 0), angle_end=(0, 0, 4096))
        table = [[0, 256] + [256] * 158]
        at0 = A._velocity(rec, (0, 0, 0), table, 0)
        at1 = A._velocity(rec, (0, 0, 0), table, 1)
        self.assertAlmostEqual(at0[1], 1.0, places=5)          # straight up
        self.assertAlmostEqual(at1[1], 1.0, places=5)          # a whole turn later
        self.assertAlmostEqual(at0[0], at1[0], places=5)


class ScreenOrigin(unittest.TestCase):
    """The battle's drawing area is not at the origin."""

    def test_the_screen_origin_is_minus_128(self):
        """`SetDefDrawEnv(env, -0x80, 0, width + 0x80, height)`: a primitive
        written at x = 0x80 lands on pixel 0, so the 4x4 grid's 128..384 is
        exactly the 256-wide screen."""
        self.assertEqual(A.SCREEN_ORIGIN_X, -0x80)
        self.assertEqual(0x80 + A.SCREEN_ORIGIN_X, 0)
        self.assertEqual((3 << 6) + 0xc0 + A.SCREEN_ORIGIN_X, 256)


class EffectCamera(unittest.TestCase):
    """`battle_camera_advance_effect_tracks` and its three keyframe tables."""

    def table(self, n, rows):
        """One `effect_camera_keyframes_21_t`-shaped table: 22n + 2 bytes."""
        thr = bytearray(2 * n)
        ang = bytearray(6 * n)
        pos = bytearray(6 * n)
        zoom = bytearray(6 * n)
        sel = bytearray(2 * n)
        for i, (t, a, p, z, f) in enumerate(rows):
            struct.pack_into('<h', thr, 2 * i, t)
            struct.pack_into('<3h', ang, 6 * i, *a)
            struct.pack_into('<3h', pos, 6 * i, *p)
            struct.pack_into('<3h', zoom, 6 * i, *z)
            struct.pack_into('<H', sel, 2 * i, f)
        return bytes(thr + ang + pos + zoom + sel) + struct.pack('<h', len(rows) - 1)

    def section(self, main=(), per=(), clean=(), phase1=100, child=0, delay=0):
        """A section 7 carrying just the durations and the three camera tables."""
        sec = bytearray(A.CAMERA_CLEANUP + 22 * 21 + 2)
        struct.pack_into('<4H', sec, 0, 0, 0, phase1, delay)
        struct.pack_into('<H', sec, 0x00a, child)
        for at, n, rows in ((A.CAMERA_TARGETS, 17, per), (A.CAMERA_MAIN, 21, main),
                            (A.CAMERA_CLEANUP, 21, clean)):
            body = self.table(n, rows or [(0, (0, 0, 0), (0, 0, 0), (0, 0, 0), 0)])
            if not rows:
                struct.pack_into('<h', body := bytearray(body), 22 * n, -1)
            sec[at:at + len(body)] = body
        return bytes(sec)

    def test_the_names_in_the_struct_are_swapped(self):
        """`position[]` holds the angles, `target[]` the position."""
        sec = self.section(main=[(4, (1, 2, 3), (4, 5, 6), (7, 8, 9), 0x0247)])
        tab = A.camera_table(sec, A.CAMERA_MAIN, 21)
        self.assertEqual(tab['angle'][0], (1, 2, 3))
        self.assertEqual(tab['position'][0], (4, 5, 6))
        self.assertEqual(tab['zoom'][0], (7, 8, 9))

    def test_a_given_zoom_of_one_is_a_third_bigger(self):
        """The tactical view is 0xC00; an effect asking for ONE asks for x1.33."""
        sec = self.section(main=[(4, (0,) * 3, (0,) * 3, (A.CAMERA_ONE,) * 3, 0x0844)])
        play = A.camera_play(sec, 6)
        # The engine steps the frame counter before it interpolates, so frame 0
        # already carries the move's first step rather than its start.
        self.assertLess(play[0]['scale'], 1.15)
        self.assertAlmostEqual(play[4]['scale'], A.CAMERA_ONE / float(A.CAMERA_ZOOM_OUT),
                               places=2)

    def test_the_cleanup_table_answers_after_the_others(self):
        """Its thresholds are relative to the end of the per-target window.

        The scanner looks *ahead*, so a cleanup keyframe at 5 past a phase 1 of
        2 is armed at frame 0 with seven frames to run, and the camera is on it
        by frame 6.
        """
        sec = self.section(main=[(2, (0,) * 3, (0,) * 3, (0,) * 3, 0)],
                           clean=[(5, (0,) * 3, (0, -40, 0), (0,) * 3, 0x0802)],
                           phase1=2, child=0, delay=0)
        play = A.camera_play(sec, 10)
        self.assertGreater(play[0]['position'][1], -20)   # barely started
        self.assertAlmostEqual(play[6]['position'][1], -40, delta=1)

    def test_the_map_centre_moves_the_camera_off_the_target(self):
        """A preview is drawn about the target; `--map-centre` says where the
        middle of the battlefield is from there. Gariland's is (140, 0, 210)
        and its roof tile (98, -96, 210), so the vector is (42, 96, 0)."""
        sec = self.section(main=[(4, (0,) * 3, (0, -40, 0), (0,) * 3, 0x0282)])
        plain = A.camera_play(sec, 4)
        moved = A.camera_play(sec, 4, centre=(42, 96, 0))
        self.assertEqual(plain[1]['position'], (0, -40, 0))
        self.assertEqual(moved[1]['position'], (42, 56, 0))

    def test_a_track_is_only_armed_while_it_is_idle(self):
        """A second keyframe does not restart a move that is still running."""
        sec = self.section(main=[(10, (0,) * 3, (0, -100, 0), (0,) * 3, 0x0802),
                                 (20, (0,) * 3, (0, 0, 0), (0,) * 3, 0x0802)])
        play = A.camera_play(sec, 12)
        self.assertAlmostEqual(play[9]['position'][1], -100, delta=2)


class EmitterChildren(unittest.TestCase):
    """`battle_effect_code_script_25_update_all_particles` spawns the children."""

    def record(self, seq=0, count=1, interval=1, life=3, flags=0, kid=0,
               kind=0, speed=(0, 0)):
        r = bytearray(0xc4)
        r[1] = seq
        struct.pack_into('<H', r, 0x02, kind)
        struct.pack_into('<H', r, 0x06, flags)
        struct.pack_into('<2h', r, 0x5c, *speed)
        struct.pack_into('<h', r, 0x94, life)
        struct.pack_into('<h', r, 0xb0, count)
        struct.pack_into('<h', r, 0xb4, interval)
        r[0xc1] = kid
        return bytes(r)

    def run_emit(self, cs, span=8):
        out, fr = [], [[]]
        seqs = [[('frame', 0, 2, 0)]]
        A._emit(out, fr, seqs, cs, 0, 0, 0, 1, span, None, False, A.camera(), None)
        return out

    def test_without_the_flag_no_child_is_born(self):
        cs = [dict(seq=0, raw=self.record(flags=0, kid=1)),
              dict(seq=0, raw=self.record())]
        self.assertEqual({d['inst'] for d in self.run_emit(cs)}, {0})

    def test_a_mid_life_child_is_born_every_frame_of_the_parent(self):
        """Mode 1 (0x4) spawns `child_emitter_mid_life` once a frame while it lives."""
        cs = [dict(seq=0, raw=self.record(flags=0x4, kid=1, life=3)),
              dict(seq=0, raw=self.record(life=2))]
        out = self.run_emit(cs)
        self.assertEqual({d['inst'] for d in out}, {0, 1})
        born = sorted({d['t'] // 2 for d in out if d['inst'] == 1})
        self.assertEqual(born, [0, 1, 2])          # one per frame of a 3-frame life

    def test_the_child_rides_its_parent(self):
        """Its anchor is the parent particle, so it moves with it."""
        cs = [dict(seq=0, raw=self.record(flags=0x4, kid=1, life=4, speed=(512, 512))),
              dict(seq=0, raw=self.record(life=2))]
        kids = [d for d in self.run_emit(cs) if d['inst'] == 1]
        ys = [d['y'] for d in sorted(kids, key=lambda d: d['t'])]
        self.assertGreater(ys[-1], ys[0])          # carried along as the parent falls

    def test_a_native_renderer_keyframe_is_not_a_particle_spawn(self):
        """`selector & 7` names a handler slot; the record is then read through
        that renderer's view, where 0x14 is `texture_u`, not a spawn position."""
        fr, seqs = [[]], [[('frame', 0, 2, 0)]]
        cs = [dict(seq=0, raw=self.record())]
        chan = [dict(k=1, frm=0, to=2, emitter=0, action=0),
                dict(k=2, frm=2, to=4, emitter=0, action=3)]
        tl = dict(phase1=8, child=0, phase2=0, spawn_delay=0,
                  child_channels=[], phase_channels=[[chan], []])
        out = A.schedule(fr, seqs, cs, tl, False)
        self.assertTrue(out)
        self.assertLess(max(d['t'] // 2 for d in out), 2)   # only the first keyframe

    def test_the_recursion_is_bounded(self):
        """A file that makes a particle its own child must not run away."""
        cs = [dict(seq=0, raw=self.record(flags=0x4, kid=0, life=3))]
        self.assertLess(len(self.run_emit(cs)), 500)


if __name__ == '__main__':
    unittest.main()
