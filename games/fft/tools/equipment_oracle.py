#!/usr/bin/env python3
"""Block B retail MIPS fixtures: permissions, physical supports and progression."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
 out=s.HERE/'campaign/x/core-equipment/oracle';out.mkdir(parents=True,exist_ok=True)
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 names=dict(cats=('main_unit_set_equippable_items','main'),init=('main_status_init_check_data','main'),
  physical=('battle_formula_apply_physical_xa_modifying_statuses_support','battle'),
  charge=('battle_formula_apply_charge','battle'),
  base=('battle_formula_calculate_base_xa','battle'),exp=('battle_unit_update_attacker_earned_experience','battle'),
  benefit=('battle_action_init_movement_ability_benefit','battle'),
  perform=('battle_action_perform_reaction_ability','battle'),
  trigger=('battle_action_check_pa_save_ma_save_speed_save_regenerator_auto_potion_gilgame_heart_usability','battle'))
 for k,(name,owner)in names.items():
  if owner=='battle':bind_function(k,name,owner,d)
 o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
 for k,(name,owner)in names.items():
  if owner=='main':o.function_hashes[k]=bind_function(k,name,owner,d)
 o.call('init')
 import unicorn
 def rand(uc,addr,size,data):
  uc.reg_write(o.reg.UC_MIPS_REG_V0,0);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 o.uc.hook_add(unicorn.UC_HOOK_CODE,rand,begin=0x8002230c,end=0x8002230c)
 path_address=0x8018f7f0
 actor,target,ability=0x801908cc,0x801d4000,0x801938c0
 def reset():
  for at,n in ((actor,448),(target,448),(ability,80)):o.uc.mem_write(at&0x1fffff,bytes(n))
  o.write(0x80192d94,actor);o.write(0x80192d98,target);o.write(0x80192d90,target+0x18c)
  o.write(0x8018f5f0,0)
 def byte(at,*v):o.uc.mem_write(at&0x1fffff,bytes(v))
 cats=bytearray()
 for job,female,support in itertools.product(range(160),range(2),range(453,462)):
  reset();byte(actor+6,64 if female else 128);o.uc.mem_write((actor&0x1fffff)+0x4a,d.job(job)['cats'])
  byte(actor+0x8f,0 if support==453 else 128>>(support-454));o.call('cats',actor)
  cats.extend(struct.pack('>BBH',job,female,0 if support==453 else support)+o.uc.mem_read((actor&0x1fffff)+0x4a,4))
 (out/'ffeq0.bin').write_bytes(cats)
 damage=[]
 # Weapon subset already supported by native ordinary/Charge; cover each XA support stage.
 for weapon,pa,charge,two,atksup,defsup,status in itertools.product((0,1,19,99),(5,11),(0,3),(0,1),(0,1,2),(0,1),(0,1)):
  reset();byte(actor+6,128);byte(target+6,128);byte(actor+0x36,pa,5,9);byte(actor+0x24,60)
  byte(actor+0x90,16 if atksup==1 else 0);byte(actor+0x91,32 if atksup==2 else 0)
  byte(target+0x90,8 if defsup else 0);byte(target+0x58+3,32 if status else 0)
  byte(ability+0x18,weapon);o.uc.mem_write((ability&0x1fffff)+0x3e,d.at(s.battle.WEAPONS+8*weapon,8))
  byte(ability+0x24,charge);byte(ability+0x19,two)
  o.call('base');o.call('charge');o.call('physical');xa,ya=struct.unpack('<HH',o.uc.mem_read((ability&0x1fffff)+14,4))
  damage.append([255 if not weapon else weapon,pa,charge,two,atksup,defsup,status,min(999,xa*ya)])
 exp=[]
 for alevel,tlevel,kill,kos,up in itertools.product((1,50,99),(1,50,99),(1,2),(0,1,4),(0,1)):
  reset();byte(actor+0x22,alevel);byte(target+0x22,tlevel);byte(target+0x15c,kos);byte(actor+0x90,32 if up else 0)
  o.call('exp',kill);exp.append([alevel,tlevel,kill==2,kos,up,o.uc.mem_read((ability&0x1fffff)+0x27,1)[0]])
 benefits=[]
 for ab,maxhp,maxmp,path,up,sid in itertools.product(range(493,497),(1,101),(1,71),(1,7),(0,1),(255,35)):
  reset();o.uc.mem_write((actor&0x1fffff)+0x28,struct.pack('<4H',1,maxhp,1,maxmp));byte(actor+0x3a,5)
  bit=ab-486;byte(actor+0x93+(bit>>3),128>>(bit&7));byte(actor+0x90,(32 if ab==495 else 64)if up else 0)
  if sid!=255:o.uc.mem_write((actor&0x1fffff)+0x58,mask(sid))
  # Resolve the original path global by the symbols in the mapped retail code.
  o.uc.mem_write(path_address&0x1fffff,bytes((path,)))
  result=o.call('benefit',actor);v=bytes(o.uc.mem_read((actor&0x1fffff)+0x18c,44))
  benefits.append([ab,maxhp,maxmp,path,up,sid,result&65535,*struct.unpack('<HH',v[6:8]+v[10:12]),v[0x28],v[0x29]])
 gil=[]
 for amount,sid in itertools.product((0,17,999),(255,31)):
  reset();o.write(0x80192d9c,439);o.write(0x8018f5f0,1)
  if sid!=255:o.uc.mem_write((target&0x1fffff)+0x58,mask(sid))
  o.uc.mem_write((target&0x1fffff)+0x1b2,struct.pack('<H',amount));o.call('perform')
  v=bytes(o.uc.mem_read((target&0x1fffff)+0x18c,44));gil.append([amount,sid,v[0],struct.unpack_from('<H',v,12)[0]])
 # Keep original reward ordering, isolating storage/unlock and level growth.
 for k,name,owner in [('reward','battle_unit_apply_action_rewards','battle'),('jlevel','battle_unit_load_job_level','battle'),
  ('storejp','battle_unit_store_jp_and_calculate_unlocked_jobs','battle'),('level','main_unit_check_level_up','main')]:
  o.function_hashes[k]=bind_function(k,name,owner,d)
 jlevel=[0];stored=[]
 def reward_external(uc,address,size,data):
  if address==s.FUNCTIONS['jlevel'][0]:
   o.write(uc.reg_read(o.reg.UC_MIPS_REG_A2),0);result=jlevel[0]
  else:
   result=0
   if address==s.FUNCTIONS['storejp'][0]:stored.append([uc.reg_read(o.reg.UC_MIPS_REG_A0),uc.reg_read(o.reg.UC_MIPS_REG_A1)])
  uc.reg_write(o.reg.UC_MIPS_REG_V0,result);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for name in ('jlevel','storejp','level'):
  at=s.FUNCTIONS[name][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,reward_external,begin=at,end=at)
 jp=[]
 for level,joblevel,up,expvalue in itertools.product((1,50,99),(0,3,7),(0,1),(0,10)):
  reset();byte(actor+3,74);byte(actor+6,128);byte(actor+0x22,level);byte(actor+0x90,64 if up else 0)
  byte(ability+0x27,expvalue,1);stored.clear();jlevel[0]=joblevel;o.call('reward',0,0x801d7000)
  display=bytes(o.uc.mem_read(0x1d7000,4));assert len(stored)==21,stored
  jp.append([level,joblevel,up,expvalue,display[1],stored[0][1]])
 lines=['/* Unchanged retail MIPS outputs; generated local-only. */']
 for name,rows in [('source_equipment_damage',damage),('source_equipment_exp',exp),('source_equipment_benefits',benefits),('source_equipment_gil',gil),('source_equipment_jp',jp)]:
  lines += [f'static const u16 {name}[][{len(rows[0])}]={{',*[' {'+','.join(map(str,r))+'},'for r in rows],'};']
 (out/'source.h').write_text('\n'.join(lines)+'\n')
 blob=bytearray()
 for rows in (damage,exp,benefits,gil,jp):
  for row in rows:blob.extend(struct.pack('>'+str(len(row))+'H',*row))
 (out/'ffeq1.bin').write_bytes(blob)
 (out/'source.json').write_text(json.dumps(dict(categories=len(cats)//8,damage=len(damage),exp=len(exp),benefits=len(benefits),gil=len(gil),jp=len(jp),functions=o.function_hashes,
  exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope='Unchanged retail category OR, physical XA stages, reward EXP staging and movement benefit preparation; supplied records, BIOS rand=0. No original full equipment UI/AI replay.'),indent=2)+'\n')
 print('Equipment oracle:',len(cats)//8,'category masks,',len(damage),'physical scalars,',len(exp),'EXP awards,',len(benefits),'movement benefits')

if __name__=='__main__':main()
