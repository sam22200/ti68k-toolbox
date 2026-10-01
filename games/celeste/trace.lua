-- trace.lua: --lib for p8trace.py. __st() returns the per-frame gameplay state of the cart in
-- the fixed field order that test_celeste.c prints (see FIELDS there). Loaded before the cart,
-- so it refers to the cart's globals only when called.
local function __code(o)
  local t = o.type
  if t == player then return "P" elseif t == player_spawn then return "S"
  elseif t == smoke then return "s" elseif t == room_title then return "T"
  elseif t == fake_wall then return "W" elseif t == fruit then return "F"
  elseif t == lifeup then return "L" end
  return "?"
end
-- Smoke is cosmetic (its position draws rnd): left out, as every rnd draw (decision Q13).
function __st()
  local codes, sum, n, p = "", 0, 0, nil
  for o in all(objects) do
    if o.type != smoke then
      codes = codes .. __code(o)
      sum += o.x + o.y
      n += 1
    end
    if p == nil and (o.type == player or o.type == player_spawn) then p = o end
  end
  local r = {frames, seconds, freeze, shake, deaths, will_restart, delay_restart, sfx_timer,
             has_dashed, got_fruit[1], n, codes, sum, __sfx}
  if p == nil then
    add(r, "none")
  elseif p.type == player then
    for v in all({"P", p.x, p.y, p.spd.x, p.spd.y, p.rem.x, p.rem.y, p.p_jump, p.p_dash, p.grace,
                  p.jbuffer, p.djump, p.dash_time, p.dash_effect_time, p.dash_target.x,
                  p.dash_target.y, p.dash_accel.x, p.dash_accel.y, p.flip.x, p.spr, p.spr_off,
                  p.was_on_ground}) do add(r, v) end
  else
    for v in all({"S", p.x, p.y, p.spd.x, p.spd.y, p.rem.x, p.rem.y, p.state, p.delay, p.spr,
                  p.target.x, p.target.y}) do add(r, v) end
  end
  return table.unpack(r)
end
