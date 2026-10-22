#!/usr/bin/env python3
"""A chapter's descriptor (campaign/ENGINE.md, M1): the per-chapter numbers the engine used to
take from generated headers, as one small archived variable read at load.

usage: chapter_desc.py opening|campaign ...
Reads <chapter>/data/{battle,interface,map}.h and the actor offsets (opening/data/units.h,
campaign/data/actors.h); writes <chapter>/data/<ffod|ffgd>.bin and the shared layout
chapter.h (CD_* word indices, identical for every chapter). Layout: "FCD1", then u16
fields (FIELDS), then the small tables (u8 rows; zodiac_days u16), each at a field's
byte offset from the start of the descriptor (0: the chapter has none).
"""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parents[1]
# Numbers: header #define name (None: computed here)
NUMBERS = [('NJOB', 'NJOB'), ('NITEM', 'NITEM'), ('NABIL', 'NABIL'), ('NSET', 'NSET'),
           ('SET_BASIC', 'SET_BASIC'), ('NRSM_PRE', 'NRSM_PRE'), ('DEFEND_JP', 'DEFEND_JP'),
           ('DEFEND_RATE', 'DEFEND_RATE'), ('NBAG', 'NBAG'), ('MAP_H', 'MAP_H'), ('MAP_W', 'MAP_W'),
           ('ACTOR_ARMED', 'ACTOR_ARMED'), ('ACTOR_OBJECTS', 'ACTOR_OBJECTS'),
           ('SC_H', None), ('SC_TOP', None)]
# Rows cut above the scenery of every view (tools/campaign_assets.py TOP); SC_H is measured
# on the views themselves (Orbonne's are cut at the bottom by tools/opening_assets.py).
SC_TOP = {'opening': 0, 'campaign': 16}
VIEW = {'opening': 'ffov1', 'campaign': 'ffgv1'}
# Offsets into the chapter's interface bank (igfx): header names
IGFX = [('JOBS', 'jobs'), ('ITEMS', 'items'), ('ABILITIES', 'abilities'), ('SKILL_SETS', 'skill_sets'),
        ('UNIT_DEFS', 'unit_defs'), ('PORTRAITS', 'UI_PORTRAITS'), ('MALE', 'UI_MALE'),
        ('FEMALE', 'UI_FEMALE'), ('TILES', 'UI_TILES'), ('UNIT_RAW', 'OPENING_RAW'),
        ('UNIT_NAMES', 'OPENING_NAMES')]
# Small tables stored in the descriptor: field, header array, element type
TABLES = [('RSM_PRE', 'rsm_pre', 'B'), ('BAG_INIT', 'bag_init', 'B'), ('GEN_BASE', 'gen_base', 'B'),
          ('GEN_VAR', 'gen_var', 'B'), ('ZODIAC_DAYS', 'zodiac_days', 'H'),
          ('ZODIAC_COMPAT', 'zodiac_compat', 'B'), ('UNIT_ZODIAC', 'opening_zodiac', 'B'),
          ('WEAPON_RANGE', 'opening_range', 'B'), ('SHIELD_EVADE', 'opening_evade', 'B'),
          ('CHARGE_TICKS', 'opening_charge_ticks', 'B'), ('JOB_ID', 'job_ids', 'B'),
          ('SET_POS', 'set_pos', 'B'), ('BAG_ITEM', 'bag_items', 'B'), ('RSM_POS', 'rsm_pos', 'B')]
# WATER: the map's water glints per view (fft.c make_glints): Orbonne's from its ffoi,
# Gariland's computed by the engine itself (campaign/water_dump.c).
# MAP_SCRIPT: the beats played on the battle map (fft.c mapbeats.inc): u16 count, then per
# sequence its stage (st.open_stage), the beats' event, MS_* flags (1 after the victory,
# 2 fades in, 4 actors placed by the beats, 8 camera set), rot, s16 camx, camy
MS_AFTER, MS_FADE, MS_PLACED, MS_CAMERA = 1, 2, 4, 8
SCRIPT = {'opening': [(3, 4, MS_FADE, 0, 0, 0),                     # the gate, from the scene
                      (6, 5, 0, 0, 0, 0),                           # player's fade; the banter
                      (4, 6, MS_AFTER | MS_FADE | MS_PLACED | MS_CAMERA, 0, 108, 68)]}   # aftermath
