#!/usr/bin/env python3
"""Private, fixed-scale monster body animation review; never shipping art.

Reuse approved family palettes/scales and the SHP compositor. Retail MIPS
provides body frames, waits, flips and script offsets. No TI/runtime change.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image, ImageDraw

import battle
import core_audit
import extract
import interface
import management_data
from monster_animation_oracle import Timeline
from monster_appearance import REPRESENTATIVE_JOBS
from monster_formation_review import thumbnail
import monster_ko_review as source
from review_files import save_image, write_changed

HERE = Path(__file__).resolve().parents[1]
POSES = (('Idle', 3), ('Walk', 12), ('Attack', 62), ('Hurt', 25), ('KO', 26))
SIZE, FEET = (64, 64), (32, 44)  # Study canvas, not a proposed cache record.


def attack_pixels(spr):
    """Bounded offline SPR nibble-RLE reader, checked against retail MIPS."""
    packed = spr[0x9200:]
    position = 0

    def nibble():
        nonlocal position
        if position >= len(packed)*2:
            raise ValueError('truncated SPR attack image')
        value = (packed[position//2] >> (4*(1-(position & 1)))) & 15
        position += 1
        return value

    pixels = []
    while len(pixels) < 0x6400*2:
        value = nibble()
        if value:
            pixels.append(value)
        else:
            count = nibble()
            if count == 0:
                count = nibble()
            elif count in (7, 8):
                marker = count
                count = nibble() | (nibble() << 4)
                if marker == 8:
                    count |= nibble() << 8
            if len(pixels)+count > 0x6400*2:
                raise ValueError('SPR transparent run exceeds attack image')
            pixels.extend([0]*count)
    return bytes((pixels[i] << 4) | pixels[i+1] for i in range(0, len(pixels), 2))


def check_reader():
    """Malformed boundaries and a complete transparent image, without a disc."""
    def resource(nibbles):
        nibbles += [0]*(len(nibbles) & 1)
        return bytes(0x9200)+bytes((nibbles[i] << 4) | nibbles[i+1]
                                  for i in range(0, len(nibbles), 2))

    # Twelve4095-pixel runs plus2060 pixels fill exactly51200 nibbles.
    exact = [0, 8, 15, 15, 15]*12+[0, 8, 12, 0, 8]
    assert attack_pixels(resource(exact)) == bytes(0x6400)
    for bad in (bytes(0x9200), resource([1, 1]),
                resource([0, 8, 15, 15, 15]*13)):
        try:
            attack_pixels(bad)
        except ValueError:
            pass
        else:
            raise AssertionError('malformed attack image accepted')
    return 4


def sha(data):
    return hashlib.sha256(data).hexdigest()


def gray_image(gray):
    pixels = np.full(gray.shape, 170, np.uint8)
    mask = gray >= 0
    pixels[mask] = 255-85*gray[mask]
    return Image.fromarray(pixels).convert('RGB')


def at_tick(steps, tick):
    total = sum(max(1, step['wait']) for step in steps)
    tick %= total
    for index, step in enumerate(steps):
        tick -= max(1, step['wait'])
        if tick < 0:
            return index
    raise AssertionError('timeline selection')


def build(args):
    disc, files = extract.disc_files(str(args.disc))
    bd = battle.Disc(str(args.disc))
    names = management_data.names(bd.bat, management_data.JOB_NAMES, 160)
    oracle = Timeline(bd)
    approved = json.loads((args.approved/'results.json').read_text())['rows']
    reference = (args.approved/'ffmr.bin').read_bytes()
    if reference[:8] != b'FMR1\x00\x10\x00\xff':
        raise ValueError('expected the approved selected-family bank')
    timelines, source_hashes, rows, pictures = {}, {}, [], []
    for family, job in enumerate(REPRESENTATIVE_JOBS):
        selection = approved[family]
        if selection['job'] != job:
            raise ValueError('approved family selection changed')
        sheet, palette = bd.at(battle.JOBS+48*job, 48)[45:47]
        shp_id, seq_id, flying, _ = bd.bat[0x80094748-battle.BAT+4*sheet:][:4]
        lba, size = struct.unpack_from('<II', bd.bat, 0x80094cd4-battle.BAT+8*sheet)
        path = next(k for k, extent in files.items() if extent[0] == lba)
        if ((files[path][1]+2047)//2048)*2048 != size:
            raise ValueError('source sheet extent mismatch')
        paths = (path, f'BATTLE/{source.SHP[shp_id]}.SHP', f'BATTLE/{source.SEQ[seq_id]}.SEQ')
        spr, shp, seq = [disc.read(*files[p]) for p in paths]
        source_hashes.update({p: sha(data) for p, data in zip(paths, (spr, shp, seq))})
        attack = attack_pixels(spr)
        if attack != oracle.attack_sheet(spr):
            raise ValueError('offline SPR attack decoding differs from retail MIPS')
        # The original uploads 200 attack rows, then copies the32-row portrait
        # strip below them. Do not read ordinary idle-page pixels for attacks.
        attack_spr = spr[:512]+attack+spr[0x8200:0x9200]+bytes(24*128)
        attack_start, sp2_start = struct.unpack_from('<HH', shp, 4)
        scale = selection['scale_milli']
        standing = source.compose(spr, source.shape(shp, selection['frame']))
        original_gray, _, original_scale = thumbnail(standing, spr, palette)
        at = 8+48+255*16+family*660
        if original_scale != scale or interface.planes(original_gray, 32) != reference[at:at+312]:
            raise ValueError('approved standing planes changed')
        row = dict(job=job, name=names[job], sheet=sheet, palette=palette,
                   scale_milli=scale, paths=paths, animations={},
                   attack_start=attack_start, attack_pixels_sha256=sha(attack), flying=bool(flying))
        images = {}
        for label, anim in POSES + ((('Hover', 6), ('Fly', 18)) if flying else ()):
            pair, pair_images = [], []
            for direction in range(2):
                animation = 2*anim+direction
                key = f'{shp_id}:{seq_id}:{animation}'
                if key not in timelines:
                    timeline = oracle.trace(seq, animation, sheet)
                    # Reentry must reset flags, offsets, loop counters and buffers.
                    if timeline != oracle.trace(seq, animation, sheet):
                        raise ValueError('retail timeline reentry differs')
                    if seq[0x406+struct.unpack_from('<I', seq, 4+4*animation)[0]] != 255:
                        if (timeline['steps'][0]['frame'], timeline['steps'][0]['wait']) != source.first_frame(seq, animation):
                            raise ValueError('direct pair disagrees with retail player')
                    timelines[key] = timeline
                timeline = timelines[key]
                frame_images, geometry = [], []
                for step in timeline['steps']:
                    if step['frame'] >= sp2_start:
                        raise ValueError('body study reached a shared SP2 resource')
                    page = attack_spr if step['frame'] >= attack_start else spr
                    canvas = source.compose(page, source.shape(shp, step['frame']))
                    if step['flags'] & ~6:
                        raise ValueError('unhandled retail body flag')
                    if step['flags'] & 6:
                        # Apply around the source feet origin, never the box centre.
                        yy, xx = np.nonzero(canvas)
                        dx = 127-xx if step['flags'] & 2 else xx
                        dy = 191-yy if step['flags'] & 4 else yy
                        if np.any((dx < 0) | (dx >= 128) | (dy < 0) | (dy >= 128)):
                            raise ValueError('flipped source body exceeds study canvas')
                        flipped = np.zeros_like(canvas)
                        flipped[dy, dx] = canvas[yy, xx]
                        canvas = flipped
                    gray, _, _ = thumbnail(canvas, spr, palette, scale_milli=scale,
                                            width=SIZE[0], height=SIZE[1], feet=FEET)
                    if label == 'Idle' and direction == 0 and not frame_images:
                        if not np.array_equal(gray[23:49, 16:48], original_gray):
                            raise ValueError('wide standing raster differs from approved planes')
                    yy, xx = np.nonzero(gray >= 0)
                    geometry.append(dict(bounds=[int(xx.min()-FEET[0]), int(yy.min()-FEET[1]),
                                                 int(xx.max()-FEET[0]), int(yy.max()-FEET[1])],
                                         gray_sha256=sha(gray.astype('i1').tobytes())))
                    frame_images.append(gray_image(gray))
                pair.append(dict(animation=animation, timeline_key=key, frames=geometry))
                pair_images.append(frame_images)
            row['animations'][label] = pair
            images[label] = pair_images
        rows.append(row)
        pictures.append(images)

    # Each row pairs the already-approved standing pose with the new action poses.
    for page in range(2):
        image = Image.new('RGB', (1280, 8*152+60), 'white')
        draw = ImageDraw.Draw(image)
        draw.text((8, 5), 'MONSTER BODY POSES: approved scale/palette; front + back; 2x nearest', fill='black')
        draw.text((8, 21), 'Private study, not the battle renderer. Full silhouettes; source SEQ offsets not projected.', fill='black')
        for col, (label, _) in enumerate(POSES):
            draw.text((col*256+8, 40), label + (' (approved first pose)' if col == 0 else ''), fill='black')
        for local in range(8):
            family = page*8+local
            y = 60+local*152
            draw.text((8, y), rows[family]['name'], fill='black')
            for col, (label, _) in enumerate(POSES):
                for direction in range(2):
                    frames = pictures[family][label][direction]
                    index = 0 if label in ('Idle', 'KO') else len(frames)//2
                    image.paste(frames[index].resize((128, 128), Image.Resampling.NEAREST),
                                (col*256+direction*128, y+18))
        save_image(image, args.out/f'poses-{page+1}.png')

    flyers = [i for i, row in enumerate(rows) if row['flying']]
    image = Image.new('RGB', (768, 60+152*len(flyers)), 'white')
    draw = ImageDraw.Draw(image)
    draw.text((8, 5), 'FLYING SHEETS: approved standing reference / original hover / original flight', fill='black')
    draw.text((8, 21), 'Same family scale and palette. Each pair front/back, 2x nearest.', fill='black')
    for col, label in enumerate(('Standing reference', 'Hover', 'Fly')):
        draw.text((col*256+8, 40), label, fill='black')
    for local, family in enumerate(flyers):
        y = 60+local*152
        draw.text((8, y), rows[family]['name'], fill='black')
        for col, label in enumerate(('Idle', 'Hover', 'Fly')):
            for direction in range(2):
                frames = pictures[family][label][direction]
                frame = frames[0 if col == 0 else len(frames)//2]
                image.paste(frame.resize((128, 128), Image.Resampling.NEAREST),
                            (col*256+direction*128, y+18))
    save_image(image, args.out/'flight.png')

    # The enlarged animation uses two original facings and their explicit mirrors.
    phases = [('Idle', 24), ('Walk', 24), ('Attack', 36), ('Hurt', 12), ('KO', 16)]
    for page in range(2):
        animation_frames = []
        for phase, ticks in phases:
            for tick in range(ticks):
                image = Image.new('RGB', (768, 4*216+42), 'white')
                draw = ImageDraw.Draw(image)
                title = {'Idle': 'Idle / hover', 'Walk': 'Walk / flight'}.get(phase, phase)
                draw.text((8, 5), f'{title.upper()} / {"front" if tick < ticks//2 else "back"} + mirror / 3x nearest', fill='black')
                draw.text((8, 21), 'Body-pose study; fixed approved scale; no battle timing/offset/CPU claim.', fill='black')
                direction = int(tick >= ticks//2)
                for local in range(8):
                    family = page*8+local
                    x, y = local%2*384, 42+local//2*216
                    draw.text((x+8, y+3), rows[family]['name'], fill='black')
                    pose = {'Idle': 'Hover', 'Walk': 'Fly'}.get(phase, phase) if rows[family]['flying'] else phase
                    pair = rows[family]['animations'][pose][direction]
                    steps = timelines[pair['timeline_key']]['steps']
                    index = at_tick(steps, (tick % max(1, ticks//2))*2)
                    frame = pictures[family][pose][direction][index]
                    # Side-by-side facing audit, feet remain at one constant row.
                    for mirror in range(2):
                        body = frame.transpose(Image.Transpose.FLIP_LEFT_RIGHT) if mirror else frame
                        image.paste(body.resize((192, 192), Image.Resampling.NEAREST), (x+mirror*192, y+20))
                animation_frames.append(image)
        save_image(animation_frames[0], args.out/f'animations-{page+1}.gif',
                   save_all=True, append_images=animation_frames[1:], duration=67, loop=0,
                   disposal=2, optimize=False)
    bounds = [frame['bounds'] for row in rows for pair in row['animations'].values()
              for direction in pair for frame in direction['frames']]
    envelope = [min(b[0] for b in bounds), min(b[1] for b in bounds),
                max(b[2] for b in bounds), max(b[3] for b in bounds)]
    report = dict(scope='Retail body timelines only, no victims/effects; audio/GPU isolated; '
                        'source coordinate offsets recorded but not projected in previews.',
                  shipping=False, approval='pending', study_canvas=SIZE, feet=FEET,
                  approved_idle_identical=16, bounds_with_outline=envelope,
                  physical_families=16, logical_jobs=48, timeline_count=len(timelines),
                  source_sha256=source_hashes, functions_sha256=oracle.hashes,
                  battle_sha256=sha(bd.bat), timelines=timelines, rows=rows)
    write_changed(args.out/'results.json', (json.dumps(report, indent=2)+'\n').encode())
    print(f'{len(timelines)} retail timelines, replayed twice; 16 approved standing planes identical; bounds {envelope}')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--disc', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--approved', type=Path, default=HERE/'campaign/x/monster-family-review')
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/monster-combat-review')
    args = ap.parse_args()
    check_reader()
    args.out.mkdir(parents=True, exist_ok=True)
    build(args)


if __name__ == '__main__':
    main()
