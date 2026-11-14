// Actual management/deployment pages with private, explicitly generated art hooks.
// The installed selectors, saves, battle code and source files stay unchanged.
#include <assert.h>
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#define game_scenario source_scenario
#include REVIEW_SOURCE
#undef game_scenario
static const u8 subjects[]={94,95,133,140,130,127};
void game_scenario(u16 n)
{
    u8 i;u16 size;
    source_scenario(0);assert(bank&&common&&cache&&save.n==7);
    review_art=rt_file("ffmr",&size);assert(review_art&&size==35768&&!memcmp(review_art,"FMR0",4));
    review_enabled=n!=0&&n<100;n%=100;save.n=10;
    for(i=1;i<8;i++){
        SaveUnit base=save.unit[1];save.unit[i]=base;
        save.unit[i].type=3;save.unit[i].job=i==7?94:subjects[i-1];save.unit[i].gfx=255;
        save.unit[i].name=(u8)(i*7);save.unit[i].level=1;save.unit[i].exp=0;
        memset(save.unit[i].eq,0,sizeof save.unit[i].eq);
        memset(save.unit[i].learned,0,sizeof save.unit[i].learned);
        world_party_clear_slot(&save.world,i);
    }
    save.world.egg[7]=0x32;
    save.unit[8]=save.unit[0];save.unit[9]=save.unit[0];
#ifdef MONSTER_DEPLOY_REVIEW
    // Visual fixture only: placement bypasses the still-closed production gate.
    memset(dp.placed,255,sizeof dp.placed);dp.n=3;
    dp.placed[12]=0;dp.placed[17]=3;dp.placed[19]=1;
    dp.unit=3;dp.row=3;dp.col=2;dp.note=dp.confirm=0;stats_unit=255;
#else
    mg.unit=n>=20?(u8)(n-19):n>=10?(u8)(n-9):3;
    mg.top=mg.slide=0;mg.screen=n>=10&&n<20?S_STATUS:S_FORM;
#endif
    dirty=1;
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    sw_init((u16)atoi(argv[1]));
    {Save before=save;assert(sw_step(0));assert(!memcmp(&before,&save,sizeof save));}
    printf("monster-formation-review %04X\n",sw_checksum());
    return sw_write_png(argv[2],1);
}
