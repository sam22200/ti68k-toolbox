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

**What this tool does NOT model, and why its playback is not the game's.** A sequence
*loops*; what makes an effect start, stagger and stop is elsewhere:

* section 2 is the **effect script**, a 9-bit-opcode interpreter (spawn child, free
  child, counters, branches, camera, step timeline);
* section 7 is the **timeline**, 6278 bytes: `phase1_duration`, `phase2_duration`,
  **`spawn_delay` -- the frames between child spawns**, and per particle channel a
  keyframe table of 25 `frame_start` boundaries with an emitter each.

So this plays every emitter's sequence once, from t=0, with no stagger and no end. That
is why the decoded tail of an effect can show something the game never reaches, and why
nothing here could find the start delay: it is in section 7, not in the emitter record.

The layouts above are the names of **adamrt/fft_decomp**, a byte-exact decompilation of
SCUS-94221; `include/fft/effect/effect_file.h`, `effect_timeline.h` and
`effect_emitter_data.h`. Everything this tool had reverse-engineered by hand matched it,
except the three things listed here.
"""
import argparse
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
    """The effect's own atlas as float RGB and alpha arrays, from section 9."""
    import numpy as np
    got = effect_rip.atlas(raw)
    if not got:
        return None
    w, h, pixels, colours = got
    lut = np.array([effect_rip.rgba(colours, i) for i in range(256)], dtype=float)
    idx = np.frombuffer(pixels, dtype=np.uint8).reshape(h, w)
    px = lut[idx]
    return px[:, :, :3], px[:, :, 3] / 255.0


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


def render(raw, fr, seqs, cs, size, anchor, scale):
    """One RGB image per displayed 30 fps frame, the whole cast composited."""
    import numpy as np
    tex = _atlas_rgb(raw)
    if tex is None:
        return []
    tex, alpha = tex
    tl = timeline(fr, seqs, cs)
    if not tl:
        return []
    if anchor is None:                          # sit the sweep on the ground line
        x0, y0, x1, y1 = extent(fr, tl)
        anchor = (int(size[0] / 2 - (x0 + x1) / 2 * scale),
                  int(size[1] * 0.92 - y1 * scale))
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


def gif(path, png_dir, n, fps=15):
    """ffmpeg, not PIL: PIL drops a frame identical to the one before it."""
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
    print('%d data files, %d MIPS routines' % (data, routine))
    print('%d cells, %d sequences, %d cast members' % (cells, seqs, casts))
    print('sequences with an opcode this does not know: %d' % len(unknown))
    for u in unknown[:10]:
        print('   E%03d seq %d  %s' % u)
    print('files with a frame record that is not 4 + 24n, past the blanks: %d  %s'
          % (len(odd), odd[:10]))
    print('files casting a sequence that does not exist: %d  %s' % (len(orphan), orphan[:10]))


# ---------------------------------------------------------------- CLI

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('id', type=int, help='effect number, e.g. 163')
    ap.add_argument('--script', action='store_true', help='the byte-code, readably')
    ap.add_argument('--cells', action='store_true', help='the section-0 frames')
    ap.add_argument('--cast', action='store_true', help='the section-3 records')
    ap.add_argument('--timeline', action='store_true', help='every draw, in tick order')
    ap.add_argument('--render', type=Path, help='PNG per frame plus a GIF, in this directory')
    ap.add_argument('--size', default='256x240', help='the render canvas (default 256x240)')
    ap.add_argument('--anchor', default=None, help='where the effect origin sits, "x,y"')
    ap.add_argument('--scale', type=float, default=1.0)
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
        ims = render(raw, fr, seqs, cs, (w, h), anchor, a.scale * (TI_SCALE if a.ti else 1.0))
        if a.ti:
            ims = [grey4(im) for im in ims]
        a.render.mkdir(parents=True, exist_ok=True)
        for i, im in enumerate(ims):
            Image.fromarray(im).save(a.render / ('f%03d.png' % i))
        gif(a.render / 'anim.gif', a.render, len(ims))
        print('%d frames -> %s' % (len(ims), a.render))


if __name__ == '__main__':
    main()
