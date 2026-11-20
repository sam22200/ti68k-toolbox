#!/usr/bin/env python3
"""Pack four-grey body PNGs into an FMB1 wide monster body bank (`ffmb`).

Generic: any poses described by a JSON manifest (format in
games/fft/campaign/MONSTER_BODY.md). Each PNG holds one body in four greys
(255, 170, 85, 0) with transparency (alpha < 128); the packer adds the one-pixel
white outline (cross dilation, as the other monster banks), crops empty rows and
16-column strips, and pools identical frames. It reads no disc and draws nothing.

    monster_body_bank.py manifest.json -o ffmb.bin [--preview sheet.png]
"""
import argparse
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image
from scipy import ndimage

JOB0, JOBS, FAMILIES, ANIMS, ROWS, FRAMES = 94, 48, 48, 10, 40, 1024
BOX_W = 64                                    # the family probe box, wider than a picture
HEADER = 8+JOBS
POSES = ('idle', 'walk', 'attack', 'hurt', 'dead')
CROSS = np.array([[0, 1, 0], [1, 1, 1], [0, 1, 0]], bool)


def body(image, foot):
    """(tones 0..3, body mask) of one PNG; tones: 0 white .. 3 black."""
    rgba = np.asarray(image.convert('LA'))
    grey, alpha = rgba[:, :, 0].astype(int), rgba[:, :, 1]
    mask = alpha >= 128
    if not mask.any():
        raise ValueError('empty body picture')
    tones = np.clip((255-grey+42)//85, 0, 3)
    return tones, mask, foot


def encode(tones, mask, foot):
    """One FMB1 frame: h, row bytes, rows above the feet, columns left of them."""
    fx, fy = foot
    pad = 2                                       # room for the outline everywhere
    tones = np.pad(tones, pad)
    mask = np.pad(mask, pad)
    fx, fy = fx+pad, fy+pad
    outline = ndimage.binary_dilation(mask, structure=CROSS)
    yy, xx = np.nonzero(outline)
    top, bottom, left = int(yy.min()), int(yy.max())+1, int(xx.min())
    width = (int(xx.max())+1-left+15)//16*16
    if width > 48 or bottom-top > ROWS:
        raise ValueError(f'body {width}x{bottom-top} exceeds 48x{ROWS}')
    if not 1 <= fy-top <= ROWS or bottom-fy > 12 or not 0 <= fx-left <= width:
        raise ValueError('feet outside the body box')
    if left+width > outline.shape[1]:
        grow = left+width-outline.shape[1]
        tones, mask, outline = (np.pad(a, ((0, 0), (0, grow))) for a in (tones, mask, outline))
    sl = (slice(top, bottom), slice(left, left+width))
    t, m, o = tones[sl], mask[sl], outline[sl]
    planes = ((t & 1).astype(bool) & m, (t & 2).astype(bool) & m, ~o, m)
    head = bytes((bottom-top, width//8, fy-top, fx-left))
    # Each plane holds its 16-column strips one after the other, every strip a run of
    # h 16-bit rows: the renderer draws an uncovered strip straight from the archive.
    return head+b''.join(np.packbits(p, axis=1).reshape(bottom-top, width//16, 2)
                         .transpose(1, 0, 2).tobytes() for p in planes)


def envelope(frames):
    """The box every picture of a family sits in: rows, row bytes, feet."""
    above = max(f[2] for f in frames)
    below = max(f[0]-f[2] for f in frames)
    left = max(f[3] for f in frames)
    right = max(8*f[1]-f[3] for f in frames)
    rows, width = above+below, (left+right+15)//16*16
    # The box is only a probe window, never a picture: it may be wider than a frame,
    # because the source shifts a body sideways between poses.
    if rows > ROWS or width > BOX_W or below > 12:
        raise ValueError(f'family box {width}x{rows} exceeds {BOX_W}x{ROWS}')
    return bytes((rows, width//8, above, left))


def pack(families):
    """families: [{'jobs': [...], 'anims': {anim index: ([frames], [waits])}}] -> bytes, info.

    A frame slot carries its own wait in ticks, so a source sequence keeps its
    original rhythm without repeating a picture; slots holding the same pixels
    share them, and identical whole sequences share their slots.
    """
    if not 1 <= len(families) <= FAMILIES:
        raise ValueError('1..48 families')
    jobmap = [255]*JOBS
    slots, physical, table, boxes = [], {}, [], []   # slots: (frame bytes, wait)
    for f, fam in enumerate(families):
        boxes.append([])
        for job in fam['jobs']:
            if not 0 <= job-JOB0 < JOBS or jobmap[job-JOB0] != 255:
                raise ValueError(f'job {job}: not a free monster job')
            jobmap[job-JOB0] = f
        for a in range(ANIMS):
            frames, waits = fam['anims'][a]
            if not frames or len(frames) > 255 or len(waits) != len(frames):
                raise ValueError(f'family {f} animation {a}: frames/waits')
            if any(not 1 <= w <= 255 for w in waits):
                raise ValueError(f'family {f} animation {a}: wait out of 1..255')
            if not 1 <= sum(waits) <= 255:
                raise ValueError(f'family {f} animation {a}: the loop must last 1..255 ticks')
            boxes[f].extend(frames)
            key = tuple(zip(frames, waits))
            if key not in physical:
                physical[key] = len(slots)
                slots.extend(key)
            table.append((physical[key], len(frames), sum(waits)))
    if len(slots) > FRAMES:
        raise ValueError('too many frame slots')
    out = bytearray(b'FMB1'+struct.pack('>HH', len(families), len(slots)))
    out += bytes(jobmap)
    out += b''.join(envelope(f) for f in boxes)      # one box per family, any pose
    out += b''.join(struct.pack('>HBB', *e) for e in table)
    base = len(out)
    store = base+2*(len(slots)+1)+len(slots)+(len(slots) & 1)
    offsets, blobs, raw = [], {}, bytearray()
    for data, _ in slots:
        if data not in blobs:
            blobs[data] = store+len(raw)
            raw += data
        offsets.append(blobs[data])
    size = store+len(raw)
    if size > 65512:
        raise ValueError(f'FMB1 is {size} bytes: one archive variable holds 65512')
    out += struct.pack(f'>{len(offsets)+1}H', *offsets, size)
    out += bytes(w for _, w in slots)+bytes(len(slots) & 1)
    out += raw
    validate(bytes(out))
    return bytes(out), dict(families=len(families), slots=len(slots),
                            pictures=len(blobs), bytes=size)


def validate(raw):
    """The cold C gate (monster_body_valid), for packaging."""
    def reject(why):
        raise ValueError(f'Invalid FMB1 bank: {why}')
    if len(raw) < HEADER or raw[:4] != b'FMB1':
        reject('magic')
    fams, slots = struct.unpack_from('>HH', raw, 4)
    if not 1 <= fams <= FAMILIES or not 1 <= slots <= FRAMES:
        reject('counts')
    base = HEADER+4*fams+4*ANIMS*fams
    waits = base+2*(slots+1)
    first = waits+slots+(slots & 1)
    if first > len(raw) or len(raw) > 65512:
        reject('size')
    if any(v != 255 and v >= fams for v in raw[8:8+JOBS]):
        reject('job map')
    if any(w == 0 for w in raw[waits:waits+slots]):
        reject('a frame slot waits no tick')
    for f in range(fams):
        rows, wb, ay, ax = raw[HEADER+4*f:HEADER+4*f+4]
        if not 1 <= rows <= ROWS or wb not in (2, 4, 6, 8) or not 1 <= ay <= ROWS \
                or rows-ay > 12 or ax > 8*wb:
            reject('family box')
    for i in range(ANIMS*fams):
        start, n, total = struct.unpack_from('>HBB', raw, HEADER+4*fams+4*i)
        if not n or not total or start+n > slots:
            reject('animation')
        if sum(raw[waits+start:waits+start+n]) != total:
            reject('animation total ticks')
    offsets = struct.unpack_from(f'>{slots+1}H', raw, base)
    for i in range(ANIMS*fams):
        start, n, _ = struct.unpack_from('>HBB', raw, HEADER+4*fams+4*i)
        box = raw[HEADER+4*(i//ANIMS):HEADER+4*(i//ANIMS)+4]
        for k in range(n):
            g = raw[offsets[start+k]:offsets[start+k]+4]
            if not (g[3] <= box[3] and g[2] <= box[2] and 8*g[1]-g[3] <= 8*box[1]-box[3]
                    and g[0]-g[2] <= box[0]-box[2]):
                reject('a picture leaves its family box')
    top = 0
    for start in offsets[:-1]:
        if start & 1 or start < first or len(raw)-start < 4:
            reject('offsets')
        h, wb, ay, ax = raw[start:start+4]
        if not 1 <= h <= ROWS or wb not in (2, 4, 6) or not 1 <= ay <= ROWS or h-ay > 12 \
                or ax > 8*wb or len(raw)-start < 4+4*h*wb:
            reject('frame header')
        n = h*wb
        p = [raw[start+4+k*n:start+4+(k+1)*n] for k in range(4)]
        if any((a | b | c) & m for a, b, m, c in zip(p[0], p[1], p[2], p[3])):
            reject('opaque bits under the transparency mask')
        top = max(top, start+4+4*n)
    if top != len(raw) or offsets[-1] != len(raw):
        reject('trailing bytes')
    return True


def load(manifest):
    """Manifest JSON -> pack() families; PNG paths relative to the manifest.

    A pose gives `wait` (the same for every frame) or `waits` (one per frame).
    """
    spec = json.loads(Path(manifest).read_text())
    root = Path(manifest).parent
    families = []
    for fam in spec['families']:
        anims = {}
        for p, pose in enumerate(POSES):
            entry = fam['anims'][pose]
            for side, facing in enumerate(('front', 'back')):
                frames = []
                for name in entry[facing]:
                    foot = entry.get('foot', fam.get('foot'))
                    frames.append(encode(*body(Image.open(root/name), foot)))
                waits = entry.get('waits')
                waits = list(waits[facing]) if isinstance(waits, dict) else \
                    list(waits) if waits else [int(entry['wait'])]*len(frames)
                anims[2*p+side] = (frames, waits)
        families.append(dict(jobs=fam['jobs'], anims=anims))
    return families


def preview(raw, path, scale=3):
    """Every physical frame on light grey, outlined, for the visual decision pass."""
    fams, slots = struct.unpack_from('>HH', raw, 4)
    base = HEADER+4*fams+4*ANIMS*fams
    offsets = struct.unpack_from(f'>{slots+1}H', raw, base)
    pictures = sorted(set(offsets[:-1]))
    cols = 8
    sheet = Image.new('L', (cols*52, (len(pictures)+cols-1)//cols*46), 170)
    for i, start in enumerate(pictures):
        h, wb, ay, ax = raw[start:start+4]
        n = h*wb
        planes = [np.unpackbits(np.frombuffer(raw[start+4+k*n:start+4+(k+1)*n], np.uint8)
                                ).reshape(wb//2, h, 16).transpose(1, 0, 2).reshape(h, 8*wb)
                  for k in range(4)]
        tone = planes[0]+2*planes[1]
        pix = np.where(planes[2] == 1, 170, 255-85*tone).astype(np.uint8)
        x, y = i % cols*52+26-ax, i//cols*46+40-ay
        sheet.paste(Image.fromarray(pix), (x, y))
    sheet.resize((sheet.width*scale, sheet.height*scale), Image.Resampling.NEAREST).save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('manifest', type=Path)
    ap.add_argument('-o', '--out', type=Path, required=True)
    ap.add_argument('--preview', type=Path)
    args = ap.parse_args()
    raw, info = pack(load(args.manifest))
    args.out.write_bytes(raw)
    if args.preview:
        preview(raw, args.preview)
    print(json.dumps(info))


if __name__ == '__main__':
    main()
