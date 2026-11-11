#!/usr/bin/env python3
"""The jobs and items every chapter shares (campaign/ENGINE.md core milestone) -> ffcj.bin
(never committed): fft.h's Job and Item records indexed by FFT's own ids, so a battle can field
any job and equipment the campaign save holds (a unit's job and equipment are FFT ids).

usage: common_tables.py DISC.cue      (writes opening/data and campaign/data/ffcj.bin)
Layout: "FCJI", 14 u16 section offsets and the payload length: jobs (Job, 14 bytes, 160 internal job/actor records, not
160 playable careers: the management menus expose the 20 generic careers; growth and multiplier per raw
stat, Move, Jump, class evade, career row), job names (14 bytes each), each job's skill set (FFT
id, a byte), items (Item, 30 bytes,
256: id, HP, MP, PA, MA, Speed, Move, Jump, WP, knife (1 a knife/ninja blade, 2 a bow: XA (PA + Speed) / 2),
name, reach (the weapon's range), shield evade, packed magic evade/shield tag and ordinary dual-weapon flag). Read from SCUS_942.21 as tools/battle.py
reads them; the names from BATTLE.BIN. Then all 512 primary ability records (JP big-endian,
other bytes verbatim), 368 default range/formula records, 14 item formula records, special
ability tables, 176 human and 48 monster packed skill sets, ability/set names, 128 status
records, and 640 normalized battle records (512 source rows plus128 item slots) with Charge timing/labels. See common.h and
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


def truth_magic_record(r):
    """Formula 0x4E (MA x Y, formula 0x08 without Faith: magic_amount power bit 7),
    single-target monster shapes only: no area, no line, no status. flags_4 may carry the
    evadeable (0x02) and direct-targeting (0x20) bits; flags_2 the no-follow-target bit."""
    if (r[8] != 0x4E or r[9] or r[11] or r[1] or r[3] & ~0x09 or r[4] & ~0x11 or
            r[5] != 0x10 or r[6] & ~0x22):
        return None
    return 0, r[10], r[0], 2*r[2] if r[3] & 8 else 255, 128 | (0 if r[3] & 1 else 1) | (2 if r[6] & 2 else 0)


def holy_sword_record(d,r):
    """Formula 0x2D (Holy Sword: Stasis Sword, Split/Crush Punch, Lightning Stab, Holy Explosion):
    a physical strike that is never evaded (flags_4 lacks 0x02) with PA x (WP + Y) in the weapon's element (the record's element
    byte is not used) and the record's one status at 100% on a hit (status type 0x20); a sword
    is required (flags_4 0x08). Single target, radius 1, or Holy Explosion's cardinal line
    (flags_1 0x15, flags_2 0x15: the Earth Slash line, caster excluded)."""
    line=r[3]==0x15 and r[4]==0x15
    if r[8]!=0x2D or r[9] or r[5]!=0x12 or r[6]!=0x08 or not line and (r[4]!=0x11 or r[3]&~0x0c or r[1]>1):
        return None
    row=d.at(battle.STATUS_SET+6*r[11],6)
    if row[0]!=0x20 or sum(v.bit_count() for v in row[1:])!=1:
        return None
    return 0, r[10], r[0], 2*r[2] if r[3]&8 else 255, 128|8|(32 if r[1] else 0)|(0 if line else 1)


def ruin_record(a,r):
    """Ruins (Arc Knight): Magic Ruin 0x1B (never evaded: flags_4 lacks 0x02; (MA+X)% Faith accuracy, MP loses
    ceil(max MP x Y / 100)), Speed/Power/Mind Ruin 0x1A ((MA+Y)% Faith
    chance, the stat loses X) on radius 1. A sword is required (flags_4 0x08)."""
    expected={0xC4:(0x1B,0xFA,0x32),0xC5:(0x1A,3,0xFA),0xC6:(0x1A,5,0xFA),0xC7:(0x1A,5,0xFA)}.get(a)
    if not expected or tuple(r)!=(4,1,0,5,1,0x12,8,0,*expected,0,3,0x14):return None
    if a==0xC4:return 11, r[10], r[0], 255, 128|32
    return (21 if a==0xC5 else 7), r[9], r[0], 255, 128|32


def mighty_sword_record(r):
    """Mighty Sword 0x2E (battle_formula_break_equipped_damage_pa_times_wp): Shellbust Stab,
    Blastar Punch, Hellcry Punch, Icewolf Bite. Never evaded (flags_4 lacks 0x02); the slot (armour, helmet,
    weapon, accessory: a monster or an empty slot is a forced failure), Maintenance, then Holy
    Sword's PA x WP strike (weapon element, critical) and the slot broken on a hit."""
    if tuple(r)!=(3,0,0,5,0x11,0x12,8,0,0x2E,0,0,0,0,0):return None
    return 0, 0, r[0], 255, 128|8


