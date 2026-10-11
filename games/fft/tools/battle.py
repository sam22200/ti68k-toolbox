#!/usr/bin/env python3
"""The Gariland battle's units from the local FFT disc -> battle.h (never committed).

usage: battle.py DISC.cue battle.h                 the data the game draws its units from
       battle.py DISC.cue --check ORACLE.state TRACE.txt [oracle.h [TURNS.txt [ATTACKS.txt [SKILLS.txt [REWARDS.txt [AI.txt]]]]]]
           every unit of the original at Ramza's first turn and their start tiles
           (tools/oracle.py) against this model; oracle.h: their raw stats, equipment, stats and
           known abilities for the C tests (never committed), with TURNS (oracle.py --turns) the
           battle's turns for the clock test, with ATTACKS (oracle.py --attack) the attacks'
           previews and the executed attacks' outcome, with SKILLS (oracle.py --skills) each
           ability's preview and outcome and the Dash and Throw Stone trials, with REWARDS
           (oracle.py --rewards) the EXP, JP and level up six actions earn, with AI (oracle.py --ai)
           the AI's turns and what it did
Read from SCUS_942.21 (tables named after adamrt/fft_decomp, RE_NOTES.md § The battle's units):
the jobs (0x800610b8, 48 bytes: growth and multiplier per raw stat, Move, Jump), the items
(0x80062eb8 primary, 12 bytes; helmet / armour HP and MP 0x80063ed8; attributes 0x800642c4,
25 bytes: PA, MA, SP, Move, Jump; weapons 0x80063ab8, 8 bytes: range, flags, formula, -, WP;
the jobs' class evade, byte 0x19), the unit generation tables (0x8005e90c base, 0x8005e93c
variance: generic male, generic female, Ramza), the zodiac's day limits (0x800661e8); from
BATTLE/ENTD*.ENT the encounters (40 bytes per unit): Gariland 0x184 (Delita, five enemies) and
the Military Academy 0x188 (the recruits who join before it); from BATTLE.BIN the zodiac
compatibility table (0x8018f600); the skill sets' lists (0x80064a94), the abilities (0x8005ebf0:
JP cost, learn rate; range data 0x8005fbf0; the items' 0x80063f98) and the status records
(0x80063fc4).
The model (FFT's own, read in the decompilation and checked here on the oracle):
- raw stat (24 bits) = base << 14 + rand15 * variance / 2 (rand15: 0..32767);
- per level above 1: raw += raw / (growth + level - 1);
- stat = raw * multiplier / 100 >> 14 (at least 1; HP, MP <= 999, Speed <= 50, PA, MA <= 99);
- then the equipment: helmet and armour HP / MP, the items' PA, MA, SP, Move, Jump (<= 7);
- an ENTD value 254 is random: Brave, Faith 45 + rand15 * 30 / 32768, birthday 1..365, an
  item: one of the items of the highest required level <= the unit's level that its job can
  equip (not rare);
- the abilities a unit knows (main_unit_calculate_abilities): its job's JP 100 + rand(0..99) at
  level 1, then each ability of its skill set in order learned if a roll passes its learn rate
  and the JP left pay for it (Ramza, made by the new game, knows Wish only).
"""
import os, struct, sys
SET_N = 6                       # abilities per skill set at most (fft.h)
sys.path.insert(0, os.path.dirname(__file__))
import extract                  # noqa: E402  (disc access)

EXE = 0x80010000 - 0x800        # SCUS_942.21: file offset = address - EXE
JOBS, ITEMS, ARMOUR, ATTRS, WEAPONS = 0x800610b8, 0x80062eb8, 0x80063ed8, 0x800642c4, 0x80063ab8
KNIFE, SWORD = 1, 3                  # item types: XA = (PA + SP) / 2, XA = PA
GEN_BASE, GEN_VAR, ZODIAC = 0x8005e90c, 0x8005e93c, 0x800661e8
BAT, ZCOMPAT = 0x80067000, 0x8018f600     # BATTLE.BIN: file offset = address - BAT; the zodiac table
# abilities: the skill sets' lists (25 bytes: 3 bytes of id bit 8, 22 ids), the abilities (8 bytes:
# JP cost, learn rate), their range data (14 bytes), the items' (3 bytes: formula, Z, status), the
# status records (6 bytes: type, 5 status bytes)
SKILLSETS, ABILITY, ABILITY_RANGE, ITEM_ABILITY, STATUS_SET = 0x80064a94, 0x8005ebf0, 0x8005fbf0, 0x80063f98, 0x80063fc4
ITEM_FIRST, THROW_ITEM = 0x170, 0x1DA       # the first item ability; Throw Item (an innate support)
SUPPORT_FIRST, MOVEMENT_FIRST = 0x1C6, 0x1E6   # reaction, support, movement ability ids
EQUIP_AXE, DEFEND, EQUIP_CHANGE = 0x1CC, 0x1DF, 0x1E0
JP_MAX = 199                                # a level 1 job's JP: 100 + rand(0..99) (main_unit_calculate_abilities)
# the formulas modelled (fft.h AB_*): Y x PA damage with a Brave knockback, statuses cancelled,
# Wish, Z x 10 HP, a revival with 1..Z HP
FORMULAS = {0x37: 'AB_DAMAGE', 0x38: 'AB_CURE', 0x3C: 'AB_WISH', 0x48: 'AB_HEAL', 0x4B: 'AB_REVIVE'}
# FFT status (id: byte id / 8, bit 0x80 >> id % 8) -> ours (fft.h ST_*); Dead: 0 HP
STATUS = {0x02: 'ST_DEAD', 0x06: 'ST_DEFEND', 0x0A: 'ST_DARK', 0x0C: 'ST_SILENCE', 0x18: 'ST_POISON'}
# the names the menus show (the game's English names)
NAMES = {0x92: 'Accumulate', 0x93: 'Dash', 0x94: 'Throw Stone', 0x95: 'Heal', 0x96: 'Yell', 0x98: 'Wish',
         0x170: 'Potion', 0x176: 'Antidote', 0x177: 'Eye Drop', 0x178: 'Echo Grass', 0x17D: 'Phoenix Down'}
