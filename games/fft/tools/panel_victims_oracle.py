#!/usr/bin/env python3
"""Retail unit eligibility and stable panel enumeration, geometry isolated."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-panel-victims/oracle';out.mkdir(parents=True,exist_ok=True)
 disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 for key,name in dict(panel='battle_target_list_units_on_panels',eligible='battle_target_is_unit_untargetable_and_store_tile',location='battle_map_calculate_location').items():bind_function(key,name,'battle',disc)
 o=s.Original(disc);unit=0x801908cc;current={}
 def location(uc,at,size,data):
  ident=(uc.reg_read(o.reg.UC_MIPS_REG_A0)-unit)//448
  uc.reg_write(o.reg.UC_MIPS_REG_V0,current['cells'][ident]);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 at=s.FUNCTIONS['location'][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,location,begin=at,end=at)
 rows=[]
 for profile in range(128):
  cells=[((i*3+profile)//3)%4 for i in range(21)];current['cells']=cells;panel=profile&3;eligible=0
  for i in range(21):
   raw=bytearray(448);state=(profile+3*i)%8;raw[1]=i if i<13 and state!=0 else 255;raw[0x47]=cells[i];raw[0x28]=100
   if state==1:raw[0x58]=32;raw[0x28]=0 # KO is still a panel recipient.
   elif state==2:raw[0x59]=128 # Stone is also still enumerated.
   elif state==3:raw[0x58]=64 # crystal
   elif state==4:raw[0x58]=4 # Jump
   elif state==5:raw[0x59]=1 # treasure
   elif state==6:raw[0x182]=64 # mounted actor is omitted
   o.uc.mem_write((unit+i*448)&0x1fffff,bytes(raw))
  for c in range(4):o.uc.mem_write((s.TILES+c*8+5)&0x1fffff,bytes((128 if c==panel else 0,)))
  o.write(0x8018f4f8,profile&1)
  for i in range(13):
   if not o.call('eligible',i,0x801d1800):eligible|=1<<i
  count=o.call('panel',0x801d1900,unit)
  ids=list(o.uc.mem_read(0x1d1900,21));chosen=ids[:count]
  assert chosen==sorted(chosen) and all(i<13 for i in chosen)
  mask=sum(1<<i for i in chosen)
  rows.append(bytes(cells[:13])+struct.pack('>HBHB',eligible,panel,mask,count)+bytes(ids[:13]))
 (out/'fpanel.bin').write_bytes(b''.join(rows))
 (out/'source.h').write_text('/* Original eligibility and panel enumeration; geometry provider isolated. */\n#define PANEL_SOURCE_CASES %d\n#define PANEL_SOURCE_STRIDE 32\n'%len(rows))
 assert all(len(row)==32 for row in rows)
 (out/'source.json').write_text(json.dumps(dict(cases=len(rows),unique_rows=len(set(rows)),functions=o.function_hashes,scope='Unchanged eligibility and enumeration instructions, including KO/Stone and crystal/treasure/Jump/mount exclusions; map-location provider supplies controlled same-panel cells. Native mounted content remains outside the current adapter.'),indent=2)+'\n')
 print('Original panel enumeration:',len(rows))
if __name__=='__main__':main()
