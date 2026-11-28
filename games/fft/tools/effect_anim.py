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
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import core_audit  # noqa: E402
import effect_rip  # noqa: E402
import extract  # noqa: E402

REC_SIZE = 196          # effect_geometry_entry_t: one particle emitter
LOOP, MOVETO, MOVEBY = 0x81, 0x82, 0x83


def sections(raw):
    """The ten sections of an effect file, or None when it is a MIPS routine."""
    if len(raw) < 40 or struct.unpack_from('<I', raw, 0)[0] != 40:
        return None
    off = struct.unpack_from('<10I', raw, 0)
    ends = list(off[1:]) + [len(raw)]
    out = []
    for i in range(10):
        a, b = off[i], ends[i] or len(raw)
        out.append(raw[a:b] if 0 < a <= b <= len(raw) else b'')
    return out


def frames(sec0):
    """[[poly, ...], ...] -- the cells of each frame, in drawing order."""
    if len(sec0) < 6:
        return []
    words = struct.unpack_from('<%dH' % (len(sec0) // 2), sec0, 0)
    tbl = []
    for v in words[2:]:                       # the table stops where it stops rising
        if v == 0 or v >= len(sec0) or (tbl and v <= tbl[-1]):
            break
        tbl.append(v)
    out = []
    head = 4 + 2 * len(tbl)                   # the table itself
    for i, t in enumerate(tbl):
        a = t + 4                             # the offset lands on the previous tail
        b = (tbl[i + 1] + 4) if i + 1 < len(tbl) else len(sec0)
        if a < head or b - a < 4 or (b - a - 4) % 24:
            out.append([])                    # the low indices point into the table:
            continue                          # cell 0 is the blank, drawn as a pause
        n = struct.unpack_from('<H', sec0, a + 2)[0]
        polys = []
        for k in range(min(n, (b - a - 4) // 24)):
            o = a + 4 + 24 * k
            tpage, clut = struct.unpack_from('<2H', sec0, o)
            # w and h are SIGNED: a negative one mirrors the cell. 23 of Stasis
            # Sword's 298 polygons carry one, and read unsigned they index 224
            # columns into a 128-wide atlas.
            u, v = sec0[o + 4], sec0[o + 5]
            w, h = struct.unpack_from('<2b', sec0, o + 6)
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
    0x26: ('spawn_emitter_particles', 2), 0x27: ('init_particle_physics', 2),
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
    """(vx, vy) of a world (dx, dy, dz): two rows of Rx(pitch) * Ry(yaw).

    At tilt low a world y reaches the screen at cos 26.5 = 0.895, and a
    horizontal offset splits between x at cos/sin of the yaw and y at 0.316.
    """
    th, ph = pitch / 4096.0 * 2 * math.pi, yaw / 4096.0 * 2 * math.pi
    st, ct, sp, cp = math.sin(th), math.cos(th), math.sin(ph), math.cos(ph)

    def project(dx, dy, dz=0):
        return (cp * dx + sp * dz, st * sp * dx + ct * dy - st * cp * dz)
    return project


def _ellipsoid(sx, sy, sz, key):
    """A repeatable point inside the spawn spread's ellipsoid."""
    h = 2166136261
    for _ in range(24):
        v = []
        for i in range(3):
            for q in key + (i,):
                h = ((h ^ (q & 0xffff)) * 16777619) & 0xffffffff
            v.append(((h >> 8) & 0xffff) / 65535.0 * 2 - 1)
        if v[0] ** 2 + v[1] ** 2 + v[2] ** 2 <= 1:
            return v[0] * sx, v[1] * sy, v[2] * sz
        key = (key[0] + 7919,) + key[1:]
    return 0, 0, 0


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


def schedule(fr, seqs, cs, tl, scatter=False, project=None):
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
    for c, kfs in enumerate(tl['child_channels']):
        for k in kfs or []:
            e = k['emitter']
            if e is None or e >= len(cs):
                continue
            rec = cs[e]['raw']
            seq = rec[1]
            if seq >= len(seqs):
                continue
            count = max(1, struct.unpack_from('<h', rec, 0xb0)[0])
            interval = max(1, struct.unpack_from('<h', rec, 0xb4)[0])
            life = struct.unpack_from('<h', rec, 0x94)[0]
            # Where this emitter sits, from `spawn_position_start` (0x14).
            # `EFFECT_LIST_NODE_WORLD` adds the particle's world position and
            # the sequence's own offset *before* the camera matrix and projects
            # them with RotTrans -- no perspective divide -- so both are world
            # units, and the quad corners alone are screen pixels. Both go
            # through the camera here, so Night Sword's ball lands high and its
            # burst on the victim. The spread at 0x20 is `--scatter`: it is the
            # ellipsoid a spawn fills, and without the particle velocities that
            # is only half the model.
            pw = struct.unpack_from('<3h', rec, 0x14)
            sw = [abs(v) for v in struct.unpack_from('<3h', rec, 0x14 + 0x0c)]
            # The spawn gate is `time % spawn_interval != 0`, and `time` is NOT
            # the record's frame counter: the child timeline passes
            # `step = state.child.step[i]++`, which `state.child.step[i] = 0`
            # resets on entering each keyframe. So the count starts at 0 inside
            # every keyframe, step 0 always passes, and an emitter spawns on the
            # keyframe's first frame and then every `interval` frames.
            # That is why Stasis Sword's shards appear at all with an interval
            # of 49 inside 2-frame keyframes -- only step 0 ever passes -- and
            # it is also why Night Sword's ball is spawned three times: interval
            # 1 across a 3-frame keyframe. Those three are in the file.
            born = k['frm']
            while born < max(k['to'], k['frm'] + 1):
                die = tl['child'] if life < 0 else min(tl['child'], born + life)
                t0 = (tl['phase1'] + born) * 2          # frames at 30 fps -> 60 Hz ticks
                t1 = (tl['phase1'] + die) * 2
                for i in range(count):
                    j = _ellipsoid(sw[0], sw[1], sw[2], (e, born, i)) if scatter \
                        else (0, 0, 0)
                    ox, oy = project(pw[0] + j[0], pw[1] + j[1], pw[2] + j[2])
                    for d in play_window(seqs[seq], len(fr), t0, t1, life < 0):
                        sx, sy = project(d['x'], d['y'])
                        out.append(dict(d, inst=e, seq=seq, chan=c,
                                        x=ox + sx, y=oy + sy))
                born += interval
    out.sort(key=lambda d: (d['t'], d['chan'], d['inst']))
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


def _tri(acc, tex, alpha, dst, src, cell):
    """Add one textured triangle to the accumulator, PS1-style (additive)."""
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
    a = (alpha[sv, su] * inside)[:, :, None]
    acc[lo_y:hi_y, lo_x:hi_x] += tex[sv, su] * a


def extent(fr, tl):
    """The box the whole animation sweeps, around its origin."""
    xs, ys = [], []
    for d in tl:
        for p in fr[d['frame']]:
            xs += [d['x'] + q[0] for q in p['pts']]
            ys += [d['y'] + q[1] for q in p['pts']]
    return (min(xs), min(ys), max(xs), max(ys)) if xs else (0, 0, 0, 0)


def render(raw, fr, seqs, cs, size, anchor, scale, raw_sections=None, scatter=False):
    """One RGB image per displayed 30 fps frame, the whole cast composited."""
    import numpy as np
    tex = _atlas_rgb(raw)
    if tex is None:
        return []
    tex, alpha = tex
    tl = schedule(fr, seqs, cs, phases(raw_sections[7]), scatter) if raw_sections else None
    if tl is None:
        tl = timeline(fr, seqs, cs)
    if not tl:
        return []
    if anchor is None:                          # fit the sweep, then sit it low
        x0, y0, x1, y1 = extent(fr, tl)
        # Night Sword sweeps 251 px against a 240-px frame, so its ball fell off
        # the top: shrink until the whole thing is inside before placing it.
        fit = min(1.0, (size[1] * 0.96) / max(1, y1 - y0),
                  (size[0] * 0.96) / max(1, x1 - x0))
        if fit < 1.0:
            scale *= fit
            print('scaled to %.2f so the %dx%d sweep fits the %dx%d frame'
                  % (scale, x1 - x0, y1 - y0, size[0], size[1]))
        anchor = (int(size[0] / 2 - (x0 + x1) / 2 * scale),
                  int(size[1] * 0.98 - y1 * scale))
        print('origin at %d,%d  (the animation sweeps x %d..%d, y %d..%d)'
              % (anchor[0], anchor[1], x0, x1, y0, y1))
    last = max(d['t'] + d['ticks'] for d in tl)
    out = []
    for t in range(0, last, 2):                        # 60 Hz ticks, 30 fps frames
        acc = np.zeros((size[1], size[0], 3), dtype=float)
        for d in tl:
            if not (d['t'] <= t < d['t'] + max(d['ticks'], 1)):
                continue
            for p in fr[d['frame']]:
                q = [(anchor[0] + (d['x'] + px) * scale,
                      anchor[1] + (d['y'] + py) * scale) for px, py in p['pts']]
                u, v, w, h = p['u'], p['v'], p['w'], p['h']
                s = [(u, v), (u + w, v), (u, v + h), (u + w, v + h)]
                cell = (min(u, u + w), min(v, v + h),
                        max(u, u + w) - 1, max(v, v + h) - 1)
                _tri(acc, tex, alpha, (q[0], q[1], q[2]), (s[0], s[1], s[2]), cell)
                _tri(acc, tex, alpha, (q[1], q[3], q[2]), (s[1], s[3], s[2]), cell)
        out.append(np.clip(acc, 0, 255).astype(np.uint8))
    return out


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
    ap.add_argument('--render', type=Path, help='PNG per frame plus a GIF, in this directory')
    ap.add_argument('--size', default='256x240', help='the render canvas (default 256x240)')
    ap.add_argument('--anchor', default=None, help='where the effect origin sits, "x,y"')
    ap.add_argument('--scale', type=float, default=1.0)
    ap.add_argument('--scatter', action='store_true',
                    help="spread each spawn through the emitter's ellipsoid "
                         '(half a model without the particle velocities)')
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
        w, h = (int(v) for v in a.size.split('x'))
        anchor = tuple(int(v) for v in a.anchor.split(',')) if a.anchor else None
        if a.ti:
            w, h = TI_W, TI_H
        ims = render(raw, fr, seqs, cs, (w, h), anchor,
                     a.scale * (TI_SCALE if a.ti else 1.0),
                     None if a.flat else sec, a.scatter)
        if a.ti:
            ims = [grey4(im) for im in ims]
        a.render.mkdir(parents=True, exist_ok=True)
        for i, im in enumerate(ims):
            Image.fromarray(im).save(a.render / ('f%03d.png' % i))
        gif(a.render / 'anim.gif', a.render, len(ims))
        print('%d frames -> %s' % (len(ims), a.render))


if __name__ == '__main__':
    main()
