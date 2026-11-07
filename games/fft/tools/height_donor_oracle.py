#!/usr/bin/env python3
"""Retail shared-panel height producer, without any riding/mount fixture."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-final-combat/oracle';out.mkdir(parents=True,exist_ok=True)
 disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 for key,name in dict(height='battle_unit_calculate_height_data',location='battle_map_calculate_location').items():bind_function(key,name,'battle',disc)
 o=s.Original(disc);unit=0x801908cc
 def location(uc,at,size,data):
  uc.reg_write(o.reg.UC_MIPS_REG_V0,0);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 at=s.FUNCTIONS['location'][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,location,begin=at,end=at)
 rows=[]
 for shared,original,donor,traits,move,depth in itertools.product((0,1),(0,1,2,3),(0,1,2,3),((0,0),(2,0),(0,2),(64,0),(0,64),(66,64)),(0,502,503),(0,2)):
  for i in range(21):
   raw=bytearray(448);raw[1]=255;o.uc.mem_write((unit+i*448)&0x1fffff,bytes(raw))
  for ident,state,status in ((0,original,traits[0]),(1,donor,traits[1])):
   raw=bytearray(448);raw[1]=ident if ident==0 or shared else 255;raw[3]=74;raw[0x47]=0;raw[0x28]=100
   raw[0x5a]=status
   if state==1:raw[0x58]=32;raw[0x28]=0
   elif state==2:raw[0x59]=128
   elif state==3:raw[0x58]=64;raw[0x28]=0
   if move:raw[0x93+((move-486)>>3)]=128>>((move-486)&7)
   o.uc.mem_write((unit+ident*448)&0x1fffff,bytes(raw))
  o.uc.mem_write(s.TILES&0x1fffff,bytes((0,0,4,(depth<<5)|1,0,0,0,0)))
  o.call('height',0x801d1800,0);result=bytes(o.uc.mem_read(0x1d1800,7));valid=not(result[3]&128)
  rows.append([shared,original,donor,*traits,move&255,move>>8,depth,int(valid),result[5]if valid else 0,result[6]if valid else 0])
 blob=bytes(v for row in rows for v in row);(out/'fheight.bin').write_bytes(blob)
 (out/'source.h').write_text('/* Unchanged retail donor/status/terrain producer; location alone is supplied. */\n#define HEIGHT_SOURCE_CASES %d\n#define HEIGHT_SOURCE_STRIDE 11\n'%len(rows))
 (out/'source.json').write_text(json.dumps(dict(cases=len(rows),functions=o.function_hashes,scope='Unchanged retail shared-panel height producer: normal/KO/Stone/crystal, first donor, Float, Frog, water movement and depth. Location provider alone returns a controlled terrain column. No rider or mount content is part of the native completion scope.'),indent=2)+'\n')
 print('Retail unmounted height producer:',len(rows),'controls')
if __name__=='__main__':main()
