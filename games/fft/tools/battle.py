#!/usr/bin/env python3
"""The Gariland battle's units from the local FFT disc -> battle.h (never committed).

usage: battle.py DISC.cue battle.h                 the data the game draws its units from
       battle.py DISC.cue --check ORACLE.state TRACE.txt [oracle.h [TURNS.txt]]   every unit of the
           original at Ramza's first turn and their start tiles (tools/oracle.py) against this
           model; oracle.h: their raw stats, equipment and stats for the C tests (never
           committed), and with TURNS (oracle.py --turns) the battle's turns for the clock test
Read from SCUS_942.21 (tables named after adamrt/fft_decomp, RE_NOTES.md § The battle's units):
the jobs (0x800610b8, 48 bytes: growth and multiplier per raw stat, Move, Jump), the items
(0x80062eb8 primary, 12 bytes; helmet / armour HP and MP 0x80063ed8; attributes 0x800642c4,
25 bytes: PA, MA, SP, Move, Jump), the unit generation tables (0x8005e90c base, 0x8005e93c
variance: generic male, generic female, Ramza), the zodiac's day limits (0x800661e8); from
BATTLE/ENTD*.ENT the encounters (40 bytes per unit): Gariland 0x184 (Delita, five enemies) and
the Military Academy 0x188 (the recruits who join before it).
The model (FFT's own, read in the decompilation and checked here on the oracle):
- raw stat (24 bits) = base << 14 + rand15 * variance / 2 (rand15: 0..32767);
- per level above 1: raw += raw / (growth + level - 1);
- stat = raw * multiplier / 100 >> 14 (at least 1; HP, MP <= 999, Speed <= 50, PA, MA <= 99);
- then the equipment: helmet and armour HP / MP, the items' PA, MA, SP, Move, Jump (<= 7);
- an ENTD value 254 is random: Brave, Faith 45 + rand15 * 30 / 32768, birthday 1..365, an
  item: one of the items of the highest required level <= the unit's level that its job can
  equip (not rare).
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(__file__))
import extract                  # noqa: E402  (disc access)

EXE = 0x80010000 - 0x800        # SCUS_942.21: file offset = address - EXE
JOBS, ITEMS, ARMOUR, ATTRS = 0x800610b8, 0x80062eb8, 0x80063ed8, 0x800642c4
GEN_BASE, GEN_VAR, ZODIAC = 0x8005e90c, 0x8005e93c, 0x800661e8
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

    def at(self, addr, n):
        return self.exe[addr - EXE:addr - EXE + n]

    def entd(self, event, slot):
        e = self.ent[event // 0x80 + 1]
        o = (event % 0x80) * 640 + slot * 40
        return e[o:o + 40]

    def job(self, j):
        b = self.at(JOBS + 48 * j, 48)
        return {'growth': list(b[13:23:2]), 'mult': list(b[14:23:2]), 'move': b[23], 'jump': b[24] & 0x7F,
                'cats': b[9:13]}

    def item(self, i):
        b = self.at(ITEMS + 12 * i, 12)
        a = self.at(ATTRS + 25 * b[7], 25)
        hp = mp = 0
        if b[3] & 0x30:
            hp, mp = self.at(ARMOUR + 2 * b[4], 2)
        return {'level': b[2], 'flags': b[3], 'type': b[5], 'hp': hp, 'mp': mp,
                'pa': a[0], 'ma': a[1], 'sp': a[2], 'move': a[3], 'jump': a[4]}

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
                        'eq': [[i] if i != NONE else [] for i in eq]})
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
                    'face': FACE[b[27] & 3] if face is None else face, 'team': team, 'eq': eq})
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


def check(d, state, trace, fixture=None, turns=None):
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
    bad, rows, sp = 0, [], []
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
    return bad


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
    f.write('static const Job jobs[NJOB] = {   // FFT job id: growth and multiplier (HP MP SP PA MA), Move, Jump\n')
    for j in jobs:
        e = d.job(j)
        f.write('    { { %s }, { %s }, %d, %d },   // %02X\n' % (', '.join(map(str, e['growth'])), ', '.join(map(str, e['mult'])),
                                                          e['move'], e['jump'], j))
    f.write('};\nstatic const Item items[NITEM] = {   // FFT item id, HP, MP, PA, MA, SP, Move, Jump\n')
    for i in items:
        it = d.item(i)
        f.write('    { 0x%02X, %d, %d, %d, %d, %d, %d, %d },\n' % (i, it['hp'], it['mp'], it['pa'], it['ma'], it['sp'], it['move'], it['jump']))
    f.write('};\n// raw stat base and variance per unit type (generic male, generic female, Ramza)\n')
    f.write('static const u8 gen_base[3][5] = { %s };\n' % ', '.join('{ %s }' % ', '.join(map(str, d.gen(t)[0])) for t in range(3)))
    f.write('static const u8 gen_var[3][5] = { %s };\n' % ', '.join('{ %s }' % ', '.join(map(str, d.gen(t)[1])) for t in range(3)))
    f.write('static const u16 zodiac_days[12] = { %s };   // a sign starts at each\n'
            % ', '.join(map(str, struct.unpack('<12H', d.at(ZODIAC, 24)))))
    f.write('// the units in FFT\'s order: sheet, job, type, level, Brave, Faith (0: drawn), birthday (0:\n'
            '// drawn), x, z, facing, team, equipment (head, body, accessory, weapon, shield: up to two\n'
            '// item indices, one drawn; 0xFF none)\n')
    f.write('static const UnitDef unit_defs[NUNIT] = {\n')
    for u in defs:
        eq = [u['eq'][k] for k in (0, 1, 2, 3, 6)]
        f.write('    { UG_%s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, { %s } },\n' % (
            u['gfx'], jobs.index(u['job']), u['type'], u['level'], u['brave'], u['faith'], u['day'], u['x'], u['z'],
            u['face'], u['team'], ', '.join('{ %s }' % ', '.join(str(items.index(s[k])) if k < len(s) else '0xFF'
                                                                for k in range(2)) for s in eq)))
    f.write('};\n')
    f.close()


def main():
    d = Disc(sys.argv[1])
    if sys.argv[2] == '--check':
        a = sys.argv + [None, None]
        sys.exit(1 if check(d, a[3], a[4], a[5], a[6]) else 0)
    write(d, sys.argv[2])


if __name__ == '__main__':
    main()