def dark_sword_record(r):
    """Dark Sword 0x2F (battle_formula_absorb_mp_pa_times_wp): never evaded, PA x WP of the
    equipped sword through Attack's physical stages and critical, no element stage, the amount
    taken from the target's MP and given to the caster (the single-panel drain packets).
    A sword is required (flags_4 0x08)."""
    if tuple(r)!=(3,0,0,5,0x11,0x12,8,1,0x2F,0,0,0,0,0):return None
    return 11, 0, r[0], 255, 128|32|8


def healing_magic_record(r):
    """The six heals and two Raises share the owned radial pipeline (radius0 too)."""
    if (r[8] not in (12,13) or r[3]&~0x09 or r[4]&~0xe1 or
            r[5]&~0xe2 or r[6]&~0x40 or r[7] or
            (r[8]==12 and (r[9] or r[11])) or (r[8]==13 and r[11]!=32)):
        return None
    return 3 if r[8]==12 else 4,r[10],r[0],2*r[2] if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)


def float_record(d,r):
    """Retail Float only: existing formula0b plus the implemented Float policy."""
    if r!=bytes.fromhex('04 01 01 00 00 e2 00 00 0b 8c 00 2e 02 08'):return None
    if d.at(battle.STATUS_SET+6*46,6)!=bytes((128,0,0,64,0,0)):return None
    return 10,46,4,255,161


def status_magic_record(d,r,mimic=0):
    """Whole all-or-nothing family, one filtered status byte, implemented policy only.
    mimic=0x10 also accepts flags3's Mime bit (Holy Magic: mime_runtime.inc reads it)."""
    if r[8]not in(10,11)or r[3]&~9 or r[4]&~0xe1 or r[5]&~(0xe2|mimic) or r[6]&~0x42 or r[7]:return None
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
    # Turn Punch (Black Goblin): Spin Fist with area vertical 1 and flags_4 0x01: never evaded.
    if r[8]==0x31 and r[0]==0 and r[1]==1 and r[2]==1 and r[3]==3 and r[4]==0x11 and r[6]==1 and not r[7] and not r[10] and not r[11]:
        return 0,0,0,255,128|32|1
    if r[8]not in(0x31,0x32)or r[1]or r[3]not in(1,9)or r[4]&~0x11 or r[6]!=2 or r[7]or r[11]:return None
    return 0,r[10],r[0],2*r[2]if r[3]&8 else 255,2   # flags_1 0x08 off (Choco Ball): any height


def draw_out_record(d,a,r):
    """All ten retail Draw Out producers; inventory item is ability id - 0x26."""
    if not 76<=a<=85:return None
    rows=((0,2,3,2,64,2,1,0,32,0,8,0,0,0),
          (0,2,3,2,64,2,1,0,32,0,12,0,0,0),
          (0,2,3,2,64,2,1,0,33,0,4,0,0,0),
          (0,2,3,2,128,2,1,0,35,0,12,0,0,0),
          (0,2,3,2,64,2,1,0,32,0,14,43,0,0),
          (0,2,3,2,128,2,1,0,34,0,0,37,0,0),
          (0,2,3,2,64,2,1,0,32,0,18,77,0,0),
          (8,8,3,17,85,2,0,0,32,0,16,0,0,0),
          (0,2,3,2,128,2,1,0,34,0,0,78,0,0),
          (0,2,3,2,64,2,1,0,32,0,30,0,0,0))
    if tuple(r)!=rows[a-76]:return None
    status={0:(0,0,0,0,0,0),43:(128,0,0,0,4,0),37:(128,0,0,0,48,0),
            77:(64,0,16,0,0,1),78:(128,0,0,0,72,0)}
    if tuple(d.at(battle.STATUS_SET+6*r[11],6))!=status[r[11]]:return None
    kind={32:0,33:11,34:10,35:3}[r[8]]
    return kind,r[11]if kind==10 else r[10],r[0],255,160|(0 if r[3]&1 else 1)


def performance_record(d,a,r):
    """All fourteen retail Songs/Dances; persistent, global and team filtered."""
    if not 86<=a<=99:return None
    k=a-86;f=28 if k<7 else 29;j=k%7
    x=100 if j<2 else 34 if a==99 else 50
    y=20 if j==0 else 10 if j==1 else 0
    status=59 if a==91 else 60 if a==98 else 0
    ticks=(6,6,8,8,10,10,20)[j]
    if tuple(r)!=(0,255,255,2,128 if k<7 else 64,4,1,0,f,x,y,status,ticks,0):return None
    sets={0:(0,0,0,0,0,0),59:(64,0,0,32,120,0),60:(64,0,56,2,134,16)}
    if tuple(d.at(battle.STATUS_SET+6*status,6))!=sets[status]:return None
    kind=(12,3,21,7,7,10,17)[j]if k<7 else(11,0,21,7,7,10,17)[j]
    value=status if j==5 else 255 if k<7 and 2<=j<=4 else 1 if 2<=j<=4 else y
    return kind,value,0,255,161


