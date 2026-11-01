#!/usr/bin/env python3
"""The jobs and items every chapter shares (campaign/ENGINE.md core milestone) -> ffcj.bin
(never committed): fft.h's Job and Item records indexed by FFT's own ids, so a battle can field
any job and equipment the campaign save holds (a unit's job and equipment are FFT ids).

usage: common_tables.py DISC.cue      (writes opening/data and campaign/data/ffcj.bin)
Layout: "FCJA", 14 u16 section offsets and the payload length: jobs (Job, 14 bytes, 160 internal job/actor records, not
160 playable careers: the management menus expose the 20 generic careers; growth and multiplier per raw
stat, Move, Jump, class evade, career row), job names (14 bytes each), each job's skill set (FFT
id, a byte), items (Item, 30 bytes,
256: id, HP, MP, PA, MA, Speed, Move, Jump, WP, knife (1 a knife/ninja blade, 2 a bow: XA (PA + Speed) / 2),
name, reach (the weapon's range), shield evade, packed magic evade/shield tag and ordinary dual-weapon flag). Read from SCUS_942.21 as tools/battle.py
reads them; the names from BATTLE.BIN. Then all 512 primary ability records (JP big-endian,
other bytes verbatim), 368 default range/formula records, 14 item formula records, special
ability tables, 176 human and 48 monster packed skill sets, ability/set names, 128 status
records, and 512 normalized battle records with Charge timing/labels. See common.h and
campaign/ABILITIES.md. Retail data is never committed.
"""
from pathlib import Path
import struct
import sys

import battle
import management_data

HERE = Path(__file__).resolve().parents[1]
JOBS, ITEMS = 0xA0, 256
SHIELD_EVADE = 0x80063eb8                       # 2 bytes a shield (ids 0x80..0x8F): physical first
ACCESSORY_EVADE = 0x80063f58                    # physical, magical; secondary_data_id
KNIFE, BOW = 1, 12                              # item types
SPECIAL, HUMAN, MONSTER = 0x80061010, 0x80064a94, 0x80065bc4
ACTION_MENUS = 0x80065CB4  #224 source skillset dispatch kinds, appended to SPECIAL
STATUS_EFFECTS = 0x80065DE4


def item_magic(d, i):
    """Source item type + secondary record; six bits suffice (retail max50)."""
    r = d.at(battle.ITEMS + 12*i, 12)
    shield = bool(r[3] & 0x40)
    base = SHIELD_EVADE if shield else ACCESSORY_EVADE if r[3] & 8 else None
    evade = d.at(base + 2*r[4] + 1, 1)[0] if base else 0
    assert evade <= 63, (i, evade)
    return (int(shield) << 7) | (evade << 1)


def affinity_bytes(d):
    """Stable source job/item IDs; deduplicate only identical five-byte masks."""
    jobs = [d.at(battle.JOBS + 48*j + 0x29, 4) + b'\0' for j in range(JOBS)]
    items = []
    for i in range(ITEMS):
        attribute = d.at(battle.ITEMS + 12*i + 7, 1)[0]
        items.append(d.at(battle.ATTRS + 25*attribute + 20, 5))
    rows = sorted(set([bytes(5), *jobs, *items]))
    assert rows[0] == bytes(5) and len(rows) < 256
    ids = {row: i for i, row in enumerate(rows)}
    return b'FEA1' + bytes((len(rows), 0)) + bytes(ids[r] for r in jobs+items) + b''.join(rows)


def status_policy_bytes(d):
    rows=d.at(STATUS_EFFECTS,40*16)
    formation=bytearray(5)
    for sid in range(40):
        if rows[sid*16+5]&2:formation[sid>>3]|=0x80>>(sid&7)
    rider=d.bat[0x8018F60C-battle.BAT:0x8018F60C-battle.BAT+5]
    return rows+formation+rider


def offensive_magic_record(r):
    """One family gate, not a spell dispatcher. Shared ordinary formula08.

    The existing radial formula08 pipeline supports ordinary HP damage, elemental
    affinity and source team/self filters. Do not erase unsupported secondary
    behavior while normalizing a record. Vertical limits on the centre use
    half-height units; the footprint retains its independent raw tolerance.
    """
    if (r[8] != 8 or r[9] or r[11] or
            r[3] & ~0x09 or r[4] & ~0xe1 or r[5] & ~0xe2 or
            r[6] & ~0x42):
        return None
    flags = 128 | (32 if r[1] else 0) | (0 if r[3] & 1 else 1) | (2 if r[6] & 2 else 0)
    return 0, r[10], r[0], 2*r[2] if r[3] & 8 else 255, flags


