#!/usr/bin/env python3
"""Measure Celeste Classic's movement numbers on the original cart (z8lua, p8trace.py).
usage: measure.py XDIR OUTDIR   (XDIR = p8extract.py output). Prints a table, writes traces."""
import os, subprocess, sys
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../..'))
PY = ROOT + '/tools/pyenv/bin/python'
TRACE = ROOT + '/.claude/skills/ti-port-pico8/scripts/p8trace.py'
X, OUT = sys.argv[1], sys.argv[2]
LIB = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lib.lua')
# player placed directly (no spawn animation) in room (rx,ry) at x,y
def pre(rx, ry, x, y):
    return (f"begin_game() load_room({rx},{ry}) for o in all(objects) do if o.type==player_spawn "
            f"then destroy_object(o) end end init_object(player,{x},{y})")

def fx(h):
    v = int(h.replace('0x', '').replace('.', ''), 16)
    return (v - (1 << 32) if v >= 1 << 31 else v) / 65536

def run(name, keys, frames, prestr, trace="px(), py(), pvx(), pvy(), pdj(), nobj()"):
    kf = os.path.join(OUT, 'keys', name + '.txt')
    open(kf, 'w').write(keys)
    r = subprocess.run([PY, TRACE, X, '--frames', str(frames), '--trace', trace, '--keys', kf,
                        '--lib', LIB, '--pre', prestr], capture_output=True, text=True, check=True)
    open(os.path.join(OUT, 'traces', name + '.txt'), 'w').write(r.stdout)
    return [[fx(w) if w.startswith('0x') else w for w in l.split()[1:]] for l in r.stdout.splitlines()]

# lab room: room (0,0) emptied, floor of tile 32 (solid) on rows 14-15, player at x=8 on it
LAB = ("for ty=0,15 do for tx=0,15 do mset(tx,ty,ty>=14 and 32 or 0) end end ")
P0 = LAB + pre(0, 0, 8, 104)
# settle 10 frames, then act
rows = run('m_run', "0\n10 RIGHT\n40\n", 60, P0)
x0 = rows[9][0]
acc = next(i for i in range(10, 60) if rows[i][2] >= 1) - 9
print(f"run: max speed {max(r[2] for r in rows)} px/frame, frames from rest to max {acc}, "
      f"x after 30 frames held {rows[39][0]-x0:+.2f} px")
stop = next(i for i in range(40, 60) if rows[i][2] == 0) - 39
print(f"stop from run (ground, release): {stop} frames")

rows = run('m_jump', "0\n10 A\n11\n", 60, P0)
y0 = rows[9][1]
apex = min(r[1] for r in rows[10:])
land = next(i for i in range(11, 60) if rows[i][1] == y0) - 9
print(f"jump (tap): apex {y0-apex:.0f} px, frames to apex {[r[1] for r in rows].index(apex)-9}, airtime {land} frames")
rows = run('m_jump_hold', "0\n10 A\n50\n", 60, P0)
print(f"jump (held): apex {y0-min(r[1] for r in rows[10:]):.0f} px (no variable jump if equal)")
rows = run('m_runjump', "0\n10 RIGHT\n40 RIGHT A\n41 RIGHT\n", 80, P0)
lx = rows[39][0]; ly = rows[39][1]
landf = next(i for i in range(41, 80) if rows[i][1] == ly)
print(f"running jump: horizontal distance {rows[landf][0]-lx:.0f} px over {landf-39} frames")

for name, k in [('m_dash_r', 'RIGHT B'), ('m_dash_u', 'UP B'), ('m_dash_ur', 'UP RIGHT B'), ('m_dash_none', 'B')]:
    # dash from the air: jump first, dash at the 3rd frame of the jump
    keys = f"0\n10 A\n11\n13 {k}\n14\n"
    rows = run(name, keys, 40, P0)
    sx, sy = rows[12][0], rows[12][1]
    ex, ey = rows[22][0], rows[22][1]
    print(f"{name}: 10 frames after the dash press dx={ex-sx:+.1f} dy={ey-sy:+.1f}; max objects {int(max(r[5] for r in rows))}; speeds f13..f18 "
          + ' '.join(f"({r[2]:.2f},{r[3]:.2f})" for r in rows[13:19]))

rows = run('m_fall', "0\n", 40, LAB + pre(0, 0, 40, 0))
vs = [r[3] for r in rows]
print(f"fall from rest: terminal speed {max(vs)} px/frame reached at frame {vs.index(max(vs))}")