SET_NAMES = {0x05: 'Basic Skill', 0x06: 'Item', 0x19: 'Guts', 0x1C: 'Guts'}
# the party's bag at a new game (main_item_init_new_game_inventory), by item id
BAG = {0xF0: 5, 0xF1: 2, 0xF2: 1, 0xF3: 1, 0xF4: 1, 0xF5: 1, 0xF6: 1, 0xF7: 2, 0xF8: 1, 0xF9: 1, 0xFA: 1,
       0xFB: 1, 0xFC: 1, 0xFD: 2}
ITEM_NAMES = {0x01: 'Dagger', 0x13: 'Broad Sword', 0x9D: 'Leather Hat', 0xBA: 'Clothes', 0xD0: 'Battle Boots'}
RAMZA_LEARNED = (0x98,)                     # a new game's Ramza knows Wish only (learned_abilities[0] = 4)
RANDOM, NONE = 0xFE, 0xFF
T_MALE, T_FEMALE, T_RAMZA = 0, 1, 2
# equipment slots of a battle unit (battle_stats_t 0x1a) and the item ids of each category
SLOTS = ('head', 'body', 'accessory', 'right weapon', 'right shield', 'left weapon', 'left shield')
CATS = {0x20: range(0x90, 0xAC), 0x10: range(0xAC, 0xD0), 0x08: range(0xD0, 0xF0),
        0x80: range(0x01, 0x80), 0x40: range(0x80, 0x90)}
# FFT facing (0 -y, 1 -x, 2 +y, 3 +x) -> ours (0 +x, 1 -x, 2 +z, 3 -z); FFT's y is our z
FACE = (3, 1, 2, 0)
GFX = ('RAMZA', 'DELITA', 'SQUIRE_M', 'SQUIRE_F', 'CHEMIST_M', 'CHEMIST_F')   # units.py's sheets
TEAM_PLAYER, TEAM_ENEMY, TEAM_GUEST = 0, 1, 2

# The battle's units in FFT's own order (battle unit slots 0-5, then 16-20: the turn order of
# equal CT follows it). (encounter, ENTD slot) or 'ramza'; the player's units stand where the
# oracle deployed them (decision Q11: a fixed placement), facing the enemies (-y).
UNITS = [
    (0x184, 0), (0x184, 1), (0x184, 2), (0x184, 3), (0x184, 4), (0x184, 5),
    ('ramza', 4, 11), ((0x188, 3), 3, 11), ((0x188, 7), 6, 12), ((0x188, 4), 5, 12), ((0x188, 2), 4, 12),
]


