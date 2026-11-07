#!/usr/bin/env python3
"""Retail arc-choice branches; explicitly isolated geometry providers.

The native64-sample ballistic arithmetic is a TI adaptation. Its independent
integer fixtures are labelled model controls, never retail trajectory equality.
"""
import itertools,json,math,struct
import surface_data as s
from status_state_oracle import bind_function

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-arcs/oracle';out.mkdir(parents=True,exist_ok=True)
 disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 names=dict(select='battle_effect_set_and_validate_arc_trajectory',sprite='battle_gfx_get_unit_spritesheet_height_by_unit_id',
  listed='battle_effect_list_valid_unit_targeting_data',center='battle_unit_get_tile_center_and_height_by_battle_id',
  solve='battle_effect_calculate_arcing_trajectory',trace='battle_effect_trace_arc_trajectory_path',stats='battle_unit_get_stats_from_battle_id',
  peak='battle_effect_calculate_arc_height_term')
 for key,name in names.items():bind_function(key,name,'battle',disc)
 o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);current={}
 for key,name in dict(sqrt='SquareRoot12',atan='ratan2').items():o.function_hashes[key]=bind_function(key,name,'main',disc)
 def external(uc,at,size,data):
  key=next(k for k in names|dict(sqrt='',atan='') if s.FUNCTIONS[k][0]==at)
  a=[uc.reg_read(r) for r in (o.reg.UC_MIPS_REG_A0,o.reg.UC_MIPS_REG_A1,o.reg.UC_MIPS_REG_A2,o.reg.UC_MIPS_REG_A3)]
  result=0
  if key=='sprite':result=36
  elif key=='listed':o.write(a[2],0x801d1000)
  elif key=='center':uc.mem_write(a[1]&0x1fffff,struct.pack('<4h',14+a[0]*112,-48,14,0))
  elif key=='solve':o.write(a[1],8192);o.write(a[2],4096);result=1
  elif key=='trace':
   hit=current['paths'][current['calls']];current['calls']+=1
   o.write(0x801b8b64,0xffffffff if hit in (1,255) else hit)
   o.write(0x801bbf3c,10);o.write(0x801b8b94,0)
   result=int(hit==1)
  elif key=='stats':result=0x801d2000+a[0]*448
  elif key=='sqrt':result=112<<12
  uc.reg_write(o.reg.UC_MIPS_REG_V0,result);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for key in (*[k for k in names if k not in ('select','peak')],'sqrt','atan'):
  addr=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=addr,end=addr)
 choices=[]
 for low,high in itertools.product((1,2,3,255),repeat=2):
  for camp,team2,team3 in itertools.product((0,1),repeat=3):
   for ident,team in ((0,camp),(2,team2),(3,team3)):
    base=0x801d2000+448*ident;o.uc.mem_write(base&0x1fffff,bytes(448));o.uc.mem_write((base+5)&0x1fffff,bytes((team<<4,)));o.uc.mem_write((base+0x1ba)&0x1fffff,bytes((team<<4,)))
   current.update(paths=(low,high),calls=0)
   result=o.call('select',0,0,1);expected=result if result<4 else 255
   enemy=lambda ident:int(ident in (2,3) and (team2 if ident==2 else team3)!=camp)
   choices.append([low,high,enemy(low),enemy(high),expected])
 o.write(0x801b8878,0xa8000)
 peaks=[]
 for distance in (0,28,56,84,112,140,168,196,224,280,336):
  value=o.call('peak',distance<<12);value=value if value<0x80000000 else value-(1<<32)
  peaks.append([distance,value])
 models=[]
 for dx,dz,rise in itertools.product((0,28,84,140,224,448),(0,28,84,224),(-1500,-120,-6,0,18,84,300)):
  discv=168**2-336*rise-dx*dx-dz*dz
  for high in (0,1):
   curve=-1 if (not dx and not dz) or discv<0 else 168-rise+(1 if high else -1)*math.isqrt(discv)
   points=[80+(rise*step*64+curve*step*(64-step))//4096 for step in (1,16,32,48,63)] if curve>=0 else [0]*5
   models.append([dx,dz,rise,high,curve,*points])
 (out/'farc.bin').write_bytes(b''.join(bytes(r)for r in choices)+b''.join(struct.pack('>10h',*r)for r in models))
 (out/'source.h').write_text('/* Retail choice branches; separate adapted ballistic controls. */\n#define ARC_CHOICES %d\n#define ARC_MODELS %d\n'%(len(choices),len(models)))
 (out/'source.json').write_text(json.dumps(dict(retail_choices=len(choices),model_controls=len(models),source_peaks=peaks,functions=o.function_hashes,
  scope='Unchanged retail arc-choice dispatcher branches with mocked traces/geometry providers; native analytic64-step curves are an adaptation, not full PS1 trajectory equality.'),indent=2)+'\n')
 print('Retail arc choices:',len(choices),'adapted ballistic controls:',len(models))
if __name__=='__main__':main()
