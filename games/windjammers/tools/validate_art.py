#!/usr/bin/env python3
"""Compare decoded ROM actor pixels with the pinned original framebuffer."""
import json
from pathlib import Path
import sys
import numpy as np

from extract_art import Art, palette_rgb, GAME, ROOT
sys.path.insert(0, str(ROOT / 'tools/neogeo'))
from check_gameplay import values, mask
from check_rules import EXTRA_FIELDS
from neogeorun import NeoGeo, sha


def depth_mask(art, video, animation):
    """Latest opaque sprite bank, from the original active VRAM strips.

    The selected court/actors use unshrunk strips. This identifies original
    occlusion before comparing colors, including the Beach foreground wall.
    """
    raw = video['vram']
    word = lambda p: int.from_bytes(raw[p:p + 2], 'big')
    depth = np.full((224, 304), -1, dtype=np.int16)
    strips = []
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
        strips.append((bank, word(bank * 128), word(bank * 128 + 2), x, y, height))
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
            xx, yy = x, y + row * 16
            if xx >= 304 or xx + 16 <= 0 or yy >= 224 or yy + 16 <= 0:
                continue
            left, top, right, bottom = max(xx, 0), max(yy, 0), min(xx + 16, 304), min(yy + 16, 224)
            block = depth[top:bottom, left:right]
            block[pixels[top - yy:bottom - yy, left - xx:right - xx] != 0] = bank
    return depth, strips


def main():
    art = Art()
    neo = NeoGeo()
    total, scenes, compared, occluded, outside_scope = 0, 0, set(), 0, 0
    try:
        provenance = json.loads((GAME / 'generated/actions.json').read_text())['metadata']
        for field in ('core_sha256', 'rom_sha256', 'bios_archive_sha256', 'fps', 'options', 'rtc_initial'):
            assert neo.metadata()[field] == provenance[field], field
        trials = [(owner, GAME / 'generated' / name, direction, True)
                  for owner, name in ((1, 'actions_serve.state'), (0, 'actions_mita.state'))
                  for direction in ('A', 'UP A', 'DOWN A')]
        # Move the free player while the other holds: stop before an automatic
        # release can introduce an unselected rear/lob/goal animation.
        trials += [(port, GAME / 'generated' / ('actions_mita.state' if port else 'actions_serve.state'), direction, False)
                   for port in (0, 1) for direction in
                   ('UP', 'DOWN', 'LEFT', 'RIGHT', 'UP LEFT', 'UP RIGHT', 'DOWN LEFT', 'DOWN RIGHT')]
        for owner, path, direction, throwing in trials:
            neo.load(path)
            for frame in range(110 if throwing else 40):
                # Windjammers queues these entity descriptors before the
                # following game-logic update. End-of-step RAM describes
                # the next scene, while current VRAM/video shows this one.
                state = {**values(neo), **{k: neo.read(*v) for k, v in EXTRA_FIELDS.items()}}
                overrides = [neo.read(0x100803), neo.read(0x100883)]
                pads = [0, 0]
                if (throwing and 4 <= frame < 6) or (not throwing and frame < 80):
                    pads[owner] = mask(direction)
                neo.pressed[:] = pads
                neo.step()
                if frame < 2:
                    continue
                original = np.array(neo.image())
                video = neo.video()
                palette = palette_rgb(video[f'palette{neo.status()["palette_bank"]}'])
                depth, strips = depth_mask(art, video, neo.status()['animation_frame'])
                for port in (0, 1):
                    p = f'p{port + 1}'
                    if state[p + '_action'] not in (0, 0x400, 0x1000, 0x1004, 0x1400):
                        outside_scope += 1
                        continue
                    pose, flip = state[p + '_pose'], bool(state[p + '_flags'] & 16)
                    image = art.pose(pose, flip, overrides[port])
                    body_banks = []
                    for tile, attr, xx, yy, height in art.last_columns:
                        match = [bank for bank, t, a, sx, sy, h in strips if
                                 (t, a, sx, sy, h) == (tile, attr, xx + state[p + '_render_x'] - 8,
                                                      yy + state[p + '_render_y'] - 16, height)]
                        assert len(match) == 1, (owner, direction, frame, port, state[p + '_action'], pose, tile, match)
                        body_banks.extend(match)
                    x, y = state[p + '_render_x'] - 8 - 40, state[p + '_render_y'] - 16 - 40
                    yy, xx = np.nonzero(image)
                    sx, sy = x + xx, y + yy
                    visible = (sx >= 0) & (sx < 304) & (sy >= 0) & (sy < 224)
                    # Exclude only genuinely later opaque VRAM banks.
                    occluded += int(np.sum(depth[sy[visible], sx[visible]] > max(body_banks)))
                    visible[visible] &= depth[sy[visible], sx[visible]] <= max(body_banks)
                    expected = palette[image[yy[visible], xx[visible]]]
                    actual = original[sy[visible], sx[visible]]
                    mismatch = np.any(expected != actual, axis=1)
                    if mismatch.any():
                        raise AssertionError((owner, direction, frame, port, pose,
                                              int(mismatch.sum()), len(actual),
                                              list(zip(sx[visible][mismatch].tolist(), sy[visible][mismatch].tolist()))[:40],
                                              expected[mismatch][:3].tolist(), actual[mismatch][:3].tolist()))
                    total += len(actual)
                    compared.add((pose, flip))
                scenes += 1
        report = dict(metadata=neo.metadata(), actor_pixels=total, scenes=scenes, trials=len(trials),
                      observed_poses=len(compared), mismatches=0,
                      outside_scope_actor_samples=outside_scope,
                      occluded_pixels=occluded,
                      exclusion='opaque later original VRAM banks, determined before RGB comparison',
                      script_sha256=sha(Path(__file__).read_bytes()))
        (GAME / 'generated/art_validation.json').write_text(json.dumps(report, indent=2) + '\n')
        print(f'Original artwork: {total} exact RGB actor pixels in {scenes} scenes, {len(compared)} observed poses')
    finally:
        neo.close()


if __name__ == '__main__':
    main()