def healing_magic_record(r):
    """The six heals and two Raises share the owned radial pipeline (radius0 too)."""
    if (r[8] not in (12,13) or r[3]&~0x09 or r[4]&~0xe1 or
            r[5]&~0xe2 or r[6]&~0x40 or r[7] or
            (r[8]==12 and (r[9] or r[11])) or (r[8]==13 and r[11]!=32)):
        return None
    return 3 if r[8]==12 else 4,r[10],r[0],2*r[2] if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)


def status_magic_record(d,r):
    """Whole all-or-nothing family, one filtered status byte, implemented policy only."""
    if r[8]not in(10,11)or r[3]&~9 or r[4]&~0xe1 or r[5]&~0xe2 or r[6]&~0x42 or r[7]:return None
    row=d.at(battle.STATUS_SET+6*r[11],6)
    # Confusion, Berserk and Frog: control/command handlers (CORE_CONTROL_STATUS.md);
    # Reraise: KO-turn revival (CORE_PROTECT.md)
    supported=bytes((18,168|16,32|8|2,254,222))
    if row[0]==16 and 0<sum(v.bit_count()for v in row[1:])<=16:
        return 13,r[11],r[0],2*r[2]if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)|(2 if r[6]&2 else 0)
    if row[0]!=128 or sum(bool(v)for v in row[1:])!=1:return None
    if any(v&~s for v,s in zip(row[1:],supported)):return None
    return 10,r[11],r[0],2*r[2]if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)|(2 if r[6]&2 else 0)


def decisive_magic_record(r):
    """Death and Quick share ordinary owned casts; status producers use the mask gate."""
    if r[8]not in(14,18)or r[3]&~9 or r[4]&~0xe1 or r[5]&~0xe2 or r[6]&~0x42 or r[7]:return None
    if r[8]==14 and (r[10]!=100 or r[11]!=41):return None
    if r[8]==18 and (r[10]or r[11]):return None
    return 16 if r[8]==14 else 17,r[11]if r[8]==14 else 0,r[0],2*r[2]if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)|(2 if r[6]&2 else 0)


def brave_magic_record(r):
    """Foxbird: formula 0x0a's evasion/accuracy with a Brave loss Y (AB_BRAVE)."""
    if r[8]!=0x61 or r[3]&~9 or r[4]&~0xe1 or r[5]&~0xe2 or r[6]&~0x42 or r[7] or r[11] or not r[10]:return None
    return 18,r[10],r[0],2*r[2]if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)|(2 if r[6]&2 else 0)


def golem_magic_record(r):
    """Golem: formula 0x14 on the caster, its side's guard pool (AB_GOLEM)."""
    if r[8]!=0x14 or r[0] or r[1] or r[7] or r[10] or r[11] or r[3]&~2 or r[6]&~1:return None
    return 19,0,0,255,128|32|1


def punch_record(d,r):
    """Punch Art support: Stigma Magic (0x33 cancel set), Chakra (0x34 HP and MP), Revive (0x35)."""
    if r[8]not in(0x33,0x34,0x35)or r[3]&~0x0b or r[4]&~0x11 or r[5]or r[6]&~1 or r[7]:return None
    row=d.at(battle.STATUS_SET+6*r[11],6)
    if r[8]==0x33:
        if row[0]!=16 or not any(row[1:]):return None
        kind,y=13,r[11]
    elif r[8]==0x34:
        if r[11]or not r[10]:return None
        kind,y=14,r[10]
    else:
        if row!=bytes((16,32,0,0,0,0)):return None
        kind,y=4,r[10]
    return kind,y,r[0],2*r[2]if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)


def strike_record(r):
    """Punch Art strikes: Wave Fist (0x31) and Repeating Fist (0x32) single-target physical
    skills; Spin Fist (0x31, radius 1 on the user, the user excluded) an immediate area cast."""
    if r[8]==0x31 and r[0]==0 and r[1]==1 and r[3]==3 and r[4]==0x11 and r[6]==3 and not r[7] and not r[11]:
        return 0,r[10],0,255,128|32|2|1
    if r[8]not in(0x31,0x32)or r[1]or r[3]!=9 or r[4]&~0x11 or r[6]!=2 or r[7]or r[11]:return None
    return 0,r[10],r[0],2*r[2],2


