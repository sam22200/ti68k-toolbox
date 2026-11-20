#!/usr/bin/env python3
"""Wide monster body poses from the disc, packed into an FMB1 bank (`ffmb`).

For each selected family appearance (monster_appearance.py) this replays the
retail body timelines with the bounded oracle, converts every displayed frame
at the family's approved scale and palette, writes them as four-grey PNGs with
a manifest and packs them with monster_body_bank.py. The source waits are kept
(halved: the original counts 60 ticks a second, the game about 30), so a
sequence keeps its rhythm without repeating a picture. A flying family hovers
in place and uses its flight for walking.

The approved previews stay monster_combat_review.py's; this writes shipping
art. ROM-derived PNGs, manifests and banks stay local, never committed.

    monster_body_assets.py --families 94,97,100,104 -o games/fft/campaign/data/ffmb.bin
"""
import argparse
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image

import battle
import core_audit
import extract
import interface
import management_data
import monster_body_bank as bank
from monster_animation_oracle import Timeline
from monster_appearance import REPRESENTATIVE_JOBS
from monster_combat_review import POSES, SIZE, FEET, attack_pixels
from monster_formation_review import thumbnail
import monster_ko_review as source

HERE = Path(__file__).resolve().parents[1]
# The ten appearances the 33 Chapter I encounters place (chapter_audit.py lists them):
# Chocobo, Goblin, Bomb, Cuar, Pisco Demon, Skeleton, Ahriman, Steel Hawk, Minitaurus, Dragon.
CHAPTER1 = (94, 97, 100, 104, 106, 109, 116, 119, 128, 136)
BANKS = 3                                # ffmb, ffmc, ffmd (monster_body.h MB_BANKS)
TONE = (255, 170, 85, 0)


