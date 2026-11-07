#!/usr/bin/env python3
"""Unchanged retail Mime rotation, eligibility and record routines in Unicorn."""
import itertools,json,struct
import surface_data as s
from status_state_oracle import bind_function

def main():
 import unicorn
 out=s.HERE/'campaign/x/core-mime/oracle';out.mkdir(parents=True,exist_ok=True)
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
 names=dict(mime_rotate='battle_map_resolve_rotated_offset_layer',mime_select='battle_action_set_mimic_ability',mime_record='battle_action_set_current_attacker_data',mime_react='battle_action_can_unit_react',mime_panels='battle_target_set_panels_for_action',mime_prepare='battle_action_prepare_attack')
 for k,n in names.items():bind_function(k,n,'battle',d)
 o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=500000
 unit=0x801908cc;record=0x8019389c;face=(3,1,2,0);rotations=[]
 def put(at,blob):o.uc.mem_write(at&0x1fffff,bytes(blob))
 def byte(at,*v):put(at,bytes(v))
 o.write(0x800e4e9c,12);o.write(0x800e4ea0,12)
 terrain=bytearray(4096)
 for i in range(512):terrain[8*i+2]=10;terrain[8*i+6]=int(i>=256)
 put(s.TILES,bytes(terrain));put(unit,bytes(448));byte(unit+0x47,5);put(unit+0x48,struct.pack('<H',5))
 for donor,mime,dx,dz in itertools.product(range(4),range(4),(-3,0,2),(-2,0,3)):
  put(0x801fe010,struct.pack('<3I',0x801d6000,0x801d6004,0x801d6008))
  result=o.call('mime_rotate',unit,dx,dz,(face[mime]-face[donor])&3)
  x,z,layer=struct.unpack('<3i',o.uc.mem_read(0x1d6000,12));rotations.append((donor,mime,dx,dz,result&65535,x,z,layer))
 floors=[]
 for current,lo,hi,lb,hb,jump in itertools.product((0,10,20),(0,5,10,15),(0,5,10,15),(0,1),(0,1),(0,1)):
  byte(s.TILES+8*(5*12+5)+2,current)
  for layer,height,blocked in ((0,lo,lb),(1,hi,hb)):
   byte(s.TILES+8*(5*12+6+256*layer)+2,height);byte(s.TILES+8*(5*12+6+256*layer)+6,blocked)
  put(0x801fe010,struct.pack('<3I',0x801d6000,0x801d6004,0x801d6008))
  result=o.call('mime_rotate',unit,1,0,jump<<8);layer=struct.unpack('<i',o.uc.mem_read(0x1d6008,4))[0]
  floors.append((current,lo,hi,lb,hb,jump,result&65535,layer if result==0 else 65535))
 # Outer selection executes unchanged. Isolate reaction eligibility and preview
 # to inspect menu flags, team/self filtering, borrowed weapons and CT cleanup.
 blocked=0;calls=[];preview=0;panels=2
 def external(uc,address,size,data):
  value=0
  if address==s.FUNCTIONS['mime_react'][0]:value=blocked
  elif address==s.FUNCTIONS['mime_panels'][0]:value=panels
  elif address==s.FUNCTIONS['mime_prepare'][0]:
   ptr=uc.reg_read(o.reg.UC_MIPS_REG_A0);action=bytes(uc.mem_read(ptr&0x1fffff,20));calls.append(dict(action=action.hex(),weapons=list(uc.mem_read((unit&0x1fffff)+0x1d,1))+list(uc.mem_read((unit&0x1fffff)+0x1f,1)),state=struct.unpack('<I',uc.mem_read(0x18f5fc,4))[0]));value=preview
  uc.reg_write(o.reg.UC_MIPS_REG_V0,value&0xffffffff);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 hooks=[]
 for k in ('mime_react','mime_panels','mime_prepare'):
  at=s.FUNCTIONS[k][0];hooks.append(o.uc.hook_add(unicorn.UC_HOOK_CODE,external,begin=at,end=at))
 selections=[]
 for skill,ab in ((0,0),(6,368),(20,382),(18,394),(21,16),(11,16),(22,86),(23,94),(17,126),(14,76),(176,265),(11,65)):
  for blocked,enemy,self_target,preview,panels in itertools.product((0,1),(0,1),(0,1),(0,-1),(2,3)):
   put(unit,bytes(448));byte(unit+0x18a,1);byte(unit+3,93);byte(unit+5,16 if enemy else 0);byte(unit+0x47,5);put(unit+0x48,struct.pack('<H',5));byte(unit+0x1d,255);byte(unit+0x1f,255)
   action=bytearray(20);action[0]=1 if self_target else 0;action[1]=skill;struct.pack_into('<H',action,2,ab)
   put(record,bytes((0,1,19,20,0,0))+struct.pack('<hh',0,0)+action)
   o.write(0x8018f5fc,0);calls.clear();result=o.call('mime_select',unit)
   selections.append(dict(skill=skill,ability=ab,blocked=blocked,enemy=enemy,self_target=self_target,preview=preview,panels=panels,result=result&65535,calls=calls[:],weapons=[o.uc.mem_read((unit&0x1fffff)+off,1)[0]for off in(0x1d,0x1f)],state=struct.unpack('<I',o.uc.mem_read(0x18f5fc,4))[0]))
 # Assert the isolated outer contract independently of the native implementation.
 for row in selections:
  menu=d.at(0x80065cb4+row['skill'],1)[0];no_mimic=menu in (0,3,6) and bool(d.at(s.battle.ABILITY_RANGE+14*row['ability']+5,1)[0]&16)
  rejected=row['blocked'] or row['enemy'] or row['self_target'] or no_mimic
  expected=0 if rejected else 65535 if row['panels']==3 or row['preview']<0 else 1
  assert row['result']==expected,row
  assert row['state']==0
  if row['calls']:assert row['calls'][0]['state']==2 and row['calls'][0]['weapons']==[19,20]
  if row['result']==65535:assert row['weapons']==[255,255]
 for hook in hooks:o.uc.hook_del(hook)
 live=[]
 from status_state_oracle import mask
 o.function_hashes['status_init']=bind_function('status_init','main_status_init_check_data','main',d);o.call('status_init')
 for sid in range(40):
  put(unit,bytes(448));byte(unit+1,0);byte(unit+6,128);put(unit+0x28,struct.pack('<H',100));byte(unit+0x47,5);put(unit+0x48,struct.pack('<H',5));put(unit+0x58,mask(sid));o.write(0x8018f5f0,0)
  byte(s.TILES+8*(5*12+5)+3,0);byte(s.TILES+8*(5*12+5)+6,0)
  live.append(int(o.call('mime_react',unit)==0))
 records=[]
 for job,follow in itertools.product((74,93),(0,1)):
  put(unit,bytes(448));byte(unit+3,job);byte(unit+5,32);byte(unit+0x47,5);put(unit+0x48,struct.pack('<H',(2<<8)|4));byte(unit+0x1d,19);byte(unit+0x1f,20)
  action=bytearray(20);action[0]=0;action[1]=11;struct.pack_into('<H',action,2,16);action[10]=6 if follow else 5;action[11]=1;struct.pack_into('<3H',action,12,8,0,7);put(unit+0x16e,action)
  target=unit+448;put(target,bytes(448));byte(target+0x47,3);put(target+0x48,struct.pack('<H',9))
  put(record,bytes(30));o.call('mime_record',unit);v=bytes(o.uc.mem_read(record&0x1fffff,30))
  if job==93:assert v==bytes(30)
  else:
   assert v[:5]==bytes((2,1,19,20,32));assert struct.unpack_from('<hh',v,6)==((-2,5)if follow else(3,3));assert v[10:]==action
  records.append(dict(job=job,follow=follow,bytes=v.hex()))
 (out/'rotations.h').write_text('/* Retail MIPS rotations, source facings converted through the measured map. */\nstatic const s16 source_rotations[][8]={\n'+''.join(' {'+','.join(str(v if v<32768 else v-65536)for v in row)+'},\n'for row in rotations)+'};\nstatic const s16 source_floors[][8]={\n'+''.join(' {'+','.join(str(v if v<32768 else v-65536)for v in row)+'},\n'for row in floors)+'};\nstatic const u8 source_mime_live[40]={'+','.join(map(str,live))+'};\n')
 (out/'source.json').write_text(json.dumps(dict(rotations=len(rotations),floors=len(floors),selections=selections,live=live,records=records,menus={str(i):d.at(0x80065cb4+i,1)[0]for i in(0,6,11,14,16,17,18,19,20,21,22,23,176)},functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope='Unchanged retail rotation and outer Mime selector. Selector isolates reaction eligibility, panel validation and effect preparation; rotation uses actual controlled terrain. No whole PS1 battle replay or fidelity claim for isolated callees.'),indent=2)+'\n')
 print('Mime original:',len(rotations),'rotations,',len(floors),'floor selections,',len(selections),'outer selector profiles')
if __name__=='__main__':main()