def allegiance_record(d,a,r):
    """Steal Heart and Invitation: owned status effects with source-specific allegiance."""
    expected={109:bytes.fromhex('03 00 00 01 51 00 00 00 29 32 00 3e 00 00'),
              116:bytes.fromhex('03 00 03 09 50 20 00 00 2a 14 00 3f 00 00')}
    if r!=expected.get(a):return None
    if d.at(battle.STATUS_SET+6*r[11],6)!=(bytes.fromhex('80 00 00 00 00 20')if a==109 else bytes.fromhex('80 00 40 00 00 00')):return None
    return 10,r[11],r[0],2*r[2]if r[3]&8 else 255,160


def allegiance_immunity(d):
    """Compact only the two newly executable statuses from all160 jobs/256 items."""
    rows=[]
    for job in range(JOBS):rows.append(d.at(battle.JOBS+48*job+0x1f,5))
    for item in range(ITEMS):
        attr=d.at(battle.ITEMS+12*item+7,1)[0]
        rows.append(d.at(battle.ATTRS+25*attr+10,5))
    out=bytearray(416)
    for i,r in enumerate(rows):out[i]=int(bool(r[1]&64))|int(bool(r[4]&32))<<1
    return bytes(out)


def line_record(a,r):
    """Earth Slash: source cardinal selection, origin-based linear area, Earth."""
    if a!=103 or tuple(r)!=(8,8,2,17,21,2,0,8,49,0,0,0,0,0):return None
    return 0,0,8,255,128|32


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


def steal_exp_record(r):
    """Steal Exp 0x28: physical evasion, then (SP'+X)%; steals min(SP+Y, 100, target EXP)."""
    if r[8]!=0x28 or r[1] or r[2]>1 or r[3]!=9 or r[4]&~0x11 or r[5] or r[6] or r[7] or r[11]:return None
    return 22,r[10],r[0],2*r[2],2


def steal_gil_record(a,r):
    """Gil Taking and Negotiate share Speed x level gil, capped by a player target's funds.
    The Thief rolls physical SP'+X; the Mediator rolls Talk Skill's MA'+X."""
    expected={0x6c:bytes.fromhex('01 00 01 09 51 00 12 00 27 c8 00 00 00 00'),
              0x7b:bytes.fromhex('03 00 03 09 51 20 00 00 2a 5a 00 00 00 00')}
    if a==0x133:a,r=0x6c,r[:5]+bytes((0,))+r[6:] if r[5]==0x10 else r   # Shine Lover: Gil Taking, monster byte
    if r!=expected.get(a):return None
    return 25,0,r[0],2*r[2],2 if a==0x6c else 128|32|1


def steal_equip_record(r):
    """Equipment steals 0x26 (Steal Helmet/Armor/Shield/Weapon/Accessory): the slot as the breaks'
    (plus the accessory; nothing to steal fails), physical evasion, then (SP'+X)% as Steal Exp,
    Maintenance resists; the item goes to the player's inventory (CORE_STEAL_EQUIP)."""
    if r[8]!=0x26 or r[1] or r[2]>1 or r[3]!=9 or r[4]&~0x51 or r[5] or r[6]!=0x12 or r[7] or r[10] or r[11]:return None
    return 24,0,r[0],2*r[2],2


def break_record(r):
    """Stat breaks 0x2b (Speed/Power/Mind Break): weapon range, physical evasion, then (PA'+Y)%;
    the stat (by ability id) loses X, at least 1. X is the record's Y, the source Y its element.
    Magic Break 0x2c: the same hit, then MP loses ceil(max MP x Y / 100) (CORE_MP_BREAK)."""
    if r[8]not in(0x2b,0x2c)or r[0]or r[1]or r[2]or r[3]!=0x20 or r[4]!=0x10 or r[5]or r[6]!=0x12 or r[7]or r[11]:return None
    return 7,r[9],0,255,2


def equip_break_record(r):
    """Equipment breaks 0x25 (Head/Armor/Shield/Weapon Break): weapon range, the slot by ability id
    (none: the default Attack), physical evasion, then (PA'+zodiac(WP+Y))%; Maintenance resists."""
    if r[8]!=0x25 or r[0]or r[1]or r[2]or r[3]!=0x20 or r[4]!=0x10 or r[5]or r[6]!=0x12 or r[7]or r[9]or r[11]:return None
    return 23,r[10],0,255,2


