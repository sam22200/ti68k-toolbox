#!/usr/bin/env python3
"""Record a key script that pushes the bubble along waypoints, by playing the ROM under PyBoy:
bot.py HALL "x,y x,y ..." FRAMES OUT.txt [--entry D]
Each logic frame (0283), the ghost goes to the spot 17 px (12 diagonally) behind the bubble opposite the next
waypoint and blows once it is there; a waypoint is reached within 6 px. The keys of every logic
frame are written as a runtime key script (the trace tests replay it), with the run's outcome."""
import sys, warnings
warnings.filterwarnings('ignore')
from pyboy import PyBoy

hall = int(sys.argv[1])
way = [tuple(int(v, 0) for v in p.split(',')) for p in sys.argv[2].split()]
frames, out = int(sys.argv[3]), sys.argv[4]
entry = int(sys.argv[sys.argv.index('--entry') + 1]) if '--entry' in sys.argv else (0 if hall == 1 else 3)

gb = PyBoy('../../roms/gb/Bubble_Ghost.gb', window='null', sound_emulated=False)
gb.load_state(open('build/pre.state', 'rb'))
m = gb.memory
st = {'poked': 0, 'fr': 0, 'wp': 0, 'keys': [], 'end': None}
pend = []
held = set()


def press(want):
    global held
    for b in held - want:
        gb.button_release(b)
    for b in want - held:
        gb.button_press(b)
    held = want


def poke(_):
    if st['poked']:
        return
    st['poked'] = 1
    m[0xC0AC], m[0xC0B0], m[0xC0B3] = hall + 1, entry, 0
    pend.append(lambda: press(set()))


def sgn(v, dead=1):
    return 0 if abs(v) <= dead else (1 if v > 0 else -1)


def logic(_):
    if not st['poked']:
        return
    bx, by, gx, gy = m[0xC16A], m[0xC16B], m[0xC161], m[0xC162]
    if m[0xC0F3]:
        st['end'] = (st['fr'], m[0xC0F3], m[0xC0AC])
    while st['wp'] < len(way) and abs(bx - way[st['wp']][0]) <= 6 and abs(by - way[st['wp']][1]) <= 6:
        st['wp'] += 1
    keys = set()
    if st['wp'] < len(way) and m[0xC168] < 2 and m[0xC15F] < 2:
        tx, ty = way[st['wp']]
        dx, dy = sgn(tx - bx, 2), sgn(ty - by, 2)
        r = 12 if dx and dy else 17                            # near: dx2 + dy2 < 484, not 256
        sx, sy = bx - r * dx, by - r * dy                      # the blowing spot
        mx, my = sgn(sx - gx), sgn(sy - gy)
        if mx > 0: keys.add('right')
        if mx < 0: keys.add('left')
        if my > 0: keys.add('down')
        if my < 0: keys.add('up')
        if abs(sx - gx) <= 2 and abs(sy - gy) <= 2 and (dx or dy):
            keys = {'a'}
    st['keys'].append(keys)
    st['fr'] += 1
    k = keys
    pend.append(lambda: press(k))


gb.hook_register(0, 0x0223, poke, None)
gb.hook_register(0, 0x0283, logic, None)
gb.button_press('a')
held = {'a'}
n = 0
while st['fr'] < frames and n < frames * 4 + 3000:
    while pend:
        pend.pop(0)()
    gb.tick()
    n += 1
names = {'up': 'UP', 'down': 'DOWN', 'left': 'LEFT', 'right': 'RIGHT', 'a': 'A'}
with open(out, 'w') as f:
    f.write('# hall %d, bot along %s (tools/bot.py): %s\n' % (hall, sys.argv[2], st['end']))
    prev = None                        # keys decided at the end of frame i drive frame i + 1
    for i, k in enumerate([set()] + st['keys']):
        if k != prev:
            f.write(('%d %s' % (i, ' '.join(sorted(names[x] for x in k)))).strip() + '\n')
            prev = k
print('waypoints reached %d/%d, end %s, %d frames' % (st['wp'], len(way), st['end'], st['fr']))
