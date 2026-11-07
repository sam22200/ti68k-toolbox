#!/usr/bin/env python3
"""Block D: unchanged retail MIPS decisions, RNG and real archived source data."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-discovery/oracle';out.mkdir(parents=True,exist_ok=True)
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 names=dict(talk='battle_formula_talk_skill_hit_ma_x_percent',train='battle_formula_apply_poach_and_train',poach='battle_action_add_poached_item_to_fur_shop_inventory',find='battle_map_determine_rare_common_item',flag='battle_map_calculate_move_find_item_flag',supporter='battle_menu_has_monster_skill_supporter_at_tile',height='battle_unit_calculate_height_data',heal='battle_formula_heal_ma_times_y',esuna='battle_formula_hit_ma_x_percent_no_evade',random='battle_formula_get_random_0_7fff',find_tile='battle_map_set_item_trap_data',variable='battle_script_get_variable')
 for k,n in names.items():bind_function(k,n,'battle',d)
 o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
 for k,n in dict(roll='main_util_roll_pass_fail',lib_rand='rand',init='main_status_init_check_data').items():o.function_hashes[k]=bind_function(k,n,'main',d)
 o.call('init');rng=1;draws=0;mapid=1;height=6
 def external(uc,address,size,data):
  nonlocal rng,draws
  if address==s.FUNCTIONS['find_tile'][0]:value=0x801d7000
  elif address==s.FUNCTIONS['variable'][0]:value=mapid
  elif address==s.FUNCTIONS['height'][0]:
   dest=uc.reg_read(o.reg.UC_MIPS_REG_A0)&0x1fffff;uc.mem_write(dest,bytes((0,0,0,0,6,0,height&255)));value=dest|0x80000000
  else:
   rng^=rng<<13;rng&=0xffffffff;rng^=rng>>17;rng^=rng<<5;rng&=0xffffffff;draws+=1;value=rng>>17
   if address==s.FUNCTIONS['roll'][0]:value=int((value*uc.reg_read(o.reg.UC_MIPS_REG_A0)>>15)>=uc.reg_read(o.reg.UC_MIPS_REG_A1))
  uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for k in ('random','roll','lib_rand','find_tile','variable','height'):
  at=s.FUNCTIONS[k][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
 a,t,v=0x801908cc,0x801d4000,0x801938c0
 def byte(at,*values):o.uc.mem_write(at&0x1fffff,bytes(values))
 def reset(ab=118,monster=0,support=0,reaction=0,brave=47,sid=255,secondary=0):
  for ptr in(a,t):
   o.uc.mem_write(ptr&0x1fffff,bytes(448));byte(ptr+0x1a,*([255]*7));byte(ptr+6,128)
   o.uc.mem_write((ptr&0x1fffff)+0x28,struct.pack('<4H',100,100,20,100));byte(ptr+0x24,brave);byte(ptr+0x26,70)
  byte(a+0x37,20);byte(t+3,94 if monster else 74);byte(t+6,32 if monster else 128)
  if support:bit=support-454;byte(a+0x8f+(bit>>3),128>>(bit&7))
  if reaction:bit=reaction-422;byte(t+0x8b+(bit>>3),128>>(bit&7))
  current=mask()if sid==255 else mask(sid);o.uc.mem_write((t&0x1fffff)+0x58,current);o.uc.mem_write((t&0x1fffff)+0x1bb,current)
  o.uc.mem_write(v&0x1fffff,bytes(80));r=d.at(s.battle.ABILITY_RANGE+14*ab,14);byte(v+0x30,*r);byte(v+0x46,*d.at(s.battle.STATUS_SET+6*r[11],6));byte(v+0x25,r[8]);o.uc.mem_write((v&0x1fffff)+0x16,struct.pack('<H',ab))
  action=bytearray(44);action[0]=1;action[42]=100;o.uc.mem_write((t&0x1fffff)+0x18c,bytes(action))
  o.write(0x80192d94,a);o.write(0x80192d98,t);o.write(0x80192d90,t+0x18c);o.write(0x80192d8c,0x80192da0);o.uc.mem_write(0x192da0,bytes(44));o.write(0x8018f5fc,0);o.write(0x8018f5f0,secondary);o.uc.mem_write(0x18f5f4,bytes(8))
 talk=[]
 for monster,support,reaction,brave,sid,secondary,seed in itertools.product((0,1),(0,473),(0,448),(0,47,100),(255,35,12),(0,1),(1,32767)):
  reset(monster=monster,support=support,reaction=reaction,brave=brave,sid=sid,secondary=secondary);rng=seed;draws=0;o.call('talk');result=bytes(o.uc.mem_read((t&0x1fffff)+0x18c,44))
  talk.append((monster,support,reaction,brave,sid,secondary,seed,result[0],draws))
 train=[]
 for support,monster,hp,damage,immune,immortal in itertools.product((470,471),(0,1),(20,21,100),(0,1,19,20,100),(0,1,2),(0,1)):
  reset(ab=0,monster=monster,support=support);o.uc.mem_write((t&0x1fffff)+0x28,struct.pack('<HH',hp,100));byte(t+5,4 if immortal else 0)
  if immune==1:byte(t+0x54,64)
  elif immune==2:byte(t+0x59,64);byte(t+0x1bc,64)
  at=(t&0x1fffff)+0x18c;action=bytearray(o.uc.mem_read(at,44));struct.pack_into('<H',action,4,damage);action[37]=128;o.uc.mem_write(at,bytes(action));o.call('train');result=bytes(o.uc.mem_read(at,44))
  train.append((support,monster,hp,damage,immune,immortal,struct.unpack_from('<H',result,16)[0]))
 poach=[]
 for job,seed in itertools.product(range(94,142),(1,32767)):
  reset(ab=0,monster=1,support=471);byte(t+3,job);o.uc.mem_write((t&0x1fffff)+0x19c,b'\x20\0');o.uc.mem_write(0x59494,bytes(256));rng=seed;draws=0;o.call('poach');inv=bytes(o.uc.mem_read(0x59494,256));found=[i for i,x in enumerate(inv)if x]
  assert len(found)==1;poach.append((job,seed,found[0],draws))
 find=[]
 maps=d.bat[0x800f5e74-s.battle.BAT:0x800f6674-s.battle.BAT]
 for mapid in range(128):
  for entry in range(4):
   pos,flags,rare,common=maps[mapid*16+entry*4:mapid*16+entry*4+4]
   if flags&32 or not(rare or common):continue
   for brave,seed in itertools.product((0,47,100),(1,32767)):
    reset(brave=brave);byte(0x801d7000,1,entry,rare,common,0,255);o.uc.mem_write(0x59414,bytes(128));rng=seed;draws=0;o.call('find',t);item=o.uc.mem_read(0x18f8a4,1)[0];flag=o.uc.mem_read(0x59414+mapid//2,1)[0]
    find.append((mapid,entry,pos,flags,brave,seed,item,flag,draws))
 supporters=[]
 for diff,team,sid,equipped in itertools.product((-6,-5,0,5,6),(0,16),(255,37,35),(0,1)):
  o.uc.mem_write(a&0x1fffff,bytes([255])*(448*21));ptr=a+448
  o.uc.mem_write(ptr&0x1fffff,bytes(448));byte(ptr,1);byte(ptr+1,1);byte(ptr+5,team);byte(ptr+0x47,1);o.uc.mem_write((ptr&0x1fffff)+0x48,struct.pack('<H',1));byte(ptr+0x92,128 if equipped else 0);byte(ptr+0x58,*(mask()if sid==255 else mask(sid)));byte(0x800e4e9c,16);byte(0x800e4ea0,16);height=6+diff
  result=o.call('supporter',0,1,1,6);supporters.append((diff&65535,team,sid,equipped,result))
 choco=[]
 for ab,ma,up,faith,undead in itertools.product((268,269),(5,14,99),(0,1),(0,100),(0,1)):
  reset(ab=ab);byte(a+0x37,ma);byte(a+0x90,4 if up else 0);byte(a+0x26,faith);byte(t+0x26,faith);byte(t+0x58,16 if undead else 0);byte(t+0x5b,128);byte(t+0x1bb,16 if undead else 0);byte(t+0x1be,128);rng=1;draws=0;o.call('heal'if ab==269 else 'esuna');result=bytes(o.uc.mem_read((t&0x1fffff)+0x18c,44))
  choco.append((ab,ma,up,faith,undead,result[0],struct.unpack_from('<H',result,4)[0],struct.unpack_from('<H',result,6)[0],struct.unpack_from('<H',result,42)[0],draws))
 groups=dict(talk=talk,train=train,poach=poach,find=find,supporters=supporters,choco=choco);blob=bytearray();lines=[]
 for name,rows in groups.items():
  lines.extend(('#define DISCOVERY_'+name.upper()+' '+str(len(rows)), '#define DISCOVERY_'+name.upper()+'_OFFSET '+str(len(blob))))
  blob.extend(b''.join(struct.pack('>'+str(len(row))+'H',*row)for row in rows))
 # A data fixture remains below one TI variable, split if the retail map census grows.
 assert len(blob)<64000,len(blob)
 lines.append('#define DISCOVERY_BYTES '+str(len(blob)))
 (out/'ffdn0.bin').write_bytes(blob);(out/'counts.h').write_text('\n'.join(lines)+'\n')
 (out/'source.json').write_text(json.dumps(dict(counts={k:len(v)for k,v in groups.items()},functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope='Unchanged retail decision routines. Controlled RNG. Discovery isolates tile lookup and current-map event variable, then runs original flag writer. Monster Skill isolates total-height calculation to exercise its exact eligibility/height boundaries; native total-height is checked separately. No original full battle, Fur Shop UI or monster art.'),indent=2)+'\n')
 print({k:len(v)for k,v in groups.items()},'bytes',len(blob))
if __name__=='__main__':main()