class Disc:
    def __init__(self, cue):
        disc, files = extract.disc_files(cue)
        self.exe = disc.read(*files['SCUS_942.21'])
        self.ent = {k: disc.read(*files['BATTLE/ENTD%d.ENT' % k]) for k in (1, 2, 3, 4)}
        self.bat = disc.read(*files['BATTLE.BIN'])

    def at(self, addr, n):
        return self.exe[addr - EXE:addr - EXE + n]

    def entd(self, event, slot):
        e = self.ent[event // 0x80 + 1]
        o = (event % 0x80) * 640 + slot * 40
        return e[o:o + 40]

    def job(self, j):
        b = self.at(JOBS + 48 * j, 48)
        return {'growth': list(b[13:23:2]), 'mult': list(b[14:23:2]), 'move': b[23], 'jump': b[24] & 0x7F,
                'cats': b[9:13], 'cev': b[0x19], 'set': b[0],
                'innate': [b[1 + 2 * k] | b[2 + 2 * k] << 8 for k in range(4)]}

    def item(self, i):
        b = self.at(ITEMS + 12 * i, 12)
        a = self.at(ATTRS + 25 * b[7], 25)
        hp = mp = 0
        if b[3] & 0x30:
            hp, mp = self.at(ARMOUR + 2 * b[4], 2)
        wp = 0
        if b[3] & 0x80:                    # a weapon: range, flags, formula, -, WP, W-Ev, element, proc
            w = self.at(WEAPONS + 8 * i, 8)
            wp = w[4] if b[5] in (KNIFE, SWORD) and w[1] & 0x80 and w[2] == 1 else 0xFF   # else: not modelled
        return {'level': b[2], 'flags': b[3], 'type': b[5], 'hp': hp, 'mp': mp, 'wp': wp,
                'pa': a[0], 'ma': a[1], 'sp': a[2], 'move': a[3], 'jump': a[4]}

    def skillset(self, k):
        """The 16 action entries of skill set k (0: none)."""
        b = self.at(SKILLSETS + 25 * k, 25)
        hi = b[0] << 16 | b[1] << 8 | b[2]
        return [b[3 + i] | (0x100 if hi & 0x800000 >> i else 0) for i in range(16)]

    def ability(self, a):
        """An action ability as fft.h's Ability: kind, Y (Z, our status bits), range, vertical
        tolerance (half units, 0xFF none), flags, JP, learn rate."""
        jp, rate = struct.unpack('<HB', self.at(ABILITY + 8 * a, 3))
        if a >= ITEM_FIRST:
            form, z, st = self.at(ITEM_ABILITY + 3 * (a - ITEM_FIRST), 3)
            rng, vert, flags = 4, 0xFF, ['AF_SELF', 'AF_ITEM']   # Throw Item: 4 tiles, any height
        else:
            r = self.at(ABILITY_RANGE + 14 * a, 14)
            rng, f1, f4, form, z, st = r[0], r[3], r[6], r[8], r[10], r[11]
            assert r[1] == 0 and r[12] == 0 and r[13] == 0, 'ability %03X: an area, a charge or MP' % a
            vert = 2 * r[2] if f1 & 0x08 else 0xFF
            flags = ['AF_SELF'] * (not f1 & 0x01) + ['AF_EVADE'] * bool(f4 & 0x02)
        kind = FORMULAS.get(form)
        y = z
        if kind in ('AB_CURE', 'AB_REVIVE'):
            b = self.at(STATUS_SET + 6 * st, 6)
            ids = [i for i in range(40) if b[1 + i // 8] & 0x80 >> i % 8]
            assert b[0] & 0x10 and all(i in STATUS for i in ids), 'ability %03X: status %s' % (a, b.hex())
            if kind == 'AB_CURE':
                y = ' | '.join(STATUS[i] for i in ids)
            else:
                assert ids == [0x02]
        lv = self.item(a - ITEM_FIRST + 0xF0)['level'] if a >= ITEM_FIRST else 0
        return {'id': a, 'kind': kind, 'y': y, 'range': rng, 'vert': vert, 'flags': flags, 'jp': jp, 'rate': rate,
                'lv': lv}

    def gen(self, t):
        return list(self.at(GEN_BASE + 12 * t, 5)), list(self.at(GEN_VAR + 5 * t, 5)), list(self.at(GEN_BASE + 12 * t + 5, 7))

    def zodiac(self, day):
        lim = struct.unpack('<12H', self.at(ZODIAC, 24))
        return (sum(day >= x for x in lim) + 9) % 12

    def candidates(self, job, level, cat):
        """FFT's random item for a slot: the items of the highest required level <= level."""
        cats, best, out = self.job(job)['cats'], 0, []
        for i in CATS[cat]:
            it = self.item(i)
            if it['flags'] & 0x02 or it['type'] >= 32 or not cats[it['type'] >> 3] & (0x80 >> (it['type'] & 7)) or it['level'] > level:
                continue
            if it['level'] > best:
                best, out = it['level'], []
            out.append(i)
        return out


def unit_defs(d):
    """The eleven units: what is fixed and what is drawn at each battle."""
    out = []
    for u in UNITS:
        if u[0] == 'ramza':        # the new game's Ramza (generated out of battle, type Ramza)
            base, var, eq = d.gen(T_RAMZA)
            out.append({'gfx': 'RAMZA', 'job': 0x01, 'type': T_RAMZA, 'level': 1, 'brave': 70, 'faith': 70,
                        'day': 0, 'x': u[1], 'z': u[2], 'face': FACE[0], 'team': TEAM_PLAYER,
                        'eq': [[i] if i != NONE else [] for i in eq], 'set': d.job(0x01)['set'],
                        'learned': RAMZA_LEARNED})
            continue
        if isinstance(u[0], tuple):
            (ev, slot), x, z, face = u[0], u[1], u[2], FACE[0]
        else:
            (ev, slot), x, z, face = u, None, None, None
        b = d.entd(ev, slot)
        male = b[1] & 0x80
        job, level = b[10], b[3]
        team = TEAM_ENEMY if (b[24] >> 4 & 3) == 1 else TEAM_GUEST if ev == 0x184 else TEAM_PLAYER
        gfx = {0x04: 'DELITA'}.get(b[0]) or ('SQUIRE_' if job == 0x4A else 'CHEMIST_') + ('M' if male else 'F')
        day = 0 if b[4] in (0, RANDOM) or b[5] in (0, RANDOM) else \
            struct.unpack('<13H', d.at(0x800661ce, 26))[b[4]] + b[5]
        # ENTD equipment: head, body, accessory, right hand, left hand
        eq = [[] for _ in SLOTS]
        for k, (slot_i, cat) in enumerate(((0, 0x20), (1, 0x10), (2, 0x08), (3, 0x80), (6, 0x40))):
            v = b[18 + k]
            if v == RANDOM:                                    # left hand: a shield, if any
                eq[slot_i] = d.candidates(job, level, cat)
            elif v != NONE:
                eq[slot_i] = [v]
        out.append({'gfx': gfx, 'job': job, 'type': T_MALE if male else T_FEMALE, 'level': level,
                    'brave': 0 if b[6] > 100 else b[6], 'faith': 0 if b[7] > 100 else b[7], 'day': day,
                    'x': b[25] if x is None else x, 'z': b[26] if z is None else z,
                    'face': FACE[b[27] & 3] if face is None else face, 'team': team, 'eq': eq,
                    'set': d.job(job)['set'] if b[0x1d] in (0, NONE) else b[0x1d], 'learned': None})
        assert b[0x1d] == NONE and b[11] == 0, 'a fixed skill set or a secondary one: not modelled'
    return out


def stats(d, job, level, raw, eq):
    """FFT's stats from the raw values, the job and the equipment (main_unit_calculate_actual_stats,
    main_unit_set_equipment_attributes)."""
    j = d.job(job)
    s = []
    for i in range(5):
        v = raw[i]
        for lv in range(2, level + 1):
            v += v // (max(j['growth'][i], 1) + lv - 1)
        v = min(v, 0xFFFFFF) * j['mult'][i] // 100 >> 14
        s.append(min(max(v, 1), (999, 999, 50, 99, 99)[i]))
    hp, mp, sp, pa, ma = s
    move, jump = j['move'], j['jump']
    for i in eq:
        if i == NONE:
            continue
        it = d.item(i)
        hp, mp = min(hp + it['hp'], 999), min(mp + it['mp'], 999)
        pa, ma, sp = pa + it['pa'], ma + it['ma'], sp + it['sp']
        move, jump = move + it['move'], min(jump + it['jump'], 7)
    return {'hp': hp, 'mp': mp, 'sp': min(sp, 50), 'pa': min(pa, 99), 'ma': min(ma, 99), 'move': move, 'jump': jump}


def turn_rows(path, sp):
    """The original's turns from the battle's start: unit, CT bonus at its end (0, 20, 40: FFT's
    +20 per Move / Act not used), every unit's CT right after the pick (0xFF: not read) and the
    units at 0 HP after the turn. Turns 1-7 (Delita, the five enemies, Ramza, all picked at CT 102)
    end at the CTs the first --turns row shows; a later turn's end is its unit's CT at the next
    row less the ticks between, counted on a living unit that did not act in between."""
    rows = [l.split() for l in open(path) if l[0] != '#']
    rows = [(int(r[0]), [int(c) for c in r[1:12]], [int(h) for h in r[12:23]]) for r in rows]
    out = [(i, rows[0][1][i] - 2, [0xFF] * 11, 0) for i in range(7)]
    for (a, ct, hp), (b, ct2, hp2) in zip(rows, rows[1:]):
        c = next(c for c in range(11) if c not in (a, b) and hp[c] and hp2[c])
        k = (ct2[c] - ct[c]) // sp[c]
        assert a != b and ct2[c] - ct[c] == k * sp[c] >= 0
        end = ct2[a] - k * sp[a]
        assert end - ct[a] in (0, 20, 40), (a, ct, ct2)
        out.append((a, end - ct[a], ct, sum(1 << j for j in range(11) if not hp2[j])))
    return out


def check(d, state, trace, fixture=None, turns=None, attacks=None, skills=None, rewards=None, ai=None):
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../.claude/skills/ti-port-ps1/scripts'))
    from psxrun import PSX
    out, null = os.dup(1), os.open(os.devnull, os.O_WRONLY)
    os.dup2(null, 1); os.dup2(null, 2)                             # the core prints
    psx = PSX(sys.argv[1])
    psx.load(state)
    os.dup2(out, 1)
    start = {}                     # the trace's first line: every unit's tile when the battle starts
    for t in open(trace).read().split('\n')[11].split()[3:]:
        k, v = t.split(':')
        start[int(k)] = tuple(int(c) for c in v.split('(')[1].rstrip(')').split(','))
    bad, rows, sp, learned, defend, bz = 0, [], [], [], [], []
    sets, lists, abil = ability_table(d, unit_defs(d))
    jobs = sorted({u['job'] for u in unit_defs(d)})
    items = sorted({i for u in unit_defs(d) for e in u['eq'] for i in e})
    for n, (u, slot) in enumerate(zip(unit_defs(d), list(range(6)) + list(range(16, 21)))):
        a = 0x801908cc + 0x1c0 * slot
        r = lambda o, k=1: psx.read(a + o, k)
        raw = [r(0x72 + 3 * i) | r(0x73 + 3 * i) << 8 | r(0x74 + 3 * i) << 16 for i in range(5)]
        eq = [r(0x1a + i) for i in range(7)]
        got = {'hp': r(0x2a, 2), 'mp': r(0x2e, 2), 'sp': r(0x38), 'pa': r(0x36), 'ma': r(0x37),
               'move': r(0x3a), 'jump': r(0x3b)}
        want = stats(d, r(3), r(0x22), raw, eq)
        base, var, _ = d.gen(u['type'])
        errs = [k for k in got if got[k] != want[k]]
        errs += ['raw %d' % i for i in range(5) if not base[i] << 14 <= raw[i] <= (base[i] << 14) + 32767 * var[i] // 2]
        errs += ['job'] * (r(3) != u['job']) + ['level'] * (r(0x22) != u['level'])
        errs += ['%s %02x' % (SLOTS[i], eq[i]) for i in range(7) if (eq[i] not in u['eq'][i]) if u['eq'][i] or eq[i] != NONE]
        for k, o in (('brave', 0x24), ('faith', 0x26)):
            if (u[k] and r(o) != u[k]) or (not u[k] and not 45 <= r(o) <= 74):
                errs.append(k)
        day = r(8, 2) & 0x1FF
        if (u['day'] and day != u['day']) or (u['type'] != T_RAMZA and r(9) >> 4 != d.zodiac(day)):
            errs.append('birthday')
        if start[slot] != (u['x'], u['z']):
            errs.append('start tile')
        # the abilities it knows: its primary set's row (generic jobs: job - Squire; else row 0)
        # must be an outcome of FFT's draw (a subset of the set, bought with at most JP_MAX JP)
        row = (u['job'] - 0x4A) if u['job'] in (0x4A, 0x4B) else 0
        bits = r(0x99 + 3 * row) << 16 | r(0x9a + 3 * row) << 8 | r(0x9b + 3 * row)
        known = [a for i, a in enumerate(d.skillset(u['set'])) if a and bits & 0x800000 >> i]
        if r(0x12) != u['set'] or r(0x13) or not all(a in lists[u['set']] for a in known) or \
                (u['learned'] is not None and tuple(known) != u['learned']) or \
                (u['learned'] is None and sum(d.ability(a)['jp'] for a in known) > JP_MAX):
            errs.append('abilities %s' % [hex(a) for a in known])
        learned.append(sum(1 << lists[u['set']].index(a) for a in known if a in lists[u['set']]))
        sup = r(0x16, 2)                  # its support: drawn among those it can afford (Ramza: none)
        if sup not in ((0,) if u['type'] == T_RAMZA else (EQUIP_AXE, DEFEND, EQUIP_CHANGE)):
            errs.append('support %03x' % sup)
        defend.append(sup == DEFEND)
        bz.append((r(0x24), r(0x26), r(9) >> 4, int(bool(r(6) & 0x40))))
        print('%2d %-9s lv %d hp %3d mp %3d sp %d pa %d ma %d mv %d jp %d br %2d fa %2d eq %s %s' % (
            slot, u['gfx'], r(0x22), got['hp'], got['mp'], got['sp'], got['pa'], got['ma'], got['move'], got['jump'],
            r(0x24), r(0x26), ' '.join('%02x' % i for i in eq[:4]), 'ok' if not errs else 'DIFFERS: ' + ', '.join(errs)))
        bad += bool(errs)
        sp.append(got['sp'])
        rows.append('    { %d, %d, { %s }, { %s }, { %d, %d, %d, %d, %d, %d, %d } },' % (
            jobs.index(r(3)) if r(3) in jobs else 0xFF, r(0x22), ', '.join('0x%06X' % v for v in raw),
            ', '.join(str(items.index(eq[k])) if eq[k] in items else '0xFF' for k in (0, 1, 2, 3, 6)),
            *(got[k] for k in ('hp', 'mp', 'sp', 'pa', 'ma', 'move', 'jump'))))
    print('%d of 11 units differ' % bad if bad else 'all 11 units equal the model')
    if fixture:
        open(fixture, 'w').write(
            '// Generated by tools/battle.py --check from the original at Ramza\'s first Gariland turn\n'
            '// (tools/oracle.py): never committed. Per unit in battle.h\'s order: job and item indices\n'
            '// as there, level, raw stats, equipment, then the original\'s max HP, max MP, SP, PA,\n'
            '// MA, Move, Jump.\n'
            'static const struct { u8 job, level; u32 raw[5]; u8 eq[5]; u16 want[7]; } oracle[11] = {\n'
            + '\n'.join(rows) + '\n};\n')
        if turns:
            t = turn_rows(turns, sp)
            open(fixture, 'a').write(
                '// The original\'s turns from the battle\'s start (tools/oracle.py --turns, the player\'s\n'
                '// units played by its AI): unit, CT bonus at the end (0, 20, 40), every unit\'s CT right\n'
                '// after the pick (0xFF: not read), the units at 0 HP after the turn (bit per unit).\n'
                '#define ORACLE_TURNS %d\n'
                'static const struct { u8 unit, bonus, ct[11]; u16 dead; } oracle_turns[ORACLE_TURNS] = {\n' % len(t)
                + '\n'.join('    { %d, %d, { %s }, 0x%03X },' % (u, b, ', '.join(map(str, c)), m) for u, b, c, m in t)
                + '\n};\n')
            print('%d turns' % len(t))
        if attacks:
            cases, trials = [], []
            for ln in open(attacks):
                w = ln.split()
                if w and w[0] == 'case':
                    v = list(map(int, w[1:]))
                    v[3] = items.index(v[3]) if v[3] in items else 0xFF
                    v[8] = FACE[v[8]]
                    cases.append(v)
                elif w and w[0] == 'trial':
                    trials.append((int(w[1]), int(w[2])))
            base = min(dm for dm, _ in trials if dm)   # a hit without a critical (the preview)
            open(fixture, 'a').write(
                '// The original\'s attack previews (tools/oracle.py --attack): Ramza on the recruit south of\n'
                '// him, poked: his PA, Speed, Brave, weapon (item index, 0xFF none), sign, female; the\n'
                '// target\'s sign, female, facing (ours); the hit %% and damage FFT shows.\n'
                '#define ORACLE_ATTACKS %d\n'
                'static const u8 oracle_attacks[ORACLE_ATTACKS][11] = {\n' % len(cases)
                + '\n'.join('    { %s },' % ', '.join(map(str, c)) for c in cases) + '\n};\n'
                '// Attacks executed in the first of them (the target facing him: 95 %%): the damage\n'
                '// without a critical, the attacks, misses, hits above it (criticals; one adding 0 is not\n'
                '// seen), total damage, pushes\n'
                'static const u16 oracle_trials[6] = { %d, %d, %d, %d, %d, %d };\n' % (
                    base, len(trials), sum(1 for dm, _ in trials if not dm),
                    sum(1 for dm, _ in trials if dm > base), sum(dm for dm, _ in trials), sum(p for _, p in trials)))
            print('%d attack cases, %d trials' % (len(cases), len(trials)))
        open(fixture, 'a').write(
            '// The abilities each unit knows in the original (bit k: its skill set\'s k-th in battle.h)\n'
            'static const u8 oracle_learned[11] = { %s };\n' % ', '.join('0x%02X' % m for m in learned)
            + '// the units whose support ability is Defend in the original (bit per unit)\n'
            'static const u16 oracle_defend = 0x%03X;\n' % sum(1 << i for i, v in enumerate(defend) if v)
            + '// their Brave, Faith, zodiac sign and sex (1: female)\n'
            'static const u8 oracle_bz[11][4] = { %s };\n' % ', '.join('{ %d, %d, %d, %d }' % v for v in bz))
        if skills:
            write_skills(fixture, skills, abil)
        if rewards:
            write_rewards(fixture, rewards)
        if ai:
            write_ai(d, fixture, ai, sets, lists, abil)
    return bad


AI_CASES = ['battle', 'near', 'critical', 'far', 'potion', 'kill', 'defend', 'delita-low', 'recruit']   # oracle.py


def write_ai(d, fixture, path, sets, lists, abil):
    """oracle.py --ai: each AI turn's start (per unit: tile, facing, HP, CT, death counter,
    statuses, the abilities it knows, Defend) and what it did, for the C tests."""
    defs, rows, reach = unit_defs(d), [], []
    for ln in open(path):
        w = ln.split()
        if w and w[0] == 'reach':
            m = w[2]
            reach.append((int(w[1]), [sum(1 << x for x in range(10) if m[z * 10 + x] == '1') for z in range(15)]))
        if not w or w[0] != 'ai':
            continue
        u = int(w[2])
        pre = [list(map(int, x.split(','))) for x in w[3:14]]
        did = list(map(int, w[15].split(',')))               # (w[14], w[27]: the tile priorities)
        post = [list(map(int, x.split(','))) for x in w[16:27]]
        units = []
        for k, v in enumerate(pre):
            x, y, f, hp, ct, dc, b, bits, sup = v[0], v[1], v[2], v[3], v[4], v[5], v[6:11], v[11:14], v[14]
            bits = bits[0] << 16 | bits[1] << 8 | bits[2]
            st = defs[k]['set']
            known = [a for i, a in enumerate(d.skillset(st)) if a and bits & 0x800000 >> i]
            mask = sum(1 << lists[st].index(a) for a in known if a in lists[st])
            if b[0] & 0x40 or b[1] & 0x01:                       # a crystal, a chest
                dc = 0xFD if b[0] & 0x40 else 0xFE
            elif hp or dc > 3:                                    # alive; the guest's KO: 0xFF
                dc = 3
            units.append((x, y, FACE[f], hp, ct, dc, our_status(b), mask, int(sup == DEFEND), post[k][3]))
        moved, acted, ab, tx, ty = did
        a = post[u]
        if not acted:
            ours = 0xFD                                           # AI_NONE
        elif ab == 0 and (tx, ty) == (0, 0) and a[6] & 0x02 and not pre[u][6] & 0x02:
            ours = 0xFE                                           # ABIL_DEFEND
        elif ab == 0:
            ours = 0xFF                                           # NO_ABIL: Attack
        else:
            ours = abil.index(ab)
        rows.append((AI_CASES.index(w[1]), u, units, moved, acted, ours, tx, ty, a[0], a[1], FACE[a[2]]))
    open(fixture, 'a').write(
        '// The original\'s AI turns (tools/oracle.py --ai): the battle played by its AI from Ramza\'s\n'
        '// first turn (case 0), then built situations each replayed with other draws: the case, the\n'
        '// unit, then per unit x, z, facing (ours), HP, CT, death counter, statuses (ours), the\n'
        '// abilities it knows, Defend, HP after the turn; what it did: moved, acted, the ability\n'
        '// (NO_ABIL Attack, ABIL_DEFEND, AI_NONE), the target tile, its tile and facing after\n'
        '#define ORACLE_AI %d\n'
        'static const struct { u8 kase, unit, u[11][10], moved, acted, ab, tx, tz, x, z, face; }\n'
        '    oracle_ai[ORACLE_AI] = {\n' % len(rows)
        + '\n'.join('    { %d, %d, { %s }, %d, %d, 0x%02X, %d, %d, %d, %d, %d },' % (
            c, u, ', '.join('{ %s }' % ', '.join(map(str, v)) for v in units), *rest)
            for c, u, units, *rest in rows) + '\n};\n'
        '// The move range the original shows (Ramza at his first turn, the recruit after him): the\n'
        '// unit, a bit per tile (bit x of row z)\n'
        'static const struct { u8 unit; u16 row[15]; } oracle_reach[%d] = { %s };\n' % (
            len(reach), ', '.join('{ %d, { %s } }' % (u, ', '.join('0x%03X' % r for r in rr)) for u, rr in reach)))
    print('%d AI turns, %d move ranges' % (len(rows), len(reach)))


def write_rewards(fixture, path):
    """oracle.py --rewards: per case Ramza's EXP before, then after: his EXP, level, max HP, HP,
    and each unit's JP gained in career slot 0 (Ramza's: Squire), for the C tests."""
    rows = []
    for ln in open(path):
        w = ln.split()
        if not w or w[0] != 'reward':
            continue
        v = list(map(int, w[2:]))
        b, a = v[:len(v) // 2], v[len(v) // 2:]
        r = 6 * 7                                    # Ramza: battle.h's unit 6
        rows.append((int(w[1] == 'kill'), int(w[1] != 'heal-none'), b[r], b[r + 4], a[r], a[r + 1], a[-3], a[-2],
                     ', '.join(str(a[7 * i + 2] - b[7 * i + 2]) for i in range(11))))
    open(fixture, 'a').write(
        '// The original\'s rewards (tools/oracle.py --rewards): Ramza acting on the recruit south of him\n'
        '// (attack, kill, a level up from 95 EXP, Potion, Heal missing, Dash): kill, did something,\n'
        '// his EXP and total JP before; after: EXP, level, max HP, HP; each unit\'s JP gained in his job\n'
        '#define ORACLE_REWARDS %d\n'
        'static const struct { u8 kill, hit, exp; u16 tjp; u8 exp2, level2; u16 max_hp2, hp2; u8 jp[11]; }\n'
        '    oracle_rewards[ORACLE_REWARDS] = {\n' % len(rows)
        + '\n'.join('    { %d, %d, %d, %d, %d, %d, %d, %d, { %s } },' % r for r in rows) + '\n};\n')
    print('%d reward cases' % len(rows))


SKILL_CASES = {'dash': 0x93, 'stone': 0x94, 'heal': 0x95, 'heal-none': 0x95, 'wish': 0x98, 'potion': 0x170,
               'antidote': 0x176, 'eye-drop-none': 0x177, 'echo-grass': 0x178, 'phoenix': 0x17D,
               'phoenix-alive': 0x17D, 'stone-defend': 0x94, 'attack-defend': None}
OUR_STATUS = {'ST_DEFEND': 1, 'ST_POISON': 2, 'ST_DARK': 4, 'ST_SILENCE': 8}   # fft.h


def our_status(b):
    """FFT's five current status bytes -> our bits (Dead is 0 HP)."""
    return sum(OUR_STATUS[STATUS[i]] for i in STATUS if STATUS[i] in OUR_STATUS and b[i // 8] & 0x80 >> i % 8)


def write_skills(fixture, path, abil):
    """oracle.py --skills: each case (Ramza with every skill on the recruit south of him) and the
    executed Dash and Throw Stone trials, for the C tests."""
    cases, trials = [], {'dash': [], 'stone': []}
    for ln in open(path):
        w = ln.split()
        if w and w[0] == 'skill':
            v = list(map(int, w[2:]))
            hit, amount, bef, aft = v[0], v[1], v[2:16], v[16:30]
            a = SKILL_CASES[w[1]]
            cases.append((abil.index(a) if a is not None else 0xFF, bef[0], our_status(bef[1:6]), hit, amount,
                          aft[0], our_status(aft[1:6]), aft[8], *aft[9:14]))
        elif w and w[0] == 'trial':
            trials[w[1]].append((int(w[2]), int(w[3])))
    hist = []
    for k in ('dash', 'stone'):
        t = trials[k]
        base = min(dm for dm, _ in t if dm)
        hist.append([len(t), sum(p for _, p in t)] + [sum(1 for dm, _ in t if dm == base * m) for m in range(5)])
        assert sum(hist[-1][2:]) == len(t), 'a damage not a multiple of %d' % base
    open(fixture, 'a').write(
        '// The original\'s abilities (tools/oracle.py --skills): Ramza on the recruit south of him (the\n'
        '// attack\'s game case) with each ability (battle.h index, 0xFF Attack), the target\'s HP and\n'
        '// statuses (ours) poked; FFT\'s preview (hit %%, amount: a random roll for Dash, Throw Stone,\n'
        '// Phoenix Down; garbage when it misses), then after the execution: the target\'s HP, statuses,\n'
        '// Ramza\'s HP, the bag (Potion, Antidote, Eye Drop, Echo Grass, Phoenix Down).\n'
        '#define ORACLE_SKILLS %d\n'
        'static const struct { u8 abil; u16 hp; u8 status, hit; u16 amount, hp2; u8 status2; u16 ramza; u8 bag[5]; }\n'
        '    oracle_skills[ORACLE_SKILLS] = {\n' % len(cases)
        + '\n'.join('    { %d, %d, %d, %d, %d, %d, %d, %d, { %d, %d, %d, %d, %d } },' % c for c in cases) + '\n};\n'
        '// Dash and Throw Stone executed in the game case: trials, pushes, then how many took 0 (a miss),\n'
        '// 1, 2, 3, 4 times XA (3)\n'
        'static const u16 oracle_skill_trials[2][7] = { %s };\n'
        % ', '.join('{ %s }' % ', '.join(map(str, h)) for h in hist))
    print('%d ability cases, %d + %d trials' % (len(cases), len(trials['dash']), len(trials['stone'])))


def ability_table(d, defs):
    """The skill sets the units use and their abilities that can be learned (a level 1 job has at
    most JP_MAX JP: dearer ones are never bought) or are fixed (Ramza's Wish)."""
    sets = sorted({u['set'] for u in defs})
    fixed = {a for u in defs for a in (u['learned'] or ())}
    jp = lambda a: struct.unpack('<H', d.at(ABILITY + 8 * a, 2))[0]
    lists = {k: [a for a in d.skillset(k) if a and (jp(a) <= JP_MAX or a in fixed)] for k in sets}
    abil = sorted({a for k in sets for a in lists[k]})
    for a in abil:
        assert d.ability(a)['kind'] and a in NAMES, 'ability %03X: not modelled' % a
    for u in defs:
        if any(a >= ITEM_FIRST for a in lists[u['set']]):
            assert THROW_ITEM in d.job(u['job'])['innate'], 'items without Throw Item: range 1'
    return sets, lists, abil


def rsm(d, k):
    """The reaction, support and movement abilities of skill set k in FFT's order: (id, JP, rate)."""
    b = d.at(SKILLSETS + 25 * k, 25)
    hi = b[0] << 16 | b[1] << 8 | b[2]
    ids = [b[19 + i] | (0x100 if hi & 0x80 >> i else 0) for i in range(6)]
    return [(a,) + struct.unpack('<HB', d.at(ABILITY + 8 * a, 3)) for a in ids if a]


def write_support(d, f, sets):
    """The support ability (main_unit_calculate_rsm): one of those the unit learned with its jobs'
    JP, drawn. With at most JP_MAX JP per job only Equip Change (Chemist, 0 JP, always learned),
    Defend and Equip Axe (Squire) can be bought; Defend comes with the Squire job's JP left after
    its abilities and the reactions and supports listed before it."""
    sq, ch = rsm(d, 0x05), rsm(d, 0x06)
    sup = lambda a: SUPPORT_FIRST <= a < MOVEMENT_FIRST
    can = [(a, jp, r) for a, jp, r in sq + ch if sup(a) and jp <= JP_MAX and a != THROW_ITEM]
    assert [a for a, _, _ in can] == [EQUIP_AXE, DEFEND, EQUIP_CHANGE], can
    assert can[2][1:] == (0, 100) and can[0][1] + can[1][1] > JP_MAX, 'Equip Change not certain, or two Squire supports'
    k = [a for a, _, _ in sq].index(DEFEND)
    f.write('// the support ability (FFT draws one of those a unit learned): Defend, learned with the Squire job\'s\n'
            '// JP (left after its abilities; for another job 100..199 and its Squire abilities drawn first)\n'
            '// after the reactions and supports before it (JP, learn rate), then one of two (Equip Change,\n'
            '// always learned)\n')
    f.write('#define SET_BASIC %d\n#define NRSM_PRE %d\nstatic const u8 rsm_pre[NRSM_PRE][2] = { %s };\n' % (
        sets.index(0x05), k, ', '.join('{ %d, %d }' % (min(jp, 255), r) for _, jp, r in sq[:k])))
    f.write('#define DEFEND_JP %d\n#define DEFEND_RATE %d\n' % sq[k][1:])


def write(d, path):
    defs = unit_defs(d)
    jobs = sorted({u['job'] for u in defs})
    items = sorted({i for u in defs for s in u['eq'] for i in s})
    assert all(len(s) <= 2 for u in defs for s in u['eq']), 'more than two random candidates'
    f = open(path, 'w')
    f.write('// Generated by tools/battle.py from the local FFT disc (SCUS_942.21, BATTLE/ENTD*.ENT): never\n'
            '// committed. The Gariland battle: its jobs, items, the unit generation tables and the\n'
            '// eleven units (fft.h: Job, Item, UnitDef), numbers as FFT has them.\n')
    f.write('#define NJOB %d\n#define NITEM %d\n#define NUNIT %d\n' % (len(jobs), len(items), len(defs)))
    f.write('static const Job jobs[NJOB] = {   // FFT job id: growth and multiplier (HP MP SP PA MA), Move, Jump,\n'
            '                                  // class evade, career row (where its JP go: Squire 0, Chemist 1)\n')
    for j in jobs:
        e = d.job(j)
        f.write('    { { %s }, { %s }, %d, %d, %d, %d },   // %02X\n' % (', '.join(map(str, e['growth'])), ', '.join(map(str, e['mult'])),
                                                                  e['move'], e['jump'], e['cev'], j - 0x4A if j in (0x4A, 0x4B) else 0, j))
    f.write('};\nstatic const Item items[NITEM] = {   // FFT item id, HP, MP, PA, MA, SP, Move, Jump; a weapon\'s WP and\n'
            '                                    // its XA (0 PA: a sword, 1 (PA + SP) / 2: a knife); its name\n')
    for i in items:
        it = d.item(i)
        assert it['wp'] != 0xFF, 'item %02X: a weapon other than a striking knife or sword (formula 1)' % i
        f.write('    { 0x%02X, %d, %d, %d, %d, %d, %d, %d, %d, %d, "%s" },\n' % (
            i, it['hp'], it['mp'], it['pa'], it['ma'], it['sp'], it['move'], it['jump'], it['wp'], it['type'] == KNIFE, ITEM_NAMES[i]))
    f.write('};\n// raw stat base and variance per unit type (generic male, generic female, Ramza)\n')
    f.write('static const u8 gen_base[3][5] = { %s };\n' % ', '.join('{ %s }' % ', '.join(map(str, d.gen(t)[0])) for t in range(3)))
    f.write('static const u8 gen_var[3][5] = { %s };\n' % ', '.join('{ %s }' % ', '.join(map(str, d.gen(t)[1])) for t in range(3)))
    f.write('static const u16 zodiac_days[12] = { %s };   // a sign starts at each\n'
            % ', '.join(map(str, struct.unpack('<12H', d.at(ZODIAC, 24)))))
    f.write('// zodiac compatibility by the signs\' distance (0 neutral, 1 bad, 2 good, 3 opposite: worst for\n'
            '// the same sex, best else)\n')
    f.write('static const u8 zodiac_compat[12] = { %s };\n' % ', '.join(map(str, d.bat[ZCOMPAT - BAT:ZCOMPAT - BAT + 12])))
    sets, lists, abil = ability_table(d, defs)
    bag = [a for a in abil if a >= ITEM_FIRST]
    f.write('// the abilities the units can know: name, kind, Y (Z, statuses), range, vertical tolerance\n'
            '// (half units, 0xFF none), flags, bag slot (0xFF none), learn rate, JP cost, an item\'s level (the\n'
            '// enemies use those of their level or less)\n')
    f.write('#define NABIL %d\nstatic const Ability abilities[NABIL] = {\n' % len(abil))
    for a in abil:
        e = d.ability(a)
        f.write('    { "%s", %s, %s, %d, %d, %s, %s, %d, %d, %d },   // %03X\n' % (
            NAMES[a], e['kind'], e['y'], e['range'], e['vert'], ' | '.join(e['flags']) or '0',
            bag.index(a) if a in bag else 0xFF, e['rate'], e['jp'], e['lv'], a))
    f.write('};\n// the skill sets: name, then its abilities in FFT\'s order (the order a unit learns them)\n')
    f.write('#define NSET %d\nstatic const SkillSet skill_sets[NSET] = {\n' % len(sets))
    for k in sets:
        ids = [str(abil.index(a)) for a in lists[k]] + ['0xFF'] * (SET_N - len(lists[k]))
        f.write('    { "%s", { %s } },   // %02X\n' % (SET_NAMES[k], ', '.join(ids), k))
    assert max(len(lists[k]) for k in sets) <= SET_N
    f.write('};\n')
    write_support(d, f, sets)
    f.write('// the party\'s bag at a new game, per bag slot (the enemies\' items do not run out)\n')
    f.write('#define NBAG %d\nstatic const u8 bag_init[NBAG] = { %s };\n' % (
        len(bag), ', '.join(str(BAG[a - ITEM_FIRST + 0xF0]) for a in bag)))
    f.write('// the units in FFT\'s order: sheet, job, type, level, Brave, Faith (0: drawn), birthday (0:\n'
            '// drawn), x, z, facing, team, equipment (head, body, accessory, weapon, shield: up to two\n'
            '// item indices, one drawn; 0xFF none), skill set, the abilities it knows (bit k: the set\'s\n'
            '// k-th; 0xFF: drawn)\n')
    f.write('static const UnitDef unit_defs[NUNIT] = {\n')
    for u in defs:
        eq = [u['eq'][k] for k in (0, 1, 2, 3, 6)]
        learned = 0xFF if u['learned'] is None else sum(1 << lists[u['set']].index(a) for a in u['learned'])
        f.write('    { UG_%s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, { %s }, %d, 0x%02X },\n' % (
            u['gfx'], jobs.index(u['job']), u['type'], u['level'], u['brave'], u['faith'], u['day'], u['x'], u['z'],
            u['face'], u['team'], ', '.join('{ %s }' % ', '.join(str(items.index(s[k])) if k < len(s) else '0xFF'
                                                                for k in range(2)) for s in eq),
            sets.index(u['set']), learned))
    f.write('};\n')
    f.close()


def main():
    d = Disc(sys.argv[1])
    if sys.argv[2] == '--check':
        a = sys.argv + [None] * 8
        sys.exit(1 if check(d, a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10]) else 0)
    write(d, sys.argv[2])


if __name__ == '__main__':
    main()
