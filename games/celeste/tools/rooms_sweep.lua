-- --lib file: function sweep() prints, for every room, its objects at load
function sweep()
  local names={}
  names[player_spawn]="spawn" names[spring]="spring" names[balloon]="balloon"
  names[fall_floor]="fall_floor" names[fruit]="fruit" names[fly_fruit]="fly_fruit"
  names[fake_wall]="fake_wall" names[key]="key" names[chest]="chest" names[platform]="platform"
  names[message]="message" names[big_chest]="big_chest" names[flag]="flag" names[room_title]="room_title"
  names[smoke]="smoke"
  for ry=0,3 do for rx=0,7 do
    load_room(rx,ry)
    local s="room "..level_index().." ("..rx..","..ry..") n="..#objects..":"
    for o in all(objects) do
      s=s.." "..(names[o.type] or "?")
      if o.type==player_spawn then s=s.."@"..flr(o.target.x)..","..flr(o.target.y) end
    end
    __out(s)
  end end
end