def talk_record(d,a,r):
    """Talk Skill 0x2a by source ability id: Persuade (CT 0), Praise/Threaten (Brave),
    Preach/Solution (Faith), Insult/Mimic Daravon (status). Y is a signed loss."""
    if r[8]!=0x2a or r[3]!=8 or r[4]!=0x10 or r[5]!=0x20 or r[6] or r[7] or r[1]>1:return None
    kind={117:17,118:18,119:18,120:20,121:20,122:10,124:10,125:10}.get(a)
    if kind is None:return None
    if kind==10:
        row=d.at(battle.STATUS_SET+6*r[11],6)
        if row[0]!=128 or sum(bool(v)for v in row[1:])!=1:return None
        y=r[11]
    else:y=0 if kind==17 else (256-r[10])&255 if a in(118,120) else r[10]
    return kind,y,r[0],2*r[2],128|32|1


def guts_record(r):
    """Guts 0x39 Yell (Speed), 0x3a Cheer Up (Brave), 0x3b Scream (Brave X, PA/MA/Speed Y):
    immediate casts that always hit. Y is a signed loss like Praise; Scream's stat Y is in element."""
    if r[8]not in(0x39,0x3a,0x3b)or r[1]or r[4]!=0x10 or r[5]!=0x10 or r[6]or r[7]or r[11]or r[3]&~2:return None
    if r[8]==0x3b:return 18,(256-r[9])&255,r[0],255,128|32|1,r[10]
    return (21 if r[8]==0x39 else 18),(256-r[10])&255,r[0],255,128|32|1,0


def percent_magic_record(r):
    """Retail percent HP and single-panel HP/MP drains, with separate caster packets."""
    if r[8]not in(9,15,16)or r[9]==0 or r[11]or r[3]&~9 or r[4]&~0xe1 or r[5]&~0xe2 or r[6]&~0x42:return None
    if r[8]!=9 and (r[1]or r[7]or not(r[3]&1)or not(r[4]&1)or r[5]&128):return None
    return 11 if r[8]==15 else 0,r[10],r[0],2*r[2]if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)|(2 if r[6]&2 else 0)


