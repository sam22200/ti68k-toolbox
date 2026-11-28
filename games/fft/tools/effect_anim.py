#!/usr/bin/env python3
"""Decode and play an original effect's animation: EFFECT/Exxx.BIN sections 0, 1 and 3.

`effect_rip.py` reads the pictures. This reads the *choreography*, so an effect can be
understood from the disc instead of from a video of the console.

    effect_anim.py 163 --script      the byte-code, one sequence per block
    effect_anim.py 163 --timeline    tick by tick: which cell is drawn where
    effect_anim.py 163 --render DIR  PNG per displayed frame, plus a GIF

What the sections hold
----------------------

**Section 0 -- the cells.** A u16 offset table (two head words, then one offset a frame,
rising; the list ends where it stops rising). *An offset points four bytes before its
record*: the four bytes it lands on are the previous record's last corner. Get that wrong
and every record reads shifted, which is how the quads came to be called "six corners".

A record is `u16 marker, u16 npoly`, then `npoly` polygons of 24 bytes:

    u16 tpage, u16 clut, u8 u, v, w, h, then FOUR points (s16 x, s16 y): TL TR BL BR

Four, not three: a record is 4 + 24*npoly, and npoly runs 1..4. The old decoder kept only
the records of exactly 28 bytes -- the single-polygon ones -- and read three of the four
corners. For Stasis Sword that is 42 frames out of 128, and the dropped ones are the
interesting ones: the shards are drawn as three polygons at a time.

**Section 1 -- the sprite sequences.** `u16 count, u16 0`, then `count` u16 offsets
*relative to byte 4*, then one byte-code stream per sequence:

    0x00..0x7f  <u8 hold> <u8 depth>   draw that frame of section 0, hold `hold` ticks
    0x81                                LOOP, back to the first byte
    0x82 <s16 x> <s16 y>                set the offset
    0x83 <s8 dx> <s8 dy>                move by (dx, dy)

Ticks are 60 Hz: the usual `02` is one frame at 30 fps. Every sequence of all 291 data
effects parses with these four and no fifth. `depth` is the ordering-table depth mode
(0 unchanged, 1 nearer 8, 2 fixed 8, 3 farthest, 4 fixed 16, 5 nearer 16).

**Section 3 -- the particle emitters**, `effect_geometry_entry_t`, 196 bytes each: byte
0x01 is the sprite sequence its particles play, 0x04 its frame group, 0xb0 the particles
per spawn and 0xb4 the frames between spawns.

**Section 2 -- the effect script** (`--program`). A 9-bit opcode in a 16-bit header,
seven selector bits above it, and 2, 4, 6 or 8 bytes wide by opcode. Stasis Sword's is
eleven instructions: initialise the particles, end if there is no target, run the parent
timeline to its total duration while updating the particles every frame, then keep
updating until no particle is left, then end. **That is the stop a sequence does not
have.**

**Section 7 -- the timeline** (`--phases`), 6278 bytes. `phase1_duration`,
`child.duration`, `phase2_duration` and `spawn_delay`, then five particle channels, each
a table of 25 keyframe boundaries naming an emitter and an action. Frames here are the
battle's 30 fps, not the sequences' 60 Hz ticks.

**This is the choreography.** Stasis Sword: 11 + 55 + 20 = 86 frames, and channel 0 of
its child timeline starts emitters 2, 3, 4, 5 and 6 at frames 2, 5, 8, 11 and 14 -- the
five shards, three frames apart. The stagger was never in the emitter record, which is
why reading that record harder was never going to find it.

**`--render` plays that timeline.** Each keyframe of a particle channel spawns its
emitter's `particle_count` particles every `spawn_interval` frames while the keyframe
lasts; each particle loops the emitter's sprite sequence for its lifetime (-1 meaning
until the effect ends), and the whole thing starts after phase 1. `--flat` gives the old
behaviour -- every sequence once from t=0 -- for comparing the two.

Each emitter is placed by its `spawn_position_start`, which is what separates Night
Sword's ball overhead from its burst on the victim.

**Still not modelled: the spawn spread and the particle velocities.** Night Sword's
emitter 2 spawns a particle a frame for 35 frames -- 315 draws of one cell. Scattered
through its spread that is the tall white column the game shows; piled on one point it is
a white dot. Scattering in screen space only smears it, so it stays out until the camera
matrix is here. Phase 1 and phase 2 draw nothing either, their own channels being empty
in the effects looked at so far.

The layouts above are the names of **adamrt/fft_decomp**, a byte-exact decompilation of
SCUS-94221; `include/fft/effect/effect_file.h`, `effect_timeline.h` and
`effect_emitter_data.h`. Everything this tool had reverse-engineered by hand matched it,
except the three things listed here.
"""
import argparse
import math
import os
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import core_audit  # noqa: E402
import effect_mesh  # noqa: E402
import effect_rip  # noqa: E402
import extract  # noqa: E402

REC_SIZE = 196          # effect_geometry_entry_t: one particle emitter
LOOP, MOVETO, MOVEBY = 0x81, 0x82, 0x83


def sections(raw):
    """The ten sections of an effect file, or None when the file holds none.

    A MIPS overlay can still carry its data file behind its code
    (`effect_rip.data_base`), which is how Holy is read at all.
    """
    base = effect_rip.data_base(raw)
    if base is None:
        return None
    raw = raw[base:]
    off = struct.unpack_from('<10I', raw, 0)
    ends = list(off[1:]) + [len(raw)]
    out = []
    for i in range(10):
        a, b = off[i], ends[i] or len(raw)
        out.append(raw[a:b] if 0 < a <= b <= len(raw) else b'')
    return out


