#!/usr/bin/env python3
"""Run the original Game Boy ROM headless under PyBoy and print a RAM trace per frame: the
reference the C port's unit tests compare against. Also the screen (PNG per frame, GIF).

usage: gbtrace.py ROM --frames N [--trace VARS] [--sym FILE] [--keys FILE] [--map A=a,...]
                  [--load STATE] [--save STATE] [--poke ADDR=VAL,...] [--every K]
                  [--shot FRAME:FILE.png,...] [--gif FILE.gif] [--diff] [--from F]

--trace  comma list of variables: a hex address (c0ad), a range (c0ad-c0b4, one byte each),
         ADDR:2 for a little-endian 16-bit word, a name from --sym (also name:2), or
         LO-HI:h for the Fletcher-16 of a range (a collision mask, a map: one field);
         each frame prints "<frame> name=hex ..." (the C port prints the same, a diff compares)
--sym    symbol file "BANK:ADDR name" lines (rgbds .sym, written by hand as the game is
         understood: games/<name>/<name>.sym); names usable in --trace
--keys   the runtime's input script: lines "<frame> <keys...>" held until the next line, keys
         UP DOWN LEFT RIGHT A B C D (as games/*/keys and --keys on the PC); frame 0 = first tick
--map    runtime key -> GB button (default A=a,B=b,C=start,D=select)
--load   PyBoy state to start from (the injection door: a room, a level, saved with --save)
--poke   bytes written before the first frame (state injection by RAM: c0b0=3,c0b1=0)
--diff   print only the variables that changed since the previous printed frame
--from   start printing at frame F (skip the boot and title)
--shot   PNG of the screen after the given frames (160x144, the 4 GB shades); with --logic,
         the picture drawn during that logic frame, i.e. the sprites and BG of the frame before
--gif    animated GIF of the whole run (every --every-th frame), for ti-view or a review
--logic  hex address in bank 0 reached once per logic frame, after the game's update (Bubble
         Ghost: 0283): frames are then counted at that address instead of per VBlank, --frames
         and --keys count logic frames, and the trace prints there. Loading, fades, intros and
         other blocking sequences take no frame: the C port's game_update() matches one to one.
         Keys of frame n+1 are set when frame n is sampled, applied before the next tick (PyBoy
         drops inputs sent from a hook in the middle of a tick).
--stall  with --logic: N VBlanks without a logic frame (game over, ending screens) end the run
         with a last line "# stalled at <frame>" (exit 0); without it, that is an error
Frame = one VBlank (59.73 Hz) without --logic.
"""
import argparse, os, sys, warnings

warnings.filterwarnings('ignore')
from pyboy import PyBoy  # noqa: E402

NAMES = ['UP', 'DOWN', 'LEFT', 'RIGHT', 'A', 'B', 'C', 'D']
GB = {'UP': 'up', 'DOWN': 'down', 'LEFT': 'left', 'RIGHT': 'right'}


def load_sym(path):
    sym = {}
    for line in open(path):
        w = line.split(';')[0].split()
        if len(w) >= 2 and ':' in w[0]:
            sym[w[1]] = int(w[0].split(':')[1], 16)
    return sym


