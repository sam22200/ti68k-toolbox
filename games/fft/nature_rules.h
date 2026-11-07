#ifndef FFT_NATURE_RULES_H
#define FFT_NATURE_RULES_H
#define NATURE_DATA (cj_section(common,CJ_BATTLE)+CJ_DISCOVERY)
// Frozen spell masks and monster lists live in existing tactical scratch/state.
#if !MOD_CORE && MOD_PART(MOD_PARTY)
static void monster_refresh(void)
{
 u8 i;W->nature_units=0;
 for(i=0;i<NUNIT;i++)if(unit_monster(&st.unit[i]))W->nature_units|=(u16)1<<i;
}
#else
static s16 monster_height(u8 who)
{
 const Unit *u=&st.unit[who];u8 j,f[3],offset=0,source=who;const u8 *p;s16 h;u8 depth;
 for(j=0;j<NUNIT;j++)if(j!=who&&unit_on_field(&st.unit[j])&&UNIT_CELL(&st.unit[j])==UNIT_CELL(u)){
  if(u->job<94||u->job>96){u=&st.unit[j];source=j;}offset=2;break;
 }
 p=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u));
 h=(p[0]<<1)+(p[1]&31);depth=p[1]>>5;cj_movement(common,u->job,st.movement[source],f);
 if(core_status_current(&u->statuses,2)&64)h+=(depth<<1)+2;
 else if(depth){if(f[2]&128)h+=depth<<1;else if(f[2]&64)h+=(depth<<1)-2;}
 return (s8)(h+offset+((core_status_current(&u->statuses,2)&6)?4:6));
}
static void monster_refresh(void)
{
 u8 i,j;W->nature_units=0;
 for(i=0;i<NUNIT;i++)if(unit_monster(&st.unit[i])){
  Unit *u=&st.unit[i];W->nature_units|=(u16)1<<i;u->set=cj_section(common,CJ_JOB_SETS)[u->job];u->learned=7;
  for(j=0;j<NUNIT;j++)if(i!=j){
   const Unit *v=&st.unit[j];s16 dx=(s16)u->x-v->x,dz=(s16)u->z-v->z,h;
   if(!unit_on_field(u)||!unit_on_field(v)||!v->hp||(core_status_current(&v->statuses,0)&0x64)||
      (core_status_current(&v->statuses,1)&0x95)||(core_status_current(&v->statuses,2)&0x0e)||
      (core_status_current(&v->statuses,3)&2)||(core_status_current(&v->statuses,4)&0x14)||
      ((u->team==TEAM_ENEMY)!=(v->team==TEAM_ENEMY))||!cj_support(common,v->job,st.support[j],478))continue;
   if((dx<0?-dx:dx)+(dz<0?-dz:dz)!=1)continue;
   h=monster_height(i)-monster_height(j);
   if(h>=-5&&h<=5){u->learned=15;break;}
  }
 }
}
#endif
#endif
