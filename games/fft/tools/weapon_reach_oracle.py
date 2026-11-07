#!/usr/bin/env python3
"""Retail weapon panel shapes and effective heights, controlled source terrain."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-weapon-reach/oracle';out.mkdir(parents=True,exist_ok=True)
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC));names=dict(wr='battle_target_calculate_weapon_range',strike='battle_target_calculate_strike_lunge_range',arc='battle_target_calculate_arc_range',vertical='battle_target_calculate_aoe_vertical_tolerance',spread='battle_target_spread_panels',neighbors='battle_target_spread_panel_to_neighbors',close='battle_target_remove_close_range',select='battle_target_can_select_tile',height='battle_unit_get_effective_height')
 for k,n in names.items():bind_function(k,n,'battle',d)
 o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
 def put(at,data):o.uc.mem_write(at&0x1fffff,bytes(data))
 def byte(at,*v):put(at,v)
 def leaf(uc,address,size,data):uc.reg_write(o.reg.UC_MIPS_REG_V0,0);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 at=s.FUNCTIONS['select'][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,leaf,begin=at,end=at)
 n=13;origin=6+6*n;unit=0x801908cc;panels=0x80192dd8;o.write(0x800e4e9c,n);o.write(0x800e4ea0,n)
 profiles=[];blob=bytearray()
 for weapon,hand,frog,base,target in itertools.product((0,19,99,71,83),(0,1),(0,1),(2,10),(0,5,10,15)):
  # Five real entries cover bare/sword, lunge, direct and arc. All four source
  # range routines execute unchanged; only final target-selection filtering is isolated.
  put(unit,bytes(448));byte(unit+0x47,6);put(unit+0x48,struct.pack('<H',6));byte(unit+0x1d,weapon if not hand else 255);byte(unit+0x1f,weapon if hand else 255)
  if frog:byte(unit+0x5a,2)
  terrain=bytearray(4096)
  for i in range(512):terrain[i*8+2]=target
  terrain[origin*8+2]=base;put(s.TILES,terrain);put(panels,bytes(2560));o.call('wr',unit)
  data=bytes(o.uc.mem_read(panels&0x1fffff,2560));selected=bytes(int(data[5*(x+y*n+256*layer)]!=0 and (x,y)!=(6,6))for layer in range(2)for y in range(n)for x in range(n))
  chosen=0 if frog else weapon;r=d.at(s.battle.WEAPONS+chosen*8,8);height=o.call('height',unit)
  profiles.append(dict(weapon=weapon,left=hand,frog=frog,base=base,target=target,flags=r[1],range=r[0],height=height,offset=len(blob)));blob.extend(selected)
 heights=[]
 for base,half,depth,move,float_status in itertools.product((0,127),(0,1),(0,2),(0,17,18,21),(0,1)):
  put(unit,bytes(448));byte(unit+0x47,6);put(unit+0x48,struct.pack('<H',6));byte(s.TILES+origin*8+2,base,(depth<<5)|half)
  if move:byte(unit+0x95,128>>((move-1)&7))
  if float_status:byte(unit+0x5a,64)
  heights.append((base,half,depth,move,float_status,o.call('height',unit)))
 (out/'fwreach.bin').write_bytes(blob)
 (out/'source.h').write_text('/* Retail panel footprints; final selector alone is isolated. */\n#define SOURCE_WEAPON_PROFILES %d\n#define SOURCE_WEAPON_WIDTH 13\nstatic const struct {u16 offset;u8 weapon,left,frog,base,target,flags,range,height;} source_weapon[]={\n'%len(profiles)+''.join(' {'+','.join(map(str,(p['offset'],p['weapon'],p['left'],p['frog'],p['base'],p['target'],p['flags'],p['range'],p['height'])))+'},\n'for p in profiles)+'};\nstatic const u8 source_weapon_heights[][6]={\n'+''.join(' {'+','.join(map(str,row))+'},\n'for row in heights)+'};\n')
 (out/'source.json').write_text(json.dumps(dict(profiles=profiles,heights=heights,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope='Unchanged retail weapon selection, strike/lunge/arc/direct panel marking, spread/close filtering and effective-height helpers; final target-selection filtering alone returns without effects. Controlled uniform dry terrain. No projectile trajectory, damage/proc or whole battle replay.'),indent=2)+'\n');print('Original weapon footprints:',len(profiles),'panels',len(blob))
if __name__=='__main__':main()
