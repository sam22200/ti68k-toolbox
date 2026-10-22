#!/usr/bin/env python3
"""The management menus' bank (fftmng, campaign/manage.c) from the local FFT disc -> ffmg.bin
(never committed).

usage: management_data.py DISC.cue OUT.bin [preview.png]
Everything the menus show or rule on, by FFT's own ids (campaign/save.h), big-endian:
- "FMG1", then u16 offsets: jobs, items, ability names, skill set names, icons, sprites,
  portraits, font, generic names (each from the bank's start);
- 21 jobs of JOB_SIZE bytes: the 20 generic ones (Squire 0x4A .. Mime 0x5D, a unit's career
  row) then Ramza's own Squire (0x01): name, FFT id, skill set, growth and multiplier per raw
  stat (HP MP SP PA MA), Move, Jump, equipment type bits, the job levels each career needs
  (a nibble each, 0 none: SCUS 0x800660c4), who may take it (0 anyone, 1 men, 2 women), its
  22 abilities (16 actions, then reactions, supports, movements: the learned bits' order) and
  their JP costs;
- 256 items of ITEM_SIZE bytes: name, slot flags (0x80 weapon, 0x40 shield, 0x20 head, 0x10
  body, 0x08 accessory), type (its equipment bit), level, HP, MP, PA, MA, Speed, Move, Jump,
  icon, WP;
- 512 ability and 64 skill set name offsets (0: none), names NUL-terminated ASCII;
- 8x8 icons (EVENT/ITEM.BIN, 24 bytes: light, dark, mask; 0: empty), 16x26 sprites (one still
  idle frame, white outline, 156 bytes) and 32x29 portraits (348 bytes) per career and sex
  (2 career + 1 for a woman), then Ramza's (index 40);
- the menus' pixel font (management_assets.pixel_fonts);
- the generic units' names (EVENT/SMALL.OUT, interface.py's): 255 men's, then 255 women's offsets.
Names come from BATTLE.BIN's lists (FFT's text encoding, 0xFE between entries).
"""
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image

import battle
import extract
import interface
import management_assets
import units

JOB_NAMES, ITEM_NAMES, ABILITY_NAMES, SET_NAMES = 0xfad41, 0xfb2c3, 0xfcb88, 0xfe93a   # BATTLE.BIN
REQUIRE = 0x800660c4                    # 19 rows of 10 bytes (Chemist .. Mime), a nibble per career
CAREERS = 20
SHEETS = 'MINA ITEM KNIGHT YUMI MONK SIRO KURO TOKI SYOU THIEF WAJU ONMYO FUSUI RYU SAMU NINJA SAN GIN ODORI MONO'.split()
ONLY = {17: 1, 18: 2}                   # Bard: men, Dancer: women
JOB_SIZE, ITEM_SIZE = 120, 14
# FFT's text: digits, capitals, small letters, then a few signs (two-byte ones: page * 208 + byte)
SIGNS = {0x3E: '!', 0x42: '+', 0x93: "'", 0xFA: ' ', 0xB0: '-', 0xB1: "'", 0xB2: '.', 0xFD: ''}   # FD: a break
PAGED = {237: '-', 2054: '.', 2061: ':', 2065: "'", 2062: '(', 2063: ')', 2195: ' ', 2196: ',', 2198: "'"}


def decode(t):
    s, i = '', 0
    while i < len(t):
        c = t[i]; i += 1
        if c < 10: s += chr(48 + c)
        elif c < 36: s += chr(55 + c)
        elif c < 62: s += chr(c - 36 + 97)
        elif c in SIGNS: s += SIGNS[c]
        elif 0xD0 <= c <= 0xDF and i < len(t):
            g = (c & 15) * 208 + t[i]; i += 1
            if g not in PAGED: raise ValueError('name glyph %d' % g)
            s += PAGED[g]
        else: raise ValueError('name byte %02X' % c)
    return ' '.join(s.split())


def names(bat, at, n):
    out = []
    for _ in range(n):
        end = bat.index(0xFE, at)
        out.append(decode(bat[at:end])); at = end + 1
    return out