def frames(sec0, group=0):
    """[[poly, ...], ...] -- the cells of each frame, in drawing order.

    The indexing is `battle_effect_render_particle_sprite`'s, and it has two
    steps that were being collapsed into one:

        g_battle_effect_frame_data = (u8*)header + header->frame_data + 4;
        frame = *(u16*)(fd + sprite_frame_index * 2
                           + *(s16*)(fd + frame_group_index)) + fd;

    So the data pointer is **four bytes into section 0**, the halfword there is
    the frame GROUP's offset (`frame_group_index` is the emitter's group times
    two, and every effect read so far holds one group, at 2), and the cell table
    begins at `fd + that`. Reading the table from `words[2]` instead put every
    cell one slot early.

    That is why Meteor had no meteor. Its emitter 0 falls for 111 frames playing
    sequence 0, which asks for cells 0 and 38 -- both of them three polygons of
    260x364, the rock that fills the screen. One slot early, cell 0 landed
    inside the table (drawn as nothing) and cell 38 on an 8x8 spark.
    """
    if len(sec0) < 8:
        return []
    fd = 4
    base = fd + struct.unpack_from('<h', sec0, fd + 2 * group)[0]
    if not 0 <= base < len(sec0):
        return []
    tbl = []
    for i in range((len(sec0) - base) // 2):  # the table stops where it stops rising
        v = struct.unpack_from('<H', sec0, base + 2 * i)[0]
        if v == 0 or fd + v >= len(sec0) or (tbl and v <= tbl[-1]):
            break
        tbl.append(v)
    out = []
    head = base + 2 * len(tbl)                # the table itself
    for i, t in enumerate(tbl):
        a = fd + t
        b = (fd + tbl[i + 1]) if i + 1 < len(tbl) else len(sec0)
        if a < head or b - a < 4 or (b - a - 4) % 24:
            out.append([])                    # an offset that lands in the table
            continue
        n = struct.unpack_from('<H', sec0, a + 2)[0]
        polys = []
        for k in range(min(n, (b - a - 4) // 24)):
            o = a + 4 + 24 * k
            tpage, clut = struct.unpack_from('<2H', sec0, o)
            # w and h are SIGNED: a negative one mirrors the cell. 23 of Stasis
            # Sword's 298 polygons carry one, and read unsigned they index 224
            # columns into a 128-wide atlas.
            # **w and h are UNSIGNED.** `battle_effect_submit_sprite_to_ordering_
            # table` writes `quad->u1 = quad->u0 + frame->width` into a u8, and
            # the struct spells both as `u8`. The mirroring that reading them
            # signed was invented to explain is the WRAP of that addition: u0 =
            # 239 plus width 200 gives u1 = 183, an interpolation that runs
            # backwards. Read signed, Meteor's rock asked for a window at
            # u = -88 -- off the atlas, so it drew as a black hole with a halo.
            # Read unsigned it is (8, 8) 160x224, and that window IS the rock.
            u, v = sec0[o + 4], sec0[o + 5]
            w, h = sec0[o + 6], sec0[o + 7]
            pts = struct.unpack_from('<8h', sec0, o + 8)
            polys.append(dict(tpage=tpage, clut=clut, u=u, v=v, w=w, h=h,
                              pts=[(pts[0], pts[1]), (pts[2], pts[3]),
                                   (pts[4], pts[5]), (pts[6], pts[7])]))
        out.append(polys)
    return out


def script(sec1):
    """[[(op, operands), ...], ...] -- one byte-code stream per sequence."""
    if len(sec1) < 4:
        return []
    cnt = struct.unpack_from('<H', sec1, 0)[0]
    if cnt == 0 or 4 + 2 * cnt > len(sec1):
        return []
    tbl = struct.unpack_from('<%dH' % cnt, sec1, 4)
    out = []
    for i, t in enumerate(tbl):
        p = t + 4
        end = (tbl[i + 1] + 4) if i + 1 < len(tbl) else len(sec1)
        ops = []
        while p < end:
            op = sec1[p]
            if op < 0x80:
                # frame_index, hold, depth_mode (the ordering-table depth)
                ops.append(('frame', op, sec1[p + 1], sec1[p + 2])); p += 3
            elif op == LOOP:
                ops.append(('loop',)); p += 1; break    # back to the first byte
            elif op == MOVETO:
                ops.append(('moveto',) + struct.unpack_from('<2h', sec1, p + 1)); p += 5
            elif op == MOVEBY:
                ops.append(('moveby',) + struct.unpack_from('<2b', sec1, p + 1)); p += 3
            else:
                ops.append(('?%02x' % op,)); break
        out.append(ops)
    return out


def cast(sec3):
    """The particle emitters of section 3: `effect_geometry_entry_t`, 196 bytes.

    Byte 0x01 is `animation_index`, the sprite sequence its particles play, and
    0x04 the frame group of that sequence. The rest is spawn geometry, curve
    bindings, lifetime, `particle_count` at 0xb0 and `spawn_interval` at 0xb4.
    """
    if len(sec3) < 20:
        return []
    ver, cnt = struct.unpack_from('<2H', sec3, 0)
    if ver != 2 or 20 + REC_SIZE * cnt != len(sec3):
        return []
    out = []
    for i in range(cnt):
        r = sec3[20 + REC_SIZE * i: 20 + REC_SIZE * (i + 1)]
        out.append(dict(seq=r[1], raw=r))
    return out


def physics(sec3):
    """`g_effect_geometry_table`'s header: the gravity and the inertia threshold.

    Effect script opcode 0x27 (`battle_effect_code_script_27_set_gravity`)
    copies both into the globals the integrator reads, so they belong to the
    file and not to any one emitter.
    """
    if len(sec3) < 20:
        return (0, 0, 0), 0
    return (struct.unpack_from('<3i', sec3, 4),
            struct.unpack_from('<i', sec3, 0x10)[0])


def play(ops, nframes):
    """A sequence as [(tick, frame, x, y, flag), ...] -- one entry per draw, in order."""
    x = y = t = 0
    out = []
    for op in ops:
        if op[0] == 'moveto':
            x, y = op[1], op[2]
        elif op[0] == 'moveby':
            x += op[1]; y += op[2]
        elif op[0] == 'frame':
            f, ticks, flag = op[1], op[2], op[3]
            if f < nframes:
                out.append(dict(t=t, frame=f, x=x, y=y, ticks=ticks, flag=flag))
            t += ticks
    return out


# ------------------------------------------- section 2: the effect script
# Opcode in the low nine bits of a 16-bit header, the seven bits above it are
# selectors. Widths are each handler's `record->pc +=` in the decompilation
# (src/battle/effects/playback.c); a jump sets the cursor instead.
SCRIPT_OPS = {
    0x00: ('jump_and_yield', 4), 0x01: ('jump', 4), 0x02: ('spawn_child', 4),
    0x03: ('free_child', 2), 0x04: ('end', 2), 0x05: ('store_high_nibble', 2),
    0x06: ('register_callback', 4), 0x07: ('call_callback', 4),
    0x08: ('camera_position', 8), 0x09: ('save_camera_position', 2),
    0x0a: ('restore_camera_position', 2), 0x0b: ('camera_rotation', 8),
    0x0c: ('save_camera_rotation', 2), 0x0d: ('restore_camera_rotation', 2),
    0x0e: ('camera_zoom', 8), 0x0f: ('save_camera_zoom', 2),
    0x10: ('set_counter', 4),
    0x11: ('jump_if_counter_eq', 6), 0x12: ('jump_if_counter_ge', 6),
    0x13: ('jump_if_counter_gt', 6), 0x14: ('jump_if_counter_le', 6),
    0x15: ('jump_if_counter_lt', 6), 0x16: ('jump_if_particles_eq', 6),
    0x17: ('jump_if_particles_gt', 6), 0x18: ('jump_if_particles_lt', 6),
    0x19: ('jump_if_children_eq', 6), 0x1a: ('jump_if_child_active', 4),
    0x1b: ('jump_if_child_inactive', 4), 0x1c: ('jump_if_counter_ne', 6),
    0x1d: ('jump_after_child_timeline', 4), 0x1e: ('jump_after_total_duration', 4),
    0x1f: ('jump_if_targets_eq', 4), 0x20: ('counter_inc', 2),
    0x21: ('counter_dec', 2), 0x22: ('counter_add', 4), 0x23: ('counter_sub', 4),
    0x24: ('restore_camera_zoom', 2), 0x25: ('update_all_particles', 2),
    0x26: ('spawn_emitter_particles', 2), 0x27: ('set_gravity', 2),
    0x28: ('step_child_timeline', 2), 0x29: ('step_parent_timeline', 4),
    0x2a: ('reset_child_timeline', 2), 0x2b: ('reset_parent_timeline', 2),
    0x2c: ('nop', 2), 0x2d: ('nop', 2),
}


def program(sec2):
    """Section 2 disassembled: [(offset, opcode, name, selectors, operands), ...]."""
    out, p = [], 0
    while p + 2 <= len(sec2):
        head = struct.unpack_from('<H', sec2, p)[0]
        op = head & 0x1ff
        name, width = SCRIPT_OPS.get(op, ('?%03x' % op, 2))
        ops = [struct.unpack_from('<h', sec2, p + 2 + 2 * i)[0]
               for i in range((width - 2) // 2) if p + 4 + 2 * i <= len(sec2)]
        out.append((p, op, name, head >> 9, ops))
        p += width
        if op == 0x04:
            break                               # end of this routine
    return out


# ------------------------------------------- section 7: the timeline
KEYFRAMES = 128         # effect_keyframe_table_t


def _keyframes(buf, at):
    if at + KEYFRAMES > len(buf):
        return None
    start = struct.unpack_from('<25H', buf, at)
    emitter = struct.unpack_from('<24B', buf, at + 0x32)
    action = struct.unpack_from('<25H', buf, at + 0x4a)
    last = struct.unpack_from('<h', buf, at + 0x7e)[0]
    if last <= 0 or not any(emitter):
        return None
    return [dict(k=k, frm=start[k - 1], to=start[k],
                 emitter=(emitter[k - 1] - 1) if emitter[k - 1] else None,
                 action=action[k])
            for k in range(1, min(last, 24) + 1)]


def phases(sec7):
    """`effect_timeline_t`: the durations, the spawn delay and the particle channels.

    The script runs the parent timeline in three stages: phase 1 for
    `phase1_duration` frames, then one child per effect target `spawn_delay`
    frames apart each running the child timeline for `child.duration`, then
    phase 2. Frames are the battle's 30 fps, not the sequences' 60 Hz ticks.
    """
    if len(sec7) < 6278:
        return None
    _, p2, p1, delay = struct.unpack_from('<4H', sec7, 0)
    child = struct.unpack_from('<H', sec7, 0x00a)[0]
    out = dict(phase1=p1, child=child, phase2=p2, spawn_delay=delay,
               total=p1 + child + p2, child_channels=[], phase_channels=[])
    for c in range(5):
        out['child_channels'].append(_keyframes(sec7, 0x00c + KEYFRAMES * c))
    for ph in range(2):
        out['phase_channels'].append(
            [_keyframes(sec7, 0x82a + KEYFRAMES * (ph * 5 + c)) for c in range(5)])
    return out


# ---------------------------------------- section 7: the four colour tracks
# `battle_effect_code_script_28_step_child_timeline` recolours four things from
# four tracks of the child timeline: the affected units, the caster, the current
# target and the screen. Each keyframe lasts `duration * 8` frames (a duration of
# 0 lasts one), and `preset` bit 7 says the colour is an offset applied through
# `battle_map_blend_background_gradient_color`'s mode rather than a gradient set
# outright. The screen's offsets are **doubled** before use.
COLOUR_TRACKS = ('affected units', 'caster', 'target', 'screen')
PALETTE_TRACK, BACKGROUND_TRACK = 0xC8, 0x12C
TRACK_KEYFRAMES = 33
# The modes of `battle_map_blend_background_gradient_color`, which fades the
# gradient toward a target built from the one it already has.
COLOUR_MODES = ('current', 'current/2', 'current grey', 'current grey/2',
                'base', 'base/2', 'base grey', 'base grey/2',
                'restore', 'base repeat', 'stop repeat')


def _track(buf, at, background):
    """One colour track: its keyframes, in order."""
    size = BACKGROUND_TRACK if background else PALETTE_TRACK
    if at + size > len(buf):
        return None
    n = TRACK_KEYFRAMES
    dur = struct.unpack_from('<%dh' % n, buf, at)
    col = struct.unpack_from('<%db' % (3 * n), buf, at + 2 * n)
    second = struct.unpack_from('<%db' % (3 * n), buf, at + 5 * n) if background else None
    pre = struct.unpack_from('<%dB' % n, buf, at + (8 if background else 5) * n)
    count = struct.unpack_from('<h', buf, at + size - 2)[0]
    out = []
    for k in range(max(0, min(count, n))):
        e = dict(k=k, frames=(dur[k] * 8) or 1, duration=dur[k], preset=pre[k],
                 mode=pre[k] & 0x7f, offset=bool(pre[k] & 0x80),
                 colour=tuple(col[3 * k:3 * k + 3]))
        if second is not None:
            e['second'] = tuple(second[3 * k:3 * k + 3])
        out.append(e)
    return out


def tracks(sec7):
    """The child timeline's four colour tracks, by name.

    They sit after its five keyframe tables and three sound tracks, eight bytes
    into the section: 0x326 for the first of them.
    """
    base = 8 + 0x326
    out = {}
    for i, name in enumerate(COLOUR_TRACKS):
        at = base + PALETTE_TRACK * min(i, 3)
        out[name] = _track(sec7, at, name == 'screen')
    return out


# `effect_timeline_t` -- the parent's own view of the same section -- keeps two
# sets of colour tracks at 0xdde, one per phase, each laid out like the child's
# (affected units, caster, target, then the wider screen track).
PARENT_TRACKS = 0xdde
PARENT_TRACKS_SIZE = 3 * PALETTE_TRACK + BACKGROUND_TRACK


def parent_tracks(sec7, phase):
    """Phase 1's or phase 2's four colour tracks, by name.

    `battle_effect_code_script_29_step_parent_timeline` advances
    `color_tracks[0]` while the frame counter is below `phase1_duration` and
    `color_tracks[1]` once phase 2 has begun, through the very
    `battle_effect_advance_map_background_track` the child timeline calls on
    its own. Reading the child's four alone leaves a file whose fade lives in
    its parent with no fade at all: Cure 4's white flash and Haste's darkening
    are both phase-1 keyframes.
    """
    base = PARENT_TRACKS + PARENT_TRACKS_SIZE * phase
    out = {}
    for i, name in enumerate(COLOUR_TRACKS):
        out[name] = _track(sec7, base + PALETTE_TRACK * min(i, 3), name == 'screen')
    return out


def track_offsets(tr, name, frames, phase1=0, double=False):
    """The screen track's colour on each 30 fps frame, as the source applies it.

    A keyframe does not set a colour, it **starts a fade** toward one:
    `battle_map_blend_background_gradient_color(mode, duration, r, g, b)` builds
    its target from the gradient already in flight and the offsets, then takes
    `duration` to get there. The modes are current, current/2, their grey, the
    base (the last applied colour), its halves and greys, and restore.

    Reading one keyframe's colour at a frame -- which is what came before --
    lands on whichever one the effect is sitting in, and Night Sword sits in a
    long `(0, 0, 0)` tail. Its flash is a **one-frame** keyframe of `(63, 63,
    63)`, doubled to `(126, 126, 126)`, which only a fade carries into the
    frames that follow.

    A preview has no map gradient, so it starts from black and the result is the
    offset the effect adds to whatever it plays over.
    """
    seq = (tr or {}).get(name) or []
    out = [(0.0, 0.0, 0.0)] * max(0, frames)
    _fade(seq, out, phase1, frames, [0.0, 0.0, 0.0], double)
    return out


def stage_offsets(stages, name, frames, double=False):
    """The same track over the three stages the parent script runs in turn.

    There is one gradient, and phase 1, the child and phase 2 each advance it
    while they are the stage that runs, so a stage picks the fade up where the
    one before it left off instead of starting from black again.
    """
    out = [(0.0, 0.0, 0.0)] * max(0, frames)
    cur = [0.0, 0.0, 0.0]
    for at, end, tr in stages:
        cur = _fade((tr or {}).get(name) or [], out, at, min(end, frames), cur, double)
    return out


def _fade(seq, out, at, end, cur, double):
    """One stage's keyframes written into `out`, and the colour it ends on."""
    frames = end
    # `base` is `g_battle_map_background_gradient_transition.colors[i]` -- the
    # map's **own** gradient colour, which a fade never touches -- and not the
    # colour last applied. Taking it for the latter left Night Sword's flash red
    # instead of white: the keyframe after it offsets `base` by (0, -128, -128),
    # which from a constant base clamps to black rather than keeping 126 of red.
    # A preview has no map, so its base is black and the result is the offset.
    cur = list(cur)
    base = [0.0, 0.0, 0.0]
    last = list(cur)                 # where the window leaves the gradient
    for e in seq:
        n = e['frames']
        off = [(2.0 if double else 1.0) * c for c in e['colour']]
        if not e['offset']:
            target = [float(c) for c in e['colour']]
        else:
            m = e['mode']
            src = cur if m < 4 else base
            if m in (2, 3, 6, 7):
                g = (src[0] * 2 + src[1] * 3 + src[2]) / (6.0 if m in (2, 6) else 12.0)
                start = [g, g, g]
            elif m in (1, 5):
                start = [c / 2.0 for c in src]
            else:
                start = list(src)
            target = start if m == 8 else [start[k] + off[k] for k in range(3)]
        # The fade runs over `duration * 8` frames, which is the keyframe's own
        # length; a duration of 0 snaps and holds for its single frame.
        target = [min(255.0, max(0.0, c)) for c in target]
        for i in range(n):
            t = 1.0 if e['duration'] == 0 else (i + 1) / float(n)
            c = [cur[k] + (target[k] - cur[k]) * t for k in range(3)]
            if 0 <= at + i < frames:
                out[at + i] = tuple(c)
                last = c
        cur = list(target)
        at += n
        if at >= frames:
            break
    return last


def screen_colour(tr, frame):
    """The screen track at a 30 fps frame: (colour, mode, is_offset).

    A keyframe is entered when the one before it runs out. **It does not give a
    background.** With `preset` bit 7 the colour is an offset applied to the
    gradient the map already has, through one of
    `battle_map_blend_background_gradient_color`'s eleven modes -- and those
    offsets are overwhelmingly negative, so a preview that painted them would
    draw a darker black. Without the bit it sets the two gradient stops outright.
    Either way it modulates a battlefield, which a preview does not have.
    """
    seq = (tr or {}).get('screen') or []
    t = 0
    for e in seq:
        if frame < t + e['frames']:
            if not e['offset']:
                return e['colour'], e['mode'], False
            return tuple(2 * c for c in e['colour']), e['mode'], True
        t += e['frames']
    return (0, 0, 0), 0, True


KEYFRAME_ACTIONS = ((0x0008, 'caster only'), (0x0010, 'post-action display'),
                    (0x0020, 'update display'), (0x0040, 'target animation'))


def action_text(a):
    bits = [n for m, n in KEYFRAME_ACTIONS if a & m]
    if a & 0x0007:
        bits.append('callback %d' % (a & 7))
    if a >> 8:
        bits.append('animation %d' % (a >> 8))
    return ', '.join(bits)


# ---------------------------------------------- the battle camera's projection
# `camera_update_matrices` is `RotMatrix(rotation)` then `ScaleMatrix(zoom)`, and
# the header names the values: pitch `CAMERA_TILT_LOW` 0x12e or `CAMERA_TILT_HIGH`
# 0x1c0 (4096 is a full turn, so 26.5 and 39.4 degrees), zoom ONE or 0xc00. The
# effect renderer projects with RotTrans, not RotTransPers -- no perspective
# divide -- so world to screen is the rotation's first two rows and nothing else.
PITCH, YAW = 0x12e, 0x200          # tilt low, and the isometric 45-degree yaw


def camera(pitch=PITCH, yaw=YAW):
    """(vx, vy) of a world (dx, dy, dz), and its depth: the battle camera matrix.

    Read out of `g_battle_camera_matrix` (0x80098a24) in a saved Gariland state
    rather than derived, because three of its signs are not the ones a plain
    `Rx(pitch) * Ry(yaw)` gives. At tilt low and the isometric yaw it holds

        [ 2896     0  -2892 ]     / 4096 = [  0.707   0.000  -0.706 ]
        [-1292  3661  -1294 ]             [ -0.315   0.894  -0.316 ]
        [ 2584  1830   2588 ]             [  0.631   0.447   0.632 ]

    which is what the parameterisation below reproduces. The signs matter: the
    depth row was being built with its x negated, and `battle_effect_render_
    particle_sprite` drops any sprite whose depth comes out below zero, so
    Meteor's rock -- born 1,354 units up and reading 4 against a mode that
    subtracts 8 -- was thrown away for its entire fall. With the measured row it
    reads 15, and survives.
    """
    th, ph = pitch / 4096.0 * 2 * math.pi, yaw / 4096.0 * 2 * math.pi
    st, ct, sp, cp = math.sin(th), math.cos(th), math.sin(ph), math.cos(ph)

    def project(dx, dy, dz=0):
        return (cp * dx - sp * dz, -st * sp * dx + ct * dy - st * cp * dz)
    project.depth = lambda dx, dy, dz: ct * sp * dx + st * dy + ct * cp * dz
    return project


def curves(sec4):
    """Section 4's animation curves, 128 being neutral.

    `battle_effect_integrate_particle_motion` reads one as
    `(curve_data + track * 0xa0 + offset)[4]`: the section is **rows of 160
    bytes whose first four are a header**, and the ramp is the 156 after it.
    The leading word was being read as a row count, which happens to be 15 in
    every effect on the disc and happens to equal what the section holds -- two
    coincidences that make a wrong model look right. A nibble is four bits, so
    sixteen rows are all that can ever be named.
    """
    rows = min(16, len(sec4) // 160)
    return [list(sec4[4 + 160 * i:4 + 160 * (i + 1)]) for i in range(rows)]


def _tint(rec, cur, n):
    """A particle's colour this frame, from its emitter's three colour curves.

    `block->header.color` starts at `EFFECT_COLOR_NEUTRAL` (128) and the particle
    renderer feeds it `curves[color_curves.r][curve_frame]` and its green and
    blue, with `curve_frame` counting the particle's own frames and wrapping at
    160. So a particle fades over its life -- which is what keeps a stack of
    them from clipping to white the way nineteen flat copies do.
    """
    if not cur:
        return (1.0, 1.0, 1.0)
    sel = struct.unpack_from('<I', rec, 0x10)[0]
    out = []
    for k in range(3):
        i = (sel >> (4 * k)) & 0xf
        out.append(cur[i][n % 160] / 128.0 if i < len(cur) else 1.0)
    return tuple(out)


def _velocity(rec, key, cur=None, step=0):
    """One particle's world displacement per frame, from its emitter's motion.

    `(0, radial_speed, 0)` rotated by the base angle plus a random spread, times
    eight, against a position carrying 12 fractional bits -- so the world step is
    `out / 512` a frame. Without a curve bound the stored spread is **halved**.
    The integration damps by `(inertia - threshold) / inertia`, and the engine's
    threshold starts at 0, which leaves the velocity constant; acceleration and
    gravity are not modelled.

    **Both the speed and the angles are lerped toward their end values** by the
    curve their `curve_indices[0]` nibble names -- 7 the radial speed, 2 the base
    angle, 3 the spread -- indexed by `step`, the emitter's own frame inside the
    keyframe, not by the particle's age (`battle_effect_spawn_emitter_particles`
    is handed that counter and does `frame % 160` on it). Reading the starts
    alone leaves every emitter whose motion is written as "from nothing to
    something" perfectly still: Haste stores speed 0 -> 1280 and a base angle
    turning 2048 -> 6144, which is exactly its two counter-rotating helices of
    roman numerals, and the preview drew them as one motionless stack.

    The third angle is the Z of `RotMatrix(rotation)`, applied to the already
    rotated vector. Dropping it costs nothing when it is zero -- which it is
    everywhere the preview had been checked -- and costs Haste its whole ring.
    """
    idx = struct.unpack_from('<I', rec, 0x08)[0]
    n_speed, n_angle, n_spread = (idx >> 28) & 0xf, (idx >> 8) & 0xf, (idx >> 12) & 0xf
    lo, hi = struct.unpack_from('<2h', rec, 0x5c)          # radial_speed_start
    if n_speed:
        f = effect_mesh.curve(cur or (), n_speed, step)
        lo2, hi2 = struct.unpack_from('<2h', rec, 0x60)    # radial_speed_end
        lo, hi = effect_mesh.lerp(lo, lo2, f), effect_mesh.lerp(hi, hi2, f)
    if lo == 0 and hi == 0:
        return 0.0, 0.0, 0.0
    a = list(struct.unpack_from('<3h', rec, 0x14 + 0x18))     # velocity_base_angle
    s = list(struct.unpack_from('<3h', rec, 0x14 + 0x24))     # velocity_spread
    if n_angle:
        f = effect_mesh.curve(cur or (), n_angle, step)
        b = struct.unpack_from('<3h', rec, 0x14 + 0x1e)
        a = [effect_mesh.lerp(a[i], b[i], f) for i in range(3)]
    if n_spread:
        f = effect_mesh.curve(cur or (), n_spread, step)
        b = struct.unpack_from('<3h', rec, 0x14 + 0x2a)
        s = [effect_mesh.lerp(s[i], b[i], f) for i in range(3)]
    else:
        s = [v / 2.0 for v in s]                              # halved without a curve
    r = _rand3(key)
    rz = _rand3(key + (7,))[0]            # the Z spread has a draw of its own
    th = (a[0] + r[0] * s[0]) / 4096.0 * 2 * math.pi
    ph = (a[1] + r[1] * s[1]) / 4096.0 * 2 * math.pi
    ps = (a[2] + rz * s[2]) / 4096.0 * 2 * math.pi
    speed = (lo + (hi - lo) * (r[2] + 1) / 2.0) / 512.0
    vx = speed * math.sin(th) * math.sin(ph)
    vy = speed * math.cos(th)
    vz = speed * math.sin(th) * math.cos(ph)
    c, sn = math.cos(ps), math.sin(ps)                        # RotMatrix's Z, applied last
    return vx * c - vy * sn, vx * sn + vy * c, vz


def _rand3(key):
    """Three repeatable values in [-1, 1]."""
    h, out = 2166136261, []
    for i in range(3):
        for q in key + (i,):
            h = ((h ^ (q & 0xffff)) * 16777619) & 0xffffffff
        out.append(((h >> 8) & 0xffff) / 65535.0 * 2 - 1)
    return out


def _spawn_offset(spread, box, key):
    """Where in its spread a particle is born.

    `EFFECT_EMITTER_BOX_SPREAD` (0x0100 of the placement flags) says "spawn
    inside the spread box; **clear, on the spread ellipsoid**". Clear is the
    common case -- 1,429 of the disc's 1,893 emitters against 464 boxes -- and
    filling the volume instead of its surface piles every particle of a stream
    near one point, which is what burned Night Sword's ball white.
    """
    sx, sy, sz = spread
    if not (sx or sy or sz):
        return 0.0, 0.0, 0.0
    v = _rand3(key)
    if box:
        return v[0] * sx, v[1] * sy, v[2] * sz
    n = math.sqrt(v[0] ** 2 + v[1] ** 2 + v[2] ** 2) or 1.0   # onto the surface
    return v[0] / n * sx, v[1] / n * sy, v[2] / n * sz


def _motion(rec, key, n, cur=None, step=0, physics=None):
    """A particle's world offset after `n` frames.

    `battle_effect_integrate_particle_motion`, to the letter:

        position += velocity
        velocity  = (mass - threshold) * velocity / mass
                  + (force << 12) / mass
                  + (gravity * weight >> 12)

    Two of those three terms were missing, and together they are the difference
    between a rock that lands and one that falls through the world.

    * **the damping is real.** `mass` is the emitter's inertia (0x44) and
      `threshold` is `g_effect_geometry_table->inertia_threshold`, which effect
      script opcode 0x27 -- `set_gravity`, not "init particle physics" -- copies
      out of the section 3 header. Meteor's is 64 against a mass of 4096: 1.6%
      of the speed goes every frame, 0.984^111 = 0.17 over the rock's life.
    * **gravity is in the same header** (0x04, `(0, 4096, 0)` in six of the
      seven effects read) and is scaled by the particle's own `weight` (0x54),
      which is **-128** for the rock -- it brakes upward.

    With the starting 27.3 units a frame alone the rock falls 3,030 units from
    1,450 up and leaves the world. With the damping and the brake it falls
    1,310 and comes to rest just above the ground, which is what the console
    shows.

    Homing, when `homing_strength` is set, steers the force toward the target
    with the original's `(step - delta) * strength / 127 + delta`, `strength`
    being -128 without a homing curve.
    """
    vel = list(_velocity(rec, key, cur, step))
    if n <= 0:
        return 0.0, 0.0, 0.0
    mass = struct.unpack_from('<h', rec, 0x44)[0] or 4096
    weight = struct.unpack_from('<h', rec, 0x54)[0]
    gravity, threshold = physics or ((0, 0, 0), 0)
    damp = (mass - threshold) / float(mass)
    pull = [(g * weight >> 12) / 4096.0 for g in gravity]
    acc = [struct.unpack_from('<2h', rec, 0x64 + 4 * k)[0] / 4096.0 for k in range(3)]
    step = [struct.unpack_from('<2h', rec, 0x7c + 4 * k)[0] / 4096.0 for k in range(3)]
    homing = struct.unpack_from('<2h', rec, 0xb8)[0] / 4096.0
    target = [v / 1.0 for v in struct.unpack_from('<3h', rec, 0x9c)]
    pos = [0.0, 0.0, 0.0]
    for _ in range(min(n, 600)):
        for k in range(3):
            pos[k] += vel[k]
            vel[k] = vel[k] * damp + acc[k] + pull[k]
        if homing:
            d = [target[k] - pos[k] for k in range(3)]
            m = math.sqrt(sum(x * x for x in d))
            d = [x / m * homing for x in d] if m else [0.0, 0.0, 0.0]
            for k in range(3):
                acc[k] += (step[k] - d[k]) * -128 / 127.0 + d[k]
        else:
            for k in range(3):
                acc[k] += step[k]
    return pos[0], pos[1], pos[2]


def play_window(ops, nframes, t0, t1, animation_driven):
    """One particle's draws, walking its sequence the way the engine walks it.

    Two things end a sequence, and neither is the 0x81:

    * **a hold of 0 freezes the cursor.** The stepper only advances when the
      hold it just loaded is non-zero (`if (node->frame_timer != 0)
      node->sequence_offset += 3;`), so a frame written `NN 00 dd` is held for
      good. Nearly every sequence ends on one, which is why the 0x81 behind it
      is usually never reached;
    * **a lifetime of -1 is not endless, it is animation-driven**
      (`EFFECT_PARTICLE_LIFETIME_ANIMATION_DRIVEN`). Such a particle is freed
      the frame its `frame_timer` reaches 0 -- that is, on the frozen frame.

    So an animation-driven particle plays its sequence exactly once and dies on
    the held frame; one with a lifetime holds that frame until its time runs
    out, and only a sequence with no held frame ever reaches the 0x81 and loops.
    Reading -1 as "endless" made every shard of Stasis Sword fall twice.
    """
    out, t, i, x, y, guard = [], t0, 0, 0, 0, 0
    while t < t1 and i < len(ops) and guard < 20000:
        guard += 1
        op = ops[i]
        if op[0] == 'moveto':
            x, y = op[1], op[2]; i += 1
        elif op[0] == 'moveby':
            x += op[1]; y += op[2]; i += 1
        elif op[0] == 'loop':
            i = 0                                   # back to the first byte
        elif op[0] == 'frame':
            f, hold, depth = op[1], op[2], op[3]
            if f < nframes:
                held = 2 if (hold == 0 and animation_driven) else \
                       (t1 - t if hold == 0 else min(hold, t1 - t))
                out.append(dict(t=t, frame=f, x=x, y=y, depth=depth, ticks=held))
            if hold == 0:
                break                               # frozen: dead, or held to the end
            t += hold; i += 1
        else:
            i += 1
    return out


def _tint_factor(cur, nibble, frame):
    """READ_CURVE against the palette table, for a caller that has a frame."""
    return effect_mesh.curve(cur or (), nibble, frame)


# `battle_effect_code_script_25_update_all_particles` walks every live particle
# once a frame and, when its `flags` carry mid-life child mode 1 or 2 (0x4 or
# 0x8), calls `battle_effect_spawn_emitter_particles` again for
# `child_emitter_mid_life` with the particle itself as the transform -- so the
# child's `EFFECT_EMITTER_ORIGIN_PARENT_PARTICLE` anchor rides along. Modes 1
# and 2 of `flags & 3` do the same once, at death, for
# `child_emitter_on_death`.
#
# Nothing in the timeline ever names those child emitters, so reading the
# channels alone leaves them unborn. Meteor's rock is one: emitter 0 is a
# **timer** whose own sequence holds a cell with no polygons for 180 ticks, and
# every frame of its fall it spawns five particles of emitter 1 -- the sequence
# that draws the rock and its trail.
MID_LIFE_CHILD_MASK, DEATH_CHILD_MASK = 0x000c, 0x0003
CHILD_DEPTH_LIMIT = 2                   # two generations is all any file uses


def _per_frame(d, t1):
    """One entry a frame over a held cell's life, so a moving particle is placed
    where it actually is on each of them."""
    end = min(d['t'] + d['ticks'], t1)
    return [dict(d, t=t, ticks=2) for t in range(d['t'], max(end, d['t'] + 2), 2)]


def _emit(out, fr, seqs, cs, e, base, frm, to, span, cur, scatter, project,
          centre, chan=0, origin=None, step0=None, depth=0, cam=None, phys=None):
    """Every draw of emitter `e`'s particles born in [frm, to), children included.

    `origin` is a function of the absolute frame giving the world point a child
    hangs off; without one the emitter uses its own anchor. `step0` is the frame
    the curves are read from: the keyframe's own start for a timeline emitter,
    and the effect's frame counter for a child, which is what the script hands
    `battle_effect_spawn_emitter_particles` in each case.
    """
    rec = cs[e]['raw']
    seq = rec[1]
    if seq >= len(seqs):
        return
    count = max(1, struct.unpack_from('<h', rec, 0xb0)[0])
    interval = max(1, struct.unpack_from('<h', rec, 0xb4)[0])
    life = struct.unpack_from('<h', rec, 0x94)[0]
    kind = struct.unpack_from('<H', rec, 0x02)[0]
    flags = struct.unpack_from('<H', rec, 0x06)[0]
    orient = kind & 2
    spawn_curve = struct.unpack_from('<I', rec, 0x08)[0] & 0xf
    anchor_mode = kind & effect_mesh.ORIGIN_MASK
    sw = [abs(v) for v in struct.unpack_from('<3h', rec, 0x14 + 0x0c)]
    box = kind & 0x0100
    home = (list(centre) if centre and anchor_mode == effect_mesh.ORIGIN_MAP_CENTRE
            else [0.0, 0.0, 0.0])
    kid = rec[0xc1] if flags & MID_LIFE_CHILD_MASK else None
    if kid is not None and (kid >= len(cs) or depth >= CHILD_DEPTH_LIMIT):
        kid = None

    # A keyframe's `to` can run far past its phase -- Holy's reach 600 against a
    # 229-frame timeline -- because `battle_effect_step_emitter_timeline` only
    # ever steps the channel while the phase's own counter is running. So the
    # SPAWNING stops at the end of the phase; what a particle does afterwards is
    # its own business, and the script's tail keeps it alive.
    born = frm
    if step0 is None:
        to = min(to, max(span, frm + 1))
    while born < to:
        # The parent reads its curves at its step inside the keyframe; a child
        # is handed `record->frame_counter`, the effect's own frame.
        step = (born - frm) if step0 is None else (base + born)
        # **A particle outlives its phase.** The effect script does not stop at
        # the end of the timeline: `jump_after_total_duration` sends it to a
        # tail that calls `update_all_particles` and loops on
        # `jump_if_particles_eq 0` until the last one is gone. Clamping a
        # lifetime to the end of its own phase -- `min(span, born + life)` --
        # cut every effect short at the point the original is still burning
        # down. A stored lifetime therefore runs its full count.
        #
        # An animation-driven one (-1) is still clamped to its phase, and that
        # is an approximation, not the engine: such a particle is freed when its
        # `frame_timer` reaches 0, i.e. on its sequence's frozen frame. Some
        # sequences loop back BEFORE reaching theirs -- Holy's sequence 0 does --
        # which makes the particle immortal on this reading, and emitter 10 then
        # drew 12,340 times out to frame 811. Whatever frees it in the original
        # is not in `battle_effect_code_script_25_update_all_particles`, so the
        # phase stays the bound until that is found.
        die = (born + life) if life >= 0 else span
        t0, t1 = (base + born) * 2, (base + die) * 2
        for i in range(count):
            key = (e, born, i, depth)
            j = _spawn_offset(sw, box, key) if scatter else (0, 0, 0)
            pw = effect_mesh.spawn_position(rec, _tint_factor(cur, spawn_curve, step))

            def at(age, _pw=pw, _j=j, _key=key, _step=step, _born=born):
                """The particle's world point `age` frames after it was born."""
                m = _motion(rec, _key, age, cur, _step, phys)
                root = origin(base + _born + age) if origin else (0.0, 0.0, 0.0)
                return [_pw[q] + _j[q] + m[q] + home[q] + root[q] for q in range(3)]

            for d0 in play_window(seqs[seq], len(fr), t0, t1, life < 0):
              # **A held cell still moves.** `play_window` returns one entry per
              # cell change, and a cell can be held for hundreds of ticks --
              # Meteor's rock holds one for 180 while it falls 2,400 units. The
              # engine re-runs `RotTrans` every frame, so a single entry placed
              # at the particle's birth is both drawn in the wrong place and
              # judged for depth there: the rock read as behind the lens on the
              # frame it was born and was dropped for its whole fall. A moving
              # particle is therefore emitted frame by frame.
              moves = any(_velocity(rec, key, cur, step)) or cam
              for d in (_per_frame(d0, t1) if moves and d0['ticks'] > 2 else [d0]):
                n = (d['t'] - t0) // 2              # frames this one has lived
                w = at(n)
                # `battle_effect_render_particle_sprite` takes the depth from
                # `RotTrans` through the CURRENT camera matrix, translation and
                # all, and drops the sprite when it comes out negative. The
                # preview applies the effect camera to the screen as a 2D pan
                # and zoom, so it has to apply the same pan here or the depth is
                # measured from a camera that never moved: Meteor's rock is born
                # 1,354 units up, which reads as behind the lens and is thrown
                # away for the whole of its fall.
                eye = cam[min(d['t'] // 2, len(cam) - 1)]['position'] if cam else None
                dw = [w[q] - eye[q] for q in range(3)] if eye else w
                ot = sprite_depth(0x280 + project.depth(*dw), d['depth'])
                if ot is None:
                    continue
                ox, oy = project(*w)
                sx, sy = project(d['x'], d['y'])
                ang = 0.0
                if orient:                          # ratan2 of the projected velocity
                    q = at(n + 1)
                    vx, vy = project(q[0] - w[0], q[1] - w[1], q[2] - w[2])
                    if vx or vy:
                        ang = math.atan2(vy, vx)
                out.append(dict(d, inst=e, seq=seq, chan=chan, tint=_tint(rec, cur, n),
                                x=ox + sx, y=oy + sy, ot=ot, angle=ang))
            if kid is not None:
                # One spawn attempt a frame for as long as this particle lives.
                _emit(out, fr, seqs, cs, kid, base, born, die, span, cur, scatter,
                      project, centre, chan, origin=at_of(at, base, born),
                      step0=0, depth=depth + 1, cam=cam, phys=phys)
        born += interval


def at_of(at, base, born):
    """`at`, readdressed from the particle's age to the effect's frame counter."""
    return lambda f: at(max(0, f - base - born))


def schedule(fr, seqs, cs, tl, scatter=True, project=None, cur=None, centre=None,
             cam=None, phys=None):
    """Every draw of the effect as its timeline orders it, in 60 Hz ticks.

    Each keyframe of a particle channel spawns its emitter's `particle_count`
    particles every `spawn_interval` frames while the keyframe lasts; each
    particle plays the emitter's sprite sequence, looping, for its lifetime
    (-1 meaning until the effect ends). The child timeline starts after phase 1.

    Not modelled: the spawn spread and velocities of the emitter's geometry, so
    the particles of one spawn land on top of each other instead of scattering.
    """
    if not tl:
        return None
    project = project or camera()
    out = []
    # `battle_effect_code_script_29_step_parent_timeline` steps
    # `particle_channels[0]` while the frame counter is below phase1_duration
    # and `particle_channels[1]` once phase 2 has begun, through the very
    # `battle_effect_step_emitter_timeline` the child timeline calls. Reading
    # the child's five channels alone therefore misses every emitter a file
    # puts in its parent: Meteor puts all nine of them there, and drew
    # nothing at all.
    groups = [(0, tl['phase_channels'][0], tl['phase1']),
              (tl['phase1'], tl['child_channels'], tl['child']),
              (tl['phase1'] + tl['child'], tl['phase_channels'][1], tl['phase2'])]
    for base, chans, span in groups:
        for c, kfs in enumerate(chans or ()):
            for k in kfs or []:
                e = k['emitter']
                if e is None or e >= len(cs):
                    continue
                # `battle_effect_code_script_28_step_child_timeline`: a
                # keyframe's `selector & 7` is zero for an ordinary particle
                # emitter and otherwise names the handler slot that takes it
                # over. Such a keyframe is NOT a particle spawn -- its geometry
                # record is read through the renderer's own view, where 0x14 and
                # 0x16 are `texture_u`/`texture_v` and not a spawn position.
                # Drawing it both ways put Meteor's big quad at (182, 246),
                # right off a 256x240 frame, while `mesh_actors` drew the real
                # one; the texture coordinates were being flown as a position.
                if k['action'] & 7:
                    continue
                _emit(out, fr, seqs, cs, e, base, k['frm'],
                      max(k['to'], k['frm'] + 1), span, cur, scatter, project,
                      centre, chan=c, cam=cam, phys=phys)
    # The ordering table is walked from its far end, so the farthest index is
    # drawn first; within one index the last primitive added is drawn first.
    out.sort(key=lambda d: (d['t'], -d['ot'], -d['chan'], -d['inst']))
    return out


def timeline(fr, seqs, cs):
    """Every draw of the whole effect, as (tick, instance, sequence, frame, x, y)."""
    out = []
    for i, c in enumerate(cs):
        if c['seq'] >= len(seqs):
            continue
        for d in play(seqs[c['seq']], len(fr)):
            out.append(dict(inst=i, seq=c['seq'], **d))
    out.sort(key=lambda d: (d['t'], d['inst']))
    return out


# ---------------------------------------------------------------- rendering

def _atlas_rgb(raw):
    """The atlas as RGB and a draw mask, blended the way the hardware does.

    The parts carry `EFFECT_SPRITE_PART_FLAG_SEMI_TRANSPARENT` and a tpage whose
    blend mode is B+F, so a texel **adds its own colour, whole**. `effect_rip`'s
    alpha -- brightness/2 for a flagged colour -- was invented to make an atlas
    legible, and weighting by it keeps bright edges while dimming the body: the
    three copies of Night Sword's ball, superposed exactly, then read as one
    ball inside two ghost outlines instead of saturating into one.

    Two colours are not drawn: palette index 0, and 0xFFFF wherever it appears.
    In Stasis Sword's blade 0xFFFF is 58 texels, **every one of them on the
    silhouette's edge** and a quarter of that edge -- a dotted white outline
    the console does not show. Treating it as a colour draws that outline.
    """
    import numpy as np
    got = effect_rip.atlas(raw)
    if not got:
        return None
    w, h, pixels, colours = got
    lut = np.zeros((256, 4))
    for i in range(1, 256):
        c = colours[i]
        if c == 0 or c == 0xFFFF:
            continue
        lut[i] = ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3, 1.0)
    idx = np.frombuffer(pixels, dtype=np.uint8).reshape(h, w)
    px = lut[idx]
    return px[:, :, :3], px[:, :, 3]


# The tpage's bits 5-6 are the hardware's semi-transparency mode, and the parts
# do not all share one: of the disc's 22,916 polygons 19,092 are B+F, 2,630 are
# B+F/4, 727 B-F and 467 B/2+F/2. `_blend` applies them; B/2+F/2 halves what is
# underneath as well, which scaling the addend alone could not express.
# Whether a part is blended at all is its own `flags & 0x200`, not the mode:
# 404 of the 22,916 are opaque (none in Stasis Sword, Night Sword or Holy).


# battle_effect_render_particle_sprite: the ordering-table index of a sprite is
# its projected z over four, which the sequence op's third byte can override.
# A sprite whose index comes out negative is not drawn at all.
SPRITE_DEPTH_LIMIT = 0x17e


def sprite_depth(vz, mode):
    """The ordering-table index, or None when the sprite is dropped."""
    d = int(vz) >> 2
    if mode == 1:
        d -= 8
    elif mode == 2:
        d = 8
    elif mode == 3:
        d = SPRITE_DEPTH_LIMIT
    elif mode == 4:
        d = 0x10
    elif mode == 5:
        d -= 0x10
    if d < 0:
        return None
    return min(d, SPRITE_DEPTH_LIMIT)


def rotate2(pts, angle):
    """The 2D matrix `battle_effect_submit_sprite_to_ordering_table` builds.

    `[[cos, -sin], [sin, cos]] >> 12` at a camera zoom of ONE, applied to the
    part's own corners before the position is added. A particle only carries an
    angle when its emitter sets `work_kind_flags` bit 1, which orients it to its
    velocity (`battle_effect_update_node_from_work`).
    """
    if not angle:
        return pts
    c, sn = math.cos(angle), math.sin(angle)
    return [(x * c - y * sn, x * sn + y * c) for x, y in pts]


def _tri(acc, cov, tex, alpha, dst, src, cell, rgb=None):
    """Rasterise one textured triangle into a primitive's own colour and coverage.

    No blending here: the GPU blends a whole primitive against the frame buffer
    once (`_quad`), and compositing triangle by triangle would blend the shared
    edge twice.
    """
    import numpy as np
    (x0, y0), (x1, y1), (x2, y2) = dst
    det = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
    if det == 0:
        return
    H, W = acc.shape[:2]
    lo_x, hi_x = max(0, int(min(x0, x1, x2))), min(W, int(max(x0, x1, x2)) + 1)
    lo_y, hi_y = max(0, int(min(y0, y1, y2))), min(H, int(max(y0, y1, y2)) + 1)
    if lo_x >= hi_x or lo_y >= hi_y:
        return
    yy, xx = np.mgrid[lo_y:hi_y, lo_x:hi_x]
    px, py = xx + 0.5, yy + 0.5
    l0 = ((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2)) / det
    l1 = ((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2)) / det
    l2 = 1.0 - l0 - l1
    inside = (l0 >= -1e-6) & (l1 >= -1e-6) & (l2 >= -1e-6)
    if not inside.any():
        return
    su = l0 * src[0][0] + l1 * src[1][0] + l2 * src[2][0]
    sv = l0 * src[0][1] + l1 * src[1][1] + l2 * src[2][1]
    # Clamp inside the cell, not the whole atlas: a pixel sampled one column past
    # the cell picks up the neighbouring sprite and draws a bright fringe round
    # everything, which is what a first pass at this did.
    u0, v0, u1, v1 = cell
    su = np.clip(su.astype(int), u0, u1)
    sv = np.clip(sv.astype(int), v0, v1)
    a = alpha[sv, su] * inside
    box = cov[lo_y:hi_y, lo_x:hi_x]
    fresh = (a > 0) & (box == 0)
    box[fresh] = 1
    texel = tex[sv, su]
    if rgb is not None:
        # POLY_GT4: each vertex carries its own colour and the hardware
        # interpolates it across the primitive, 0x80 being neutral. Painting
        # the whole quad in one of the four turns a graded wash into a slab,
        # which is what the 4x4 grid and the ring mesh looked like.
        c = (l0[:, :, None] * np.array(rgb[0]) + l1[:, :, None] * np.array(rgb[1])
             + l2[:, :, None] * np.array(rgb[2]))
        texel = texel * c / 128.0
    acc[lo_y:hi_y, lo_x:hi_x][fresh] = texel[fresh]


def _blend(fb, prim, cov, abe, mode):
    """Composite one primitive into the frame buffer the way the GPU does.

    `battle_effect_submit_sprite_to_ordering_table` gives every part a POLY_FT4
    with code 0x2c, or 0x2e when `flags & 0x200`: 0x2e is the only one the
    hardware blends, and then the tpage's bits 5-6 pick which of the four modes.

    **The frame buffer is 15 bits.** Each primitive's result saturates and is
    quantised to 5 bits a channel the moment it is written, in draw order.
    Accumulating every part in float and clipping once at the end -- which is
    what this did -- leaves a wide, dim shape dim: on the console the same shape
    reaches white in two or three passes, because each pass clamps at 31. That is
    the whole of the "the main layer is too transparent" look.
    """
    import numpy as np
    m = cov > 0
    if not m.any():
        return
    f = np.floor(prim[m] / 8.0)              # 5 bits a channel, as the GPU sees it
    b = np.floor(fb[m] / 8.0)
    if not abe:
        out = f                              # code 0x2c: the texel replaces
    elif mode == 0:
        out = b / 2 + f / 2
    elif mode == 1:
        out = b + f
    elif mode == 2:
        out = b - f
    else:
        out = b + f / 4
    fb[m] = np.clip(np.floor(out), 0, 31) * 8


def extent(fr, tl):
    """The box the whole animation sweeps, around its origin."""
    xs, ys = [], []
    for d in tl:
        for p in fr[d['frame']]:
            xs += [d['x'] + q[0] for q in p['pts']]
            ys += [d['y'] + q[1] for q in p['pts']]
    return (min(xs), min(ys), max(xs), max(ys)) if xs else (0, 0, 0, 0)


_HANDLERS = None


def handler_table():
    """`effect_handlers.json`, read once: which overlay carries which renderer."""
    global _HANDLERS
    if _HANDLERS is None:
        import json
        with open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               'effect_handlers.json')) as f:
            _HANDLERS = json.load(f)
    return _HANDLERS


def mesh_actors(raw, sec, cur, effect_id=None):
    """The overlay's own mesh renderers, and when the timeline runs each of them.

    `battle_effect_code_script_28_step_child_timeline` is explicit: a keyframe's
    `action` is the geometry index plus one, and its `selector & 7` is zero for
    an ordinary particle emitter, otherwise `slot = (selector & 7) - 1` and the
    handler in that slot is called with that geometry index and the keyframe's
    own step counter.

    The slots are filled by script opcode 0x06
    (`battle_effect_code_script_06_load_callback_pointer`): four bytes, the
    second `>> 2` being the slot and the s16 at +2 the routine. Holy's are
    0x00, 0x04, 0x08, 0x0c -- slots 0 to 3 -- holding 7, 8, 9 and 8.

    Which routine each id is remains an inference: E015 carries exactly three
    handlers, the ids are three ascending values with one repeated, and
    `target/effect.yaml` lists the three in address order. Taking them in that
    order puts the ring stack in slot 0 -- and the timeline then drives geometry
    entry 4 through it for a hundred frames, which is the one entry whose
    lattice measures 368 units tall and 3 wide. Two readings that never met
    agreeing on the beam is the evidence for it.
    """
    out = []
    slots = {}
    s2 = sec[2]
    i = 0
    while i + 2 <= len(s2):                       # walk the script by its own widths
        op = s2[i]
        if op not in SCRIPT_OPS:
            break
        if op == 6 and i + 4 <= len(s2):
            slots[s2[i + 1] >> 2] = struct.unpack_from('<h', s2, i + 2)[0]
        if op in (0x00, 0x01):                    # a jump sets the cursor; stop reading
            break
        i += SCRIPT_OPS[op][1]
    if not slots:
        return out
    # Every overlay loads at 0x801c2500, so a routine pointer in
    # `g_battle_effect_file_ptrs` is an offset into whichever one is loaded, and
    # `effect_handlers.json` says which function sits there and where that
    # renderer's own brightness table is. No ordering, no guess.
    table = handler_table()
    over = table['overlays'].get(str(effect_id)) if effect_id is not None else None
    ph = phases(sec[7])
    if not ph or not over:
        return out
    # The parent timeline runs phase 1, then the child, then phase 2, and each
    # has its own five channels; a handler called from a phase channel is called
    # the same way. Walking the child's alone left most overlays' renderers
    # unreached, and left the child's own windows short by phase 1's frames.
    groups = [(0, ph['phase_channels'][0]),
              (ph['phase1'], ph['child_channels']),
              (ph['phase1'] + ph['child'], ph['phase_channels'][1])]
    for base, chans in groups:
        for chan in chans or ():
            for kf in chan or ():
                sel = kf['action'] & 7
                if not sel or kf['emitter'] is None:
                    continue
                routine = slots.get(sel - 1)
                if routine is None or not 0 <= routine < len(table['routines']):
                    continue
                found = over.get(str(table['routines'][routine]))
                if not found:
                    continue                      # a family not transcribed yet
                family, table_off, width = found
                rec = sec[3][20 + 196 * kf['emitter']:20 + 196 * (kf['emitter'] + 1)]
                bright = effect_mesh.brightness_table(raw, table_off, width=width)
                if len(rec) < 196 or not bright:
                    continue
                out.append(dict(frm=base + kf['frm'], to=base + kf['to'],
                                stack=effect_mesh.FAMILY[family](rec, bright, cur)))
    return out


def p_pts(fr, d):
    """Every corner of one draw's parts, in order, ready to be turned together."""
    out = []
    for p in fr[d['frame']]:
        out.extend(p['pts'])
    return out


# ------------------------------------------------- section 7: the effect camera
# `battle_camera_advance_effect_tracks(frame)` drives three tracks off the same
# keyframe tables: rotation (selector bit 0), position (bit 1) and zoom (bit 2).
# **The struct's field names are misleading.** The dispatcher reads
# `main.position[k]` for `battle_camera_execute_angle_command` and
# `main.target[k]` for `battle_camera_execute_position_command`, so the array
# called `position` holds the ANGLES and the one called `target` the position.
#
# Three tables answer, in this order: `main` while the frame is under
# `phase1_duration`, the 17-entry for-each-target table of the timeline itself
# while the children run, and `cleanup` after them.
CAMERA_MAIN, CAMERA_CLEANUP, CAMERA_TARGETS = 0x14e6, 0x16b6, 0x6b2
CAMERA_SOURCE_MASK, CAMERA_MODE_MASK = 0x1e0, 0x1e00
CAMERA_MODES = {0x000: 'idle', 0x200: 'snap', 0x400: 'cosine',
                0x600: 'cosine q12', 0x800: 'linear', 0xa00: 'add accelerating',
                0xc00: 'add', 0xe00: 'add decelerating', 0x1000: 'shake growing',
                0x1200: 'shake', 0x1400: 'shake decaying'}
CAMERA_SOURCES = {0x000: 'target', 0x020: 'current yaw/roll', 0x040: 'given',
                  0x060: 'saved pitch', 0x080: 'map centre', 0x0c0: 'current',
                  0x100: 'saved', 0x140: 'caster', 0x180: 'target average',
                  0x1c0: 'cursor tile'}
# `battle_camera_zoom_map` has exactly two levels: ONE zoomed in, 0xC00
# zoomed out. The tactical view a spell is cast from is the zoomed-out one, and
# an effect that writes `zoom = ONE` -- which Haste and Meteor both do on their
# first keyframe -- is therefore asking for x1.33, not for nothing. Starting the
# preview at ONE made every such keyframe a no-op; the console's own frames
# align onto the scene at 1.4-1.6 by correlation, which is that 1.33 within the
# precision of a measurement taken through a fade.
CAMERA_ONE = 4096                       # libgte's ONE: zoomed in
CAMERA_ZOOM_OUT = 0xC00                 # the tactical view an effect starts from


def camera_table(sec7, at, n):
    """One keyframe table: `effect_camera_keyframes_21_t` at 21, the timeline's at 17.

    Both are the same 22n + 2 layout -- thresholds, angles, positions, zooms,
    selectors, last index -- which is why 0x6b2 through 0x828 reads with the
    very same code as 0x14e6.
    """
    if len(sec7) < at + 22 * n + 2:
        return None
    vec = lambda base: [struct.unpack_from('<3h', sec7, base + 6 * i) for i in range(n)]
    return dict(thresholds=list(struct.unpack_from('<%dh' % n, sec7, at)),
                angle=vec(at + 2 * n), position=vec(at + 8 * n), zoom=vec(at + 14 * n),
                selector=list(struct.unpack_from('<%dH' % n, sec7, at + 20 * n)),
                last=struct.unpack_from('<h', sec7, at + 22 * n)[0])


def camera_tables(sec7):
    """The three tables the dispatcher picks between: main, for-each-target, cleanup."""
    return (camera_table(sec7, CAMERA_MAIN, 21),
            camera_table(sec7, CAMERA_TARGETS, 17),
            camera_table(sec7, CAMERA_CLEANUP, 21))


def camera_find(frame, mask, tabs, phase1, spawn_delay, phase2_delay, hits):
    """`battle_camera_find_active_keyframe`: (table, index, end frame) or None.

    The first keyframe of `mask`'s channel still ahead of `frame`. The main
    table covers the frames before phase 1 ends, the for-each-target table
    repeats every `spawn_delay` per hit target, and the cleanup table is
    relative to the end of that window.
    """
    main, per, clean = tabs
    if main and frame < phase1:
        for i in range(main['last'] + 1):
            if frame < main['thresholds'][i] and mask & main['selector'][i]:
                return 0, i, main['thresholds'][i]
    delay = spawn_delay if hits else 0
    gap = phase2_delay if hits else 0
    if per and delay and frame < phase1 + (hits - 1) * delay:
        at = 0
        local = -1
        if frame >= phase1:
            at = (frame - phase1) // delay
            local = frame - phase1 - at * delay
        for first in (True, False):
            if not first:
                at += 1
                local = -1
                if at >= hits:
                    break
            for i in range(per['last'] + 1):
                if per['thresholds'][i] >= delay:
                    break
                if local < per['thresholds'][i] and mask & per['selector'][i]:
                    return 1, i, per['thresholds'][i] + at * delay + phase1
    if per and hits:
        at = hits - 1
        start = phase1 + at * delay
        if frame < start + gap:
            local = frame - start if frame >= start else -1
            for i in range(per['last'] + 1):
                if local < per['thresholds'][i] and mask & per['selector'][i]:
                    return 1, i, per['thresholds'][i] + at * delay + phase1
    end = phase1 + (hits - 1) * delay + gap
    local = frame - end if frame >= end else -1
    if clean:
        for i in range(clean['last'] + 1):
            if local < clean['thresholds'][i] and mask & clean['selector'][i]:
                return 2, i, clean['thresholds'][i] + end
    return None


def _q13(a, b, t):
    """`battle_effect_lerp_component_q13`: t is q13, 0x2000 being one."""
    return [a[k] + (b[k] - a[k]) * t / 0x2000 for k in range(3)]


def _factor(mode, frame, duration):
    """The q13 t of `battle_effect_interpolate_vector_q13_{linear,cos}`."""
    if duration <= 0:
        return 0x2000
    if mode in (0x400, 0x600):                      # cosine: ONE - rcos(half a turn)
        return CAMERA_ONE - int(round(math.cos((frame << 11) / duration
                                               / 4096.0 * 2 * math.pi) * 4096))
    return (frame << 13) // duration


class _Track:
    """One camera track's running state: start, target, mode, duration, frame.

    `battle_camera_update_effect_rotation_and_zoom` steps all three the same
    way, so one class covers rotation, position and zoom. Only the modes a
    preview can mean anything by are played: snap, the two cosines, linear and
    the three adds. The shakes are random offsets about the start and are left
    at the start, which is what they average to.
    """

    def __init__(self, value):
        self.cur = list(value)
        self.start = list(value)
        self.target = list(value)
        self.mode = 0
        self.duration = 0
        self.frame = 0

    def command(self, flags, duration, vec, absolute, centre=None):
        """`execute_{angle,position,zoom}_command`: pick the source, arm the mode."""
        src = flags & CAMERA_SOURCE_MASK
        if src in (0x0c0, 0x100):                   # current, or saved, plus the offset
            base = self.cur if src == 0x0c0 else self.start
            self.target = [base[k] + vec[k] for k in range(3)]
        elif src == 0x080 and centre:               # the map centre, plus the offset
            self.target = [centre[k] + vec[k] for k in range(3)]
        elif src in CAMERA_SOURCES:
            # Every other source is a point of the battle -- the target, the
            # caster, the map centre, or a zoom given outright -- and a preview
            # is drawn about that point, so in its own frame of reference the
            # source is the origin and the keyframe's vector is the whole of it.
            self.target = list(vec)
        else:
            return
        self.mode = flags & CAMERA_MODE_MASK
        self.duration = duration
        self.frame = 0

    def step(self):
        m, d = self.mode, self.duration
        if m == 0:
            return self.cur
        if m == 0x200:                              # snap
            self.cur = list(self.target)
            self.start = list(self.cur)
            self.mode, self.frame = 0, d
        elif m in (0x400, 0x600, 0x800):            # cosine, cosine q12, linear
            if d > self.frame:
                self.frame += 1
                self.cur = _q13(self.start, self.target, _factor(m, self.frame, d))
            if d == self.frame:
                self.start = list(self.target)
                self.mode = 0
        elif m in (0xa00, 0xc00, 0xe00):            # add, accelerating, decelerating
            if d > self.frame:
                self.frame += 1
                if m == 0xc00:
                    step = self.target
                else:
                    t = _factor(0x800, self.frame, d)
                    step = (_q13([0, 0, 0], self.target, t) if m == 0xa00
                            else _q13(self.target, [0, 0, 0], t))
                self.cur = [self.cur[k] + step[k] for k in range(3)]
            if d == self.frame:
                self.target = list(self.cur)
                self.start = list(self.cur)
                self.mode = 0
        else:                                       # the three shakes: their mean
            if d > self.frame:
                self.frame += 1
            if d == self.frame:
                self.mode = 0
        return self.cur


def camera_play(sec7, frames, hits=1, zoom0=CAMERA_ZOOM_OUT, centre=None):
    """The effect camera frame by frame, about the target: angles, position, zoom.

    `battle_camera_advance_effect_tracks` arms a track only while it is idle,
    and `battle_camera_update_effect_rotation_and_zoom` steps it one frame.
    The preview has no battle behind it, so the camera starts on the target
    with the battle zoom (`ONE`) and every keyframe reads as an offset from
    there -- which is exactly what the ones that matter say: Haste and Meteor
    both zoom from the caster's own view onto the receiver.
    """
    tabs = camera_tables(sec7)
    if not any(tabs):
        return []
    ph = phases(sec7) or {}
    # `phases` reads 0x00a as the child's duration; the struct calls it
    # `phase2_delay`, and the keyframe scanner bounds the per-target window with
    # it. Same halfword, two names for one thing.
    p1, delay, gap = ph.get('phase1', 0), ph.get('spawn_delay', 0), ph.get('child', 0)
    tracks = [_Track((0, 0, 0)), _Track((0, 0, 0)), _Track((zoom0,) * 3)]
    names = ('angle', 'position', 'zoom')
    out = []
    for f in range(max(0, frames)):
        for bit, (tr, name) in enumerate(zip(tracks, names)):
            if tr.mode:
                continue
            got = camera_find(f, 1 << bit, tabs, p1, delay, gap, hits)
            if not got:
                continue
            which, i, end = got
            tab = tabs[which]
            tr.command(tab['selector'][i], end - f, tab[name][i], name == 'zoom',
                       None if name == 'zoom' else centre)
        zoom = tuple(tracks[2].step())
        out.append(dict(angle=tuple(tracks[0].step()),
                        position=tuple(tracks[1].step()),
                        zoom=zoom, scale=zoom[0] / float(zoom0 or 1)))
    return out


def _scene_through(scene, anchor, at, zoom):
    """The scene picture as the effect camera sees it this frame.

    The same map `p -> at + (p - anchor) * zoom` the quads go through, run
    backwards because `Image.transform` samples the source for each output
    pixel. Past the edges of the scene picture it fills with black, which is
    what the console shows too: Meteor's camera climbs right off the
    battlefield and the frames there are black with sparks in them.
    """
    from PIL import Image as _I
    k = 1.0 / (zoom or 1.0)
    return scene.transform(scene.size, _I.AFFINE,
                           (k, 0.0, anchor[0] - at[0] * k,
                            0.0, k, anchor[1] - at[1] * k),
                           resample=_I.BILINEAR)


def render(raw, fr, seqs, cs, size, anchor, scale, raw_sections=None, scatter=True, grow=False,
           effect_id=None, over=None, eye=False, centre=None):
    """One RGB image per displayed 30 fps frame, the whole cast composited."""
    import numpy as np
    tex = _atlas_rgb(raw)
    if tex is None:
        return []
    tex, alpha = tex
    eye_track = (camera_play(raw_sections[7], (phases(raw_sections[7]) or {}).get('total', 0),
                             centre=centre)
                 if eye and raw_sections else None)
    tl = schedule(fr, seqs, cs, phases(raw_sections[7]), scatter,
                  cur=curves(raw_sections[4]), centre=centre, cam=eye_track,
                  phys=physics(raw_sections[3])) if raw_sections else None
    if tl is None:
        tl = timeline(fr, seqs, cs)
    mesh = mesh_actors(raw, raw_sections, curves(raw_sections[4]), effect_id) if raw_sections else []
    if not tl and not mesh:
        return []
    tl = tl or []
    if anchor is None:                          # fit the sweep, then sit it low
        x0, y0, x1, y1 = extent(fr, tl) if tl else (-64, -64, 64, 64)
        # Night Sword sweeps 251 px against a 240-px frame, so its ball fell off
        # the top: shrink until the whole thing is inside before placing it.
        fit = min(1.0, (size[1] * 0.96) / max(1, y1 - y0),
                  (size[0] * 0.96) / max(1, x1 - x0))
        if fit < 1.0 and grow:
            # Grow the frame rather than shrink the effect. Shrinking is what a
            # 256x240 default did to Night Sword -- 0.84, which thins its blade
            # to two pixels and takes the contrast with it, and the blade then
            # reads as too faint when nothing about it is.
            size = (max(size[0], int((x1 - x0) / 0.96) + 2),
                    max(size[1], int((y1 - y0) / 0.96) + 2))
            print('frame grown to %dx%d so the %dx%d sweep fits at 1:1'
                  % (size[0], size[1], x1 - x0, y1 - y0))
            fit = 1.0
        if fit < 1.0:
            scale *= fit
            print('scaled to %.2f so the %dx%d sweep fits the %dx%d frame'
                  % (scale, x1 - x0, y1 - y0, size[0], size[1]))
        anchor = (int(size[0] / 2 - (x0 + x1) / 2 * scale),
                  int(size[1] * 0.98 - y1 * scale))
        print('origin at %d,%d  (the animation sweeps x %d..%d, y %d..%d)'
              % (anchor[0], anchor[1], x0, x1, y0, y1))
    # An effect whose picture is entirely native -- E071 is one -- has no sprite
    # timeline at all, and its length is its handlers' windows instead.
    last = max([d['t'] + d['ticks'] for d in tl] + [m['to'] * 2 for m in mesh])
    out = []
    prim = np.zeros((size[1], size[0], 3), dtype=float)
    cov = np.zeros((size[1], size[0]), dtype=float)
    flat = camera()

    def project3(x, y, z):
        """The battle camera, plus a depth for the ordering table's own sort."""
        sx, sy = flat(x, y, z)
        return sx, sy, 0xC0 - int((z - x) / 8)
    under = scene = None
    if over:
        from PIL import Image as _I
        scene = _I.open(over).convert('RGB').resize(size)
        under = np.asarray(scene).astype(float)
    ph = (phases(raw_sections[7]) or {}) if raw_sections else {}
    ph1, chd = ph.get('phase1', 0), ph.get('child', 0)
    # The three stages the parent script runs in turn, each with the colour
    # tracks `battle_effect_code_script_29_step_parent_timeline` advances while
    # it is the one that runs: phase 1's, then the child's, then phase 2's.
    stages = ([(0, ph1, parent_tracks(raw_sections[7], 0)),
               (ph1, ph1 + chd, tracks(raw_sections[7])),
               (ph1 + chd, 1 << 30, parent_tracks(raw_sections[7], 1))]
              if raw_sections else [])
    # The effect lasts the parent timeline's three stages, not just until its
    # last sprite: `battle_effect_code_script_29_step_parent_timeline` keeps
    # stepping the colour tracks once every emitter is spent. Meteor's two
    # seconds of white screen are phase 2, after the last quad is drawn, and
    # stopping at the last draw cut them off.
    nfr = max(last // 2 + 1, ph1 + chd + ph.get('phase2', 0))
    last = max(last, nfr * 2)
    # `--eye` plays the effect camera over the preview: the scene picture and
    # every quad move and scale together, which is the only way the preview can
    # be compared with the console frame for frame. Without it the battle
    # camera stands still and Meteor's rock stays the size its quads make it.
    cam = camera_play(raw_sections[7], nfr, centre=centre) if eye and raw_sections else []
    # The screen track offsets the background gradient; `battle_map_color_field`
    # colours the map and the units from the affected-units track. A preview's
    # scene IS the map, so both are applied to it -- and only to it, since
    # without `--over` there is no map to recolour.
    #
    # They fire at the right frame and with the right magnitude: Night Sword's
    # blade matches the console to a point or two of mean luminance all the way
    # up, and the flash arrives on the frame it should. **It comes out red where
    # the console is white**, because the keyframe after it reads `base` as
    # (126, 126, 126) and offsets it by (0, -128, -128). Whether `base` is the
    # colour just applied or the one before the fade began is the part of
    # `battle_map_blend_background_gradient_color` still unread.
    screen = stage_offsets(stages, 'screen', nfr, double=True)
    field = stage_offsets(stages, 'affected units', nfr)
    for t in range(0, last, 2):                        # 60 Hz ticks, 30 fps frames
        # `--over` starts each frame on a picture of the battlefield instead of
        # black. The effect's parts are B+F against it exactly as against
        # anything else, which is the only way to see what the console sees:
        # the file has no scene in it, and its colour tracks only darken one.
        fb = np.zeros((size[1], size[0], 3), dtype=float)
        f30 = t // 2
        ax, ay, sc, bg = anchor[0], anchor[1], scale, under
        if cam:
            c = cam[min(f30, len(cam) - 1)]
            zoom = c['scale']
            # The camera looks at `position`, so the world origin -- where the
            # preview's anchor sits -- moves the other way, by that point
            # projected and scaled like everything else.
            px, py = flat(*c['position'])
            ax, ay = anchor[0] - px * zoom, anchor[1] - py * zoom
            sc = scale * zoom
            if scene is not None:
                bg = np.asarray(_scene_through(scene, anchor, (ax, ay), zoom)).astype(float)
        if bg is not None:
            fb[:] = bg
            if f30 < len(field):
                fb[:] = np.clip(fb + np.array(field[f30]), 0, 255)
            if f30 < len(screen):
                fb[:] = np.clip(fb + np.array(screen[f30]), 0, 255)
        for d in tl:
            if not (d['t'] <= t < d['t'] + max(d['ticks'], 1)):
                continue
            # The part's own corners turn with the sprite before its position
            # is added, exactly as the 2D matrix of submit_sprite does.
            corners = rotate2(p_pts(fr, d), d.get('angle', 0.0))
            k = 0
            for p in fr[d['frame']]:
                q = [(ax + (d['x'] + px) * sc, ay + (d['y'] + py) * sc)
                     for px, py in corners[k:k + 4]]
                k += 4
                u, v = p['u'], p['v']
                u1, v1 = (u + p['w']) & 0xff, (v + p['h']) & 0xff
                s = [(u, v), (u1, v), (u, v1), (u1, v1)]
                cell = (min(u, u1), min(v, v1), max(u, u1) - 1, max(v, v1) - 1)
                tint = d.get('tint')
                tx = tex * np.array(tint) if tint else tex
                # One POLY_FT4 a part, rasterised whole and then blended once.
                prim[:] = 0
                cov[:] = 0
                _tri(prim, cov, tx, alpha, (q[0], q[1], q[2]), (s[0], s[1], s[2]), cell)
                _tri(prim, cov, tx, alpha, (q[1], q[3], q[2]), (s[1], s[3], s[2]), cell)
                _blend(fb, prim, cov, (p['tpage'] & 0x200) != 0, (p['clut'] >> 5) & 3)
        # The overlay's own renderers, after the sprites: their quads come back
        # farthest first, which is the ordering table's own answer.
        f30 = t // 2
        for m in mesh:
            if not (m['frm'] <= f30 < m['to']):
                continue
            st = m['stack']
            qs = (st.quads(f30 - m['frm']) if st.SCREEN
                  else st.quads(f30 - m['frm'], project3))
            for q in qs:
                if q.get('screen'):
                    # Screen-space quads: the source writes their corners once
                    # and never projects them, so they land wherever the battle's
                    # own drawing area puts them -- and that area is NOT at the
                    # origin. `battle_state_init_deployment_display` calls
                    # `SetDefDrawEnv(env, -0x80, 0, width + 0x80, height)`, so a
                    # primitive written at x = 0x80 is drawn at pixel 0. The 4x4
                    # grid writes `(column << 6) + 0x80`, i.e. 128..384, which is
                    # exactly the 256-wide screen once that -128 is applied.
                    # Centring the grid on our frame instead left Meteor's rock
                    # three quarters off the right edge.
                    off = (SCREEN_ORIGIN_X + (size[0] - 256) // 2,
                           (size[1] - 240) // 2)
                    d = [(off[0] + x, off[1] + y) for x, y in q['pts']]
                else:
                    d = [(ax + x * sc, ay + y * sc) for x, y in q['pts']]
                uv = q['uv']
                cell = (min(p[0] for p in uv), min(p[1] for p in uv),
                        max(p[0] for p in uv) - 1, max(p[1] for p in uv) - 1)
                g = q['rgb']
                prim[:] = 0
                cov[:] = 0
                _tri(prim, cov, tex, alpha, (d[0], d[1], d[2]), (uv[0], uv[1], uv[2]), cell,
                     (g[0], g[1], g[2]))
                _tri(prim, cov, tex, alpha, (d[1], d[3], d[2]), (uv[1], uv[3], uv[2]), cell,
                     (g[1], g[3], g[2]))
                _blend(fb, prim, cov, q['abe'], q['mode'])
        out.append(np.clip(fb, 0, 255).astype(np.uint8))
    return out


SCREEN_ORIGIN_X = -0x80   # battle draw env: SetDefDrawEnv(env, -0x80, 0, ...)
TI_W, TI_H = 160, 100
TI_SCALE = 0.55                  # the original's 256-wide picture onto 160 px


def grey4(im):
    """The TI's four greys: the effect is additive, so quantise what it adds."""
    import numpy as np
    a = im.astype(np.int32)                        # uint8 * 77 wraps: everything black
    lum = (a[:, :, 0] * 77 + a[:, :, 1] * 151 + a[:, :, 2] * 28) >> 8
    lvl = np.digitize(lum, [10, 40, 100])          # 0 black .. 3 white
    out = (lvl * 85).astype(np.uint8)
    return np.repeat(out[:, :, None], 3, axis=2)


def gif(path, png_dir, n, fps=30):
    """ffmpeg, not PIL: PIL drops a frame identical to the one before it.

    30 fps, not 15: one PNG is one battle frame and the battle runs at 30. At 15
    the whole effect played at half the speed the console shows.
    """
    import subprocess
    if n == 0:
        return
    pal = png_dir / 'palette.png'
    try:
        subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(fps),
                        '-i', str(png_dir / 'f%03d.png'), '-vf', 'palettegen',
                        str(pal)], check=True)
        subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(fps),
                        '-i', str(png_dir / 'f%03d.png'), '-i', str(pal),
                        '-lavfi', 'paletteuse=dither=none', str(path)], check=True)
        pal.unlink()
    except (OSError, subprocess.CalledProcessError) as e:
        print('no GIF (%s)' % e)


def audit(disc, files):
    """Decode every effect on the disc, and say what the grammar did not cover."""
    data = routine = 0
    cells = seqs = casts = 0
    unknown, odd, orphan = [], [], []
    scripts = bad_op = no_end = off_boundary = tls = 0
    tmin, tmax = 10 ** 9, 0
    for n in range(512):
        key = 'EFFECT/E%03d.BIN' % n
        if key not in files or files[key][1] == 0:
            continue
        raw = disc.read(*files[key])
        sec = sections(raw)
        if sec is None:
            routine += 1
            continue
        data += 1
        fr, sq, cs = frames(sec[0]), script(sec[1]), cast(sec[3])
        cells += len(fr); seqs += len(sq); casts += len(cs)
        blanks = next((i for i, f in enumerate(fr) if f), len(fr))
        if not fr or any(not f for f in fr[blanks:]):
            odd.append(n)
        for i, ops in enumerate(sq):
            if any(o[0].startswith('?') for o in ops):
                unknown.append((n, i, [o[0] for o in ops if o[0].startswith('?')]))
            if not ops or ops[-1][0] != 'loop':
                unknown.append((n, i, ['no loop']))
        if any(c['seq'] >= len(sq) for c in cs):
            orphan.append(n)
        pr = program(sec[2])
        if pr:
            scripts += 1
            bounds = {o for o, _, _, _, _ in pr}
            if any(nm.startswith('?') for _, _, nm, _, _ in pr):
                bad_op += 1
            if pr[-1][1] != 0x04:
                no_end += 1
            # every jump operand is a byte offset into the script: it must land on
            # an instruction. This is what checks the widths taken from the decomp.
            for _, op, nm, _, ops in pr:
                if nm.startswith('jump') and ops and ops[-1] not in bounds:
                    off_boundary += 1
                    break
        t = phases(sec[7])
        if t:
            tls += 1
            tmin = min(tmin, t['total']); tmax = max(tmax, t['total'])
    print('%d data files, %d MIPS routines' % (data, routine))
    print('%d cells, %d sequences, %d cast members' % (cells, seqs, casts))
    print('sequences with an opcode this does not know: %d' % len(unknown))
    for u in unknown[:10]:
        print('   E%03d seq %d  %s' % u)
    print('files with a frame record that is not 4 + 24n, past the blanks: %d  %s'
          % (len(odd), odd[:10]))
    print('files casting a sequence that does not exist: %d  %s' % (len(orphan), orphan[:10]))
    print('scripts: %d disassembled, %d with an unknown opcode, %d ending without `end`,'
          ' %d with a jump off an instruction boundary'
          % (scripts, bad_op, no_end, off_boundary))
    print('timelines: %d read; durations %d..%d frames at 30 fps' % (tls, tmin, tmax))


# ---------------------------------------------------------------- CLI

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('id', type=int, help='effect number, e.g. 163')
    ap.add_argument('--script', action='store_true', help='the byte-code, readably')
    ap.add_argument('--cells', action='store_true', help='the section-0 frames')
    ap.add_argument('--cast', action='store_true', help='the section-3 records')
    ap.add_argument('--timeline', action='store_true', help='every draw, in tick order')
    ap.add_argument('--program', action='store_true', help='section 2: the effect script')
    ap.add_argument('--phases', action='store_true', help='section 7: durations and channels')
    ap.add_argument('--tracks', action='store_true', help='section 7: the four colour tracks')
    ap.add_argument('--map-centre', dest='map_centre', default=None,
                    help='the world vector from the target to the map centre, "x,y,z"; '
                         'Gariland from the roof tile is 42,96,0')
    ap.add_argument('--eye', action='store_true',
                    help='play the effect camera over the render: pan, zoom, scene and all')
    ap.add_argument('--camera', action='store_true',
                    help='section 7: the effect camera, keyframes then frame by frame')
    ap.add_argument('--render', type=Path, help='PNG per frame plus a GIF, in this directory')
    ap.add_argument('--size', default=None,
                    help='the render canvas; by default 256x240, grown to fit the effect at 1:1')
    ap.add_argument('--anchor', default=None, help='where the effect origin sits, "x,y"')
    ap.add_argument('--over', default=None,
                    help='a picture of the battlefield to draw the effect onto')
    ap.add_argument('--scale', type=float, default=1.0)
    ap.add_argument('--no-scatter', action='store_true', dest='no_scatter',
                    help='ignore the emitters\' spawn spread, piling each stream on one point')
    ap.add_argument('--flat', action='store_true',
                    help='ignore the timeline: play every sequence once from t=0')
    ap.add_argument('--ti', action='store_true',
                    help='render the TI screen instead: 160x100, four greys')
    ap.add_argument('--audit', action='store_true',
                    help='decode every effect on the disc and report what did not parse')
    a = ap.parse_args()

    disc, files = extract.disc_files(str(core_audit.DEFAULT_DISC))
    if a.audit:
        audit(disc, files)
        return
    key = 'EFFECT/E%03d.BIN' % a.id
    if key not in files or files[key][1] == 0:
        sys.exit('%s is empty' % key)
    raw = disc.read(*files[key])
    sec = sections(raw)
    if sec is None:
        sys.exit('E%03d is a MIPS routine, not data' % a.id)
    fr, seqs, cs = frames(sec[0]), script(sec[1]), cast(sec[3])
    print('E%03d  %d cells, %d sequences, %d cast members'
          % (a.id, len(fr), len(seqs), len(cs)))

    if a.camera:
        sec7 = sec[7] if len(sec) > 7 else b''
        names = ('main', 'for each target', 'cleanup')
        for tab, name in zip(camera_tables(sec7), names):
            if not tab or tab['last'] < 0:
                continue
            print(' %s, %d keyframes' % (name, tab['last'] + 1))
            for i in range(tab['last'] + 1):
                sel = tab['selector'][i]
                what = [n for b, n in ((1, 'angle'), (2, 'position'), (4, 'zoom'))
                        if sel & b]
                print('   f%-4d %-22s %-14s %s  angle %s  pos %s  zoom %s'
                      % (tab['thresholds'][i],
                         CAMERA_MODES.get(sel & CAMERA_MODE_MASK, '?'),
                         CAMERA_SOURCES.get(sel & CAMERA_SOURCE_MASK, '?'),
                         ','.join(what) or '-',
                         tab['angle'][i], tab['position'][i], tab['zoom'][i]))
        ph = phases(sec7) or {}
        play = camera_play(sec7, ph.get('total', 0))
        prev = None
        for f, c in enumerate(play):
            row = (c['position'], c['zoom'])
            if row != prev:
                print('   frame %3d  pos %-20s zoom %-20s  x%.2f'
                      % (f, '%d,%d,%d' % tuple(int(v) for v in c['position']),
                         '%d,%d,%d' % tuple(int(v) for v in c['zoom']), c['scale']))
            prev = row

    if a.cells:
        for i, polys in enumerate(fr):
            print(' cell %3d  %d poly%s' % (i, len(polys), '' if len(polys) == 1 else 's'))
            for p in polys:
                print('      (%3d,%3d) %3dx%-4d %s' % (p['u'], p['v'], p['w'], p['h'],
                                                        ' '.join('%4d,%-4d' % q for q in p['pts'])))
    if a.script:
        for i, ops in enumerate(seqs):
            used = [c for c, m in enumerate(cs) if m['seq'] == i]
            print(' sequence %2d   cast %s' % (i, used or '-'))
            t = 0
            for op in ops:
                if op[0] == 'frame':
                    print('   t%+5d  cell %3d  x%d ticks%s'
                          % (t, op[1], op[2], '' if op[3] == 1 else '   depth %d' % op[3]))
                    t += op[2]
                elif op[0] == 'moveto':
                    print('   t%+5d  move to  %5d,%-5d' % (t, op[1], op[2]))
                elif op[0] == 'moveby':
                    print('   t%+5d  move by  %5d,%-5d' % (t, op[1], op[2]))
                else:
                    print('   t%+5d  %s' % (t, op[0]))
            print('   loops after %d ticks (%d frames at 30 fps)' % (t, t // 2))
    if a.cast:
        words = [struct.unpack_from('<%dh' % (REC_SIZE // 2), c['raw'], 0) for c in cs]
        print(' %-4s %-4s %s' % ('#', 'seq', 'fields that differ between records'))
        varying = [w for w in range(REC_SIZE // 2)
                   if len({x[w] for x in words}) > 1 and w != 0]
        print('      byte  ' + ' '.join('%6d' % (2 * w) for w in varying))
        for i, c in enumerate(cs):
            print(' %-4d %-4d ' % (i, c['seq']) + ' '.join('%6d' % words[i][w] for w in varying))
    if a.program:
        for off, op, name, sel, ops in program(sec[2]):
            print('   %04x  %02x %-26s %s%s' % (off, op, name,
                  ' '.join('%6d' % v for v in ops), '   sel %02x' % sel if sel else ''))
    if a.tracks:
        tr = tracks(sec[7])
        for name in COLOUR_TRACKS:
            seq = tr.get(name)
            if not seq:
                continue
            print(' %s' % name)
            t = 0
            for e in seq:
                print('   f%4d..%-4d  %-13s %-6s %s%s'
                      % (t, t + e['frames'],
                         COLOUR_MODES[e['mode']] if e['mode'] < len(COLOUR_MODES) else e['mode'],
                         'offset' if e['offset'] else 'set', e['colour'],
                         ('  lower ' + str(e['second'])) if 'second' in e else ''))
                t += e['frames']
    if a.phases:
        t = phases(sec[7])
        if not t:
            print(' no timeline')
        else:
            print(' phase 1 %d frames, child %d, phase 2 %d  -> %d frames at 30 fps'
                  ' (spawn delay %d between children)'
                  % (t['phase1'], t['child'], t['phase2'], t['total'], t['spawn_delay']))
            for label, chans in (('child', t['child_channels']),
                                 ('phase 1', t['phase_channels'][0]),
                                 ('phase 2', t['phase_channels'][1])):
                for c, kfs in enumerate(chans):
                    if not kfs:
                        continue
                    print('   %s, particle channel %d' % (label, c))
                    for k in kfs:
                        if k['emitter'] is None and not k['action']:
                            continue
                        print('      f%3d..%-3d  %s%s'
                              % (k['frm'], k['to'],
                                 'emitter %-3d' % k['emitter'] if k['emitter'] is not None
                                 else '           ',
                                 '  ' + action_text(k['action']) if k['action'] else ''))
    if a.timeline:
        for d in timeline(fr, seqs, cs):
            print(' t%4d (f%3d)  inst %2d seq %2d  cell %3d  at %5d,%-5d  %d ticks'
                  % (d['t'], d['t'] // 2, d['inst'], d['seq'], d['frame'], d['x'], d['y'], d['ticks']))
    if a.render:
        from PIL import Image
        w, h = (int(v) for v in (a.size or '256x240').split('x'))
        anchor = tuple(int(v) for v in a.anchor.split(',')) if a.anchor else None
        centre = [int(v) for v in a.map_centre.split(',')] if a.map_centre else None
        if a.ti:
            w, h = TI_W, TI_H
        ims = render(raw, fr, seqs, cs, (w, h), anchor,
                     a.scale * (TI_SCALE if a.ti else 1.0),
                     None if a.flat else sec, not a.no_scatter,
                     grow=a.size is None and not a.ti, effect_id=a.id, over=a.over,
                     eye=a.eye, centre=centre)
        if a.ti:
            ims = [grey4(im) for im in ims]
        a.render.mkdir(parents=True, exist_ok=True)
        for i, im in enumerate(ims):
            Image.fromarray(im).save(a.render / ('f%03d.png' % i))
        gif(a.render / 'anim.gif', a.render, len(ims))
        print('%d frames -> %s' % (len(ims), a.render))


if __name__ == '__main__':
    main()