def percent_magic_record(r):
    """Retail percent HP and single-panel HP/MP drains, with separate caster packets."""
    if r[8]not in(9,15,16)or r[9]==0 or r[11]or r[3]&~9 or r[4]&~0xe1 or r[5]&~0xe2 or r[6]&~0x42:return None
    if r[8]!=9 and (r[1]or r[7]or not(r[3]&1)or not(r[4]&1)or r[5]&128):return None
    return 11 if r[8]==15 else 0,r[10],r[0],2*r[2]if r[3]&8 else 255,128|32|(0 if r[3]&1 else 1)|(2 if r[6]&2 else 0)


def secret_fist_record(d,a,r):
    """Physical status family0x50: the Monk's exact Death Sentence producer.

    Other producers (monster/weapon procs) need their own eligibility fixtures.
    """
    if a!=0x68 or bytes(r)!=bytes((1,0,0,9,17,2,0,0,0x50,50,0,61,0,0)):return None
    if d.at(battle.STATUS_SET+6*r[11],6)!=bytes((128,0,0,0,0,1)):return None
    return 10,r[11],1,0,128|32


def monster_status_magic_record(d,a,r):
    """Monster formula 0x3D: Mind Blast (radius 1, one random status of Confusion/Berserk) and
    Death Sentence (single panel): (MA+X)% without Faith, never evaded (flags_4 0)."""
    expected={0x120:((3,1,1,0,0x11,0x10,0,0,0x3D,0x23,0,0x67,0,0),bytes((0x40,0,0x10,0x08,0,0))),
              0x130:((3,0,0,1,0x11,0x30,0,0,0x3D,0x28,0,0x3D,0,0),bytes((0x80,0,0,0,0,1)))}.get(a)
    if not expected or tuple(r)!=expected[0] or d.at(battle.STATUS_SET+6*r[11],6)!=expected[1]:return None
    return 10,r[11],r[0],255,128|32|(0 if r[3]&1 else 1)


def missing_hp_record(d,a,r):
    """Damage = the caster's missing HP. Goblin Punch 0x4F: formula 0x50's physical evade
    (flags_4 0x12) and (MA+X)% accuracy, a single target. Self Destruct 0x52: radius 2 around
    the caster, never evaded, Oil at 100% on the others; the caster takes its own HP."""
    if a==0x10F and tuple(r)==(1,0,1,9,0x11,0x10,0x12,0,0x4F,0x23,0,0,0,0):return 0,0,r[0],2*r[2],128|32|2
    if (a==0x115 and tuple(r)==(0,2,3,2,0x10,0x10,1,0,0x52,0,0,0x65,0,0) and
            d.at(battle.STATUS_SET+6*r[11],6)==bytes((0x80,0,0,0x80,0,0))):return 0,0,0,255,128|32|1
    return None


def beaking_record(r):
    """Beaking (Cocatoris), formula 0x55: magical evasion (flags_4 0x12), (MA+X)% without
    Faith, the target's PA loses Y; a single target at range 1."""
    if tuple(r)!=(1,0,1,9,0x11,0x10,0x12,0,0x55,0x2d,2,0,0,0):return None
    return 7,r[10],r[0],2*r[2],128|32|2


def odd_soundwave_record(d,r):
    """Odd Soundwave (Squidlarkin), formula 0x38 (the status application alone): its cancel set
    on radius 2 around the caster, the caster included, area vertical 1; always applied, a miss
    when nothing is removed. Core formula id 100 (cast_rules.h: FORMULA_AREA_CLEANSE)."""
    if tuple(r)!=(0,2,1,2,0x10,0x10,1,0,0x38,0,0,0x37,0,0):return None
    if d.at(battle.STATUS_SET+6*r[11],6)!=bytes((0x10,0,0,0x70,0x78,0x82)):return None
    return 13,r[11],0,255,128|32|1


def status_strike_record(d,r):
    """Monster status strikes, formula 0x50 (Eye Gouge, Poison Nail, Beak): Secret Fist's
    (MA+X)% physical-status accuracy after physical evasion (flags_4 0x12: evadeable, Blade
    Grasp), one status of an all-or-nothing set, a single target at range 1."""
    if r[8]!=0x50 or r[0]!=1 or r[1] or r[3]!=9 or r[4]!=0x11 or r[5]!=0x10 or r[6]!=0x12 or r[7] or r[10] or r[12] or r[13]:
        return None
    row=d.at(battle.STATUS_SET+6*r[11],6)
    if row[0]!=0x80 or sum(v.bit_count() for v in row[1:])!=1:return None
    return 10,r[11],r[0],2*r[2],128|32|2


