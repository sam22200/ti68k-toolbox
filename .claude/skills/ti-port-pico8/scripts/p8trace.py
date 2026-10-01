#!/usr/bin/env python3
"""Run the original PICO-8 cart headless under z8lua and print a state trace per frame:
the reference the C port's unit tests compare against (no PICO-8 licence, no screen).

usage: p8trace.py DIR --frames N --trace EXPR [--keys FILE] [--pre CODE] [--lib FILE]
                  [--seed N] [--map A=4,B=5] [--every K] [--no-draw]

DIR       output of p8extract.py (code.lua, mem.bin)
--trace   Lua expression(s) evaluated after each frame, e.g. "player_x(), room.x, freeze";
          numbers print as exact 16.16 hex (tostr(v,true)), so the C port can print the same
          bits and a diff compares them; "__sfx" lists the sounds played in the frame
--keys    the runtime's input script format: lines "<frame> <keys...>" held until the next
          line, keys UP DOWN LEFT RIGHT A B C D (as games/*/keys files and --keys on the PC)
--map     runtime key -> PICO-8 button (default A=4 (O), B=5 (X), C=4, D=5)
--pre     Lua code run once after _init (state injection, e.g. "load_room(3,1)")
--lib     Lua file loaded after the shim, before the cart (override a function: the port's
          own sin table, a deterministic rnd the port also uses, a helper for --trace)
--seed    srand(N) before the cart runs (default: the shim's srand(0), as p8num.h)
--every   print one frame in K (default 1)
--no-draw do not call _draw (by default it runs: carts often change state in _draw)
Exit status 1 on a Lua error, with the frame number printed.
"""
import argparse, os, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../..'))
Z8LUA = os.path.join(ROOT, 'tools/z8lua/z8lua')
SHIM = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'p8shim.lua')
NAMES = ['UP', 'LEFT', 'DOWN', 'RIGHT', 'A', 'B', 'C', 'D']
P8 = {'LEFT': 0, 'RIGHT': 1, 'UP': 2, 'DOWN': 3}


def lua_str(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('dir')
    ap.add_argument('--frames', type=int, required=True)
    ap.add_argument('--trace', required=True)
    ap.add_argument('--keys')
    ap.add_argument('--pre', default='')
    ap.add_argument('--lib')
    ap.add_argument('--seed', type=int)
    ap.add_argument('--map', default='A=4,B=5,C=4,D=5')
    ap.add_argument('--every', type=int, default=1)
    ap.add_argument('--no-draw', action='store_true')
    a = ap.parse_args()
    if not os.path.exists(Z8LUA):
        sys.exit('z8lua missing: git clone https://github.com/samhocevar/z8lua tools/z8lua && make -C tools/z8lua')

    bmap = dict(P8)
    for kv in a.map.split(','):
        k, v = kv.split('=')
        bmap[k.strip().upper()] = int(v)
    script = []                                   # (frame, [pico-8 buttons])
    if a.keys:
        for line in open(a.keys):
            w = line.split('#')[0].split()
            if not w or not w[0].isdigit():
                continue
            script.append((int(w[0]), sorted({bmap[k] for k in w[1:] if k in bmap})))
    mem = open(os.path.join(a.dir, 'mem.bin'), 'rb').read().hex()
    code = open(os.path.join(a.dir, 'code.lua')).read()
    lib = open(a.lib).read() if a.lib else ''

    keys_lua = ','.join('{%d,{%s}}' % (f, ','.join(map(str, b))) for f, b in script)
    runner = f'''
__fps = _update60 and 60 or 30
__frame = 0
local __keys, __kp = {{{keys_lua}}}, 1
local function __fmt(v)
  if type(v) == "number" then return tostr(v, true) end
  if type(v) == "table" then
    local s = ""
    for i = 1, #v do s = s .. (i > 1 and "," or "") .. __fmt(v[i]) end
    return "{{" .. s .. "}}"
  end
  return tostr(v)
end
local function __trace() return {a.trace} end
if _init then _init() end
{a.pre}
for f = 0, {a.frames - 1} do
  __frame = f
  while __kp + 1 <= #__keys and __keys[__kp + 1][1] <= f do __kp += 1 end
  __held = {{}}
  if #__keys > 0 and __keys[__kp][1] <= f then
    for b in all(__keys[__kp][2]) do __held[b] = true end
  end
  for b = 0, 5 do __hold[b] = __held[b] and (__hold[b] or 0) + 1 or 0 end
  __sfx = {{}}
  local ok, err = pcall(function()
    if _update60 then _update60() elseif _update then _update() end
    {'' if a.no_draw else 'if _draw then _draw() end'}
  end)
  if not ok then __out("error at frame " .. f .. ": " .. tostr(err)) __exit = 1 break end
  if f % {a.every} == 0 then
    local r = table.pack(__trace())
    local s = tostr(f)
    for i = 1, r.n do s = s .. " " .. __fmt(r[i]) end
    __out(s)
  end
end
if __exit then error("trace stopped") end
'''
    seed = f'srand({a.seed})\n' if a.seed is not None else ''
    full = (open(SHIM).read() + '\n' + f'__mem_load({lua_str(mem)})\n' + seed + lib + '\n'
            + code + '\n' + runner)
    with tempfile.NamedTemporaryFile('w', suffix='.lua', delete=False) as t:
        t.write(full)
    r = subprocess.run([Z8LUA, t.name], stdout=sys.stdout, stderr=subprocess.PIPE, text=True)
    if r.returncode:
        sys.stderr.write(r.stderr.replace(t.name, 'p8trace'))
        sys.stderr.write(f'(combined script kept in {t.name})\n')
        sys.exit(1)
    os.unlink(t.name)


if __name__ == '__main__':
    main()
