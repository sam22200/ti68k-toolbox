#!/usr/bin/env python3
"""The reference: FFT run headless (psxrun's pcsx_rearmed core) from a new game to Ramza's first
turn at Gariland, saved as a core state (never committed). RE_NOTES.md § Battle oracle.

usage: oracle.py DISC.cue OUT.state [trace.txt]
       oracle.py --turns DISC.cue STATE N turns.txt
       oracle.py --attack DISC.cue STATE N M attacks.txt
The trace lists, from the battle's start to Ramza's turn, every frame where a unit's HP, CT,
position or the turn unit changed (the turn order and the AI turns as the game plays them).
Deterministic: the same inputs and pokes at the same frames give the same battle (the RNG
included). The path:
1. New game: START / CIRCLE taps through the logos, Ramza's name (kept as typed) and birthday;
   from frame 2000 the script variables CURRENT_EVENT = 6, NEXT_SCENARIO = 1 are written
   every frame until the game moves on, so the scenario loader (ATTACK.OUT's
   attack_load_scenario_conditionals, read in adamrt/fft_decomp) loads event 7: the Military
   Academy scene before Gariland, where the recruits join (Orbonne is skipped).
2. The academy scene runs; its save prompt is cancelled with CROSS; event 9 (Gariland, ENTD
   0x184, map 22) loads its deployment screen.
3. Deployment: Hampsten and Greg (Squires) placed with the pad, then the two Chemists (party
   slots 3 and 6) written into the deployment grid; START, CIRCLE: the battle starts.
4. CIRCLE taps through the intro until READY!, the AI turns run alone, CIRCLE through the
   dialogue that opens Ramza's turn: his command menu is open.
--turns goes on from that state: Ramza waits (menu Wait, facing kept), then the player's units
lose their player-controlled flag (team +0x05 bit 0x08, poked every frame) so the AI plays the
whole battle; each of the first N turns of a living unit is written as one line, the unit and
every unit's CT and HP right after the clock picked it (has_turn +0x186 rising on the turn
unit; the AI's trial writes, which flicker it, are skipped).
--attack goes on from that state too: Ramza attacks the recruit south of him (unit 20, on +y:
menu Act, Attack, the help box closed, LEFT, CIRCLE; then Execute, OK). N cases with the attacker's PA, Speed,
Brave, weapon (none, Dagger, Broad Sword), sign and sex and the target's sign, sex and facing
poked first (seeded draws): the hit % and damage FFT previews (g_battle_menu_active_turn_banner
0x8014d056 / 0x8014d058). Then M attacks executed in the game's first case (Broad Sword, PA 5,
the target facing him: 95 %), each after a different number of cursor moves (every preview
draws rand(), so each attack rolls other numbers): the HP it took (0: a miss) and whether the
target was pushed a tile (knockback).
"""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../.claude/skills/ti-port-ps1/scripts'))
from psxrun import PSX, PAD                       # noqa: E402

VARS = 0x8005771c                                 # g_main_script_variables (s32 words)
EVENT, NEXT = 0x27, 0x64
UNITS, UNIT_SIZE = 0x801908cc, 0x1c0              # g_battle_unit_stats: 21 units
DEPLOY_COUNT, DEPLOY_GRID = 0x801cd058, 0x801dcbe4   # deployed units, roster id per 5x5 tile


def key(name):
    return 1 << PAD[name]


def taps(pattern):
    """Frames of a 'KEY:wait ...' sequence: each key held 4 frames, then wait frames."""
    out = []
    for t in pattern.split():
        k, w = (t.split(':') + ['30'])[:2]
        out += ([] if k == 'W' else [key(k)] * 4) + [0] * int(w)
    return out


def run(psx, frames, press=lambda f: 0, hook=None):
    for f in range(frames):
        psx.pressed = press(f) if callable(press) else (press[f] if f < len(press) else 0)
        psx.run()
        if hook:
            hook(psx, f)


def var(p, i):
    return p.read(VARS + 4 * i, 4)


