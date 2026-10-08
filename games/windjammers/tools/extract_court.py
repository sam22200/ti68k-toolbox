#!/usr/bin/env python3
"""Validate reference Beach scenery, then build the requested native court.

Only our extractor is kept in git. ROM pixels, snapshots and generated
planes remain local. Source ownership comes from VRAM depth, not RGB matches.
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image

from extract_art import Art, palette_rgb, GAME, ROOT
from neogeorun import sha
from layout import number_pixels


def decode(art, raw, animation):
    word = lambda p: int.from_bytes(raw[p:p + 2], 'big')
    scene = np.zeros((224, 304), dtype=np.uint16)
    depth = np.full(scene.shape, -1, dtype=np.int16)
    layers, descriptors = {}, []
    x = y = height = 0
    for bank in range(381):
        zoom, size, position = (word(offset + 2 * bank) for offset in (0x10000, 0x10400, 0x10800))
        if size & 64:
            x += 16
        else:
            x, y, height = (position >> 7) - 8, ((512 - (size >> 7)) & 511) - 16, size & 63
            if x >= 472:
                x -= 512
        if not height:
            continue
        assert zoom == 0xfff, (bank, zoom)
        layer = np.zeros_like(scene)
        tiles = []
        for row in range(min(height, 32)):
            tile, attr = word(bank * 128 + row * 4), word(bank * 128 + row * 4 + 2)
            tile += (attr & 240) << 12
            if attr & 8:
                tile = (tile & ~7) | (animation & 7)
            elif attr & 4:
                tile = (tile & ~3) | (animation & 3)
            pixels = art.tile(tile)
            if attr & 1:
                pixels = pixels[:, ::-1]
            if attr & 2:
                pixels = pixels[::-1]
            yy = y + row * 16
            left, top, right, bottom = max(x, 0), max(yy, 0), min(x + 16, 304), min(yy + 16, 224)
            if left < right and top < bottom:
                block = layer[top:bottom, left:right]
                source = pixels[top - yy:bottom - yy, left - x:right - x]
                opaque = source != 0
                block[opaque] = ((attr >> 8) << 4) | source[opaque].astype(np.uint16)
            tiles.append([tile, attr])
        opaque = layer != 0
        scene[opaque] = layer[opaque]
        depth[opaque] = bank
        layers[bank] = layer
        descriptors.append(dict(bank=bank, x=x, y=y, height=height, tiles=tiles))
    return scene, depth, layers, descriptors


def compose(layers, banks):
    plane = np.zeros((224, 304), dtype=np.uint16)
    owner = np.full(plane.shape, -1, dtype=np.int16)
    for bank in banks:
        opaque = layers[bank] != 0
        plane[opaque] = layers[bank][opaque]
        owner[opaque] = bank
    return plane, owner


def fit(plane):
    # Same playable crop as native physics: X=16..304, Y=72..200.
    # FBNeo presents CPU coordinates minus (8,16).
    return np.array(Image.fromarray(plane).crop((8, 56, 296, 184)).resize(
        (160, 100), Image.Resampling.NEAREST))


def goal_band():
    """Native point band geometry, centered about LCD rows49/50."""
    rail = np.zeros((100, 8), dtype=np.uint8)
    rail[31:69] = 2
    rail[30] = rail[69] = 3
    rail[1:99, 7] = 3
    assert np.array_equal(rail, rail[::-1]), 'Goal band must be centered vertically'
    return rail


def zone_numbers(target, x):
    glyphs = number_pixels()
    for top, value, color in ((12, 3, 3), (45, 5, 0), (78, 3, 3)):
        for y, row in enumerate(glyphs[value]):
            for column in range(6):
                if row & (0x80 >> column):
                    target[top + y, x + column] = color


def main():
    out = GAME / 'generated'
    out.mkdir(exist_ok=True)
    art = Art()
    door = ROOT / 'sources/windjammers_neogeo/gameplay/serve'
    status = json.loads((door / 'status.json').read_text())
    assert not status['darken'] and status['sprites_enabled']
    palette_raw = (door / f'palette{status["palette_bank"]}.bin').read_bytes()
    palette = palette_rgb(palette_raw)
    vram = (door / 'vram.bin').read_bytes()
    scene, depth, layers, descriptors = decode(art, vram, status['animation_frame'])
    original_path = door.parent / 'serve.png'
    original = np.array(Image.open(original_path).convert('RGB'))
    yy, xx = np.indices(scene.shape)
    checked = (depth >= 192) & (yy >= 40) & (yy < 176)
    assert np.array_equal(palette[scene[checked]], original[checked]), 'C-ROM scene RGB mismatch'

    base, base_owner = compose(layers, range(192, 212))
    goals, goal_owner = compose(layers, range(262, 268))
    net, net_owner = compose(layers, [228])
    # The mesh shadow is baked into palette40's sand, not a separate sprite.
    # Select its narrow source strip and retain its darker original pixels.
    shadow_mask = ((xx >= 144) & (xx < 154) & (yy >= 56) & (yy < 184) &
                   (base >> 4 == 0x40) & ((base & 15) >= 1) & ((base & 15) <= 4))
    sy, sx = np.nonzero(shadow_mask & (yy >= 56) & (yy < 184))
    assert len(sx), 'Missing baked net shadow'
    assert sx.min() >= 140 and sx.max() <= 160, (sx.min(), sx.max())
    asset_checks = {}
    for name, plane, owner, selection in (
            ('goals', goals, goal_owner, goals != 0),
            ('net', net, net_owner, net != 0),
            ('shadow', base, base_owner, shadow_mask)):
        visible = selection & (yy >= 56) & (yy < 184) & (depth == owner)
        assert visible.any() and np.array_equal(palette[plane[visible]], original[visible]), name
        asset_checks[name] = int(visible.sum())

    target = np.zeros((100, 160), dtype=np.uint8)
    # Requested native skin: numbered zones, untouched white sand, no shadow.
    band = goal_band()
    target[:, :8] = band
    target[:, 152:] = band[:, ::-1]
    zone_numbers(target, 1); zone_numbers(target, 153)

    # Readable net: black silhouette, pale two-pixel core and dark mesh joins.
    # Its white halo merges with the clean white floor; there is no ground shadow.
    target[1:99, 78:82] = 3
    target[2:98, 79:81] = 1
    for top in range(5, 95, 8):
        target[top:top + 2, 79:81] = 2
    target[0, :] = target[99, :] = 3

    header = ['/* Native numbered point bands, plain white sand, outlined net without shadow. */']
    for strip in range(5):
        pixels = target[:, strip * 32:(strip + 1) * 32]
        for name, bit in (('light', 1), ('dark', 2)):
            rows = [sum(int(bool(value & bit)) << (31 - x) for x, value in enumerate(row)) for row in pixels]
            header.append(f'static const u32 court_{strip}_{name}[100] = {{' +
                          ','.join(f'0x{row:08x}UL' for row in rows) + '};')
    header.append('static const RtSprite court_strips[5] = {')
    header += [f'    {{32,100,court_{i}_light,court_{i}_dark,RT_NULL}},' for i in range(5)]
    header.append('};')
    (out / 'court.h').write_text('\n'.join(header) + '\n')
    grey = np.array((255, 170, 85, 0), dtype=np.uint8)[target]
    Image.fromarray(grey).resize((640, 400), Image.Resampling.NEAREST).save(out / 'court.png')
    report = dict(program_sha256=sha(art.program), c_sha256=sha(bytes(art.raw)),
                  vram_sha256=sha(vram), palette_sha256=sha(palette_raw),
                  original_png_sha256=sha(original_path.read_bytes()), script_sha256=sha(Path(__file__).read_bytes()),
                  scene_rgb_pixels=int(checked.sum()), asset_rgb_pixels=asset_checks,
                  mismatches=0, goal_banks=list(range(262, 268)), net_banks=[228],
                  background_banks=list(range(192, 212)), sand_pixels=0,
                  sand_patches=[], plane_bytes=4000,
                  native_goal_art='3/5/3 labels; mirrored band geometry, upright digits on both sides',
                  native_goal_rows=[[1, 31], [31, 69], [69, 99]],
                  native_goal_center=49.5,
                  native_net_art='four-pixel outlined net with two-pixel core; no ground shadow',
                  native_shadow=False,
                  original_descriptors=[d for d in descriptors if d['bank'] in range(192, 212)
                                        or d['bank'] == 228 or d['bank'] in range(262, 268)])
    (out / 'court.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'ROM court: {report["scene_rgb_pixels"]} exact RGB scene pixels; '
          f'reference panels/net/shadow {asset_checks}; centered 3/5/3 bands; outlined net, no shadow or sand marks')


if __name__ == '__main__':
    main()
