-- p8shim.lua: the PICO-8 API without a screen, for running a cart headless under z8lua
-- (PICO-8 syntax and 16.16 fixed point). p8trace.py prepends it to the cart and appends the
-- frame loop. Drawing calls are no-ops that only keep the state a game may read back (camera,
-- palette); memory, map, flags, buttons, rnd and the table helpers behave like PICO-8.

__out = print   -- the host print, before the cart's print is defined below

-- memory: 0x0000 sheet, 0x1000 shared sheet/map, 0x2000 map, 0x3000 flags, 0x4300 user,
-- 0x5e00 cartdata; filled from mem.bin by __mem_load
__mem = {}
function __mem_load(hex)
  for i = 0, #hex / 2 - 1 do
    __mem[i] = tonum("0x" .. sub(hex, 2 * i + 1, 2 * i + 2))
  end
  __rom = {}
  for i = 0, 0x30ff do __rom[i] = __mem[i] end
end
function peek(a, n)
  a = flr(a)
  if n and n > 1 then
    local r = {}
    for i = 0, n - 1 do r[i + 1] = __mem[a + i] or 0 end
    return table.unpack(r)
  end
  return __mem[a] or 0
end
function poke(a, ...)
  a = flr(a)
  for i, v in ipairs({...}) do __mem[a + i - 1] = band(v, 0xff) end
end
function peek2(a) return peek(a) + shl(peek(a + 1), 8) end
function poke2(a, v) poke(a, band(v, 0xff), band(lshr(v, 8), 0xff)) end
function peek4(a) return bor(peek2(a + 2), lshr(peek2(a), 16)) end  -- 16.16, little-endian
function poke4(a, v) poke2(a, shl(v, 16)) poke2(a + 2, v) end
function memcpy(d, s, n)
  local t = {}
  for i = 0, n - 1 do t[i] = __mem[s + i] or 0 end
  for i = 0, n - 1 do __mem[d + i] = t[i] end
end
function memset(d, v, n) for i = 0, n - 1 do __mem[d + i] = v end end
function reload(d, s, n)
  d, s, n = d or 0, s or 0, n or 0x4300
  for i = 0, n - 1 do __mem[d + i] = __rom[s + i] or 0 end
end
function cstore() end

-- map and sprites (out of range: mget 0, sget 0, writes ignored)
local function maddr(x, y)
  x, y = flr(x), flr(y)
  if x < 0 or x > 127 or y < 0 or y > 63 then return nil end
  if y < 32 then return 0x2000 + y * 128 + x end
  return 0x1000 + (y - 32) * 128 + x
end
function mget(x, y) local a = maddr(x, y) return a and __mem[a] or 0 end
function mset(x, y, v) local a = maddr(x, y) if a then __mem[a] = band(v, 0xff) end end
function fget(n, f)
  local v = __mem[0x3000 + band(flr(n), 0xff)] or 0
  if f == nil then return v end
  return band(v, shl(1, f)) != 0
end
function fset(n, f, v)
  local a = 0x3000 + band(flr(n), 0xff)
  if v == nil then __mem[a] = f return end
  local m = shl(1, f)
  __mem[a] = v and bor(__mem[a], m) or band(__mem[a], bnot(m))
end
function sget(x, y)
  x, y = flr(x), flr(y)
  if x < 0 or x > 127 or y < 0 or y > 127 then return 0 end
  local b = __mem[y * 64 + flr(x / 2)]
  return x % 2 == 0 and band(b, 15) or band(lshr(b, 4), 15)
end
function sset(x, y, c)
  x, y = flr(x), flr(y)
  if x < 0 or x > 127 or y < 0 or y > 127 then return end
  local a = y * 64 + flr(x / 2)
  local b = __mem[a]
  if x % 2 == 0 then __mem[a] = bor(band(b, 0xf0), band(c, 15))
  else __mem[a] = bor(band(b, 0x0f), shl(band(c, 15), 4)) end
end

-- drawing: state only
__cam_x, __cam_y, __pal = 0, 0, {}
function camera(x, y)
  local ox, oy = __cam_x, __cam_y
  __cam_x, __cam_y = flr(x or 0), flr(y or 0)
  return ox, oy
end
function pal(a, b)
  if a == nil then __pal = {} elseif type(a) == "table" then for k, v in pairs(a) do __pal[k] = v end
  else __pal[a] = b end