def battle_records(d):
    """Shared native execution records. Unverified families remain explicitly disabled.

    Existing Orbonne adaptations (weapon skills, stat breaks) remain deliberate;
    their actual formula ids are retained for the module's dispatch.
    """
    # kind, Y, range, vertical, flags (fft.h); byte 10 is FFT's formula id.
    adapted = {
        0x92: (6, 1, 0, 0, 1), 0x93: (0, 4, 1, 2, 0), 0x94: (0, 2, 4, 255, 2),
        0x98: (2, 0, 1, 6, 0), 0x9B: (0, 2, 2, 255, 8 | 32),
        0xA5: (0, 0, 3, 255, 8 | 16),
        0x8F: (7, 2, 1, 6, 2), 0x8E: (7, 3, 1, 6, 2),
    }
    symbols = dict(AB_DAMAGE=0, AB_CURE=1, AB_WISH=2, AB_HEAL=3, AB_REVIVE=4,
                   ST_DEFEND=1, ST_POISON=2, ST_DARK=4, ST_SILENCE=8,
                   AF_SELF=1, AF_EVADE=2, AF_ITEM=4)
    out = bytearray()
    for a in range(512):
        primary = d.at(battle.ABILITY + 8 * a, 8)
        jp = int.from_bytes(primary[:2], 'little')
        rate = primary[2]
        kind, y, rng, vert, flags, bag, level, formula, element = 255, 0, 0, 255, 0, 255, 0, 0, 0
        # Generic magic skill sets share one tested family. Monster/special
        # producers outside this source table interval remain independently gated.
        raw = d.at(battle.ABILITY_RANGE + 14*a, 14) if 0<a<0x50 else None
        # Punch Art (0x64..0x6b) and Talk Skill (0x74..0x7d) are read beyond the magic interval.
        art = d.at(battle.ABILITY_RANGE + 14*a, 14) if 0x64<=a<=0x6b else None
        magic = offensive_magic_record(raw) if raw else None
        healing = healing_magic_record(raw) if raw else None
        status = status_magic_record(d,raw) if raw else None
        percent = percent_magic_record(raw) if raw else None
        decisive = decisive_magic_record(raw) if raw else None
        brave = brave_magic_record(raw) if raw else None
        golem = golem_magic_record(raw) if raw else None
        punch = punch_record(d,art) if art else None
        strike = strike_record(art) if art else None
        speech = d.at(battle.ABILITY_RANGE + 14*a, 14) if 0x74<=a<=0x7d else None
        talk = talk_record(d,a,speech) if speech else None
        # Guts (0x96..0x99) is read beyond the magic interval too.
        guts = guts_record(d.at(battle.ABILITY_RANGE + 14*a, 14)) if 0x96<=a<=0x99 else None
        if a in adapted:
            kind, y, rng, vert, flags = adapted[a]
            formula = d.at(battle.ABILITY_RANGE + 14 * a, 14)[8]
        elif 0x170<=a<=0x175 or a==0x17d:
            form,z,status_set=d.at(battle.ITEM_ABILITY+3*(a-0x170),3)
            assert form in (0x48,0x49,0x4a,0x4b)
            kind={0x48:3,0x49:12,0x4a:14,0x4b:4}[form]
            y,rng,vert,flags,bag,formula=z,4,255,5,a-0x170,form
            level=d.item(a-0x170+0xf0)['level']
        elif 0x179<=a<=0x17c:
            form,z,status_set=d.at(battle.ITEM_ABILITY+3*(a-0x170),3)
            row=d.at(battle.STATUS_SET+6*status_set,6)
            assert form==0x38 and row[0]==16 and 0<sum(v.bit_count()for v in row[1:])<=16
            kind,y,rng,vert,flags,bag,formula=13,status_set,4,255,5,a-0x170,form
            level=d.item(a-0x170+0xf0)['level']
        elif a in (0x95, 0x176, 0x177, 0x178):
            rec = d.ability(a)
            kind = symbols[rec['kind']]
            y = rec['y'] if isinstance(rec['y'], int) else sum(symbols[t.strip()] for t in rec['y'].split('|'))
            rng, vert = rec['range'], rec['vert']
            flags = sum(symbols[t] for t in rec['flags'])
            level = rec['lv']
            if a >= 0x170:
                bag = a - 0x170
                formula = d.at(battle.ITEM_ABILITY + bag * 3, 3)[0]
            else:
                formula = d.at(battle.ABILITY_RANGE + a * 14, 14)[8]
        elif 0x196 <= a < 0x19E:
            # CT and power are separate source fields: +7/+10/+20 are not
            # consecutive powers, and power must never index the CT table.
            y = d.at(0x80061044 + 2 * (a - 0x196), 2)[1]
            kind, rng, flags, formula = 0, 5, 8 | 2 | 64, 1
        elif magic:
            kind, y, rng, vert, flags = magic
            formula, element = raw[8], raw[7]
        elif healing:
            if raw[8]==13:
                assert d.at(battle.STATUS_SET+6*raw[11],6)==bytes((16,32,0,0,0,0))
            kind,y,rng,vert,flags=healing;formula=raw[8]
        elif decisive:
            kind,y,rng,vert,flags=decisive;formula=raw[8]
        elif brave:
            kind,y,rng,vert,flags=brave;formula=raw[8]
        elif golem:
            kind,y,rng,vert,flags=golem;formula=raw[8]
        elif punch:
            kind,y,rng,vert,flags=punch;formula=art[8]
        elif strike:
            kind,y,rng,vert,flags=strike;formula=art[8]
        elif guts:
            kind,y,rng,vert,flags,element=guts;formula=d.at(battle.ABILITY_RANGE + 14*a, 14)[8]
        elif talk:
            kind,y,rng,vert,flags=talk;formula=speech[8]
        elif percent:
            kind,y,rng,vert,flags=percent;formula,element=raw[8],raw[7]
        elif status:
            kind,y,rng,vert,flags=status;formula=raw[8]
        out.extend(struct.pack('>8BH2B', kind, y, rng, vert, flags, bag, rate, level, jp, formula, element))
    assert len(out) == 512 * 12
    out.extend(d.at(0x80061044, 16)[::2])        # Charge's eight CT counts
    out.extend(b'Charge +1\0Charge +2\0')         # reviewed battle label spacing
    return bytes(out)


