#!/usr/bin/env python3
"""Retail physical weapon XA/YA, Charge, support/status and gun arithmetic."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function,mask

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-weapon-formulas/oracle';out.mkdir(parents=True,exist_ok=True)
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 for k,n in dict(base='battle_formula_calculate_base_xa',charge='battle_formula_apply_charge',physical='battle_formula_apply_physical_xa_modifying_statuses_support',critical='battle_formula_calculate_critical_hit',gun='battle_formula_gun_damage',recoil='battle_formula_calculate_knockback',random='battle_formula_get_random_0_7fff',init='battle_action_init_current_ability_strike_data',used_weapon='battle_action_store_used_weapon').items():bind_function(k,n,'battle',d)
 o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
 o.function_hashes['lib_rand']=bind_function('lib_rand','rand','main',d)
 a,t,v=0x801908cc,0x801d4000,0x801938c0;queue=[];draws=[];recoils=[]
 def external(uc,address,size,data):
  value=0
  if address==s.FUNCTIONS['lib_rand'][0]:
   assert queue,'unexpected original RNG';value=queue.pop(0);draws.append(value)
  else:recoils.append(1)
  uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for name in ('lib_rand','recoil'):
  at=s.FUNCTIONS[name][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at)
 def byte(at,*values):o.uc.mem_write(at&0x1fffff,bytes(values))
 weapons=[0,1,11,19,33,38,48,51,59,67,71,77,83,92,95,99,107,115,119,122]
 profiles=[]
 for weapon in weapons:
  kind=d.item(weapon)['type'];variable=kind in(6,9,17)
  for pa,ma,sp,brave in [(5,11,9,60),(1,1,1,0),(99,97,50,100),(2,1,3,1)]:
   for charge in (0,3):
    for base in ((0,16384,32767)if variable else(16384,)):
     profiles.append((weapon,pa,ma,sp,brave,charge,0,255,255,base,32767,0,0))
 # Focused critical order and shared supports/statuses, without a Cartesian census.
 for weapon in (0,38,48,59,71,95,107,115):
  for check,amp in ((0,0),(0,32767)):
   profiles.append((weapon,5,11,9,60,3,0,255,255,32767,check,amp,0))
  for support,actor_status,target_status in [(0,20,255),(0,255,35),(0,255,22),(465,255,255),(472,255,255),(0,255,25),(0,255,26)]:
   profiles.append((weapon,7,11,9,60,0,support,actor_status,target_status,16384,32767,0,0))
  profiles.append((weapon,5,11,9,60,0,0,255,255,32767,0,32767,1))
 rows=[]
 for weapon,pa,ma,sp,brave,power,support,as_id,ts_id,base,check,amp,preview in profiles:
  for ptr in(a,t):
   o.uc.mem_write(ptr&0x1fffff,bytes(448));byte(ptr+6,128);byte(ptr+0x1a,*([255]*7))
  o.uc.mem_write(v&0x1fffff,bytes(80));o.uc.mem_write((t&0x1fffff)+0x18c,bytes(44))
  o.write(0x80192d94,a);o.write(0x80192d98,t);o.write(0x80192d90,t+0x18c);o.write(0x8018f5fc,2 if preview else 0)
  byte(a+0x36,pa,ma,sp);byte(a+0x24,brave);byte(v+0x18,weapon);byte(v+0x24,power)
  o.uc.mem_write((v&0x1fffff)+0x3e,d.at(s.battle.WEAPONS+8*weapon,8))
  if as_id!=255:o.uc.mem_write((a&0x1fffff)+0x58,mask(as_id))
  if ts_id!=255:o.uc.mem_write((t&0x1fffff)+0x58,mask(ts_id))
  if support:
   bit=support-454;byte(a+0x8f+(bit>>3),128>>(bit&7))
  variable=d.item(weapon)['type']in(6,9,17);queue[:]=[]if preview else([base]if variable else[])+[check]+([amp]if check==0 else[]);draws.clear();recoils.clear()
  if weapon==71:o.call('gun')
  else:o.call('base');o.call('charge');o.call('physical');o.call('critical')
  xa,ya=struct.unpack('<HH',o.uc.mem_read((v&0x1fffff)+14,4));critical=o.uc.mem_read((t&0x1fffff)+0x18d,1)[0]
  assert not queue,(weapon,queue)
  rows.append([255 if weapon==0 else weapon,pa,ma,sp,brave,power,support,as_id,ts_id,base,check,amp,preview,xa,ya,xa*ya if power else min(999,xa*ya),critical,len(draws)])
 dual=[]
 for weapon in range(1,128):
  w=d.at(s.battle.WEAPONS+8*weapon,8);kind=d.item(weapon)['type']
  if not(w[1]&8 and w[2]==1 and not(w[6]or w[7])and kind not in(1,2,3)):continue
  o.uc.mem_write(a&0x1fffff,bytes(448));o.uc.mem_write(v&0x1fffff,bytes(80))
  byte(a+0x16f,1);byte(0x80065cb5,8);byte(a+0x1d,weapon,255,weapon);byte(a+0x91,1);o.write(0x8018f5f0,0);o.call('init',a)
  result=[weapon,kind,o.uc.mem_read((v&0x1fffff)+2,1)[0]]
  for hand in(0,1):
   byte(v+3,hand);o.call('used_weapon',a+0x16e);result.append(o.uc.mem_read((v&0x1fffff)+0x18,1)[0])
  assert result[2:]==[2,weapon,weapon];dual.append(result)
 (out/'source.h').write_text('/* Generated unchanged retail primitive fixtures, external big-endian rows. */\n#define SOURCE_WEAPON_DAMAGE_CASES '+str(len(rows))+'\n#define SOURCE_WEAPON_DAMAGE_FIELDS 18\n')
 with(out/'source.h').open('a')as f:
  f.write('static const u8 source_weapon_dual[][5]={\n')
  for row in dual:f.write(' {'+','.join(map(str,row))+'},\n')
  f.write('};\n')
 (out/'fwform.bin').write_bytes(b''.join(struct.pack('>18H',*row)for row in rows))
 (out/'source.json').write_text(json.dumps(dict(cases=rows,new_dual=dual,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope='Actual base-XA/Charge/physical support/status/zodiac/critical and ordinary gun instructions. Retail random helper executes unchanged; only libc rand values and recoil commit isolated. Existing native999 ordinary cap; Charge retains its uncapped scalar. No elemental, healing/drain weapon, magical gun, additional status/spell proc or full dispatcher trace.'),indent=2)+'\n')
 print('Original physical weapon cases:',len(rows),flush=True)
if __name__=='__main__':main()
