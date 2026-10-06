#!/usr/bin/env python3
"""Extract half-scale GHZ1 art and outlined actors from the local REV00 ROM.

All decoded pictures and binary banks remain ignored. The live original
supplies decompressed level/enemy patterns, mappings, chunks and palettes;
Sonic's mapping/DPLC/art addresses are verified for the pinned ROM hash.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image, ImageFilter, ImageOps, ImageDraw

from extract import ROOT, GAME, ROM_HASH, WORLD_W
sys.path.insert(0, str(ROOT / 'tools/md'))
from mdrun import MD


def word(data, pos):
    return int.from_bytes(data[pos:pos + 2], 'big')


def tile(patterns, attr):
    raw = np.frombuffer(patterns[(attr & 2047) * 32:(attr & 2047) * 32 + 32], dtype=np.uint8)
    out = np.empty((8, 8), dtype=np.uint8)
    out[:, 0::2] = (raw >> 4).reshape(8, 4)
    out[:, 1::2] = (raw & 15).reshape(8, 4)
    if attr & 0x800:
        out = out[:, ::-1]
    if attr & 0x1000:
        out = out[::-1]
    return np.where(out, out + ((attr >> 9) & 48), 0).astype(np.uint8)


def mapping(rom, patterns, address, frame, base=0):
    """Compose five-byte MD mapping pieces, including column-major tiles."""
    p = address + word(rom, address + frame * 2)
    count = rom[p]
    out = np.zeros((96, 96), dtype=np.uint8)
    for i in range(count):
        y, size, attr, x = struct.unpack_from('>bBHb', rom, p + 1 + i * 5)
        w, h = (size >> 2 & 3) + 1, (size & 3) + 1
        shape = np.zeros((h * 8, w * 8), dtype=np.uint8)
        for tx in range(w):
            for ty in range(h):
                shape[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = tile(
                    patterns, ((attr + base) & ~0x1800) + tx * h + ty)
        if attr & 0x800:
            shape = shape[:, ::-1]
        if attr & 0x1000:
            shape = shape[::-1]
        dest = out[48 + y:48 + y + h * 8, 48 + x:48 + x + w * 8]
        dest[shape != 0] = shape[shape != 0]
    return out


def sonic_patterns(rom, frame):
    p = 0x217fe + word(rom, 0x217fe + frame * 2)
    result = bytearray(65536)
    target = 0x780 * 32
    for i in range(rom[p]):
        cue = word(rom, p + 1 + i * 2)
        count = ((cue >> 12) + 1) * 32
        source = 0x21afe + (cue & 4095) * 32
        result[target:target + count] = rom[source:source + count]
        target += count
    return bytes(result)


def pack_tile(gray, endian):
    words = []
    for row in gray:
        dark = sum(1 << (15 - x) for x, c in enumerate(row) if c & 2)
        light = sum(1 << (15 - x) for x, c in enumerate(row) if c & 1)
        words.extend((dark, light))
    return struct.pack(endian + '32H', *words)


def sprite(indexed, palette, flip=False, item=False):
    if flip:
        indexed = indexed[:, ::-1]
    # 2x2 box sampling, retaining coverage of thin limbs; no runtime resizing.
    colors = palette[indexed].astype(np.uint16)
    cover = indexed != 0
    cov = cover.reshape(48, 2, 48, 2).sum(axis=(1, 3))
    rgb = (colors * cover[:, :, None]).reshape(48, 2, 48, 2, 3).sum(axis=(1, 3))
    rgb = rgb // np.maximum(cov, 1)[:, :, None]
    lum = (rgb[:, :, 0] * 3 + rgb[:, :, 1] * 6 + rgb[:, :, 2]) // 10
    gray = np.where(lum > 210, 0, np.where(lum > 145, 1, np.where(lum > 85, 2, 3))).astype(np.uint8)
    # Blue quills and dark limbs stay dark; bright shoes/eyes survive the reduction.
    mask = Image.fromarray(np.uint8(cov != 0) * 255)
    outline = (cov != 0) if item else np.array(mask.filter(ImageFilter.MaxFilter(3))) != 0
    if item:
        gray = np.maximum(gray, 2)  # small rings/logs need a dark readable body
    box = Image.fromarray(np.uint8(outline) * 255).getbbox()
    if box is None:
        raise ValueError('empty sprite mapping')
    x, y, right, bottom = box
    w = next(v for v in (8, 16, 32) if right - x <= v)
    h = bottom - y
    planes = [[], [], []]
    for yy in range(y, bottom):
        light = dark = transparent = 0
        for xx in range(w):
            px = x + xx
            c = int(gray[yy, px]) if px < 48 and cov[yy, px] else 0
            light = light << 1 | (c & 1)
            dark = dark << 1 | (c >> 1)
            transparent = transparent << 1 | int(px >= 48 or not outline[yy, px])
        for plane, value in zip(planes, (light, dark, transparent)):
            plane.append(value)
    preview = np.zeros((h, w, 4), dtype=np.uint8)
    for yy in range(h):
        for xx in range(w):
            if not (planes[2][yy] & (1 << (w - 1 - xx))):
                c = ((planes[0][yy] >> (w - 1 - xx)) & 1) + 2 * ((planes[1][yy] >> (w - 1 - xx)) & 1)
                preview[yy, xx] = (255 - c * 85,) * 3 + (255,)
    return (w, h, x - 24, y - 24, planes, Image.fromarray(preview))


def main():
    rompath = ROOT / 'roms/md/Sonic_1.md'
    rom = rompath.read_bytes()
    assert hashlib.sha256(rom).hexdigest() == ROM_HASH
    md = MD(rompath)
    try:
        md.load(ROOT / 'sources/sonic1_md/start.state')
        md.lib.retro_run()
        vram, cram, regs = md.vdp()
        ram = md.dump()
        palette = np.array([tuple(((md.read(0xfffb00 + i * 2, 2) >> shift) & 7) * 255 // 7
                           for shift in (1, 5, 9)) for i in range(64)], dtype=np.uint8)
        actors = []
        # Relevant Sonic poses, including slope walking/running and injury.
        for frame in [1, *range(6, 51), 55, 56, 57, 85]:
            image = mapping(rom, sonic_patterns(rom, frame), 0x211e2, frame, 0x780)
            actors.append((f'sonic_{frame}', image))
        for kind, count, name in [(0x40, 3, 'moto'), (0x22, 6, 'buzz'), (0x2b, 2, 'chop'),
                                  (0x25, 8, 'ring'), (0x11, 1, 'bridge'), (0x23, 4, 'missile')]:
            md.load(ROOT / 'sources/sonic1_md/start.state')
            for address in range(0xffd800, 0xfff000, 4):
                md.write(address, 0, 4)
            md.write(0xffd800, kind); md.write(0xffd804, 4)
            md.write(0xffd808, 192 << 16, 4); md.write(0xffd80c, 940 << 16, 4)
            if kind == 0x23:
                md.write(0xffd828, 1)  # Newtron-mode direct missile initialization
            if kind == 0x11:
                md.write(0xffd828, 1)
            for _ in range(12):
                md.lib.retro_run()
            address, base = md.read(0xffd804, 4), md.read(0xffd802, 2)
            if not 0 < address < len(rom):
                raise ValueError(f'{name}: missing ROM mapping {address:x}')
            for frame in range(count):
                try:
                    image = mapping(rom, vram, address, frame, base)
                except Exception as exc:
                    raise ValueError(f'{name} frame{frame} map{address:x} gfx{base:x}') from exc
                actors.append((f'{name}_{frame}', image))
        # Read real decompressed 16x16 blocks and 256x256 chunk descriptors.
        cache = {}
        world = np.zeros((2048, WORLD_W), dtype=np.uint8)
        for ty in range(128):
            for tx in range(WORLD_W // 16):
                chunk = ram[0xa400 + (ty >> 4) * 128 + (tx >> 4)] & 127
                desc = word(ram, (chunk - 1) * 512 + (((ty & 15) << 4) + (tx & 15)) * 2) if chunk else 0
                if desc not in cache:
                    block = np.zeros((16, 16), dtype=np.uint8)
                    for yy in range(2):
                        for xx in range(2):
                            attr = word(ram, 0xb000 + (desc & 2047) * 8 + (yy * 2 + xx) * 2)
                            block[yy * 8:yy * 8 + 8, xx * 8:xx * 8 + 8] = tile(vram, attr)
                    if desc & 0x800:
                        block = block[:, ::-1]
                    if desc & 0x1000:
                        block = block[::-1]
                    cache[desc] = block
                world[ty * 16:ty * 16 + 16, tx * 16:tx * 16 + 16] = cache[desc]
        # Background ROM pattern snapshot flattened into the opaque TileMap.
        # Use a fully populated 256px repeat; the unused right half of the live
        # VDP name table still contains stale/empty cells at the start state.
        background = np.zeros((256, 256), dtype=np.uint8)
        base = (regs[4] & 7) << 13
        for yy in range(32):
            for xx in range(32):
                attr = word(vram, base + (yy * 64 + xx) * 2)
                background[yy * 8:yy * 8 + 8, xx * 8:xx * 8 + 8] = tile(vram, attr)
        rgb = palette[world].astype(np.uint16)
        lum = (rgb[:, :, 0] * 3 + rgb[:, :, 1] * 6 + rgb[:, :, 2]) // 10
        foreground = np.where(lum > 180, 0, np.where(lum > 110, 1, np.where(lum > 45, 2, 3)))
        bgrgb = palette[background].astype(np.uint16)
        bglum = (bgrgb[:, :, 0] * 3 + bgrgb[:, :, 1] * 6 + bgrgb[:, :, 2]) // 10
        # Background uses only white/light grey to protect actor readability.
        sky = int(np.argmax(np.bincount(background[background != 0], minlength=64)))
        bggray = np.uint8((bglum < 170) & (background != 0) & (background != sky))
        backdrop = bggray[(np.arange(2048)[:, None] - 754) & 255, np.arange(WORLD_W)[None, :] & 255]
        gray = np.where(world != 0, foreground, backdrop).astype(np.uint8)
        # Box filter then round to the nearest LCD level, without dithering.
        gray = ((gray.reshape(1024, 2, WORLD_W // 2, 2).sum(axis=(1, 3)) + 2) >> 2).astype(np.uint8)
        tiles, tile_ids, view = [], {}, bytearray()
        for yy in range(64):
            for xx in range(52):
                block = gray[yy * 16:yy * 16 + 16, xx * 16:xx * 16 + 16]
                key = block.tobytes()
                if key not in tile_ids:
                    tile_ids[key] = len(tiles); tiles.append(block.copy())
                view.append(tile_ids[key])
        sprites = []
        labels = []
        for name, image in actors:
            for flip in (False, True):
                sprites.append(sprite(image, palette, flip, name.startswith(('ring_', 'bridge_'))))
                labels.append(name + ('_left' if flip else '_right'))
        (GAME / 'generated').mkdir(exist_ok=True)
        (GAME / 'x').mkdir(exist_ok=True)
        Image.fromarray(palette[world]).save(GAME / 'x/foreground.png')
        Image.fromarray(np.uint8(255 - gray * 85)).save(GAME / 'x/world-grey.png')
        # Review the complete half-scale atlas on a representative grey ground.
        sheet = Image.new('RGBA', (480, ((len(sprites) + 11) // 12) * 48), (170, 170, 170, 255))
        pen = ImageDraw.Draw(sheet)
        for i, (name, spr) in enumerate(zip(labels, sprites)):
            x, y = i % 12 * 40, i // 12 * 48
            sheet.alpha_composite(spr[5], (x + 2, y + 2)); pen.text((x + 1, y + 32), name.split('_')[0][:5] + name.split('_')[1], fill='black')
        sheet.save(GAME / 'x/atlas.png')
        assert len(tiles) <= 256, f'{len(tiles)} art tiles exceed byte TileMap indices'
        table = 32 + len(view) + len(tiles) * 64
        offset = table + len(sprites) * 8
        for endian, suffix in [('=', ''), ('>', '.be')]:
            out = bytearray(b'SNA1' + struct.pack('>6H', 52, 64, len(tiles), len(sprites), table, offset))
            out.extend(bytes(32 - len(out))); out.extend(view)
            for block in tiles:
                out.extend(pack_tile(block, endian))
            records, bitmap = bytearray(), bytearray()
            for w, h, dx, dy, planes, _ in sprites:
                bitmap.extend(bytes(-len(bitmap) & ((w >> 3) - 1)))
                records.extend(struct.pack('>BBbbHH', w, h, dx, dy, offset + len(bitmap), 0))
                fmt = {8: 'B', 16: 'H', 32: 'I'}[w]
                for plane in planes:
                    bitmap.extend(struct.pack(endian + str(h) + fmt, *plane))
            out.extend(records); out.extend(bitmap)
            # Pre-shifted rows for the short-lived dense ring burst. Keeping
            # them in the archived art bank saves 32-bit variable shifts and
            # RAM on the 68000. Header word16 points to four 1600-byte tables.
            out.extend(bytes(-len(out) & 3))
            struct.pack_into('>H', out, 16, len(out))
            for frame in range(4):
                planes = sprites[labels.index(f'ring_{frame}_right')][4]
                for dx in range(25):
                    for plane in planes[:2]:
                        out.extend(struct.pack(endian + '8I', *(row << (24 - dx) for row in plane)))
            assert len(out) < 65518, f'art bank too large: {len(out)}'
            (GAME / ('sonart' + suffix + '.bin')).write_bytes(out)
        header = ['/* Generated indices; ROM-derived assets stay local. */']
        for i, name in enumerate(labels):
            header.append(f'#define ART_{name.upper()} {i}')
        header.append(f'#define ART_COUNT {len(sprites)}')
        (GAME / 'generated/art_ids.h').write_text('\n'.join(header) + '\n')
        (GAME / 'x/art.json').write_text(json.dumps({'rom_sha256': ROM_HASH, 'tiles': len(tiles),
            'sprites': len(sprites), 'bytes': len(out), 'labels': labels,
            'sonic_mapping': '0x211e2', 'sonic_dplc': '0x217fe', 'sonic_art': '0x21afe'}, indent=2) + '\n')
        print('ROM art:', len(tiles), 'tiles,', len(sprites), 'sprites,', len(out), 'bytes')
    finally:
        md.close()


if __name__ == '__main__':
    main()
