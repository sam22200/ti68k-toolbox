#!/usr/bin/env python3
"""Decode local C chips and ordinary actor poses into outlined grey sprites.

Descriptor interpretation follows program 012DEE..013086; planar decoding
follows the pinned FBNeo NeoDecodeSprites. Generated banks stay ignored.
"""
import json
from pathlib import Path
import sys
import numpy as np
from PIL import Image, ImageFilter

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / 'tools/neogeo'))
from romset import read_chips, cpu_program, DEFAULT_ROM
from neogeorun import sha


class Art:
    def __init__(self):
        chips = read_chips(DEFAULT_ROM)
        self.program = cpu_program(chips['065-p1.p1'])
        self.raw = bytearray(0x400000)
        self.raw[0::2] = chips['065-c1.c1'] + chips['065-c3.c3']
        self.raw[1::2] = chips['065-c2.c2'] + chips['065-c4.c4']
        self.tiles = {}

    def word(self, address, signed=False):
        return int.from_bytes(self.program[address:address + 2], 'big', signed=signed)

    def pointer(self, address):
        return int.from_bytes(self.program[address:address + 4], 'big')

    def tile(self, number):
        if number not in self.tiles:
            raw = np.frombuffer(self.raw, dtype=np.uint8, count=128, offset=number * 128).reshape(2, 16, 4)
            tile = np.zeros((16, 16), dtype=np.uint8)
            for half, source in enumerate((1, 0)):
                for x in range(8):
                    for significance, plane in enumerate((0, 2, 1, 3)):
                        tile[:, half * 8 + x] |= ((raw[source, :, plane] >> x) & 1) << significance
            self.tiles[number] = tile
        return self.tiles[number]

    def pose(self, number, flipped=False, palette_override=0):
        if flipped:
            number += self.word(0x32000)
        ptr = self.pointer(0x32020 + 4 * number)
        flags, columns = self.program[ptr:ptr + 2]
        assert flags & 128 and 0 < columns <= 4, (number, flags, columns)
        ptr += 2
        canvas = np.zeros((80, 80), dtype=np.uint16)
        self.last_columns = []
        x = y = 0
        for col in range(columns):
            control, tile, attr = (self.word(ptr + offset) for offset in (0, 2, 4))
            if palette_override:
                attr = (attr & 255) | (palette_override << 8)
            ptr += 6
            height = control & 63
            if control & 64:
                x += 16
            else:
                y, x = self.word(ptr, True), self.word(ptr + 2, True)
                ptr += 4
            self.last_columns.append((tile, attr, x, y, height))
            for row in range(height):
                pixels = self.tile(tile + row)
                if attr & 1:
                    pixels = pixels[:, ::-1]
                if attr & 2:
                    pixels = pixels[::-1]
                yy, xx = y + 40 + 16 * row, x + 40
                block = canvas[yy:yy + 16, xx:xx + 16]
                assert block.shape == (16, 16)
                nonzero = pixels != 0
                block[nonzero] = ((attr >> 8) << 4) | pixels[nonzero].astype(np.uint16)
        return canvas

    def animation(self, port, action, direction):
        char = 12 if port else 20
        ptr = self.pointer(0x20a3e + char)
        ptr = self.pointer(ptr + (action >> 8))
        ptr = self.pointer(ptr + (action & 255))
        ptr = self.pointer(ptr + (((direction + 16) & 224) >> 3))
        interval = self.program[ptr + 1]
        poses = []
        for offset in range(2, 40, 2):
            word = self.word(ptr + offset)
            if word == 0:
                break
            poses.append(word & 32767)
            if word & 32768:
                break
        assert poses and interval > 0
        return interval, poses


def palette_rgb(raw):
    # Pinned core's six parallel conductances, including the shared dark bit.
    resistors = np.array((8200, 3900, 2200, 1000, 470, 220), dtype=float)
    weights = (255 / resistors) / np.sum(1 / resistors)
    values = np.frombuffer(raw, dtype='>u2').astype(np.uint32)
    red = ((values & 0x0f00) >> 4) | ((values >> 11) & 8) | ((values >> 13) & 4)
    green = (values & 0x00f0) | ((values >> 10) & 8) | ((values >> 13) & 4)
    blue = ((values & 0x000f) << 4) | ((values >> 9) & 8) | ((values >> 13) & 4)
    return np.stack([sum(((channel >> (bit + 2)) & 1) * weight
                         for bit, weight in enumerate(weights)) for channel in (red, green, blue)], axis=1).round().astype(np.uint8)