# Per chapter (campaign/ENGINE.md M4): Ramza's unit, a unit's frame cost (k cycles: Orbonne's
# dry shader, Gariland's water check); per sprite (units.h UG_*) its portrait (0: the same
# index) and whether it is a Chemist's body (armed frames, Status)
RAMZA = {'opening': 0, 'campaign': 6}
UNIT_K = {'opening': 16, 'campaign': 20}
STATUS = {'opening': 0, 'campaign': 1}          # Status: 0 the text-only page, 1 the full one
START = {'opening': 1, 'campaign': 0}           # 1: a story start (opening.inc), 0: the battle at once
# The campaign save (campaign/save.h): PARTY 1, the player's units are the campaign's party
# (written at the victory; Orbonne's are not: the flashback starts a new party); WON, the story
# flag + 1 its victory sets (0 none). JOB_ID, SET_POS, BAG_ITEM, DEFEND_ID, EQUIP_CHANGE_ID: FFT's
# ids of the chapter's jobs, skill set entries (their learned bit), bag slots and supports;
# RSM_POS, EQUIP_CHANGE_POS: the learned bits of the reactions and supports a unit draws.
PARTY = {'opening': 0, 'campaign': 1}
WON = {'opening': 1, 'campaign': 2}
SAVE_IDS = ('DEFEND_ID', 'EQUIP_CHANGE_ID', 'EQUIP_CHANGE_POS')
FIELDS = [f for f, _ in NUMBERS] + [f for f, _ in IGFX] + [f for f, _, _ in TABLES] + \
    ['WATER', 'MAP_SCRIPT', 'RAMZA', 'UNIT_K', 'GFX_PORTRAIT', 'GFX_CHEMIST', 'STATUS', 'START',
     'PARTY', 'WON'] + list(SAVE_IDS)
NAMES = {'opening': 'ffod', 'campaign': 'ffgd'}


def unzx0(stream):
    with tempfile.TemporaryDirectory() as d:
        src = Path(d) / 'i'; dst = Path(d) / 'o'; src.write_bytes(stream)
        subprocess.run([HERE.parents[1] / 'tools/bin/dzx0', '-f', src, dst], check=True, capture_output=True)
        return dst.read_bytes()


def numbers(text, values):
    """Evaluate #define name <expr> for simple numeric expressions (NUNIT-sized tables)."""
    for name, expr in re.findall(r'#define (\w+) ([^\n/]+)', text):
        expr = expr.strip().rstrip('UL')
        try:
            values[name] = int(eval(expr.replace('UL', ''), {}, dict(values)))
        except Exception:
            pass
    return values


