#!/usr/bin/env python3
"""Private standing-monster thumbnails/portraits for visual review, not shipping.

Reuse the existing SHP compositor, source SEQ first-pair reader, palette mapping,
white outline and portrait converter. Keep the complete silhouette in a32x26
formation cell; large families shrink uniformly rather than losing body parts.
"""
import hashlib
import struct

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

import battle
import extract
import interface
import management_data
import monster_ko_review as source
from review_files import save_image, write_changed


def thumbnail(canvas, spr, palette):
    yy, xx = np.mgrid[:128, :128]
    opaque = canvas != 0
    # One family scale for all three palettes; preserve the source feet origin.
    for scale in range(600, 199, -1):
        x = np.floor((xx+.5-64)*scale/1000).astype(int)+16
        y = np.floor((yy+.5-96)*scale/1000).astype(int)+21
        if np.all(~opaque | ((x >= 1) & (x < 31) & (y >= 1) & (y < 25))):
            break
    else:
        raise ValueError('complete source silhouette does not fit formation')
    valid = (x >= 0) & (x < 32) & (y >= 0) & (y < 26)
    cell = (y*32+x)[valid]

    def count(values):
        return np.bincount(cell, weights=values[valid], minlength=832).reshape(26, 32)

    pal = np.frombuffer(spr, '<u2', 16, palette*32).astype(int)
    rgb = np.stack((pal & 31, (pal >> 5) & 31, (pal >> 10) & 31), 1)*255/31
    lum = rgb @ np.array([.299, .587, .114])*31/255
    tones = np.where(lum <= 7, 3, np.where(lum <= 16, 2, np.where(lum <= 22, 1, 0)))
    tones[1] = 3
    total, body = count(np.ones_like(opaque)), count(opaque)
    mask = (body*2 >= total) & (total > 0)
    hist = np.stack([count(opaque & (canvas != 1) & (tones[canvas] == g)) for g in range(4)], 2)
    gray = np.full((26, 32), -1, dtype=int)
    gray[mask] = np.where(hist.sum(2) > 0, hist.argmax(2), 3)[mask]
    cross = np.array([[0, 1, 0], [1, 1, 1], [0, 1, 0]])
    edge = mask & ~ndimage.binary_erosion(mask, structure=cross)
    ring = ndimage.binary_dilation(mask, structure=cross) & ~mask
    gray[edge], gray[ring] = 3, 0
    color = np.full((26, 32, 3), 170, np.uint8)
    for c in range(3):
        channel = count(rgb[canvas, c]*opaque)/body.clip(1)
        color[:, :, c][mask] = channel[mask].round().astype(np.uint8)
    assert not np.any(mask[[0, -1]])
    assert not np.any(mask[:, [0, -1]])
    return gray, Image.fromarray(color), scale


def build(cue, out):
    disc, files = extract.disc_files(str(cue))
    bd = battle.Disc(str(cue))
    names = management_data.names(bd.bat, management_data.JOB_NAMES, 160)
    small = disc.read(*files['EVENT/SMALL.OUT'])
    entries = small[0xec+struct.unpack_from('<I', small, 0x70)[0]:].replace(b'\xff', b'\xfe').split(b'\xfe')
    # Source name class0x0300 / text0x4300; same existing SMALL.OUT list as humans.
    nicknames = [interface.decode(raw).encode('ascii') for raw in entries[768:1023]]
    assert len(nicknames) == 255 and all(len(n) < 16 for n in nicknames)
    bank = bytearray(b'FMR0'+struct.pack('>HH', 48, 255))
    bank += b''.join(n.ljust(16, b'\0') for n in nicknames)
    sheet_cache, rows, hashes = {}, [], {'EVENT/SMALL.OUT': hashlib.sha256(small).hexdigest()}
    atlas = Image.new('RGB', (1008, 16*118+48), 'white')
    draw = ImageDraw.Draw(atlas)
    draw.text((8, 6), 'FORMATION PROPOSAL: source RGB / four greys / portrait; 3x nearest', fill='black')
    draw.text((8, 24), 'Complete bodies; same family scale across palettes. Review only, no combat art.', fill='black')
    for job in range(94, 142):
        record = bd.at(battle.JOBS+48*job, 48)
        sheet, palette = record[45:47]
        if sheet not in sheet_cache:
            meta = bd.bat[0x80094748-battle.BAT+4*sheet:][:4]
            lba = struct.unpack_from('<I', bd.bat, 0x80094cd4-battle.BAT+8*sheet)[0]
            path = next(k for k, v in files.items() if v[0] == lba)
            paths = (path, f'BATTLE/{source.SHP[meta[0]]}.SHP', f'BATTLE/{source.SEQ[meta[1]]}.SEQ')
            spr, shp, seq = [disc.read(*files[p]) for p in paths]
            hashes.update({p: hashlib.sha256(v).hexdigest() for p, v in zip(paths, (spr, shp, seq))})
            frame, wait = source.first_frame(seq, 6)
            sheet_cache[sheet] = spr, source.compose(spr, source.shape(shp, frame)), frame, wait
        spr, canvas, frame, wait = sheet_cache[sheet]
        gray, rgb, scale = thumbnail(canvas, spr, palette)
        # SPR stores portrait palettes in its upper eight CLUT rows.
        portrait_spr = bytearray(spr)
        portrait_spr[8*32:9*32] = spr[(8+palette)*32:(9+palette)*32]
        face = management_data.portrait(portrait_spr)
        bank += interface.planes(gray, 32)+interface.planes(face, 32)
        x, y = ((job-94) % 3)*336, ((job-94)//3)*118+48
        draw.text((x+6, y+2), f'{job} {names[job]}  {scale/1000:.3f}', fill='black')
        def image(pixels):
            values = np.full(pixels.shape, 170, np.uint8)
            mask = pixels >= 0
            values[mask] = (255-85*pixels[mask]).astype(np.uint8)
            return Image.fromarray(values).convert('RGB')
        atlas.paste(rgb.resize((96, 78), Image.Resampling.NEAREST), (x+2, y+22))
        atlas.paste(image(gray).resize((96, 78), Image.Resampling.NEAREST), (x+104, y+22))
        atlas.paste(image(face).resize((96, 87), Image.Resampling.NEAREST), (x+206, y+22))
        rows.append(dict(job=job, name=names[job], sheet=sheet, palette=palette,
                         frame=frame, wait=wait, scale_milli=scale))
    assert len(bank) == 8+255*16+48*(312+348)
    write_changed(out/'ffmr.bin', bytes(bank))
    save_image(atlas, out/'atlas.png')
    return dict(rows=rows, source_sha256=hashes, bank_sha256=hashlib.sha256(bank).hexdigest(),
                source_scope='Retail job/sheet/SHP/direct SEQ records and original monster names; static first idle only',
                proposal='Uniform fit at most0.6; 32x26 thumbnail, 32x29 portrait; no human eye/skin edits',
                shipping=False, optimized=False)
