#!/usr/bin/env python3
"""The reference: FFT run headless (psxrun's pcsx_rearmed core) from a new game to Ramza's first
turn at Gariland, saved as a core state (never committed). RE_NOTES.md § Battle oracle.

usage: oracle.py DISC.cue OUT.state [trace.txt]
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


def main():
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