def build(d):
    job_names = management_data.names(d.bat, management_data.JOB_NAMES, JOBS)
    item_names = management_data.names(d.bat, management_data.ITEM_NAMES, ITEMS)
    body = bytearray(b'FCJA' + bytes(30))
    offs = [len(body)]
    for j in range(JOBS):
        e = d.job(j)
        row = j - 0x4A if 0x4A <= j < 0x4A + 20 else 0
        body += bytes(e['growth'] + e['mult'] + [e['move'], e['jump'], e['cev'], row])
    offs.append(len(body))
    for j in range(JOBS):
        body += job_names[j][:13].encode('ascii').ljust(14, b'\0')
    offs.append(len(body))
    body += bytes(d.job(j)['set'] for j in range(JOBS))
    offs.append(len(body))
    for i in range(ITEMS):
        it = d.item(i)
        w = d.at(battle.WEAPONS + 8 * i, 8) if it['flags'] & 0x80 else bytes(8)
        knife = 1 if it['type'] in (KNIFE, 2) else 2 if it['type'] == BOW else 0
        sev = d.at(SHIELD_EVADE + 2 * (i - 0x80), 1)[0] if 0x80 <= i < 0x90 else 0
        rec = bytes([i, it['hp'], it['mp'], it['pa'], it['ma'], it['sp'], it['move'], it['jump'], w[4], knife])
        assert len(item_names[i]) <= 16, (i, item_names[i])
        # Verified ordinary melee subset; elemental/proc/special formulas stay gated.
        dual = bool(it['flags'] & 0x80 and w[1] & 8 and w[2] == 1
                    and it['type'] in (1, 2, 3) and not w[6] and not w[7])
        rec += item_names[i].encode('ascii').ljust(17, b'\0') + bytes([w[0], sev, int(dual) | item_magic(d, i)])
        assert len(rec) == 30
        body += rec
    def section(data):
        if len(body) & 1: body.append(0)
        offs.append(len(body)); body.extend(data)

    primary = bytearray(d.at(battle.ABILITY, 512 * 8))
    for at in range(0, len(primary), 8):
        primary[at:at + 2] = primary[at:at + 2][::-1]
    section(primary)
    section(d.at(battle.ABILITY_RANGE, 0x170 * 14))
    section(d.at(battle.ITEM_ABILITY, 14 * 3))
    innate = bytearray(20)
    for job in range(JOBS):
        if 0x1dd in d.job(job)['innate']:innate[job>>3] |= 1 << (job&7)
    section(d.at(SPECIAL, 168) + d.at(ACTION_MENUS, 224) + innate + affinity_bytes(d))
    section(d.at(HUMAN, 0xB0 * 25))
    section(d.at(MONSTER, 48 * 5))

    def name_section(names):
        if len(body) & 1: body.append(0)
        base = len(body); offs.append(base); body.extend(bytes(2 * len(names)))
        where = {}
        for k, name in enumerate(names):
            if not name.strip(): continue
            if name not in where:
                where[name] = len(body); body.extend(name.encode('ascii') + b'\0')
            struct.pack_into('>H', body, base + 2 * k, where[name])

    name_section(management_data.names(d.bat, management_data.ABILITY_NAMES, 512))
    name_section(management_data.names(d.bat, management_data.SET_NAMES, 64))
    section(d.at(battle.STATUS_SET, 128 * 6)+status_policy_bytes(d))
    section(battle_records(d))
    assert len(offs) == 14 and len(body) < 65000
    struct.pack_into('>15H', body, 4, *offs, len(body))
    return bytes(body)


def main():
    body = build(battle.Disc(sys.argv[1]))
    for chapter in ('opening', 'campaign'):
        folder = HERE / chapter / 'data'
        folder.mkdir(parents=True, exist_ok=True)
        (folder / 'ffcj.bin').write_bytes(body)
    print('ffcj: %d bytes (%d internal job/actor records, 20 generic careers, %d items, '
          '512 abilities, 224 skill sets)' % (len(body), JOBS, ITEMS))


if __name__ == '__main__':
    main()
