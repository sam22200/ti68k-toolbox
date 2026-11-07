#!/usr/bin/env python3
"""Bounded retail MIPS movement configuration, bonuses and Teleport RNG contract."""
import json,struct
import surface_data as s
from status_state_oracle import bind_function
IDS=(0,486,487,488,489,490,491,492,498,500,501,502,503,504,506,507)

def main():
 import unicorn
 d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC));o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=1000000
 for key,name,section in (("movement","battle_move_store_unit_movement_to_scratchpad","battle"),("bonus","main_unit_calculate_move_jump","main"),("teleport","battle_move_calculate_teleport_chances","battle"),("weather","battle_move_get_weather_terrain_cost","battle"),("roll","main_util_roll_pass_fail","main")):
  o.function_hashes[key]=bind_function(key,name,section,d)
 weather=1;roll=0;draws=0
 def hook(uc,address,size,data):
  nonlocal draws
  if address==s.FUNCTIONS["weather"][0]:value=weather
  else:
   maximum=uc.reg_read(o.reg.UC_MIPS_REG_A0);threshold=uc.reg_read(o.reg.UC_MIPS_REG_A1)
   if threshold&0x80000000:threshold-=0x100000000
   value=int((roll*maximum>>15)>=threshold);draws+=1
  uc.reg_write(o.reg.UC_MIPS_REG_V0,value);uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
 for key in("weather","roll"):
  at=s.FUNCTIONS[key][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,hook,begin=at,end=at)
 o.write(0x8018f4f0,0x801d6000);o.write(0x8018f4e8,0x801d1000);o.write(0x800e4e9c,3);o.write(0x800e4ea0,1)
 for j in range(21):o.uc.mem_write(0x1908cc+448*j,bytes(448));o.uc.mem_write(0x1908cd+448*j,b"\xff")
 o.uc.mem_write(0x1908cd,b"\0");o.uc.mem_write(0x1908cf,b"\x4a");o.uc.mem_write(0x1908d2,b"\x80")
 for j in range(512):o.uc.mem_write((s.TILES&0x1fffff)+8*j,bytes((0,0,2,0,0,0,1,0)))
 configs=[];bonuses=[];tele=[]
 for ab in IDS:
  flags=bytearray(3)
  if ab:flags[(ab-486)>>3]|=128>>((ab-486)&7)
  for status in(0,2,4,64):
   for weather in(1,3):
    o.uc.mem_write(0x1d0000,bytes(40));o.uc.mem_write(0x19095f,bytes(flags));o.uc.mem_write(0x190926,bytes((status,)));o.uc.mem_write(0x190906,bytes((4,3)))
    o.call("movement",0);c=bytes(o.uc.mem_read(0x1d0000,40));cost=bytes(o.uc.mem_read(0x1d5000,64))
    configs.append([ab,status,weather,*[c[i]for i in(2,4,5,6,14,15,16,17,18,19,20,27,28,29,36,37)],*cost])
 for flags in(0,128,64,32,16,8,4,252):
  for move,jump in((4,3),(252,7)):
   o.uc.mem_write(0x19095f,bytes((flags,0,0)));o.uc.mem_write(0x190906,bytes((move,jump)));o.call("bonus",0x801908cc,1)
   bonuses.append([flags,move,jump,*o.uc.mem_read(0x190906,2)])
 for distance in(0,3,4,5,8,13,14,31):
  for roll in(0,16384,32767):
   c=bytearray(40);c[6]=4;c[10]=distance;c[36]=8;o.uc.mem_write(0x1d0000,bytes(c));draws=0
   result=o.call("teleport");tele.append([distance,roll,result,draws])
 out=s.HERE/"campaign/x/core-movement/oracle";out.mkdir(parents=True,exist_ok=True)
 lines=["/* Original movement functions, isolated weather and controlled RNG. */", "static const u8 source_movement_costs[384]={"+",".join(map(str,d.at(0x8005ea50,384)))+"};"]
 for name,rows,typ in(("movement_config",configs,"u16"),("movement_bonus",bonuses,"u16"),("movement_teleport",tele,"u16")):
  lines += [f"static const {typ} source_{name}[][{len(rows[0])}]={{"]+[" {"+",".join(map(str,r))+"},"for r in rows]+["};"]
 (out/"source.h").write_text("\n".join(lines)+"\n")
 (out/"source.json").write_text(json.dumps(dict(configurations=configs,bonuses=bonuses,teleport=tele,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),scope="Unmodified retail movement config, Move/Jump bonus and Teleport helper; weather return and RNG isolated. Not complete original pathing/AI/mount/animation replay."),indent=2)+"\n")
 print("Original movement:",len(configs),"configurations,",len(bonuses),"bonus boundaries,",len(tele),"Teleport trials")
if __name__=="__main__":main()