def ticks(wait):
    """Source waits count 60 a second; a frame of the game is about half of that."""
    return max(1, (wait+1)//2)


def png(gray, path):
    """One body picture: four greys on transparency, its own black edge, no ring."""
    rgba = np.zeros(gray.shape+(4,), np.uint8)
    rgba[..., :3] = np.array(TONE, np.uint8)[np.clip(gray, 0, 3)][..., None]
    rgba[..., 3] = np.where(gray >= 0, 255, 0)
    Image.fromarray(rgba).save(path)


def family_poses(bd, disc, files, oracle, job, selection, out, reference):
    """The family's five poses, front and back: PNG names and waits per pose."""
    sheet, palette = bd.at(battle.JOBS+48*job, 48)[45:47]
    shp_id, seq_id, flying, _ = bd.bat[0x80094748-battle.BAT+4*sheet:][:4]
    lba, size = struct.unpack_from('<II', bd.bat, 0x80094cd4-battle.BAT+8*sheet)
    path = next(k for k, extent in files.items() if extent[0] == lba)
    if ((files[path][1]+2047)//2048)*2048 != size:
        raise ValueError('source sheet extent mismatch')
    spr, shp, seq = [disc.read(*files[p]) for p in
                     (path, f'BATTLE/{source.SHP[shp_id]}.SHP', f'BATTLE/{source.SEQ[seq_id]}.SEQ')]
    attack = attack_pixels(spr)
    if attack != oracle.attack_sheet(spr):
        raise ValueError('offline SPR attack decoding differs from retail MIPS')
    # The original uploads 200 attack rows, then the 32-row portrait strip below.
    attack_spr = spr[:512]+attack+spr[0x8200:0x9200]+bytes(24*128)
    attack_start, sp2_start = struct.unpack_from('<HH', shp, 4)
    scale = selection['scale_milli']
    standing = source.compose(spr, source.shape(shp, selection['frame']))
    original_gray, _, original_scale = thumbnail(standing, spr, palette)
    at = 8+48+255*16+REPRESENTATIVE_JOBS.index(job)*660
    if original_scale != scale or interface.planes(original_gray, 32) != reference[at:at+312]:
        raise ValueError('approved standing planes changed')

    anims = {}
    for pose, (label, anim) in zip(bank.POSES, POSES):
        if flying:                            # hovering stands, flight walks
            label, anim = {'Idle': ('Hover', 6), 'Walk': ('Fly', 18)}.get(label, (label, anim))
        entry = {'foot': list(FEET), 'waits': {}}
        for side, facing in enumerate(('front', 'back')):
            timeline = oracle.trace(seq, 2*anim+side, sheet)
            if timeline != oracle.trace(seq, 2*anim+side, sheet):
                raise ValueError('retail timeline reentry differs')
            names, waits = [], []
            for k, step in enumerate(timeline['steps']):
                if step['frame'] >= sp2_start:
                    raise ValueError('body reached a shared SP2 resource')
                page = attack_spr if step['frame'] >= attack_start else spr
                canvas = source.compose(page, source.shape(shp, step['frame']))
                if step['flags'] & ~6:
                    raise ValueError('unhandled retail body flag')
                if step['flags'] & 6:         # around the source feet origin, not the box
                    yy, xx = np.nonzero(canvas)
                    dx = 127-xx if step['flags'] & 2 else xx
                    dy = 191-yy if step['flags'] & 4 else yy
                    if np.any((dx < 0) | (dx >= 128) | (dy < 0) | (dy >= 128)):
                        raise ValueError('flipped source body leaves the canvas')
                    flipped = np.zeros_like(canvas)
                    flipped[dy, dx] = canvas[yy, xx]
                    canvas = flipped
                gray, _, _ = thumbnail(canvas, spr, palette, scale_milli=scale,
                                       width=SIZE[0], height=SIZE[1], feet=FEET, outline=False)
                name = f'{job}-{pose}-{facing}-{k}.png'
                png(gray, out/name)
                names.append(name)
                waits.append(ticks(step['wait']))
            entry[facing] = names
            entry['waits'][facing] = waits
        anims[pose] = entry
    return anims, bool(flying)


def split(families):
    """Families into archive variables, in order: a chapter may need more than one."""
    parts, current = [], []
    for family in families:
        trial = current+[family]
        try:
            bank.pack(trial)
        except ValueError as why:
            if 'archive variable' not in str(why) or not current:
                raise
            parts.append(current)
            current = [family]
        else:
            current = trial
    if current:
        parts.append(current)
    if len(parts) > BANKS:
        raise ValueError(f'{len(parts)} banks: the loader reads ffmb..ffm{chr(ord("a")+BANKS)}')
    return parts


def build(args):
    disc, files = extract.disc_files(str(args.disc))
    bd = battle.Disc(str(args.disc))
    names = management_data.names(bd.bat, management_data.JOB_NAMES, 160)
    oracle = Timeline(bd)
    approved = json.loads((args.approved/'results.json').read_text())['rows']
    reference = (args.approved/'ffmr.bin').read_bytes()
    if reference[:8] != b'FMR1\x00\x10\x00\xff':
        raise ValueError('expected the approved selected-family bank')
    args.poses.mkdir(parents=True, exist_ok=True)
    manifest, report = {'families': []}, []
    for job in args.families:
        family = REPRESENTATIVE_JOBS.index(job)
        selection = approved[family]
        if selection['job'] != job:
            raise ValueError('approved family selection changed')
        anims, flying = family_poses(bd, disc, files, oracle, job, selection, args.poses, reference)
        jobs = [94+3*family+k for k in range(3)]
        manifest['families'].append(dict(jobs=jobs, anims=anims))
        report.append(dict(job=job, name=names[job], jobs=jobs, flying=flying,
                           scale_milli=selection['scale_milli'],
                           frames={p: len(a['front'])+len(a['back']) for p, a in anims.items()}))
    path = args.poses/'manifest.json'
    path.write_text(json.dumps(manifest, indent=1))
    loaded = bank.load(path)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    banks, infos = [], []
    for part, families in enumerate(split(loaded)):
        raw, info = bank.pack(families)
        out = args.out if not part else args.out.with_name(
            args.out.name.replace('ffmb', 'ffm'+chr(ord('b')+part)))
        out.write_bytes(raw)
        banks.append(str(out))
        infos.append(info)
        if args.preview:
            bank.preview(raw, args.preview if not part else args.preview.with_name(
                args.preview.stem+str(part)+args.preview.suffix))
    for row, families in zip(report, (f for part, fams in enumerate(split(loaded))
                                      for f in [part]*len(fams))):
        row['bank'] = families
    (args.poses/'families.json').write_text(json.dumps(
        dict(banks=dict(zip(banks, infos)), families=report), indent=2)+'\n')
    for name, info in zip(banks, infos):
        print(Path(name).name+': '+json.dumps(info))
    for row in report:
        print(f"  ffm{chr(ord('b')+row['bank'])} {row['name']:<14} jobs {row['jobs']} "
              + ' '.join(f"{p}:{n}" for p, n in row['frames'].items()))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--disc', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--approved', type=Path, default=HERE/'campaign/x/monster-family-review')
    ap.add_argument('--families', default=','.join(map(str, CHAPTER1)),
                    help="representative jobs, or 'all' (16 families exceed one variable)")
    ap.add_argument('-o', '--out', type=Path, default=HERE/'campaign/data/ffmb.bin')
    ap.add_argument('--poses', type=Path, default=HERE/'campaign/x/monster-body/poses')
    ap.add_argument('--preview', type=Path)
    args = ap.parse_args()
    args.families = list(REPRESENTATIVE_JOBS) if args.families == 'all' else \
        [int(v) for v in args.families.split(',')]
    for job in args.families:
        if job not in REPRESENTATIVE_JOBS:
            raise SystemExit(f'{job} is not a selected family appearance')
    build(args)


if __name__ == '__main__':
    main()
