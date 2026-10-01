-- helpers for p8trace.py --trace (reference side only)
function pl() for o in all(objects) do if o.type==player then return o end end end
function px() local p=pl() return p and p.x or -1 end
function py() local p=pl() return p and p.y or -1 end
function pvx() local p=pl() return p and p.spd.x or 0 end
function pvy() local p=pl() return p and p.spd.y or 0 end
function pdj() local p=pl() return p and p.djump or -1 end
function pgr() local p=pl() return p and p.grace or -1 end
function nobj() return #objects end
function lvl() return level_index() end
-- collision work counter: objects scanned by obj.collide in the frame (upper bound), and calls
__ci, __cc = 0, 0
function ci() local v = __ci __ci = 0 return v end
function cc() local v = __cc __cc = 0 return v end
function __wrap_collide()
  local io = init_object
  init_object = function(t, x, y)
    local o = io(t, x, y)
    if o then local c = o.collide o.collide = function(ty, ox, oy) __ci += #objects __cc += 1 return c(ty, ox, oy) end end
    return o
  end
end
