#include "nature_rules.h"
static u8 discovery_find(u8 who,u8 map)
{
 Unit *u=&st.unit[who];const u8 *row;u8 f[3],i,bit,item;
 if(map>=128||!u->hp||!unit_on_field(u))return 0;
 cj_movement(common,u->job,st.movement[who],f);if(!(f[2]&1))return 0;
 row=NATURE_DATA+96+((u16)map<<4);
 for(i=0;i<4;i++,row+=4){
  if((row[1]&32)||!(row[2]|row[3])||(row[0]>>4)!=u->x||(row[0]&15)!=u->z||(row[1]>>7)!=node_layer(UNIT_CELL(u)))continue;
  bit=128>>(((map&1)<<2)+i);if(battle_save.discovered[map>>1]&bit)return 0;
  item=row[((u16)(muls16((s16)rand15(),100)>>15)<100-u->brave)?2:3];
  battle_save.discovered[map>>1]|=bit;
  if(item&&item!=255&&unit_home_team(u)!=TEAM_ENEMY){
   u16 count=battle_save.inv[item];u8 j,k;
   for(j=0;j<battle_save.n;j++)for(k=0;k<5;k++)if(battle_save.unit[j].eq[k]==item)count++;
   for(j=0;j<NUNIT;j++)if(st.unit[j].slot<battle_save.n){
    for(k=0;k<5;k++){if(battle_save.unit[st.unit[j].slot].eq[k]==item)count--;if(st.unit[j].eq[k]==item)count++;}
   }
   if(count<99)battle_save.inv[item]++;
   else{u32 money=qlong(battle_save.money)+(cj_word(NATURE_DATA+96+2048+64+((u16)item<<1))>>2);qput(battle_save.money,money>99999999UL?99999999UL:money);}
   if(item>=240)st.bag[item-240]=battle_save.inv[item];
   st.pick=item;st.abil=NO_ABIL;st.pick_t=1;st.mode=M_PICK;
  }
  return 1;
 }
 return 0;
}
static u8 discovery_move(u8 who)
{
 u16 loc=CD(LOCATION);return loc<64?discovery_find(who,NATURE_DATA[96+2048+loc]):0;
}
