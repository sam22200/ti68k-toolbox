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
import sys

HERE = Path(__file__).resolve().parents[1]
# Numbers: header #define name (None: computed here)
NUMBERS = [('NJOB', 'NJOB'), ('NITEM', 'NITEM'), ('NABIL', 'NABIL'), ('NSET', 'NSET'),
           ('SET_BASIC', 'SET_BASIC'), ('NRSM_PRE', 'NRSM_PRE'), ('DEFEND_JP', 'DEFEND_JP'),
           ('DEFEND_RATE', 'DEFEND_RATE'), ('NBAG', 'NBAG'), ('MAP_H', 'MAP_H'),
           ('ACTOR_ARMED', 'ACTOR_ARMED'), ('ACTOR_OBJECTS', 'ACTOR_OBJECTS')]
# Offsets into the chapter's interface bank (igfx): header names
IGFX = [('JOBS', 'jobs'), ('ITEMS', 'items'), ('ABILITIES', 'abilities'), ('SKILL_SETS', 'skill_sets'),
        ('UNIT_DEFS', 'unit_defs'), ('PORTRAITS', 'UI_PORTRAITS'), ('MALE', 'UI_MALE'),
        ('FEMALE', 'UI_FEMALE'), ('TILES', 'UI_TILES'), ('UNIT_RAW', 'OPENING_RAW'),
        ('UNIT_NAMES', 'OPENING_NAMES'), ('WATER', 'OPENING_WATER')]
# Small tables stored in the descriptor: field, header array, element type
TABLES = [('RSM_PRE', 'rsm_pre', 'B'), ('BAG_INIT', 'bag_init', 'B'), ('GEN_BASE', 'gen_base', 'B'),
          ('GEN_VAR', 'gen_var', 'B'), ('ZODIAC_DAYS', 'zodiac_days', 'H'),
          ('ZODIAC_COMPAT', 'zodiac_compat', 'B'), ('UNIT_ZODIAC', 'opening_zodiac', 'B'),
          ('WEAPON_RANGE', 'opening_range', 'B'), ('SHIELD_EVADE', 'opening_evade', 'B'),
          ('CHARGE_TICKS', 'opening_charge_ticks', 'B')]
FIELDS = [f for f, _ in NUMBERS] + [f for f, _ in IGFX] + [f for f, _, _ in TABLES]
NAMES = {'opening': 'ffod', 'campaign': 'ffgd'}


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
    m = re.search(r'\b' + name + r'((?:\[[^]]*\])+)\s*=\s*\{', text)
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
    values = {}
    for t in texts:
        numbers(t, values)
    offsets = {}
    for t in texts:
        offsets.update({k: int(v) for k, v in re.findall(r'#define (\w+) \(\(const \w+ \*\)\(igfx \+ (\d+)UL\)\)', t)})
        offsets.update({k: int(v) for k, v in re.findall(r'#define ((?:UI|OPENING)_\w+) (\d+)UL', t)})
    head = b'FCD1' + bytes(2 * len(FIELDS))
    body = bytearray(head)
    fields = {}
    for f, name in NUMBERS:
        fields[f] = values[name]
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
    struct.pack_into('>%dH' % len(FIELDS), body, 4, *(fields[f] for f in FIELDS))
    (folder / (NAMES[chapter] + '.bin')).write_bytes(bytes(body))
    print(NAMES[chapter], len(body), 'bytes')


def header():
    lines = ['// Generated by tools/chapter_desc.py: a chapter descriptor\'s layout (campaign/ENGINE.md).',
             '#define CD_MAGIC 0x46434431UL   // "FCD1"']
    lines += ['#define CD_%s %d' % (f, 2 + i) for i, f in enumerate(FIELDS)]   # u16 word index
    path = HERE / 'chapter.h'
    text = '\n'.join(lines) + '\n'
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


if __name__ == '__main__':
    header()
    for chapter in sys.argv[1:]:
        build(chapter)
