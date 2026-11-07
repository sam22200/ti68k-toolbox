#!/usr/bin/env python3
"""Unchanged retail strike/lunge interval decisions with explicit height adapters."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-melee-collision/oracle';out.mkdir(parents=True,exist_ok=True)
 disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 for key,name in dict(lunge='battle_target_validate_lunging_target',overlap='battle_target_validate_height_overlap',height='battle_unit_calculate_height_data',occupant='battle_target_get_unit_id_if_tile_targetable').items():bind_function(key,name,'battle',disc)
 o=s.Original(disc);rows=[];current={}
 def leaf(uc,at,size,data):
  a0=uc.reg_read(o.reg.UC_MIPS_REG_A0);a1=uc.reg_read(o.reg.UC_MIPS_REG_A1)
  if at==s.FUNCTIONS['height'][0]:
   ident=a1 if a1<4 else 255
   span=current['spans'].get(ident)
   feet,top=span if span else (0,0)
   uc.mem_write(a0&0x1fffff,bytes((0,0,0,ident if span else 128,(top-feet)&255,feet,top)))
   result=a0
  else:
   layer=uc.reg_read(o.reg.UC_MIPS_REG_A2);result=current['ids'][layer] if layer<2 else 255
   if result==255:result=0xffffffff
  uc.reg_write(o.reg.UC_MIPS_REG_V0,result);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for key in ('height','occupant'):
  addr=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,leaf,begin=addr,end=addr)
 unit=0x801908cc;o.uc.mem_write(unit&0x1fffff,bytes(448));o.write(0x800e4e9c,3);o.write(0x800e4ea0,1)
 for uw,tw,body,distance,terrain,occupant in itertools.product((0,4,8,20),(0,5,10,18),(4,6,8),(1,2),(0,6,12,255),(0,1,2,3)):
  ut=(uw+body)&255;tt=(tw+6)&255
  floors=(terrain,255 if occupant!=3 else 8)
  spans={0:(uw,ut),1:(tw,tt)};ids=[255,255]
  if occupant in (1,3):spans[2]=(terrain if terrain!=255 else 4,(terrain if terrain!=255 else 4)+6);ids[0]=2
  if occupant in (2,3):spans[3]=(8,14);ids[1]=3
  current.update(spans=spans,ids=ids)
  for layer,floor in enumerate(floors):
   raw=bytearray(8);raw[2]=floor>>1;raw[3]=floor&1;raw[6]=floor==255
   o.uc.mem_write((s.TILES+8*(1+256*layer))&0x1fffff,bytes(raw))
  o.write(0x801fe010,1)
  value=o.call('lunge',0,distance,0,0);expected=value if value<4 else 255
  rows.append([uw,ut,tw,tt,1,distance,*floors,*spans.get(2,(0,0)),ids[0],*spans.get(3,(0,0)),ids[1],expected])
 blob=b''.join(bytes(row) for row in rows);(out/'fmelee.bin').write_bytes(blob)
 (out/'source.h').write_text('/* Unchanged retail lunge control flow; explicit height/occupancy adapters. */\n#define MELEE_SOURCE_CASES %d\n#define MELEE_SOURCE_FIELDS 15\n'%len(rows))
 (out/'source.json').write_text(json.dumps(dict(profiles=len(rows),functions=o.function_hashes,scope='Retail interval/lunge control flow; external height and occupant providers are controlled inputs, not a whole dispatcher oracle.'),indent=2)+'\n');print('Original melee decisions:',len(rows))
if __name__=='__main__':main()
