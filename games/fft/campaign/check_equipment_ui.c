// First-frame equipment feedback, draft cancellation and atomic Done command.
#define EQUIPMENT_UI_CHECK
#include "check_equipment.c"
static u16 pictures;
static void capture(void)
{
 const u8 *l=rt_light,*d=rt_dark;u16 y,x;u32 h=17;
 for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];}
 digest=(digest<<1|digest>>31)^h;pictures++;
#ifndef __m68k__
 {char file[160];sprintf(file,"/tmp/fft-equipment-ui-%u.png",pictures);sw_write_png(file,4);}
#endif
}
static void picture(void)
{
 u8 i;ZB(61);equipment_control(3);ZE(61);CHECK(W->status_n>=1,1);capture();
 for(i=0;i<5;i++){ZB(62);equipment_control(3);ZE(62);}capture();
}
static void press(u32 pressed){rt_prev=0;rt_keys=pressed;ZB(63);game_update();ZE(63);rt_keys=0;}
static void ui_trials(void)
{
 Unit *u;u32 seed;u16 hp,mp;State before;u8 inv[256];u8 i;
 arena();u=&st.unit[U_RAMZA];u->job=76;st.support[U_RAMZA]=480;st.mode=M_ACTS;st.sub=2+u->defend;seed=rng32;
 press(K_ENTER);CHECK(st.mode,M_EQUIP);CHECK(st.acted,0);picture();
 before=st;memcpy(inv,battle_save.inv,256);st.sub=3;press(K_ENTER);CHECK(st.status_col,1);CHECK(W->ai_no,1);picture();
 press(K_B);CHECK(st.status_col,0);press(K_B);CHECK(st.mode,M_ACTS);CHECK(st.acted,0);CHECK(memcmp(inv,battle_save.inv,256)==0,1);CHECK(rng32==seed,1);
 battle_save.inv[19]=1;battle_save.inv[20]=2;equipment_control(1);st.sub=3;press(K_ENTER);CHECK(W->ai_no,3);picture();
 st.sub=1;press(K_ENTER);CHECK(W->ai_tt[3],19);CHECK(u->eq[3],255);CHECK(battle_save.inv[19],1);picture();
 st.sub=5;hp=u->hp;mp=u->mp;press(K_ENTER);CHECK(st.mode,M_MENU);CHECK(st.acted,1);CHECK(u->eq[3],19);CHECK(battle_save.inv[19],0);CHECK(u->hp<=hp,1);CHECK(u->mp<=mp,1);CHECK(rng32==seed,1);
 // Long inventory pages and repeated open/cancel do not change stock or action history.
 arena();u=&st.unit[U_RAMZA];u->job=76;st.support[U_RAMZA]=480;
 for(i=19;i<35;i++)battle_save.inv[i]=1;
 equipment_control(1);st.sub=3;press(K_ENTER);CHECK(W->ai_no>8,1);st.sub=8;picture();press(K_B);press(K_B);CHECK(st.acted,0);
}
#ifndef __m68k__
int main(void){sw_init(61);ui_trials();printf("equipment_ui %08lX failures %lu checks %lu pictures %u\n",(unsigned long)digest,(unsigned long)failures,(unsigned long)checks,pictures);return failures!=0;}
#else
void _main(void){equipment_runtime_main();ui_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
