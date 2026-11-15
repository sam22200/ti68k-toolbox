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
    return head+b''.join(np.packbits(p, axis=1).tobytes() for p in planes)


def pack(families):
    """families: [{'jobs': [...], 'anims': {anim index: ([frames], wait)}}] -> bytes, info."""
    if not 1 <= len(families) <= FAMILIES:
        raise ValueError('1..48 families')
    jobmap = [255]*JOBS
    records, physical, table = [], {}, []
    for f, fam in enumerate(families):
        for job in fam['jobs']:
            if not 0 <= job-JOB0 < JOBS or jobmap[job-JOB0] != 255:
                raise ValueError(f'job {job}: not a free monster job')
            jobmap[job-JOB0] = f
        for a in range(ANIMS):
            frames, wait = fam['anims'][a]
            if not frames or not 1 <= wait <= 255 or len(frames) > 255:
                raise ValueError(f'family {f} animation {a}: frames/wait')
            # A sequence is stored contiguously; identical whole sequences are pooled.
            key = tuple(frames)
            if key not in physical:
                physical[key] = len(records)
                records.extend(frames)
            table.append((physical[key], len(frames), wait))
    if len(records) > FRAMES:
        raise ValueError('too many frames')
    out = bytearray(b'FMB1'+struct.pack('>HH', len(families), len(records)))
    out += bytes(jobmap)
    out += b''.join(struct.pack('>HBB', *e) for e in table)
    offset = len(out)+2*(len(records)+1)
    offsets = [offset]
    for raw in records:
        offset += len(raw)
        offsets.append(offset)
    if offset > 65512:
        raise ValueError('FMB1 exceeds one archive variable')
    out += struct.pack(f'>{len(offsets)}H', *offsets)
    raw = bytes(out)+b''.join(records)
    validate(raw)
    return raw, dict(families=len(families), frames=len(records), bytes=len(raw))


def validate(raw):
    """The cold C gate (monster_body_valid), for packaging."""
    def reject(why):
        raise ValueError(f'Invalid FMB1 bank: {why}')
    if len(raw) < HEADER or raw[:4] != b'FMB1':
        reject('magic')
    fams, frames = struct.unpack_from('>HH', raw, 4)
    if not 1 <= fams <= FAMILIES or not 1 <= frames <= FRAMES:
        reject('counts')
    base = HEADER+4*ANIMS*fams
    last = base+2*(frames+1)
    if last > len(raw) or len(raw) > 65512:
        reject('size')
    if any(v != 255 and v >= fams for v in raw[8:8+JOBS]):
        reject('job map')
    for i in range(ANIMS*fams):
        first, n, wait = struct.unpack_from('>HBB', raw, HEADER+4*i)
        if not n or not wait or first+n > frames:
            reject('animation')
    for start, end in zip(*(lambda o: (o, o[1:]))(struct.unpack_from(f'>{frames+1}H', raw, base))):
        if start != last or start & 1 or end > len(raw) or end-start < 4:
            reject('offsets')
        h, wb, ay, ax = raw[start:start+4]
        if not 1 <= h <= ROWS or wb not in (2, 4, 6) or not 1 <= ay <= ROWS or h-ay > 12 \
                or ax > 8*wb or end-start != 4+4*h*wb:
            reject('frame header')
        n = h*wb
        p = [raw[start+4+k*n:start+4+(k+1)*n] for k in range(4)]
        if any((a | b | c) & m for a, b, m, c in zip(p[0], p[1], p[2], p[3])):
            reject('opaque bits under the transparency mask')
        last = end
    if last != len(raw):
        reject('trailing bytes')
    return True


def load(manifest):
    """Manifest JSON -> pack() families; PNG paths relative to the manifest."""
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
                anims[2*p+side] = (frames, int(entry['wait']))
        families.append(dict(jobs=fam['jobs'], anims=anims))
    return families


def preview(raw, path, scale=3):
    """Every physical frame on light grey, outlined, for the visual decision pass."""
    fams, frames = struct.unpack_from('>HH', raw, 4)
    base = HEADER+4*ANIMS*fams
    offsets = struct.unpack_from(f'>{frames+1}H', raw, base)
    cols = 8
    sheet = Image.new('L', (cols*52, (frames+cols-1)//cols*46), 170)
    for i, start in enumerate(offsets[:-1]):
        h, wb, ay, ax = raw[start:start+4]
        n = h*wb
        planes = [np.unpackbits(np.frombuffer(raw[start+4+k*n:start+4+(k+1)*n], np.uint8)
                                ).reshape(h, 8*wb) for k in range(4)]
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