def geomancy_record(d,a,r):
    """All twelve retail terrain attacks: MA*(PA+Y)/2, element and one conditional status."""
    if not 126<=a<=137:return None
    elements=(0,4,0,0,8,16,128,4,16,32,16,128)
    expected=bytes((5,1,0,0,0x11,0,0x80,elements[a-126],0x24,0,2,65+a-126,0,0))
    if r!=expected:return None
    row=d.at(battle.STATUS_SET+6*r[11],6)
    if row[0]!=128 or sum(v.bit_count()for v in row[1:])!=1:return None
    return 0,r[10],r[0],255,128|32|1


THROW_TYPES=(32,1,3,9,5,2,6,15,16,4,14,33)


def throw_records(d):
    """Archive-only item commands; 512+item keeps source learn ids untouched."""
    assert d.at(0x80061020,12)==bytes(THROW_TYPES)
    learned=d.at(HUMAN+20*25,25)
    slots={learned[3+k]|(256 if learned[k>>3]&(128>>(k&7))else 0):k for k in range(16)}
    out=bytearray()
    for item in range(128):
        it=d.item(item);w=d.at(battle.WEAPONS+8*item,8)
        enabled=bool(it['flags']&128 and w[1]&2 and it['type']in THROW_TYPES)
        pos=slots[382+THROW_TYPES.index(it['type'])]if enabled else 255
        jp=int.from_bytes(d.at(battle.ABILITY+8*(382+THROW_TYPES.index(it['type'])),2),'little')if enabled else 0
        out.extend(struct.pack('>8BH2B',0 if enabled else 255,w[4],255,255,6,item,pos,it['level'],jp,99,w[6]))
    return out


def math_tables(d):
    """Value predicates and source career/list positions, read in place from Flash."""
    assert d.at(0x80061054,8)==bytes((128,64,32,16,8,4,2,1))
    primes=set(d.at(0x80066198,54));values=bytes((int(v in primes)|int(v!=0 and v%5==0)<<1|int(v!=0 and v%4==0)<<2|int(v!=0 and v%3==0)<<3)for v in range(256))
    slots=bytearray([255]*60)
    for career in(5,6,7,11):
        skillset=d.job(0x4a+career)['set'];r=d.at(HUMAN+25*skillset,25)
        for k in range(16):
            ab=r[3+k]|(256 if r[k>>3]&(128>>(k&7))else 0)
            if ab and ab<60 and d.at(battle.ABILITY_RANGE+14*ab+5,1)[0]&64:
                assert slots[ab]==255;slots[ab]=(career<<4)|k
    return values+slots


# Ovelia's Holy Magic (sets 0x24, 0x31): Deathspell 2 cancels ten statuses (Esuna's family).
# Mbarrier adds Reraise, Regen, Protect, Shell and Haste over two bytes, beyond the one-byte
# status result: adapted (user decision 2026-11-10) to its one-byte part, all or nothing,
# stored in the unreferenced source status row 0x7F (rows 0x7A..0x7F are empty and unused).
HOLY_MAGIC = (0xA7, 0xD4)
MBARRIER_ROW = 0x7F
MBARRIER_STATUSES = bytes((128, 0, 0, 0, 0x78, 0))   # Regen, Protect, Shell, Haste


class MbarrierRows:
    """The disc's status rows with Mbarrier's adapted row at MBARRIER_ROW."""
    def __init__(self, d): self.d = d
    def at(self, addr, n):
        if addr == battle.STATUS_SET + 6 * MBARRIER_ROW and n == 6: return MBARRIER_STATUSES
        return self.d.at(addr, n)


def status_rows(d):
    rows = bytearray(d.at(battle.STATUS_SET, 128 * 6))
    assert not any(rows[6 * MBARRIER_ROW:6 * MBARRIER_ROW + 6]), 'status row 0x7F is no longer empty'
    rows[6 * MBARRIER_ROW:6 * MBARRIER_ROW + 6] = MBARRIER_STATUSES
    return bytes(rows)


def dash_like(d,a):
    """Monster copies of Dash (0x93, formula0x37): only the vertical tolerance, the monster byte,
    Y and two flags_4 bits may differ: evadeable (0x02) and Blade Grasp (0x10, read raw by the core)."""
    if not 0x100<=a<0x170:return False
    r=d.at(battle.ABILITY_RANGE+14*a,14);ref=d.at(battle.ABILITY_RANGE+14*0x93,14)
    return (r[:2]==ref[:2] and 1<=r[2]<=3 and r[3:5]==ref[3:5] and r[5] in(0,0x10) and not r[6]&~0x12
            and r[7:10]==ref[7:10] and r[11:]==ref[11:])