def parse_vars(spec, sym):
    out = []
    for item in filter(None, (s.strip() for s in spec.split(','))):
        size = 1
        if ':' in item:
            item, size = item.split(':')
        if size == 'h':
            lo, hi = (int(x, 16) for x in item.split('-'))
            out.append((item.lower(), lo, -(hi - lo + 1)))
            continue
        size = int(size)
        if item in sym:
            out.append((item, sym[item], size))
        elif '-' in item:
            lo, hi = (int(x, 16) for x in item.split('-'))
            out += [('%04x' % a, a, 1) for a in range(lo, hi + 1)]
        else:
            out.append((item.lower(), int(item, 16), size))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('rom')
    ap.add_argument('--frames', type=int, required=True)
    ap.add_argument('--trace', default='')
    ap.add_argument('--sym')
    ap.add_argument('--keys')
    ap.add_argument('--map', default='A=a,B=b,C=start,D=select')
    ap.add_argument('--load')
    ap.add_argument('--save')
    ap.add_argument('--poke', default='')
    ap.add_argument('--poke-at')
    ap.add_argument('--boot-keys', default='')
    ap.add_argument('--every', type=int, default=1)
    ap.add_argument('--from', dest='start', type=int, default=0)
    ap.add_argument('--diff', action='store_true')
    ap.add_argument('--shot', default='')
    ap.add_argument('--gif')
    ap.add_argument('--logic')
    ap.add_argument('--max-ticks', type=int, default=200000)
    ap.add_argument('--stall', type=int, default=0)
    a = ap.parse_args()

    bmap = dict(GB)
    for kv in a.map.split(','):
        k, v = kv.split('=')
        bmap[k.strip().upper()] = v.strip()
    script = []
    if a.keys:
        for line in open(a.keys):
            w = line.split('#')[0].split()
            if w and w[0].isdigit():
                bad = [k for k in w[1:] if k.upper() not in NAMES]
                if bad:
                    sys.exit('gbtrace: unknown key %s in %s' % (bad, a.keys))
                script.append((int(w[0]), {bmap[k.upper()] for k in w[1:]}))
    sym = load_sym(a.sym) if a.sym else {}
    tvars = parse_vars(a.trace, sym)
    shots = {}
    for s in filter(None, a.shot.split(',')):
        f, path = s.split(':', 1)
        shots[int(f)] = path

    gb = PyBoy(a.rom, window='null', sound_emulated=False)
    if a.load:
        with open(a.load, 'rb') as f:
            gb.load_state(f)
    pokes = [(int(k, 16), int(v, 0)) for k, v in
             (kv.split('=') for kv in filter(None, a.poke.split(',')))]

    pending = []    # key changes asked inside a hook: PyBoy drops inputs sent mid-tick

    def tick():
        while pending:
            pending.pop(0)()
        gb.tick()

    boot = {bmap[k.upper()] for k in a.boot_keys.split()} if a.boot_keys else set()
    for b in boot:
        gb.button_press(b)

    def do_pokes(done):
        if not done:
            for k, v in pokes:
                gb.memory[k] = v
            pending.append(lambda: [gb.button_release(b) for b in boot])
            pending.append(lambda: set_keys(0))
            st['poked'] = 1
            done.append(1)

    st = {'held': set(), 'si': 0, 'prev': None, 'frame': 0}
    frames = []

    def set_keys(fr):
        while st['si'] < len(script) and script[st['si']][0] <= fr:
            want = script[st['si']][1]
            for b in st['held'] - want:
                gb.button_release(b)
            for b in want - st['held']:
                gb.button_press(b)
            st['held'], st['si'] = want, st['si'] + 1

    grab = []      # --logic: screens saved after the tick (a hook runs mid-frame, while the PPU
                   # draws: the buffer then mixes new and old lines)

    def shoot(fr):
        if fr in shots:
            gb.screen.image.convert('RGB').save(shots[fr])
        if a.gif and fr % a.every == 0:
            frames.append(gb.screen.image.convert('P'))

    def sample(fr):
        if a.logic:
            grab.append(fr)
        else:
            shoot(fr)
        if tvars and fr >= a.start and (fr - a.start) % a.every == 0:
            vals = []
            for name, addr, size in tvars:
                if size < 0:
                    ha = hb = 0
                    for k in range(-size):
                        ha = (ha + gb.memory[addr + k]) % 255
                        hb = (hb + ha) % 255
                    vals.append((name, '%04x' % (hb << 8 | ha)))
                    continue
                v = gb.memory[addr] | (gb.memory[addr + 1] << 8 if size == 2 else 0)
                vals.append((name, '%0*x' % (2 * size, v)))
            prev = st['prev']
            if not a.diff or vals != prev:
                shown = vals if not a.diff or prev is None else \
                    [v for v, p in zip(vals, prev) if v != p]
                print(fr, ' '.join('%s=%s' % v for v in shown))
            st['prev'] = vals

    if a.poke_at:
        gb.hook_register(0, int(a.poke_at, 16), do_pokes, [])
    else:
        do_pokes([])

    if a.logic:
        def on_logic(_):
            if a.poke_at and not st.get('poked'):   # the scenario starts at --poke-at
                return
            fr = st['frame']
            if fr < a.frames:
                sample(fr)
                st['frame'] = fr + 1
                pending.append(lambda: set_keys(fr + 1))
        gb.hook_register(0, int(a.logic, 16), on_logic, None)
        ticks = idle = 0
        while st['frame'] < a.frames and ticks < a.max_ticks:
            before = st['frame']
            tick()
            ticks += 1
            while grab:
                shoot(grab.pop(0))
            idle = 0 if st['frame'] != before else idle + 1
            if a.stall and idle >= a.stall:
                print('# stalled at %d' % st['frame'])
                break
        if st['frame'] < a.frames and not (a.stall and idle >= a.stall):
            sys.exit('gbtrace: %d logic frames after %d ticks (game over, pause, wrong --logic?)'
                     % (st['frame'], ticks))
    else:
        for fr in range(a.frames):
            if fr:
                set_keys(fr)
            tick()
            sample(fr)
    if a.save:
        with open(a.save, 'wb') as f:
            gb.save_state(f)
    if a.gif and frames:
        frames[0].save(a.gif, save_all=True, append_images=frames[1:], loop=0,
                       duration=int(1000 * a.every / 59.73))
    gb.stop(save=False)


if __name__ == '__main__':
    main()