def boot(psx):
    def press(f):
        if f > 300 and f % 60 < 4:
            return key('START') if (f // 60) % 3 == 0 else key('CIRCLE')
        return 0
    skip = {'done': False}
    def hook(p, f):
        if not skip['done'] and f > 2000:
            if var(p, EVENT) in (0, 6):
                p.write(VARS + 4 * EVENT, 6, 4); p.write(VARS + 4 * NEXT, 1, 4)
            else:
                skip['done'] = True
    run(psx, 12000, press, hook)
    run(psx, 8001, lambda f: (key('START') if (f // 60) % 3 == 0 else key('CIRCLE')) if f % 60 < 4 else 0)
    run(psx, 4001, lambda f: (key('CROSS') if f % 60 < 4 else 0) if f < 400 else (key('CIRCLE') if f % 60 < 4 else 0))


def deploy(psx):
    run(psx, len(taps('CROSS:40 R1:40')) + 1, taps('CROSS:40 R1:40'))
    s = taps('UP:30 CIRCLE:60 R1:40 DOWN:20 LEFT:30 CIRCLE:60 R1:40 UP:20 UP:30 CIRCLE:60 R1:40 R1:40 '
             'R1:40 LEFT:20 LEFT:30 CIRCLE:60')
    run(psx, len(s) + 1, s)
    def put(p, f):
        if f == 0:
            p.write(DEPLOY_GRID + 5 * 2 + 2, 3); p.write(DEPLOY_GRID + 5 * 2 + 1, 6); p.write(DEPLOY_COUNT, 5)
    run(psx, len(taps('W:30')) + 1, taps('W:30'), put)
    for t in ('START:60', 'CIRCLE:300'):
        run(psx, len(taps(t)) + 1, taps(t))


TURN_UNIT = 0x8018f520                            # g_battle_turn_unit_id


def first_turn(psx, log):
    seen = {'last': None, 'f': 0}
    def watch(p, f):
        seen['f'] += 1
        st = [p.read(TURN_UNIT)]
        for i in range(21):
            a = UNITS + UNIT_SIZE * i
            if p.read(a + 1) != 0xff and p.read(a + 0x2a, 2):
                st.append((i, p.read(a + 0x28, 2), p.read(a + 0x39), p.read(a + 0x47), p.read(a + 0x48)))
        if st != seen['last']:
            log.append('%5d turn %2d  ' % (seen['f'], st[0]) + ' '.join('%d:hp%d,ct%d,(%d,%d)' % u for u in st[1:]))
            seen['last'] = st
    run(psx, 5000, lambda f: key('CIRCLE') if f % 60 < 4 and f < 1700 else 0, watch)
    for t in ['CIRCLE:300', 'W:600'] + ['CIRCLE:150'] * 6:
        run(psx, len(taps(t)) + 1, taps(t), watch)


def unit_table(p):
    rows = []
    for i in range(21):
        a = UNITS + UNIT_SIZE * i
        r = lambda o, s=1: p.read(a + o, s)
        if r(0x01) == 0xff or (r(0x28, 2) == 0 and r(0x2a, 2) == 0):
            continue
        rows.append('%2d spr %02x id %02x job %02x team %02x lv %2d br %2d fa %2d hp %3d/%3d mp %3d/%3d pa %2d ma %2d '
                    'sp %2d ct %3d mv %d jp %d x %2d y %2d' % (
                        i, r(0), r(1), r(3), r(5), r(0x22), r(0x24), r(0x26), r(0x28, 2), r(0x2a, 2), r(0x2c, 2),
                        r(0x2e, 2), r(0x36), r(0x37), r(0x38), r(0x39), r(0x3a), r(0x3b), r(0x47), r(0x48)))
    return rows


IDS = [0, 1, 2, 3, 4, 5, 16, 17, 18, 19, 20]     # the battle's units in RAM order


def turns(psx, n):
    s = taps('DOWN:20 DOWN:30 CIRCLE:60 CIRCLE:60')   # Ramza's menu: Wait, then the facing
    run(psx, len(s) + 1, s)
    rows, last = [], {'ht': [0] * 21, 'row': None}
    def hook(p, f):
        for i in (16, 17, 18, 19, 20):
            a = UNITS + UNIT_SIZE * i + 5
            if p.read(a) & 8:
                p.write(a, p.read(a) & ~8)
        tu = p.read(TURN_UNIT)
        for i in IDS:
            h = p.read(UNITS + UNIT_SIZE * i + 0x186)
            if h and not last['ht'][i] and i == tu and len(rows) < n:
                cts = [p.read(UNITS + UNIT_SIZE * j + 0x39) for j in IDS]
                hps = [p.read(UNITS + UNIT_SIZE * j + 0x28, 2) for j in IDS]
                prev = last['row']
                # a flicker: the same unit again with no other unit's CT grown
                if not (prev and prev[0] == i and all(c <= d for k, (c, d) in enumerate(zip(cts, prev[1]))
                                                      if IDS[k] != i)):
                    rows.append((i, cts, hps))
                    last['row'] = rows[-1]
            last['ht'][i] = h
    f = 0
    while len(rows) < n and f < 60000:
        run(psx, 600, lambda k: key('CIRCLE') if k % 90 < 4 else 0, hook)
        f += 600
    return ['%2d  ' % IDS.index(i) + ' '.join('%3d' % c for c in cts) + '  ' + ' '.join('%3d' % h for h in hps)
            for i, cts, hps in rows]


PREVIEW = 0x8014d056                              # projected damage (s16), hit % (u16)
ATTACKER, TARGET = 16, 20                         # Ramza (4, 11), the recruit at (4, 12)


def attack_case(psx, c):
    """Poke one case (attacker PA, SP, Brave, weapon, sign, sex; target sign, sex, facing)."""
    a, t = UNITS + UNIT_SIZE * ATTACKER, UNITS + UNIT_SIZE * TARGET
    pa, sp, br, wp, az, af, tz, tf, face = c
    psx.write(a + 0x36, pa); psx.write(a + 0x38, sp); psx.write(a + 0x24, br); psx.write(a + 0x1d, wp)
    for u, z, f in ((a, az, af), (t, tz, tf)):
        psx.write(u + 9, (psx.read(u + 9) & 0x0F) | z << 4)
        psx.write(u + 6, (psx.read(u + 6) & 0x3F) | (0x40 if f else 0x80))
    psx.write(t + 0x49, (psx.read(t + 0x49) & 0xF0) | face)


def to_target(psx):
    s = taps('DOWN:30 CIRCLE:40 CIRCLE:60 CIRCLE:60 LEFT:40')
    run(psx, len(s) + 1, s)


def attacks(psx, state, n, m):
    import random
    rng, rows = random.Random(17), []
    for _ in range(n):
        c = (rng.randint(1, 15), rng.randint(3, 12), rng.randint(20, 97), rng.choice((0xFF, 0x01, 0x13)),
             rng.randrange(12), rng.randrange(2), rng.randrange(12), rng.randrange(2), rng.randrange(4))
        psx.load(state)
        attack_case(psx, c)
        to_target(psx)
        s = taps('CIRCLE:60'); run(psx, len(s) + 1, s)
        rows.append('case ' + ' '.join(map(str, c)) + '  %d %d' % (psx.read(PREVIEW + 2, 2), psx.read(PREVIEW, 2)))
    t = UNITS + UNIT_SIZE * TARGET
    for k in range(m):
        psx.load(state)
        to_target(psx)
        s = taps(' '.join(['RIGHT:8 LEFT:8'] * (k % 40)) + ' W:%d CIRCLE:60 CIRCLE:60 CIRCLE:300' % (1 + k // 40))
        hp = psx.read(t + 0x28, 2)
        run(psx, len(s) + 1, s)
        rows.append('trial %d %d' % (hp - psx.read(t + 0x28, 2), (psx.read(t + 0x47), psx.read(t + 0x48)) != (4, 12)))
    return rows


ITEMS = 0x800596e0                               # g_main_item_quantities: one byte per item id
POISON, DARK, SILENCE, DEAD, DEFEND = (3, 0x80), (1, 0x20), (1, 0x08), (0, 0x20), (0, 0x02)   # status byte, bit
GUTS = 'DOWN:30 CIRCLE:40 DOWN:20 CIRCLE:60 '    # Act, Ramza's own skill set (poked to 0x19)
ITEM = 'DOWN:30 CIRCLE:40 DOWN:20 DOWN:20 CIRCLE:60 '   # Act, Item (the secondary set, poked)
# name, menu entry, target pokes (HP, statuses); Guts: Accumulate Dash Throw Stone Heal Yell Wish
SKILLS = [('dash', GUTS, 1, None, ()), ('stone', GUTS, 2, None, ()), ('heal', GUTS, 3, None, (POISON, DARK)),
          ('heal-none', GUTS, 3, None, ()), ('wish', GUTS, 5, 10, ()), ('potion', ITEM, 0, 10, ()),
          ('antidote', ITEM, 6, None, (POISON,)), ('eye-drop-none', ITEM, 7, None, ()),
          ('echo-grass', ITEM, 8, None, (SILENCE,)), ('phoenix', ITEM, 13, 0, (DEAD,)),
          ('phoenix-alive', ITEM, 13, None, ()), ('stone-defend', GUTS, 2, None, (DEFEND,)),
          ('attack-defend', 'DOWN:30 CIRCLE:40 ', 0, None, (DEFEND,))]


def skill_setup(psx):
    """Ramza given every Gariland skill: primary set 0x19 (Squire's four, Yell, Wish), secondary
    Item with all fourteen items learned, Throw Item (range 4)."""
    a = UNITS + UNIT_SIZE * ATTACKER
    psx.write(a + 0x12, 0x19); psx.write(a + 0x13, 0x06)
    for k, v in enumerate((0xFC, 0, 0, 0xFF, 0xFC, 0)):
        psx.write(a + 0x99 + k, v)
    psx.write(a + 0x91, psx.read(a + 0x91) | 0x08)


def skill_state(psx):
    a, t = UNITS + UNIT_SIZE * ATTACKER, UNITS + UNIT_SIZE * TARGET
    return [psx.read(t + 0x28, 2), *(psx.read(t + 0x58 + k) for k in range(5)), psx.read(t + 0x47), psx.read(t + 0x48),
            psx.read(a + 0x28, 2), psx.read(ITEMS + 0xF0), psx.read(ITEMS + 0xF6), psx.read(ITEMS + 0xF7),
            psx.read(ITEMS + 0xF8), psx.read(ITEMS + 0xFD)]


def skills(psx, state, m):
    rows = []
    t = UNITS + UNIT_SIZE * TARGET
    for name, menu, k, hp, sts in SKILLS:
        psx.load(state)
        skill_setup(psx)
        if hp is not None:
            psx.write(t + 0x28, hp, 2)
        for b, bit in sts:                      # current (+0x58) and inflicted (+0x1bb: what a cure removes)
            for o in (0x58, 0x1bb):
                psx.write(t + o + b, psx.read(t + o + b) | bit)
        s = taps(menu + 'DOWN:20 ' * k + 'CIRCLE:60 CIRCLE:60 LEFT:40 CIRCLE:60')
        run(psx, len(s) + 1, s)
        pv = (psx.read(PREVIEW + 2, 2), psx.read(PREVIEW, 2))
        before = skill_state(psx)
        s = taps('CIRCLE:60 CIRCLE:600'); run(psx, len(s) + 1, s)   # Wish's cost lands after ~5 s
        rows.append('skill %s  %d %d  %s  %s' % (name, pv[0], pv[1], ' '.join(map(str, before)),
                                                 ' '.join(map(str, skill_state(psx)))))
    for name, k in (('dash', 1), ('stone', 2)):
        for j in range(m):
            psx.load(state)
            skill_setup(psx)
            s = taps(GUTS + 'DOWN:20 ' * k + 'CIRCLE:60 CIRCLE:60 LEFT:40 ' + ' '.join(['RIGHT:8 LEFT:8'] * (j % 40))
                     + ' W:%d CIRCLE:60 CIRCLE:60 CIRCLE:300' % (1 + j // 40))
            hp = psx.read(t + 0x28, 2)
            run(psx, len(s) + 1, s)
            rows.append('trial %s %d %d' % (name, hp - psx.read(t + 0x28, 2),
                                            (psx.read(t + 0x47), psx.read(t + 0x48)) != (4, 12)))
    return rows


def main():
    if sys.argv[1] == '--skills':
        out = os.dup(1)
        os.dup2(os.open(os.devnull, os.O_WRONLY), 1)
        psx = PSX(sys.argv[2])
        rows = skills(psx, sys.argv[3], int(sys.argv[4]))
        open(sys.argv[5], 'w').write(
            '# skill NAME  preview damage (HP; a heal shown as such) hit%  then before and after the\n'
            '# execution: target HP, status bytes 0-4, x, y; Ramza HP; Potion, Antidote, Eye Drop,\n'
            '# Echo Grass, Phoenix Down in the bag\n# trial NAME: HP taken, pushed\n' + '\n'.join(rows) + '\n')
        os.write(out, ('%d rows\n' % len(rows)).encode())
        return
    if sys.argv[1] == '--attack':
        out = os.dup(1)
        os.dup2(os.open(os.devnull, os.O_WRONLY), 1)
        psx = PSX(sys.argv[2])
        rows = attacks(psx, sys.argv[3], int(sys.argv[4]), int(sys.argv[5]))
        open(sys.argv[6], 'w').write('# case: attacker PA SP Brave weapon sign female, target sign female facing (FFT)'
                                     '  hit% damage\n# trial: HP taken, pushed\n' + '\n'.join(rows) + '\n')
        os.write(out, ('%d rows\n' % len(rows)).encode())
        return
    if sys.argv[1] == '--turns':
        out = os.dup(1)
        os.dup2(os.open(os.devnull, os.O_WRONLY), 1)
        psx = PSX(sys.argv[2])
        psx.load(sys.argv[3])
        rows = turns(psx, int(sys.argv[4]))
        open(sys.argv[5], 'w').write('# unit (battle.h order), CT x 11 after the pick, HP x 11\n' + '\n'.join(rows) + '\n')
        os.write(out, ('%d turns\n' % len(rows)).encode())
        return
    out = os.dup(1)
    os.dup2(os.open(os.devnull, os.O_WRONLY), 1)      # the core prints to stdout
    psx = PSX(sys.argv[1])
    log = []
    boot(psx); deploy(psx); first_turn(psx, log)
    psx.save(sys.argv[2])
    psx.image().save(os.path.splitext(sys.argv[2])[0] + '.png')
    t = '\n'.join(unit_table(psx)) + '\n'
    if len(sys.argv) > 3:
        open(sys.argv[3], 'w').write(t + '\n'.join(log) + '\n')
    os.write(out, t.encode())


if __name__ == '__main__':
    main()
