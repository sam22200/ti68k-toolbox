#!/usr/bin/env python3
"""Block C unchanged retail MIPS: physical evasion, guards and HP/MP staging."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-combat/oracle';out.mkdir(parents=True,exist_ok=True)
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 for k,n in dict(physical='battle_formula_calculate_physical_evade',guards='battle_action_check_reflect_blade_grasp_and_arrow_guard',
  catch='battle_formula_apply_catch',resource='battle_action_check_mp_switch_distribute_and_damage_split_usability',
  random='battle_formula_get_random_0_7fff').items():bind_function(k,n,'battle',d)
 o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
 for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data').items():o.function_hashes[k]=bind_function(k,n,'main',d)
 o.call('init');rng=1;draws=0
 def external(uc,address,size,data):
  nonlocal rng,draws
  rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1;value=rng>>17
  if address==s.FUNCTIONS['roll'][0]:value=int((value*uc.reg_read(o.reg.UC_MIPS_REG_A0)>>15)>=uc.reg_read(o.reg.UC_MIPS_REG_A1))
  uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for k in('random','roll','lib_rand'):
  at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
 a,t,v=0x801908cc,0x801d4000,0x801938c0
 def byte(at,*values):o.uc.mem_write(at&0x1fffff,bytes(values))
 def reset(reaction,brave,sid,execute=1,secondary=0):
  for ptr in(a,t):
   o.uc.mem_write(ptr&0x1fffff,bytes(448));byte(ptr+0x1a,*([255]*7));byte(ptr+6,128)
   o.uc.mem_write((ptr&0x1fffff)+0x28,struct.pack('<4H',70,100,11,100))
  byte(t+0x24,brave)
  if reaction:bit=reaction-422;byte(t+0x8b+(bit>>3),128>>(bit&7))
  current=mask()if sid==255 else mask(sid);o.uc.mem_write((t&0x1fffff)+0x58,current);o.uc.mem_write((t&0x1fffff)+0x1bb,current)
  o.uc.mem_write(v&0x1fffff,bytes(80));byte(v+0x25,1);byte(v+0x30,*d.at(s.battle.ABILITY_RANGE,14))
  action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write((t&0x1fffff)+0x18c,bytes(action))
  o.write(0x80192d94,a);o.write(0x80192d98,t);o.write(0x80192d90,t+0x18c);o.write(0x80192d8c,0x80192da0);o.uc.mem_write(0x192da0,bytes(44))
  o.write(0x8018f5fc,0 if execute else 2);o.write(0x8018f5f0,secondary)
 bow=next(i for i in range(128)if d.item(i)['type']==12)
 profiles=[]
 for reaction,support,side in itertools.product((0,447,449,451),(0,469),range(3)):
  for seed in(1,32767):profiles.append((19,128,232,reaction,support,side,255,47,seed,0,0))
 for reaction,sid,brave,secondary in itertools.product((451,452,450),(255,35),(0,100),(0,1)):
  profiles.append((bow,128,232,reaction,0,0,sid,brave,32767,1 if reaction==450 else 0,secondary))
 for sid in(10,11,26,27,3,4,21,22):profiles.append((19,128,232,449,0,0,sid,47,1,0,0))
 for kind,reaction,support,side in itertools.product((2,3),(447,449,451),(0,469),range(3)):
  profiles.append((19,128,232,reaction,support,side,255,47,32767,kind,0))
 rows=[]
 for p in profiles:
  weapon,shield,acc,reaction,support,side,sid,brave,seed,thrown,secondary=p;results=[]
  for execute in(0,1):
   reset(reaction,brave,sid,execute,secondary);rng=seed;draws=0
   byte(t+0x1c,acc,19,255,255,shield);byte(t+0x43,d.job(74)['cev']);byte(t+0x47,1)
   o.uc.mem_write((t&0x1fffff)+0x48,struct.pack('<H',(1,0,3)[side]<<8))
   if support:byte(a+0x90,1) # Concentrate469, support set2 bit0
   byte(v+0x18,weapon)
   if thrown==1:byte(v+0x16,126,1);byte(v+0x25,99)
   elif thrown>=2:
    ab=148 if thrown==2 else 147;byte(v+0x16,ab,0);r=d.at(s.battle.ABILITY_RANGE+14*ab,14);byte(v+0x30,*r);byte(v+0x25,r[8])
   o.call('catch'if thrown==1 else 'guards')
   before=bytes(o.uc.mem_read((t&0x1fffff)+0x18c,44))
   if before[0]:o.call('physical')
   result=bytes(o.uc.mem_read((t&0x1fffff)+0x18c,44));rid=struct.unpack_from('<H',result,14)[0]
   results.append((struct.unpack_from('<H',result,42)[0]if result[0]else 0,2 if rid==450 else result[0],draws))
  rows.append((*p,results[0][0],*results[1][1:]))
 resources=[]
 for reaction,amount,brave,seed,sid,secondary in itertools.product((444,445,446),(0,1,17,101,999),(0,100),(1,32767),(255,35),(0,1)):
  reset(reaction,brave,sid,1,secondary);rng=seed;draws=0;healing=reaction==444
  at=(t&0x1fffff)+0x18c;action=bytearray(o.uc.mem_read(at,44));struct.pack_into('<h',action,6 if healing else 4,amount);action[37]=64 if healing else 128;o.uc.mem_write(at,bytes(action));o.call('resource')
  result=bytes(o.uc.mem_read(at,44));rid=struct.unpack_from('<H',result,14)[0];payload=struct.unpack_from('<H',result,38)[0]
  resources.append((reaction,amount,brave,seed,sid,secondary,rid,payload,struct.unpack_from('<H',result,4)[0],struct.unpack_from('<H',result,8)[0],draws))
 # Counter Magic captures the unchanged incoming ability, including misses.
 for k,n in dict(dispatch='battle_action_dispatch_target_reaction_ability',consume='battle_action_check_and_consume_mp',
  hamedo='battle_reaction_prepare_hamedo_for_pending_action',weapon_target='battle_target_validate_weapon_target',
  list_targets='battle_target_list_units_on_panels',counter='battle_action_store_counter_ability',strike='battle_action_init_current_ability_strike_data').items():
  o.function_hashes[k]=bind_function(k,n,'battle',d)
 counters=[]
 for ab,brave,sid,secondary,math,hit in itertools.product((1,16,20),(0,100),(255,35),(0,1),(0,1),(0,1)):
  reset(435,brave,sid,1,secondary);rng=32767;draws=0
  byte(v+0x14,21 if math else 11);o.uc.mem_write((v&0x1fffff)+0x16,struct.pack('<H',ab))
  byte(v+0x25,d.at(s.battle.ABILITY_RANGE+14*ab,14)[8]);byte(v+0x30,*d.at(s.battle.ABILITY_RANGE+14*ab,14));byte(t+0x18c,hit)
  o.call('dispatch');result=bytes(o.uc.mem_read((t&0x1fffff)+0x18c,44))
  counters.append((ab,brave,sid,secondary,math,hit,struct.unpack_from('<H',result,14)[0],draws))
 costs=[]
 for ab,mp,half,sid,mime in itertools.product((16,20),(0,5,100),(0,1),(255,12,22,37),(0,1)):
  reset(435,100,sid,1,1);byte(t+3,0x5d if mime else 74);byte(t+0x16f,11)
  o.uc.mem_write((t&0x1fffff)+0x170,struct.pack('<H',ab));o.uc.mem_write((t&0x1fffff)+0x2c,struct.pack('<H',mp))
  byte(t+0x90,128 if half else 0);o.uc.mem_write(s.TILES&0x1fffff,bytes(8))
  result=o.call('consume',t);remaining=struct.unpack('<H',o.uc.mem_read((t&0x1fffff)+0x2c,2))[0]
  costs.append((ab,mp,half,sid,mime,int(result==0),remaining))
 hamedos=[];calls=[];count=1;tid=10;reject=0
 def isolate(uc,address,size,data):
  result=0
  if address==s.FUNCTIONS['consume'][0]:result=reject
  elif address==s.FUNCTIONS['list_targets'][0]:
   dest=uc.reg_read(o.reg.UC_MIPS_REG_A0);o.uc.mem_write(dest&0x1fffff,bytes((tid,)));result=count
  elif address==s.FUNCTIONS['counter'][0]:
   calls.append([uc.reg_read(o.reg.UC_MIPS_REG_A1)&255,uc.reg_read(o.reg.UC_MIPS_REG_A2)&65535,uc.reg_read(o.reg.UC_MIPS_REG_A3)])
  uc.reg_write(o.reg.UC_MIPS_REG_V0,result);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for key in('consume','weapon_target','list_targets','counter','strike'):
  at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,isolate,begin=at,end=at)
 for brave,count,tid,reject in itertools.product((0,100),(0,1,2),(0,10),(0,3)):
  base=0x801908cc;o.uc.mem_write(base&0x1fffff,bytes(448*21));u=base+10*448
  byte(base+0x16f,1);byte(u+0x8e,1);byte(u+0x24,brave);o.uc.mem_write((u&0x1fffff)+0x28,struct.pack('<4H',100,100,10,10))
  o.write(0x8018f5f0,0);o.write(0x8018f5fc,0);o.write(0x801d8000,0);calls.clear();rng=1;draws=0
  result=o.call('hamedo',0,0x801d8000);ability=struct.unpack('<H',o.uc.mem_read(0x1d8000,2))[0]
  hamedos.append(dict(brave=brave,count=count,target=tid,mp_rejection=reject,result=result,ability=ability,draws=draws,constructor=calls[:]))
 extra=b''.join(struct.pack('>'+str(len(r))+'H',*r)for r in counters+costs)
 (out/'ffco1.bin').write_bytes(extra)
 (out/'offensive.json').write_text(json.dumps(dict(counter_stages=counters,costs=costs,hamedo=hamedos,isolation='Hamedo isolates incoming MP check, weapon-target validation, target list, counter preview and strike initialization; actual outer retail eligibility/Brave/selection/control flow remains unchanged.'),indent=2)+'\n')
 blob=b''.join(struct.pack('>'+str(len(r))+'H',*r)for r in rows+resources)
 (out/'ffco0.bin').write_bytes(blob)
 (out/'counts.h').write_text('#define COMBAT_EVADES %d\n#define COMBAT_RESOURCES %d\n#define COMBAT_COUNTERS %d\n#define COMBAT_COSTS %d\n'%(len(rows),len(resources),len(counters),len(costs)))
 (out/'source.json').write_text(json.dumps(dict(evades=len(rows),resources=len(resources),fields_evade='weapon shield acc reaction support side sid brave seed thrown secondary preview hit draws',fields_resource='reaction amount brave seed sid secondary reaction_id payload hp_damage mp_damage draws',functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope='Unchanged physical evade and pre-formula guard/Catch, resource dispatch before HP; controlled RNG. Human targets, ordinary weather. No original whole battle/AI replay.'),indent=2)+'\n')
 print('Combat oracle:',len(rows),'evasion,',len(resources),'resource profiles')
if __name__=='__main__':main()
