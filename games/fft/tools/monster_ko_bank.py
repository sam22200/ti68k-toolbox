#!/usr/bin/env python3
"""Encode approved source Dead poses as optional FKO1 chapter art, never core art.

Raw variable-height 32-bit rows are read in place; normal/mirrored directions
share physical records. No change to human frames or the 48-entry actor cache.
This bounded builder covers the six reviewed palette-zero samples only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np

import battle
import core_audit
import extract
import monster_ko_review as review

JOBS, HEADER = 160, 328
REV = bytes(int(f'{i:08b}'[::-1], 2) for i in range(256))


def mirror(raw):
    h, x, y, flags = raw[:4]
    return bytes((h, 32-x, y, flags)) + b''.join(
        bytes(REV[v] for v in raw[i:i+4][::-1]) for i in range(4, len(raw), 4))


def encode(image, mask, inner):
    """Crop empty rows only, preserving the reviewed scale and feet anchor."""
    yy, _ = np.nonzero(mask)
    top, bottom = int(yy.min()), int(yy.max())+1
    color = (255-np.asarray(image)[:, :, 0])//85
    planes = (((color & 1) != 0) & mask, ((color & 2) != 0) & mask, ~mask, inner)
    raw = bytes((bottom-top, 16, (28-top) & 255, 0))
    raw += b''.join(np.packbits(p[top:bottom], axis=1).tobytes() for p in planes)
    return raw


def pack(entries):
    """job -> [front,back] raw poses; pool complete records and their mirrors."""
    mapping = [65535]*JOBS
    pairs, records, physical, pair_ids = [], [], {}, {}

    def store(raw):
        if raw in physical:
            return physical[raw]
        flipped = mirror(raw)
        if flipped in physical:
            return physical[flipped] | 32768
        key = len(records)
        physical[raw] = key
        records.append(raw)
        return key

    for job, poses in sorted(entries.items()):
        if not 0 <= job < JOBS or len(poses) != 2:
            raise ValueError('invalid source job/pose pair')
        normal = tuple(store(p) for p in poses)
        pair = (*normal, *(v ^ 32768 for v in normal))
        if pair not in pair_ids:
            pair_ids[pair] = len(pairs)
            pairs.append(pair)
        mapping[job] = pair_ids[pair]
    if not pairs:
        raise ValueError('no reviewed poses')
    out = bytearray(b'FKO1'+struct.pack('>HH', len(pairs), len(records)))
    out += struct.pack('>160H', *mapping)
    out += b''.join(struct.pack('>4H', *p) for p in pairs)
    offset = HEADER+8*len(pairs)+2*(len(records)+1)
    offsets = [offset]
    for raw in records:
        offset += len(raw)
        offsets.append(offset)
    if offset > 65512:
        raise ValueError('FKO1 exceeds one archive variable')
    out += struct.pack(f'>{len(offsets)}H', *offsets)
    return bytes(out)+b''.join(records), dict(jobs=sorted(entries), pairs=len(pairs),
                                           physical_frames=len(records))


def validate(raw):
    """Shipping gate mirrors the cold C bounds and plane invariants."""
    def reject():
        raise ValueError('Invalid FKO1 monster Dead bank')
    if len(raw) < HEADER or raw[:4] != b'FKO1':
        reject()
    pairs, frames = struct.unpack_from('>HH', raw, 4)
    if not 1 <= pairs <= JOBS or not 1 <= frames <= 4*JOBS:
        reject()
    base = HEADER+8*pairs
    last = base+2*(frames+1)
    if last > len(raw) or len(raw) > 65512:
        reject()
    if any(v != 65535 and v >= pairs for v in struct.unpack_from('>160H', raw, 8)):
        reject()
    if any((v & 32767) >= frames for v in struct.unpack_from(f'>{4*pairs}H', raw, HEADER)):
        reject()
    offsets = struct.unpack_from(f'>{frames+1}H', raw, base)
    for start, end in zip(offsets, offsets[1:]):
        if start != last or start & 1 or end > len(raw) or end-start < 4:
            reject()
        h, x, y, flags = raw[start:start+4]
        if not 1 <= h <= 26 or x != 16 or y > 28 or h-y > 12 or flags or end-start != 4+16*h:
            reject()
        data = raw[start+4:end]
        for row in range(h):
            a, b, mask, inner = [struct.unpack_from('>I', data, 4*(p*h+row))[0] for p in range(4)]
            if (a | b | inner) & mask:
                reject()
        last = end
    if last != len(raw):
        reject()
    return True


def build(disc, files, bd):
    entries, rows, hashes = {}, [], {}
    for name, sheet in review.SAMPLES:
        meta = bd.bat[0x80094748-battle.BAT+4*sheet:][:4]
        shp_id, seq_id = meta[:2]
        lba = struct.unpack_from('<I', bd.bat, 0x80094cd4-battle.BAT+8*sheet)[0]
        path = next(k for k, v in files.items() if v[0] == lba)
        paths = [path, f'BATTLE/{review.SHP[shp_id]}.SHP', f'BATTLE/{review.SEQ[seq_id]}.SEQ']
        spr, shp, seq = [disc.read(*files[p]) for p in paths]
        hashes.update({p: hashlib.sha256(d).hexdigest() for p, d in zip(paths, (spr, shp, seq))})
        poses = []
        for animation in (52, 53):
            frame, wait = review.first_frame(seq, animation)
            if wait:
                raise ValueError('reviewed Dead pose is not fixed')
            canvas = review.compose(spr, review.shape(shp, frame))
            poses.append(encode(*review.preview(canvas, spr, True, with_masks=True)))
        jobs = []
        for job in range(JOBS):
            raw = bd.at(battle.JOBS + 48*job, 48)
            if raw[45] == sheet and raw[46] == 0:
                jobs.append(job)
                entries[job] = poses
        if not jobs:
            raise ValueError('reviewed palette-zero sheet has no source job')
        rows.append(dict(name=name, sheet=sheet, jobs=jobs))
    bank, info = pack(entries)
    validate(bank)
    return bank, dict(**info, rows=rows, source_sha256=hashes,
                      scope='Six approved palette-zero samples only; no standing/action art or unreviewed palettes')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--disc', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--out', type=Path, default=review.HERE/'campaign/x/monster-ko/bank')
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    disc, files = extract.disc_files(str(args.disc))
    bank, info = build(disc, files, battle.Disc(str(args.disc)))
    (args.out/'ffko.bin').write_bytes(bank)
    info.update(bytes=len(bank), bank_sha256=hashlib.sha256(bank).hexdigest())
    (args.out/'bank.json').write_text(json.dumps(info, indent=2)+'\n')
    print(f'FKO1: {len(bank)} archived bytes; {info["pairs"]} pairs, {info["physical_frames"]} physical poses')


if __name__ == '__main__':
    main()
