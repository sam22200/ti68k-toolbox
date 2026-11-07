#!/usr/bin/env python3
"""Unchanged retail weapon arithmetic, conditional status and spell-proc packets."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-weapons/oracle';out.mkdir(parents=True,exist_ok=True)
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 names=dict(base='calculate_base_xa',charge='apply_charge',physical='calculate_physical_damage',damage='calculate_weapon_damage',heal='weapon_heal',gun='gun_damage',magic='magic_gun',absorb='apply_hp_absorption',status='apply_status',proc='store_reaction_proc_id_and_target',recoil='calculate_knockback',random='get_random_0_7fff')
 for k,n in names.items():bind_function(k,'battle_formula_'+n,'battle',d)
 bind_function('finalize','battle_action_finalize_target_current_action','battle',d)
 bind_function('weather','battle_map_get_effective_weather','battle',d)
 o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=300000
 for k,n in [('lib_rand','rand'),('init','main_status_init_check_data')]:o.function_hashes[k]=bind_function(k,n,'main',d)
 o.call('init')
 a,t,v,aa=0x801908cc,0x801d4000,0x801938c0,0x80192da0
 seed=1;draws=[];weather=0
 def random():
  nonlocal seed
  seed^=(seed<<13)&0xffffffff;seed^=seed>>17;seed^=(seed<<5)&0xffffffff;seed&=0xffffffff
  return seed>>17
 def external(uc,address,size,data):
  value=0
  if address==s.FUNCTIONS['lib_rand'][0]:value=random();draws.append(value)
  elif address==s.FUNCTIONS['weather'][0]:value=weather
  uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for name in ('lib_rand','recoil','weather','finalize'):
  at=s.FUNCTIONS[name][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
 def byte(at,*values):o.uc.mem_write(at&0x1fffff,bytes(values))
 def read(at,n):return bytes(o.uc.mem_read(at&0x1fffff,n))
 def profile(weapon,**kw):
  p=dict(weapon=weapon,pa=5,ma=11,sp=9,brave=60,af=70,tf=60,power=0,asu=0,tsu=0,ast=255,tst=255,az=0,tz=0,sex=0,weather=0,absorb=0,null=0,half=0,weak=0,strength=0,seed=1,preview=0)
  p.update(kw);return p
 profiles=[]
 # Every equippable weapon record plus all three non-equipment ball records.
 for w in range(125):
  profiles.extend([profile(w),profile(w,preview=1)])
 special=[w for w in range(125)if any(d.at(s.battle.WEAPONS+8*w,8)[i]for i in (6,7))or d.at(s.battle.WEAPONS+8*w,8)[2]in(4,6,7)]
 for w in special:
  raw=d.at(s.battle.WEAPONS+8*w,8);e=raw[6]
  for rnd in (2,17,32,63,101,731):profiles.append(profile(w,seed=rnd))
  for power in (3,20):profiles.append(profile(w,power=power))
  if e:
   for key in ('absorb','null','half','weak','strength'):profiles.append(profile(w,**{key:e}))
   profiles.append(profile(w,half=e,weak=e,strength=e,power=3))
  if raw[2]==1 and raw[7] in(26,28):
   for st in(10,12,16,22,24,29,30,36,37,39):profiles.append(profile(w,tst=st))
  if raw[2]in(6,7):
   for st in(3,4,22,26,35):profiles.append(profile(w,tst=st,power=3,tz=3))
  if raw[2]==4:
   for af,tf in [(0,70),(70,0),(100,100),(1,1)]:profiles.append(profile(w,af=af,tf=tf))
   for st in (16,22,21,27,32,33,38,39):profiles.append(profile(w,tst=st))
   for st in(32,33):profiles.append(profile(w,ast=st))
   for wx in range(8):profiles.append(profile(w,weather=wx))
   for su in(467,468):profiles.append(profile(w,asu=su))
   profiles.append(profile(w,tsu=468))
   profiles.append(profile(w,asu=467,tsu=468,tst=27,strength=e,weak=e,power=20))
 # Exact probability boundaries, keeping the unchanged retail random helper.
 def samples(initial,n=8):
  state=initial;values=[]
  for _ in range(n):
   state^=(state<<13)&0xffffffff;state^=state>>17;state^=(state<<5)&0xffffffff;state&=0xffffffff
   values.append((state>>17)*100//32768)
  return values
 for pct in(9,10,39,40,99):
  initial=next(i for i in range(1,65536)if samples(i,1)[0]==pct)
  for w in(74,75,76):profiles.append(profile(w,seed=initial))
 for w in(3,53,68,113):
  variable=d.item(w)['type']in(6,9,17);critical=5 if variable else 4
  for pct in(18,19):
   initial=next(i for i in range(1,65536)if samples(i)[critical]>=4 and samples(i)[critical+1]==pct)
   profiles.append(profile(w,seed=initial,tst=24 if w==113 else 255))
 for w in(23,61,75,53,93):
  for preview in(0,1):profiles.append(profile(w,pa=99,ma=99,sp=50,brave=100,power=20,seed=731,preview=preview))
 profiles.append(profile(31,tst=22))
 profiles.append(profile(94,tst=34))
 rows=[];records=[]
 for p in profiles:
  w=p['weapon'];raw=d.at(s.battle.WEAPONS+8*w,8);f=raw[2];seed=p['seed'];weather=p['weather'];draws.clear()
  for ptr in(a,t):
   o.uc.mem_write(ptr&0x1fffff,bytes(448));byte(ptr+6,128);byte(ptr+0x1a,*([255]*7));o.uc.mem_write((ptr&0x1fffff)+0x2a,struct.pack('<HHHH',999,999,100,100))
  for ptr in(v,aa):o.uc.mem_write(ptr&0x1fffff,bytes(80))
  o.write(0x80192d94,a);o.write(0x80192d98,t);o.write(0x80192d90,t+0x18c);o.write(0x80192d8c,aa);o.write(0x8018f5fc,2 if p['preview']else 0)
  byte(a+0x36,p['pa'],p['ma'],p['sp']);byte(a+0x24,p['brave'],p['af']);byte(t+0x25,p['tf']);o.uc.mem_write((a&0x1fffff)+8,struct.pack('<H',p['az']<<12));o.uc.mem_write((t&0x1fffff)+8,struct.pack('<H',p['tz']<<12))
  byte(a+6,64 if p['sex'] else 128);byte(t+6,128)
  byte(v+0x18,w);byte(v+0x24,p['power']);byte(v+0x12,p['tf'],p['af']);byte(t+0x18c,1)
  o.uc.mem_write((v&0x1fffff)+0x3e,raw)
  if f==1 and raw[7]:o.uc.mem_write((v&0x1fffff)+0x46,d.at(s.battle.STATUS_SET+6*raw[7],6))
  for ptr,sid,su in((a,p['ast'],p['asu']),(t,p['tst'],p['tsu'])):
   current=mask(*(([sid]if sid!=255 else[])+([35]if ptr==t else[])))
   o.uc.mem_write((ptr&0x1fffff)+0x58,current);o.uc.mem_write((ptr&0x1fffff)+0x1bb,current)
   if su:
    bit=su-454;byte(ptr+0x8f+(bit>>3),128>>(bit&7))
  o.uc.mem_write((a&0x1fffff)+0x6d,bytes((0,0,0,0,p['strength'])))
  o.uc.mem_write((t&0x1fffff)+0x6d,bytes(p[k]for k in('absorb','null','half','weak'))+b'\0')
  # Native guard-free evade still draws four physical sources, even at zero.
  if not p['preview']and f not in(3,4,6):
   for _ in range(4):random();draws.append(-1)
  conditional=1
  if f in(1,2):
   conditional=o.call('damage')
   if conditional==0:
    if f==1:o.call('status')
    else:o.call('proc')
  elif f==3:o.call('gun')
  elif f==4:o.call('magic')
  elif f==6:o.call('heal')
  elif f==7:
   o.call('base');o.call('charge');o.call('physical');o.call('absorb')
  else:raise AssertionError(f)
  action=read(t+0x18c,44);dmg,heal=struct.unpack_from('<hh',action,4);kind=0 if not action[0]else 3 if action[37]&64 else 0
  amount=heal if kind==3 else dmg
  if amount>999 and not p['power']:amount=999
  packet=0
  if f==7:packet=16384|(32768 if kind==3 else 0)
  if f==2 and conditional==0:packet=raw[7]
  if f==1 and conditional==0:
   chosen=[i for i in range(40)if action[27+(i>>3)]&(128>>(i&7))]
   assert len(chosen)<=1,(p,chosen)
   if chosen:packet=(chosen[0]+1)<<8
   if any(action[32:37]):packet=32768|(raw[7]<<8)
  if not action[0]:kind=8;amount=0;packet=0
  fields=[p[k]for k in('weapon','pa','ma','sp','brave','af','tf','power','asu','tsu','ast','tst','az','tz','sex','weather','absorb','null','half','weak','strength','seed','preview')]+[amount&65535,kind,action[1],packet,len(draws),seed>>16,seed&65535]
  assert len(fields)==30
  records.append(struct.pack('>30H',*fields));rows.append(dict(input=p,amount=amount,kind=kind,critical=action[1],packet=packet,draws=len(draws),after=seed))
 (out/'fwfx.bin').write_bytes(b''.join(records))
 (out/'source.h').write_text('/* Generated retail weapon packets, big-endian30-word records. */\n#define WEAPON_EFFECT_CASES '+str(len(rows))+'\n#define WEAPON_EFFECT_FIELDS 30\n')
 (out/'source.json').write_text(json.dumps(dict(cases=len(rows),rows=rows,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope='Unchanged retail formula instructions, conditional19% status/proc selection and status filtering. Native xorshift supplied only at libc rand; original random helper runs. Weather getter, recoil and post-arithmetic drain finalizer isolated. No source full animation/dispatcher certification.'),indent=2)+'\n')
 print('Original weapon-effect profiles:',len(rows),flush=True)
if __name__=='__main__':main()