end
function palt() end
function cls() end
function spr() end
function sspr() end
function map() end
mapdraw = map
function tline() end
function pset() end
function pget() return 0 end
function rect() end
function rectfill() end
function circ() end
function circfill() end
function oval() end
function ovalfill() end
function line() end
function print(s, x) return (x or 0) + 4 * #tostr(s) end
function cursor() end
function color() end
function fillp() end
function clip() end
function flip() end
function sfx(n) add(__sfx, n) end
function music(n) add(__sfx, "m" .. tostr(n)) end
function menuitem() end
function extcmd() end
function printh(s) __out(tostr(s)) end
function stat(n) return 0 end
function time() return __frame / __fps end
t = time
function cartdata() return false end
function dget(i) return peek4(0x5e00 + 4 * i) end
function dset(i, v) poke4(0x5e00 + 4 * i, v) end

-- buttons: __held[i] for this frame (set by the loop), btnp with PICO-8's repeat
-- (pressed, then after 15 frames every 4 frames, counted in 30 fps frames)
__held, __hold = {}, {}
function btn(i, p)
  if p and p != 0 then return i == nil and 0 or false end
  if i == nil then
    local m = 0
    for b = 0, 5 do if __held[b] then m = bor(m, shl(1, b)) end end
    return m
  end
  return __held[i] == true
end
function btnp(i, p)
  if p and p != 0 then return false end
  local n = __hold[i] or 0
  return n == 1 or (n > 15 and (n - 16) % 4 == 0)
end

-- rnd/srand: PICO-8's own generator (decompiled in zepto8 src/pico8/vm.cpp, same in ccleste):
-- step: a = rotl(a, 16) + b; b += a (u32); rnd(x) = a % bits(x) (unsigned), rnd() = rnd(1);
-- srand(s): b = bits(s) & 0x7fffffff (0 -> 0xdeadbeef), a = b ^ 0xbead29ba, 32 steps.
-- PICO-8 seeds at random on start; here the start state is srand(0), as p8num.h.
-- Unsigned modulo on 16.16 values is exact for x < 16384 (bits < 2^30), rnd(negative) unsupported.
function srand(x)
  __rb = band(x or 0, 0x7fff.ffff)
  if __rb == 0 then __rb = 0xdead.beef end
  __ra = bxor(__rb, 0xbead.29ba)
  for i = 1, 32 do __ra = rotl(__ra, 16) + __rb __rb += __ra end
end
srand(0)
function rnd(x)
  if type(x) == "table" then
    if #x == 0 then return nil end
    return x[flr(rnd(#x)) + 1]
  end
  x = x or 1
  if x == 0 then return 0 end
  __ra = rotl(__ra, 16) + __rb
  __rb += __ra
  local a = __ra
  if a >= 0 then return a % x end
  local m = lshr(a, 1) % x                -- a = 2 (a >>> 1) + bit 0, as unsigned
  m = m + m + band(a, 0x0.0001)
  if m >= x then m -= x end
  return m
end

-- z8lua sin/cos return 1.1055 instead of 1 at the exact quarter turns (table read one past its
-- end, verified 2026-10-01): clamp, the true value there is +-1
do
  local s0, c0 = sin, cos
  sin = function(a) local r = s0(a) if r > 1 then return 1 elseif r < -1 then return -1 end return r end
  cos = function(a) local r = c0(a) if r > 1 then return 1 elseif r < -1 then return -1 end return r end
end

-- table helpers (PICO-8 bios semantics; all/foreach survive deleting the current element)
function add(t, v, i)
  if t == nil then return end
  if i then
    for k = #t, i, -1 do t[k + 1] = t[k] end
    t[i] = v
  else t[#t + 1] = v end
  return v
end
function del(t, v)
  if t == nil then return end
  for i = 1, #t do
    if t[i] == v then
      for k = i, #t - 1 do t[k] = t[k + 1] end
      t[#t] = nil
      return v
    end
  end
end
function deli(t, i)
  if t == nil then return end
  i = i or #t
  local v = t[i]
  if v == nil then return end
  for k = i, #t - 1 do t[k] = t[k + 1] end
  t[#t] = nil
  return v
end
function count(t, v)
  if t == nil then return 0 end
  if v == nil then return #t end
  local n = 0
  for i = 1, #t do if t[i] == v then n += 1 end end
  return n
end
function all(t)
  if t == nil then return function() end end
  local i, n = 0, #t
  return function()
    if #t >= n then i += 1 end   -- the current element was deleted: the next one is at i
    n = #t
    return t[i]
  end
end
function foreach(t, f) for v in all(t) do f(v) end end
function sub(s, i, j) return string.sub(s, i or 1, j) end
cocreate, coresume, costatus, yield = coroutine.create, coroutine.resume, coroutine.status, coroutine.yield