def accumulate_like(d,a):
    """Accumulate's (0x92) source record but for the monster byte and Y (Gather Power: PA+2)."""
    r=d.at(battle.ABILITY_RANGE+14*a,14);ref=d.at(battle.ABILITY_RANGE+14*0x92,14)
    return r[:5]==ref[:5] and r[5] in(0,0x10) and r[6:10]==ref[6:10] and r[11:]==ref[11:]


def monster_attack_record(d,a):
    """Monster basic physical attacks (Tackle, Bite, Scratch...): Choco Attack's (265) exact
    source record, formula01 range1, only the vertical tolerance may differ."""
    if not 0x100<=a<0x170 or a==265:return False
    r=d.at(battle.ABILITY_RANGE+14*a,14);ref=d.at(battle.ABILITY_RANGE+14*265,14)
    return r[:2]==ref[:2] and r[3:]==ref[3:] and 1<=r[2]<=3


def battle_records(d):
    """Shared native execution records. Unverified families remain explicitly disabled.

    Existing Orbonne adaptations (weapon skills) remain deliberate;
    their actual formula ids are retained for the module's dispatch.
    """
    # kind, Y, range, vertical, flags (fft.h); byte 10 is FFT's formula id.
    adapted = {
        0x92: (6, 1, 0, 0, 1), 0x93: (0, 4, 1, 2, 0), 0x94: (0, 2, 4, 255, 2),
        0x98: (2, 0, 1, 6, 0),
        0xA5: (0, 0, 3, 255, 8 | 16),
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
        raw = d.at(battle.ABILITY_RANGE + 14*a, 14) if 0<a<0x50 or a in HOLY_MAGIC else None
        # Punch Art (0x64..0x6b) and Talk Skill (0x74..0x7d) are read beyond the magic interval.
        art = d.at(battle.ABILITY_RANGE + 14*a, 14) if 0x64<=a<=0x6b or a in(0xE1,0x10A,0x110) else None   # Grenade's Small Bomb, Choco Ball, Turn Punch
        magic = offensive_magic_record(raw) if raw else None
        healing = healing_magic_record(raw) if raw else None
        if a == 0xD4: raw = raw[:11] + bytes((MBARRIER_ROW,)) + raw[12:]
        status = (float_record(d,raw) if a==38 else status_magic_record(MbarrierRows(d) if a == 0xD4 else d,raw,0x10 if a in HOLY_MAGIC else 0)) if raw else None
        percent = percent_magic_record(raw) if raw else None
        decisive = decisive_magic_record(raw) if raw else None
        brave = brave_magic_record(raw) if raw else None
        golem = golem_magic_record(raw) if raw else None
        draw_out = draw_out_record(d,a,d.at(battle.ABILITY_RANGE+14*a,14)) if 76<=a<=85 else None
        performance = performance_record(d,a,d.at(battle.ABILITY_RANGE+14*a,14)) if 86<=a<=99 else None
        allegiance = allegiance_record(d,a,d.at(battle.ABILITY_RANGE+14*a,14)) if a in(109,116) else None
        punch = punch_record(d,art) if art else None
        strike = (line_record(a,art) or strike_record(art)) if art else None
        speech = d.at(battle.ABILITY_RANGE + 14*a, 14) if 0x74<=a<=0x7d else None
        talk = talk_record(d,a,speech) if speech else None
        # Guts (0x96..0x99) is read beyond the magic interval too.
        guts = guts_record(d.at(battle.ABILITY_RANGE + 14*a, 14)) if 0x96<=a<=0x99 else None
        steal = steal_exp_record(d.at(battle.ABILITY_RANGE + 14*a, 14)) if a==0x73 else None
        gil = steal_gil_record(a,d.at(battle.ABILITY_RANGE + 14*a,14)) if a in(0x6c,0x7b,0x133) else None
        geomancy = geomancy_record(d,a,d.at(battle.ABILITY_RANGE+14*a,14)) if 126<=a<=137 else None
        secret = (secret_fist_record(d,a,d.at(battle.ABILITY_RANGE+14*a,14)) if a==0x68 else
                  status_strike_record(d,d.at(battle.ABILITY_RANGE+14*a,14)) if a in (0x111,0x11B,0x132) else None)
        monster_status = monster_status_magic_record(d,a,d.at(battle.ABILITY_RANGE+14*a,14)) if a in (0x120,0x130) else None
        stat_break = break_record(d.at(battle.ABILITY_RANGE + 14*a, 14)) if 0x8e<=a<=0x91 else None
        gear_steal = steal_equip_record(d.at(battle.ABILITY_RANGE + 14*a, 14)) if 0x6e<=a<=0x72 else None
        gear_break = equip_break_record(d.at(battle.ABILITY_RANGE + 14*a, 14)) if 0x8a<=a<=0x8d else None
        if a in (265,268,269) or monster_attack_record(d,a):
            r=d.at(battle.ABILITY_RANGE+14*a,14)
            if a==265 or a!=268 and a!=269:kind,y,rng,vert,flags,formula=0,0,r[0],2*r[2],10,1
            elif a==268:kind,y,rng,vert,flags,formula=13,r[11],0,255,161,81
            else:kind,y,rng,vert,flags,formula=3,r[10],0,255,161,76
        elif 0x9B <= a <= 0x9F and holy_sword_record(d, d.at(battle.ABILITY_RANGE + 14*a, 14)):
            kind, y, rng, vert, flags = holy_sword_record(d, d.at(battle.ABILITY_RANGE + 14*a, 14))
            formula = 0x2D                 # element 0: the weapon's, read at run time
        elif 0xC4 <= a <= 0xC7 and ruin_record(a, d.at(battle.ABILITY_RANGE + 14*a, 14)):
            kind, y, rng, vert, flags = ruin_record(a, d.at(battle.ABILITY_RANGE + 14*a, 14))
            formula = d.at(battle.ABILITY_RANGE + 14*a, 14)[8]
        elif 0xA0 <= a <= 0xA3 and mighty_sword_record(d.at(battle.ABILITY_RANGE + 14*a, 14)):
            kind, y, rng, vert, flags = mighty_sword_record(d.at(battle.ABILITY_RANGE + 14*a, 14))
            formula = 0x2E                 # element 0: the weapon's, read at run time
        elif a == 0xA4 and dark_sword_record(d.at(battle.ABILITY_RANGE + 14*a, 14)):
            kind, y, rng, vert, flags = dark_sword_record(d.at(battle.ABILITY_RANGE + 14*a, 14))
            formula = 0x2F
        elif a in adapted:
            kind, y, rng, vert, flags = adapted[a]
            formula = d.at(battle.ABILITY_RANGE + 14 * a, 14)[8]
        elif dash_like(d, a):
            r = d.at(battle.ABILITY_RANGE + 14*a, 14)   # Cat Kick, Tail Swing: rand(1..Y) x PA, knockback
            kind, y, rng, vert, flags = 0, r[10], r[0], 2*r[2], 2 if r[6] & 2 else 0
            formula = 0x37
        elif a == 0x143 and accumulate_like(d, a):
            kind, y, rng, vert, flags = 6, d.at(battle.ABILITY_RANGE + 14*a, 14)[10], 0, 0, 1   # Gather Power: PA+Y
            formula = 0x36
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
        elif 414<=a<422:
            kind,y,formula=27,d.at(0x80061054+a-414,1)[0],254
        elif 0x17e<=a<0x18a:
            # Source learning selectors are not commands without a selected item.
            kind,formula=26,99
        elif 394<=a<406:
            horizontal,vertical=d.at(0x8006102c+2*(a-394),2)
            kind,rng,vert,formula=0,horizontal,vertical*2,100
        elif 0x196 <= a < 0x19E:
            # CT and power are separate source fields: +7/+10/+20 are not
            # consecutive powers, and power must never index the CT table.
            y = d.at(0x80061044 + 2 * (a - 0x196), 2)[1]
            kind, rng, flags, formula = 0, 5, 8 | 2 | 64, 1
        elif magic:
            kind, y, rng, vert, flags = magic
            formula, element = raw[8], raw[7]
        elif 0x100 <= a < 0x170 and truth_magic_record(d.at(battle.ABILITY_RANGE + 14*a, 14)):
            r = d.at(battle.ABILITY_RANGE + 14*a, 14)
            kind, y, rng, vert, flags = truth_magic_record(r)
            formula, element = 0x4E, r[7]
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
        elif draw_out:
            kind,y,rng,vert,flags=draw_out;formula=d.at(battle.ABILITY_RANGE+14*a+8,1)[0]
            level=d.item(a-0x26)["level"]
        elif performance:
            kind,y,rng,vert,flags=performance;formula=28 if a<93 else 29
        elif allegiance:
            kind,y,rng,vert,flags=allegiance;formula=d.at(battle.ABILITY_RANGE+14*a+8,1)[0]
        elif punch:
            kind,y,rng,vert,flags=punch;formula=art[8]
        elif strike:
            kind,y,rng,vert,flags=strike;formula=art[8];element=art[7]
        elif gear_break:
            kind,y,rng,vert,flags=gear_break;formula=0x25;element=y   # Y, the hit percent's addend
        elif stat_break:
            kind,y,rng,vert,flags=stat_break;formula,element=d.at(battle.ABILITY_RANGE + 14*a, 14)[8:11:2]   # Y
        elif gear_steal:
            kind,y,rng,vert,flags=gear_steal;formula,element=0x26,d.at(battle.ABILITY_RANGE + 14*a, 14)[9]   # X
        elif steal:
            kind,y,rng,vert,flags=steal;formula=0x28;element=d.at(battle.ABILITY_RANGE + 14*a, 14)[9]   # X
        elif gil:
            kind,y,rng,vert,flags=gil;formula,element=d.at(battle.ABILITY_RANGE + 14*a,14)[8:10]
        elif geomancy:
            kind,y,rng,vert,flags=geomancy;formula=0x24;element=d.at(battle.ABILITY_RANGE+14*a+7,1)[0]
        elif secret:
            kind,y,rng,vert,flags=secret;formula=0x50   # status set in Y; X stays in the raw record
        elif monster_status:
            kind,y,rng,vert,flags=monster_status;formula=0x3D
        elif a in (0x10F,0x115) and missing_hp_record(d,a,d.at(battle.ABILITY_RANGE+14*a,14)):
            kind,y,rng,vert,flags=missing_hp_record(d,a,d.at(battle.ABILITY_RANGE+14*a,14));formula=d.at(battle.ABILITY_RANGE+14*a,14)[8]
        elif a==0x135 and beaking_record(d.at(battle.ABILITY_RANGE+14*a,14)):
            kind,y,rng,vert,flags=beaking_record(d.at(battle.ABILITY_RANGE+14*a,14));formula=0x55
        elif a==0x11F and odd_soundwave_record(d,d.at(battle.ABILITY_RANGE+14*a,14)):
            kind,y,rng,vert,flags=odd_soundwave_record(d,d.at(battle.ABILITY_RANGE+14*a,14));formula=100
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
    out.extend(throw_records(d))
    out.extend(d.at(0x80061044, 16)[::2])        # Charge's eight CT counts
    out.extend(b'Charge +1\0Charge +2\0')         # reviewed battle label spacing
    out.extend(bytes(d.item(i)['level'] for i in range(ITEMS)))   # required levels (Weapon Break)
    out.extend(d.at(0x8005e9d0,64))             # source surface -> Geomancy ability
    out.extend(allegiance_immunity(d))        # two bits per source job/item, no new live cache
    out.extend(math_tables(d))
    return bytes(out)


def equipment_tables(d):
    """Source category/support masks and item kind/slot/weapon flags, cold reads."""
    out=bytearray()
    for job in range(JOBS):
        j=d.job(job);bits=bytearray(4)
        for ab in j['innate']:
            if 454<=ab<486:bits[(ab-454)>>3]|=128>>((ab-454)&7)
        out.extend(j['cats']+bits)
    for item in range(256):
        it=d.item(item)
        out.extend((it['type'],it['flags'],d.at(battle.WEAPONS+8*item+1,1)[0] if it['flags']&128 else 0))
    assert len(out)==2048
    return out


def build(d):
    job_names = management_data.names(d.bat, management_data.JOB_NAMES, JOBS)
    item_names = management_data.names(d.bat, management_data.ITEM_NAMES, ITEMS)
    body = bytearray(b'FCJI' + bytes(30))
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
        sev = w[5] if it['flags']&128 else d.at(SHIELD_EVADE + 2 * (i - 0x80), 1)[0] if 0x80 <= i < 0x90 else d.at(ACCESSORY_EVADE+2*d.at(battle.ITEMS+12*i+4,1)[0],1)[0] if it['flags']&8 else 0
        rec = bytes([i, it['hp'], it['mp'], it['pa'], it['ma'], it['sp'], it['move'], it['jump'], w[4], knife])
        assert len(item_names[i]) <= 16, (i, item_names[i])
        # Every source-compatible hand; each weapon formula owns its effects.
        dual = bool(it['flags'] & 0x80 and w[1] & 8)
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
    section(status_rows(d)+status_policy_bytes(d))
    movement=bytearray(d.at(0x8005ea50,384))
    for job in range(160):
        bits=bytearray(3)
        for ab in d.job(job)['innate']:
            if 486<=ab<510:bits[(ab-486)>>3]|=128>>((ab-486)&7)
        movement.extend(bits)
    maps=d.bat[0x800f5e74-battle.BAT:0x800f6674-battle.BAT]
    assert len(maps)==2048
    aliases=bytearray([255]*64);aliases[7]=22;aliases[19]=56
    section(battle_records(d)+movement+equipment_tables(d)+d.at(0x80066064,96)+maps+aliases+b''.join(struct.pack('>H',int.from_bytes(d.at(battle.ITEMS+12*i+8,2),'little'))for i in range(256))+d.at(battle.WEAPONS,128*8))
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