def main():
    art = Art()
    out = GAME / 'generated'
    report = json.loads((out / 'actions.json').read_text())
    assert sha(art.program) == report['program_sha256']
    door = ROOT / 'sources/windjammers_neogeo/gameplay/serve'
    bank = json.loads((door / 'status.json').read_text())['palette_bank']
    palette = palette_rgb((door / f'palette{bank}.bin').read_bytes())
    sequences = []
    poses = set()
    pose_palettes = {}
    palette_ids = [report['trials']['p2_A']['initial'][f'p{port + 1}_palette'] for port in (0, 1)]
    for port in (0, 1):
        for action in (0, 0x400, 0x1004, 0x1400, 0x1000):
            default = 192 if port else 64
            for direction in (0, 32, 64, 96, 128, 160, 192, 224):
                used = action == 0x400 or direction == default or (
                    action in (0x1400, 0x1000) and direction in (default - 32, default + 32))
                source_direction = direction if used else default
                interval, seq = art.animation(port, action, source_direction)
                flipped = source_direction >= 128
                sequences.append((port, action, direction, interval, seq, flipped))
                poses.update((p, flipped) for p in seq)
                pose_palettes.update({p: palette_ids[port] for p in seq})
    # Include every observed ordinary action frame, even if its direction
    # changes transiently while the original settles a diagonal catch.
    for trial in report['trials'].values():
        for row in trial['rows'][:110]:
            for port in (0, 1):
                poses.add((row[f'p{port + 1}_pose'], bool(row[f'p{port + 1}_flags'] & 16)))
                pose_palettes[row[f'p{port + 1}_pose']] = row[f'p{port + 1}_palette']
    banks, indices, previews = [], {}, []
    for ident, (pose, flipped) in enumerate(sorted(poses)):
        canvas = art.pose(pose, flipped, pose_palettes[pose])
        rgba = np.zeros((80, 80, 4), dtype=np.uint8)
        rgba[:, :, :3] = palette[canvas]
        rgba[:, :, 3] = (canvas != 0) * 255
        image = Image.fromarray(rgba)
        bbox = image.getbbox()
        assert bbox
        # Keep the half-scale origin fixed; max-pool alpha preserves thin limbs.
        rgba = rgba.reshape(40, 2, 40, 2, 4)
        alpha = rgba[:, :, :, :, 3].max(axis=(1, 3)) > 0
        colors = rgba[:, :, :, :, :3].astype(np.uint16)
        counts = (rgba[:, :, :, :, 3] > 0).sum(axis=(1, 3))
        average = np.sum(colors * (rgba[:, :, :, :, 3:4] > 0), axis=(1, 3)) / np.maximum(counts[:, :, None], 1)
        luma = (average[:, :, 0] * 77 + average[:, :, 1] * 150 + average[:, :, 2] * 29) / 256
        levels = np.where(luma < 72, 3, np.where(luma < 145, 2, np.where(luma < 218, 1, 0)))
        mask_image = Image.fromarray((alpha * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(3))
        mask = np.array(mask_image) != 0
        ys, xs = np.nonzero(mask)
        left, top, right, bottom = min(xs), min(ys), max(xs) + 1, max(ys) + 1
        assert right - left <= 32
        width = 16 if right - left <= 16 else 32
        height = bottom - top
        planes = []
        for plane in (1, 2, 0):
            data = []
            for y in range(top, bottom):
                word = 0
                for x in range(width):
                    xx = left + x
                    bit = ((levels[y, xx] & plane) != 0 and alpha[y, xx] if plane else not mask[y, xx]) if xx < 40 else plane == 0
                    if bit:
                        word |= 1 << (width - 1 - x)
                data.append(word)
            planes.append(data)
        indices[(pose, flipped)] = ident
        banks.append((width, height, left - 20, top - 20, planes))
        preview = np.full((height, width), 216, dtype=np.uint8)
        for y in range(height):
            for x in range(min(width, 40 - left)):
                if mask[top + y, left + x]:
                    preview[y, x] = (216, 152, 88, 24)[levels[top + y, left + x]] if alpha[top + y, left + x] else 216
        previews.append(Image.fromarray(preview).convert('RGB'))
    header = ['/* Local ROM-derived outlined actor bank. */']
    for ident, (width, height, x, y, planes) in enumerate(banks):
        kind = 'u16' if width == 16 else 'u32'
        for suffix, data in zip(('light', 'dark', 'mask'), planes):
            header.append(f'static const {kind} art_{ident}_{suffix}[{height}] = {{' + ','.join(f'0x{v:x}UL' for v in data) + '};')
    header.append('static const WjArt actor_art[] = {')
    header += [f'  {{{{{w},{h},art_{i}_light,art_{i}_dark,art_{i}_mask}},{x},{y}}},' for i, (w, h, x, y, _) in enumerate(banks)]
    header.append('};')
    for divisor in (5, 6):
        header.append(f'static const u8 art_div{divisor}[256] = {{' + ','.join(str(i // divisor) for i in range(256)) + '};')
    # A small lookup selects an actual ROM animation; playback timing is native.
    header.append('static const WjAnimation actor_animation[2][5][8] = {')
    for port in (0, 1):
        header.append('  {')
        for action in (0, 0x400, 0x1004, 0x1400, 0x1000):
            header.append('    {')
            for p, a, d, interval, seq, flipped in sequences:
                if (p, a) != (port, action):
                    continue
                ids = [indices[(pose, flipped)] for pose in seq]
                assert len(ids) <= 16
                header.append('      {' + str(interval) + ',' + str(len(ids)) + ',' +
                              str(interval * len(ids)) + ',{' + ','.join(map(str, ids)) + '}},')
            header.append('    },')
        header.append('  },')
    header.append('};')
    (out / 'art.h').write_text('\n'.join(header) + '\n')
    sheet = Image.new('RGB', (480, ((len(previews) + 11) // 12) * 36), '#989898')
    for i, preview in enumerate(previews):
        sheet.paste(preview, ((i % 12) * 40, (i // 12) * 36))
    sheet.resize((960, sheet.height * 2), Image.Resampling.NEAREST).save(out / 'actors.png')
    (out / 'art.json').write_text(json.dumps(dict(program_sha256=sha(art.program),
        c_sha256=sha(bytes(art.raw)), poses=len(banks), sequences=len(sequences),
        raw_bank_bytes=int(sum(w // 8 * h * 3 for w, h, *_ in banks)),
        script_sha256=sha(Path(__file__).read_bytes()),
        actions_sha256=sha((out / 'actions.json').read_bytes()),
        palette_sha256=sha((door / f'palette{bank}.bin').read_bytes()),
        palette_overrides=palette_ids,
        timing='native playback of original pose sequences'), indent=2) + '\n')
    print(f'ROM actors: {len(banks)} outlined poses, {len(sequences)} animations')


if __name__ == '__main__':
    main()
