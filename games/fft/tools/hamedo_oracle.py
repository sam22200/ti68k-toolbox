#!/usr/bin/env python3
"""Retail outer Hamedo: Brave precedes incapacity and failed return previews."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-hamedo/oracle';out.mkdir(parents=True,exist_ok=True)
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 names=dict(hamedo='battle_reaction_prepare_hamedo_for_pending_action',consume='battle_action_check_and_consume_mp',weapon='battle_target_validate_weapon_target',targets='battle_target_list_units_on_panels',counter='battle_action_store_counter_ability',strike='battle_action_init_current_ability_strike_data',chance='battle_action_calculate_chance_to_react',react='battle_action_can_unit_react')
 for k,n in names.items():bind_function(k,n,'battle',d)
 o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=500000
 for k,n in dict(roll='main_util_roll_pass_fail',status_init='main_status_init_check_data').items():o.function_hashes[k]=bind_function(k,n,'main',d)
 o.call('status_init');rng=1;draws=0;reachable=1;trace=[];incoming=0;tid=10;count=1
 def hook(uc,address,size,data):
  nonlocal rng,draws
  value=0;key=next(k for k in('consume','weapon','targets','counter','strike','roll')if s.FUNCTIONS[k][0]==address);trace.append(key)
  if key=='targets':
   dest=uc.reg_read(o.reg.UC_MIPS_REG_A0);uc.mem_write(dest&0x1fffff,bytes((tid,)));value=count
  elif key=='counter':value=0 if reachable else -1
  elif key=='consume':value=incoming if uc.reg_read(o.reg.UC_MIPS_REG_A0)==0x801908cc else 0
  elif key=='roll':
   rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1
   value=int(((rng>>17)*uc.reg_read(o.reg.UC_MIPS_REG_A0)>>15)>=uc.reg_read(o.reg.UC_MIPS_REG_A1))
  uc.reg_write(o.reg.UC_MIPS_REG_V0,value&0xffffffff);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for k in('consume','weapon','targets','counter','strike','roll'):
  at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,hook,begin=at,end=at)
 base=0x801908cc;t=base+10*448;rows=[];records=[]
 for brave,sid,reachable,seed in itertools.product((0,50,100),(255,35,37),(0,1),(1,32767)):
  o.uc.mem_write(base&0x1fffff,bytes(448*21));o.uc.mem_write((t&0x1fffff)+0x8e,b'\x01');o.uc.mem_write((t&0x1fffff)+0x24,bytes((brave,)))
  o.uc.mem_write((t&0x1fffff)+0x58,bytes(5)if sid==255 else mask(sid));o.uc.mem_write((base&0x1fffff)+0x16f,b'\x01')
  o.write(0x8018f5f0,0);o.write(0x8018f5fc,0);o.write(0x801d8000,0);rng=seed;draws=0;trace.clear()
  result=o.call('hamedo',0,0x801d8000);accepted=int(result==10)
  assert draws==1,(brave,sid,reachable,seed,result,trace)
  if sid!=255 or not reachable:assert not accepted
  rows.append((brave,sid,reachable,seed,accepted,draws));records.append(dict(input=rows[-1],trace=trace[:]))
 # The original outer selection does not draw when the incoming menu/target/MP
 # gate fails, including self and multiple-panel occupants.
 controls=[]
 for menu,tid,count,incoming in ((11,10,1,0),(1,0,1,0),(1,10,2,0),(1,10,1,3)):
  o.uc.mem_write(base&0x1fffff,bytes(448*21));o.uc.mem_write((base&0x1fffff)+0x16f,bytes((menu,)));o.uc.mem_write((base&0x1fffff)+0x170,struct.pack('<H',16));o.uc.mem_write((t&0x1fffff)+0x8e,b'\x01');o.uc.mem_write((t&0x1fffff)+0x24,b'\x64')
  o.write(0x8018f5f0,0);o.write(0x8018f5fc,0);rng=1;draws=0;trace.clear();result=o.call('hamedo',0,0x801d8000);assert result==0xffffffff and draws==0
  controls.append(dict(menu=menu,target=tid,count=count,mp_reject=incoming,result=result,draws=draws,trace=trace[:]))
 (out/'source.h').write_text('/* Hash-verified retail outer Hamedo, controlled preview geometry and RNG. */\nstatic const struct {u16 seed;u8 brave,sid,reachable,accepted,draws;} source_hamedo[]={\n'+''.join(' {'+','.join(map(str,(r[3],r[0],r[1],r[2],r[4],r[5])))+'},\n'for r in rows)+'};\n')
 (out/'source.json').write_text(json.dumps(dict(profiles=records,controls=controls,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope='Unchanged retail outer Hamedo, Brave and status eligibility. MP validation, incoming weapon target/list, counter geometry/preview and strike setup are isolated. No original projectile geometry or whole battle replay.'),indent=2)+'\n');print('Original Hamedo:',len(rows),'Brave/status/return profiles and',len(controls),'early gates')
if __name__=='__main__':main()