def icon(sheet, palettes, rec):
    """An item's 16x16 picture as an 8x8 TI icon (management_assets.equipment_icons' way)."""
    pal_id, sprite = rec[0], rec[1]
    x, y = (sprite % 15) * 16, 32 + (sprite // 15) * 16
    if y + 16 > 256: return None
    px = sheet[y:y + 16, x:x + 16]; pal = palettes[pal_id]
    lum = (pal & 31) * .299 + ((pal >> 5) & 31) * .587 + ((pal >> 10) & 31) * .114
    g = np.zeros((8, 8), np.uint8); opaque = np.zeros((8, 8), bool)
    for yy in range(8):
        for xx in range(8):
            v = px[yy * 2:yy * 2 + 2, xx * 2:xx * 2 + 2]; v = v[v != 0]
            if v.size:
                opaque[yy, xx] = True; m = float(lum[v].mean())
                g[yy, xx] = 3 if m <= 7 else 2 if m <= 16 else 1 if m <= 24 else 0
    if not opaque.any(): return None
    return b''.join(np.packbits(m, axis=1).tobytes() for m in ((g & 1) != 0, (g & 2) != 0, ~opaque))


def portrait(spr):
    """interface.py's portrait: the sheet's single-facing face, 32 x 29, outlined."""
    pal = np.frombuffer(spr[8 * 32:9 * 32], '<u2').astype(int)
    packed = np.frombuffer(spr[0x8200:0x9200], np.uint8).reshape(32, 128)
    sheet = np.empty((32, 256), np.uint8); sheet[:, ::2] = packed & 15; sheet[:, 1::2] = packed >> 4
    scaled = np.array(Image.fromarray(np.rot90(sheet[:, 80:128])).resize((17, 27), Image.Resampling.NEAREST))
    im = np.full((29, 32), -1, dtype=int)
    im[1:28, 1:18] = interface.greys(scaled, pal)
    return interface.outline(im)


def main():
    cue, out = sys.argv[1], Path(sys.argv[2])
    d = battle.Disc(cue)
    disc, files = extract.disc_files(cue)
    bat = d.bat
    jobn, itemn = names(bat, JOB_NAMES, 0xA0), names(bat, ITEM_NAMES, 256)
    abiln, setn = names(bat, ABILITY_NAMES, 512), names(bat, SET_NAMES, 64)
    assert (jobn[0x4A], jobn[0x5D], itemn[0x13], itemn[0xFD], abiln[0x92], abiln[0x1E0], setn[5], setn[0x19]) == \
        ('Squire', 'Mime', 'Broad Sword', 'Phoenix Down', 'Accumulate', 'Equip Change', 'Basic Skill', 'Guts')
    strings = bytearray(); where = {}

    def string(s):
        if not s: return None
        if s not in where:
            where[s] = len(strings); strings.extend(s.encode('ascii') + b'\0')
        return where[s]

    req = d.at(REQUIRE, 19 * 10)
    jobs = []
    for k in range(CAREERS + 1):
        jid = 0x4A + k if k < CAREERS else 0x01
        j = d.job(jid)
        sk = d.skillset(j['set']) + [a for a, in battle.rsm_raw(d, j['set'])]
        cost = [struct.unpack('<H', d.at(battle.ABILITY + 8 * a, 2))[0] if a else 0 for a in sk]
        rows = req[(k - 1) * 10:k * 10] if 0 < k < CAREERS else bytes(10)
        jobs.append((string(jobn[jid]), jid, j['set'], j['growth'], j['mult'], j['move'], j['jump'], j['cats'],
                     rows, ONLY.get(k, 0), sk, cost))
    raw = disc.read(*files['EVENT/ITEM.BIN'])
    a = np.frombuffer(raw[:32768], np.uint8).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8); sheet[:, ::2] = a & 15; sheet[:, 1::2] = a >> 4
    palettes = np.frombuffer(raw[32768:], '<u2').reshape(16, 16).astype(int)
    icons = [bytes(16) + bytes([255] * 8)]; icon_of = {}
    items = []
    for i in range(256):
        b = d.at(battle.ITEMS + 12 * i, 12)
        it = d.item(i)
        wp = d.at(battle.WEAPONS + 8 * i, 8)[4] if b[3] & 0x80 else 0
        k = 0
        if i and (b[0], b[1]) not in icon_of:
            p = icon(sheet, palettes, b)
            if p: icon_of[(b[0], b[1])] = len(icons); icons.append(p)
        if i: k = icon_of.get((b[0], b[1]), 0)
        items.append((string(itemn[i]) if i else None, b[3], b[5], b[2], it['hp'], it['mp'], it['pa'], it['ma'],
                      it['sp'], it['move'], it['jump'], k, wp))
    shp = {t: units.shp_frames(disc.read(*files['BATTLE/%s.SHP' % t])) for t in ('TYPE1', 'TYPE2')}
    sprites, faces, previews = [], [], []
    for k in range(CAREERS + 1):
        for g in ('MW' if k < CAREERS else ' '):
            name = 'BATTLE/%s_%s.SPR' % (SHEETS[k], g) if k < CAREERS else 'BATTLE/RAMUZA.SPR'
            if name not in files:                  # Bard, Dancer: the other sex's
                name = name.replace('_W', '_M') if g == 'W' else name.replace('_M', '_W')
            spr = disc.read(*files[name])
            im = units.sprite(spr, shp['TYPE2' if k == 1 else 'TYPE1'][9])
            face = portrait(spr)
            sprites.append(interface.planes(im, 16)); faces.append(interface.planes(face, 32))
            previews.append(np.concatenate((np.pad(im, ((3, 0), (0, 16)), constant_values=-1), face), 0))
    font = management_assets.pixel_fonts()

    body = bytearray(b'FMG1' + bytes(18))
    offs = []

    def section():
        if len(body) & 1: body.append(0)
        offs.append(len(body))

    section()
    for (nm, jid, st, gr, mu, mv, jp, cats, rows, only, sk, cost) in jobs:
        rec = struct.pack('>HBB5B5BBB4s10sBB', 0, jid, st, *gr, *mu, mv, jp, bytes(cats), rows, only, len(sk))
        rec += struct.pack('>22H22H', *sk, *cost)
        assert len(rec) == JOB_SIZE, len(rec)
        body += rec
    section()
    for it in items:
        body += struct.pack('>H12B', 0, *it[1:])
    assert ITEM_SIZE == 14
    section(); body += bytes(2 * 512)
    section(); body += bytes(2 * 64)
    section()
    for p in icons: body += p
    section()
    for p in sprites: body += p
    section()
    for p in faces: body += p
    section(); body += font
    section(); body += bytes(2 * 510)
    small = disc.read(*files['EVENT/SMALL.OUT'])
    entries = small[0xec + struct.unpack_from('<I', small, 0x70)[0]:].replace(b'\xff', b'\xfe').split(b'\xfe')
    people = [interface.decode(e) for first in (256, 512) for e in entries[first:first + 255]]
    if len(body) & 1: body.append(0)
    text_at = len(body); body += strings
    put = lambda at, s: struct.pack_into('>H', body, at, 0 if s is None else text_at + s)
    for k, j in enumerate(jobs): put(offs[0] + JOB_SIZE * k, j[0])
    for k, it in enumerate(items): put(offs[1] + ITEM_SIZE * k, it[0])
    for k, s in enumerate(abiln): put(offs[2] + 2 * k, string(s) if s.strip() else None)
    for k, s in enumerate(setn): put(offs[3] + 2 * k, string(s) if s.strip() else None)
    for k, s in enumerate(people): put(offs[8] + 2 * k, string(s))
    body = body[:text_at] + strings                # (the names put last are added here)
    struct.pack_into('>9H', body, 4, *offs)
    assert len(body) < 65000, len(body)
    out.write_bytes(bytes(body))
    if len(sys.argv) > 3:
        a = np.concatenate(previews, 1)
        rgb = np.where(a < 0, 170, np.array([255, 170, 85, 0])[a.clip(0)]).astype(np.uint8)
        Image.fromarray(rgb).resize((a.shape[1] * 3, a.shape[0] * 3), Image.Resampling.NEAREST).save(sys.argv[3])
    print('%s: %d bytes (21 jobs, 256 items, %d icons, 41 sprites and portraits)' % (out.name, len(body), len(icons)))


if __name__ == '__main__':
    main()