def table(text, name, values):
    m = re.search(r'\b' + name + r'((?:\[[^]]*\])+)(?:\s*__attribute__\(\(unused\)\))?\s*=\s*\{', text)
    if not m:
        return None
    depth, at = 0, m.end() - 1                 # the balanced initializer (several per line)
    for end in range(at, len(text)):
        depth += text[end] == '{'
        depth -= text[end] == '}'
        if not depth:
            break
    init = text[at:end + 1]
    dims = [int(eval(d, {}, dict(values))) for d in re.findall(r'\[([^]]*)\]', m[1])]
    count = 1
    for d in dims:
        count *= d

    def flat(s):
        """Nested braces, zero-padded per row (C initializer rules for these tables)."""
        s = s.strip()
        if not s.startswith('{'):
            return [int(s, 0)]
        inner, depth, parts, at = s[1:-1], 0, [], 0
        for i, c in enumerate(inner):
            depth += c == '{'
            depth -= c == '}'
            if c == ',' and depth == 0:
                parts.append(inner[at:i]); at = i + 1
        parts.append(inner[at:])
        return [x for p in parts if p.strip() for x in flat(p)]

    def shaped(s, dims):
        s = s.strip()
        if len(dims) == 1:
            row = flat(s)
            return row + [0] * (dims[0] - len(row))
        inner = s[1:-1]
        depth, parts, at = 0, [], 0
        for i, c in enumerate(inner):
            depth += c == '{'
            depth -= c == '}'
            if c == ',' and depth == 0:
                parts.append(inner[at:i]); at = i + 1
        parts.append(inner[at:])
        rows = [shaped(p, dims[1:]) for p in parts if p.strip()]
        size = 1
        for d in dims[1:]:
            size *= d
        out = [x for r in rows for x in r]
        return out + [0] * (dims[0] * size - len(out))

    out = shaped(init, dims)
    assert len(out) == count, (name, dims, len(out))
    return out


def build(chapter):
    folder = HERE / chapter / 'data'
    maps = folder / 'map.h'                    # Gariland's is the root map.h
    texts = [(folder / n).read_text() for n in ('battle.h', 'interface.h')]
    texts.append((maps if maps.exists() else HERE / 'map.h').read_text())
    actors = folder / ('units.h' if chapter == 'opening' else 'actors.h')
    texts.append(actors.read_text())
    values = {'SET_N': int(re.search(r'#define SET_N (\d+)', (HERE / 'fft.h').read_text())[1])}
    for t in texts:
        numbers(t, values)
    offsets = {}
    for t in texts:
        offsets.update({k: int(v) for k, v in re.findall(r'#define (\w+) \(\(const \w+ \*\)\(igfx \+ (\d+)UL\)\)', t)})
        offsets.update({k: int(v) for k, v in re.findall(r'#define ((?:UI|OPENING)_\w+) (\d+)UL', t)})
    head = b'FCD1' + bytes(2 * len(FIELDS))
    body = bytearray(head)
    fields = {}
    values['SC_TOP'] = SC_TOP[chapter]
    view = (folder / (VIEW[chapter] + '.bin')).read_bytes()
    if view[:2] == b'V2':                      # split banks (extract.pack_view): colours first
        values['SC_H'] = len(unzx0(view[4:4 + int.from_bytes(view[2:4], 'big')])) // (2 * 40)
    else:
        values['SC_H'] = len(unzx0(view)) // (6 * 40)
    for f, name in NUMBERS:
        fields[f] = values[name or f]
    for f, name in IGFX:
        fields[f] = offsets.get(name, 0)
    text = '\n'.join(texts)
    for f, name, typ in TABLES:
        rows = table(text, name, values)
        if rows is None:
            fields[f] = 0; continue
        if len(body) & 1:
            body.append(0)
        fields[f] = len(body)
        body += struct.pack('>%d%s' % (len(rows), typ), *rows)
    water = water_block(chapter, folder, offsets)
    if water:
        if len(body) & 1:
            body.append(0)
        fields['WATER'] = len(body); body += water
    else:
        fields['WATER'] = 0
    fields['MAP_SCRIPT'] = 0
    if chapter in SCRIPT:
        if len(body) & 1:
            body.append(0)
        fields['MAP_SCRIPT'] = len(body)
        body += struct.pack('>H', len(SCRIPT[chapter]))
        for seq in SCRIPT[chapter]:
            body += struct.pack('>4B2h', *seq)
    fields['RAMZA'], fields['UNIT_K'], fields['STATUS'] = RAMZA[chapter], UNIT_K[chapter], STATUS[chapter]
    fields['START'] = START[chapter]
    fields['PARTY'], fields['WON'] = PARTY[chapter], WON[chapter]
    for f in SAVE_IDS:
        fields[f] = values.get(f, 0)
    gfx = sprites((folder / 'units.h' if chapter == 'opening' else HERE / 'units.h').read_text())
    portrait = [g if g <= 1 else g - 1 for g in range(len(gfx))] if chapter == 'opening' else None
    for f, rows in (('GFX_PORTRAIT', portrait), ('GFX_CHEMIST', [int('CHEMIST' in g) for g in gfx])):
        fields[f] = 0
        if rows:                               # (opening: its portrait table has no Delita)
            if len(body) & 1:
                body.append(0)
            fields[f] = len(body); body += bytes(rows)
    struct.pack_into('>%dH' % len(FIELDS), body, 4, *(fields[f] for f in FIELDS))
    (folder / (NAMES[chapter] + '.bin')).write_bytes(bytes(body))
    print(NAMES[chapter], len(body), 'bytes')


