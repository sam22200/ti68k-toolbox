#!/usr/bin/env python3
"""Unchanged retail direct-shot recipient routing, with geometry providers isolated."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-direct-interception/oracle';out.mkdir(parents=True,exist_ok=True)
 disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 names=dict(select='battle_effect_check_direct_trajectory_to_target',
  sprite='battle_gfx_get_unit_spritesheet_height_by_unit_id',
  listed='battle_effect_list_valid_unit_targeting_data',
  center='battle_unit_get_tile_center_and_height_by_battle_id',
  trace='battle_effect_trace_projectile_path')
 for key,name in names.items():bind_function(key,name,'battle',disc)
 o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);current={}
 for key,name in dict(sqrt='SquareRoot12',sqrt0='SquareRoot0',atan='ratan2').items():
  o.function_hashes[key]=bind_function(key,name,'main',disc)
 def external(uc,at,size,data):
  key=next(k for k in (*names,'sqrt','sqrt0','atan')if s.FUNCTIONS[k][0]==at)
  a=[uc.reg_read(r)for r in (o.reg.UC_MIPS_REG_A0,o.reg.UC_MIPS_REG_A1,o.reg.UC_MIPS_REG_A2,o.reg.UC_MIPS_REG_A3)]
  result=0
  if key=='sprite':result=36
  elif key=='listed':
   assert a[0]==1 and a[1]==0xffffffff
   o.write(a[2],0x801d1000)
  elif key=='center':uc.mem_write(a[1]&0x1fffff,struct.pack('<4h',14+a[0]*28,-48,14,0))
  elif key=='trace':
   current['calls']+=1;o.write(0x801b8b64,0xffffffff if current['hit']==255 else current['hit']);result=current['clear']
  elif key=='sqrt':result=84<<12
  elif key=='sqrt0':result=84
  uc.reg_write(o.reg.UC_MIPS_REG_V0,result);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for key in (*[k for k in names if k!='select'],'sqrt','sqrt0','atan'):
  addr=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=addr,end=addr)
 rows=[]
 for target,clear,hit in itertools.product((0,2,3,10,12),(0,1),(0,2,3,12,255)):
  current.update(clear=clear,hit=hit,calls=0);result=o.call('select',1,0,target)
  assert current['calls']==1
  expected=result if result<13 else 255
  assert expected==(target if clear else hit)
  rows.append([target,clear,hit,expected])
 (out/'fdir.bin').write_bytes(bytes(v for row in rows for v in row))
 (out/'source.h').write_text('/* Retail recipient routing; trace/geometry are supplied controls. */\n#define DIRECT_SOURCE_CASES %d\n'%len(rows))
 (out/'source.json').write_text(json.dumps(dict(cases=len(rows),functions=o.function_hashes,
  scope='Unchanged direct-target dispatcher, including target/recipient slot0 and terrain sentinel. Trace, unit geometry and math-library providers are controlled. Native fixed-point sampling, body-height producers and complete PS1 RNG are not certified by these routing controls.'),indent=2)+'\n')
 print('Retail direct-shot routing controls:',len(rows),flush=True)
if __name__=='__main__':main()
