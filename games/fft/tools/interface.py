#!/usr/bin/env python3
"""Local FFT portraits, names and cursor plus AMS preshifted text -> ffti (never committed)."""
from pathlib import Path
import struct
import sys
import numpy as np
from PIL import Image
import extract
from units import CHARS


def decode(data):
    chars = []
    data = data.replace(b"\xd9\xc1", b"\xb1").replace(b"\xd9\xb6", b"\xb2")
    for c in data:
        if c < 10: chars.append(chr(48 + c))
        elif c < 36: chars.append(chr(65 + c - 10))
        elif c < 62: chars.append(chr(97 + c - 36))
        elif c == 0xfa: chars.append(' ')
        elif c == 0xb0: chars.append('-')
        elif c == 0xb1: chars.append("'")
        elif c == 0xb2: chars.append(".")
        else: raise ValueError('name character %02x' % c)
    return ''.join(chars)


def planes(im, width):
    bits = np.array([1 << (width - 1 - x) for x in range(width)], dtype=np.uint32)
    out = bytearray()
    for mask in (((im >= 0) & ((im & 1) != 0)), ((im >= 0) & ((im & 2) != 0)), im < 0):
        out += b''.join(int((r * bits).sum()).to_bytes(width // 8, 'big') for r in mask)
    return out


def greys(px, pal):
    lum = (pal & 31) * .299 + (pal >> 5 & 31) * .587 + (pal >> 10 & 31) * .114
    g = np.where(lum <= 7, 3, np.where(lum <= 16, 2, np.where(lum <= 22, 1, 0)))
    return np.where(px == 0, -1, g[px])


def outline(im):
    m = im >= 0
    p = np.pad(m, 1)
    im[(p[:-2, 1:-1] | p[2:, 1:-1] | p[1:-1, :-2] | p[1:-1, 2:]) & ~m] = 0
    return im


def check(cue, state):
    import oracle
    disc, files = extract.disc_files(cue)
    psx = oracle.PSX(cue); psx.load(state)
    vram = np.frombuffer(bytes(psx.vram), '<u2').reshape(512, 1024)
    count = 0
    for _, name, _ in CHARS:
        spr = disc.read(*files['BATTLE/%s.SPR' % name])
        slots = []
        for slot in range(16):
            x, y = 0x340 + (slot >> 3) * 64, 0x100 + (slot & 7) * 32
            if vram[y:y + 32, x:x + 64].tobytes() == spr[0x8200:0x9200]: slots.append(slot)
        assert slots, (name, 'portrait sheet not matched to original VRAM')
        print(name, 'portrait sheet equals original VRAM slots', slots)
        count += 32 * 48
    frame = disc.read(*files['EVENT/FRAME.BIN'])
    sheet = np.frombuffer(frame[0x1000:0x9000], '<u2').reshape(256, 64)
    assert np.array_equal(vram[384:408, 976:980], sheet[128:152, 16:20])
    bat = disc.read(*files['BATTLE.BIN'])
    pal = np.frombuffer(bat[0x80094b64 - 0x80067000:0x80094b64 - 0x80067000 + 32], '<u2')
    assert np.array_equal(vram[0x1e2, 64:80] & 0x7fff, pal & 0x7fff)
    # Every displayed generic name in the oracle belongs to the decoded source list.
    small = disc.read(*files['EVENT/SMALL.OUT'])
    base = 0xec + struct.unpack_from('<I', small, 0x70)[0]
    names = small[base:].replace(b'\xff', b'\xfe').split(b'\xfe')
    male = {decode(raw) for raw in names[256:511]}; female = {decode(raw) for raw in names[512:767]}
    for slot in (1, 2, 3, 4, 5, 17, 18, 19, 20):
        a = oracle.UNITS + oracle.UNIT_SIZE * slot
        raw = bytes(psx.ram[psx.addr(a + 0x12c):psx.addr(a + 0x12c) + 16])
        text = decode(raw.replace(b'\xff', b'\xfe').split(b'\xfe')[0])
        assert text in male | female, (slot, text)
    print('%d portrait source pixels, FRAME texture/palette and nine original names checked' % count)


def main():
    disc, files = extract.disc_files(sys.argv[1])
    out = bytearray()
    defines = []
    def section(name):
        if len(out) & 1: out.append(0)
        defines.append('#define UI_%s %dUL' % (name, len(out)))
    # Each 5-row glyph pre-shifted to all 16 word positions. No variable shifts in the blit.
    osfile = Path(__file__).resolve().parents[3] / 'tools/rom/TI89Titanium_OS.89u'
    font = osfile.read_bytes()[1023838:1023838 + 1536]
    section('ADV')
    out += bytes(font[6 * c] for c in range(32, 127))
    section('FONT')
    for c in range(32, 127):
        for sh in range(16):
            out += struct.pack('>5I', *(int(v) << (24 - sh) for v in font[c * 6 + 1:c * 6 + 6]))
    section('PORTRAITS')
    previews = []
    for _, name, _ in CHARS:
        spr = disc.read(*files['BATTLE/%s.SPR' % name])
        pal = np.frombuffer(spr[8 * 32:9 * 32], '<u2').astype(int)
        packed = np.frombuffer(spr[0x8200:0x9200], np.uint8).reshape(32, 128)
        sheet = np.empty((32, 256), np.uint8)
        sheet[:, ::2] = packed & 15; sheet[:, 1::2] = packed >> 4
        # The original portrait quad: (u,v)=(128,0),(128,31),(80,0),(80,31).
        source = np.rot90(sheet[:, 80:128])
        scaled = np.array(Image.fromarray(source).resize((17, 27), Image.Resampling.NEAREST))
        im = np.full((29, 32), -1, dtype=int)
        im[1:28, 1:18] = greys(scaled, pal)
        outline(im)
        previews.append(im[:, :19].copy())
        out += planes(im, 32)
        mirrored = np.full_like(im, -1); mirrored[:, :19] = im[:, :19][:, ::-1]
        out += planes(mirrored, 32)
    section('MARKER')
    frame = disc.read(*files['EVENT/FRAME.BIN'])
    packed = np.frombuffer(frame[0x1000:0x9000], np.uint8).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8); sheet[:, ::2] = packed & 15; sheet[:, 1::2] = packed >> 4
    bat = disc.read(*files['BATTLE.BIN'])
    pal = np.frombuffer(bat[0x80094b64 - 0x80067000:0x80094b64 - 0x80067000 + 32], '<u2').astype(int)
    marker = np.full((14, 16), -1, dtype=int)
    px = sheet[128:152, 64:80][::2, ::2]
    marker[1:13, 4:12] = greys(px, pal)
    outline(marker); out += planes(marker, 16)
    small = disc.read(*files['EVENT/SMALL.OUT'])
    base = 0xec + struct.unpack_from('<I', small, 0x70)[0]
    entries = small[base:].replace(b'\xff', b'\xfe').split(b'\xfe')
    for name, first in (('MALE', 256), ('FEMALE', 512)):
        section(name)
        for raw in entries[first:first + 255]:
            s = decode(raw).encode('ascii'); assert len(s) <= 11, s
            out += s.ljust(12, b'\0')
    Path('interface.h').write_text('// Generated locally by tools/interface.py; never committed.\n' + '\n'.join(defines) + '\n')
    Path('ffti.bin').write_bytes(out)
    Path('x').mkdir(exist_ok=True)
    rgb = np.array([255, 170, 85, 0], np.uint8)
    tiles = [np.where(im < 0, 170, rgb[im.clip(0)]) for im in previews]
    Image.fromarray(np.concatenate(tiles, axis=1)).resize((19 * 6 * 4, 29 * 4), Image.Resampling.NEAREST).save('x/m26_portraits.png')
    Image.fromarray(np.where(marker < 0, 170, rgb[marker.clip(0)])).resize((64, 56), Image.Resampling.NEAREST).save('x/m26_marker.png')
    print('ffti: %d bytes; six mirrored portraits, original marker, 510 names, preshifted AMS text' % len(out))


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--check': check(sys.argv[2], sys.argv[3])
    else: main()