def sprites(text):
    """units.h's UG_* names in order (UNIT_GFX_N ends them)."""
    m = re.search(r'enum\s*\{\s*(UG_RAMZA[^}]*)\}', text)
    names = [n.strip() for n in m[1].split(',')]
    return [n for n in names[:names.index('UNIT_GFX_N')]]


def water_block(chapter, folder, offsets):
    if chapter == 'opening':
        bank = (folder / 'ffoi.bin').read_bytes(); at = offsets['OPENING_WATER']
        end = 8
        for rot in range(4):
            p = struct.unpack_from('>H', bank, at + 2 * rot)[0]
            for _ in range(4):
                p += 2 + 2 * struct.unpack_from('>H', bank, at + p)[0]
            end = max(end, p)
        return bank[at:at + end]
    path = folder / 'water.bin'
    return path.read_bytes() if path.exists() else b''


def dump_water():
    """Gariland's glints, computed by the engine with the descriptor just written."""
    exe = HERE / 'campaign/x/water_dump'
    exe.parent.mkdir(exist_ok=True)
    root = HERE.parents[1]
    subprocess.run(['cc', '-std=gnu99', '-O1', '-w', '-o', exe, HERE / 'campaign/water_dump.c',
                    root / 'runtime/core/rt_core.c', root / 'runtime/platform-sw/rt_sw.c'], check=True)
    subprocess.run([exe], cwd=HERE / 'campaign/data', check=True)


def header():
    lines = ['// Generated by tools/chapter_desc.py: a chapter descriptor\'s layout (campaign/ENGINE.md).',
             '#define CD_MAGIC 0x46434431UL   // "FCD1"']
    lines += ['#define CD_%s %d' % (f, 2 + i) for i, f in enumerate(FIELDS)]   # u16 word index
    path = HERE / 'chapter.h'
    text = '\n'.join(lines) + '\n'
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def maxima():
    """chapters.h: the largest map and scene of every chapter's descriptor (fft.h: the
    battle engine's buffers, one program for all of them, campaign/ENGINE.md M4)."""
    index = {f: 2 + i for i, f in enumerate(FIELDS)}
    top = {'MAP_W': 0, 'MAP_H': 0, 'SC_H': 0}
    for chapter, name in NAMES.items():
        path = HERE / chapter / 'data' / (name + '.bin')
        if path.exists():
            desc = path.read_bytes()
            for f in top:
                top[f] = max(top[f], struct.unpack_from('>H', desc, 2 * index[f])[0])
    text = ('// Generated by tools/chapter_desc.py: the largest map and scene of the chapters.\n'
            '#define MAP_WMAX %d\n#define MAP_HMAX %d\n#define SC_HMAX %d\n' % (top['MAP_W'], top['MAP_H'], top['SC_H']))
    path = HERE / 'chapters.h'
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


if __name__ == '__main__':
    header()
    for chapter in sys.argv[1:]:
        build(chapter)
        if chapter == 'campaign':
            dump_water(); build(chapter)
    maxima()
