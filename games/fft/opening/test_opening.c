#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define FFT_BATTLE
#include "../fft.h"
#include "../../../runtime/platform-sw/rt_sw.h"
static void press(u32 k){assert(sw_step(k));assert(sw_step(0));}
int main(void)
{
    unsigned i,frames;u8 prior;
    if(access("ffoa.bin",R_OK))assert(!chdir("data"));
    // The title and chapel are the scene player's (campaign/test_scene.c): the program
    // opens on the gate, at the end of the scene player's fade.
    sw_init(0);assert(st.open_stage==3&&st.open_fade==9);
    for(i=0;i<40;i++)assert(sw_step(0));
    assert(st.open_reveal>30);press(K_CATALOG);assert(st.open_white);
    press(K_CATALOG);assert(!st.open_white);
    prior=st.open_beat;for(i=0;i<8&&st.open_beat==prior;i++)press(K_ENTER);assert(st.open_beat>prior);   // (pages)
    // Every original message, including continuation pages, reaches the fight.
    for(i=0;(st.open_stage||st.open_fade)&&i<1000;i++)press(K_ENTER);
    assert(!st.open_stage&&!st.open_fade&&i<1000&&st.banner);
    sw_init(1);assert(st.act==0&&!st.ai&&st.mode==M_MENU);
    for(i=0;i<NUNIT;i++){
        const Unit *u=&st.unit[i];assert(u->x<MAP_W&&u->z<MAP_H);
        assert(u->max_hp>50&&u->hp==u->max_hp&&u->sp>=6);
    }
    assert(st.unit[0].max_hp==154&&st.unit[1].max_hp==207&&st.unit[2].max_hp==210);
    assert(!strcmp(unit_name(1),"Agrias")&&!strcmp(unit_name(2),"Gafgarion"));
    assert(st.unit[0].team==TEAM_PLAYER&&st.unit[1].team==TEAM_GUEST&&st.unit[5].team==TEAM_ENEMY);
    assert(!map_tiles[0].walk); // Original surface 0x11, depth four; no human wading here.
    // The actual turn menu enters Move, accepts a reachable tile, and animates it.
    press(K_ENTER);assert(st.mode==M_TARGET&&st.sel==0);
    {u8 x=st.unit[0].x,z=st.unit[0].z,c;
        for(c=0;c<MAP_W*MAP_H;c++)if(st.reach[c]!=255&&st.reach[c]&& !unit_at(c%MAP_W,c/MAP_W))break;
        assert(c<MAP_W*MAP_H);st.cx=c%MAP_W;st.cz=c/MAP_W;press(K_ENTER);
        assert(st.mode==M_WALK);for(i=0;i<400&&st.mode==M_WALK;i++)assert(sw_step(0));
        assert(st.mode==M_MENU&&(st.unit[0].x!=x||st.unit[0].z!=z));
        press(K_B);assert(st.unit[0].x==x&&st.unit[0].z==z); // ordinary Move undo
    }
    // Status/dialogue pixels alias idle portrait panels: leaving Status must
    // rebuild those panels rather than displaying its old text over the map.
    sw_init(1);st.menu=MN_STATUS;press(K_ENTER);assert(st.mode==M_STATUS);
    for(i=0;i<20;i++)assert(sw_step(0));
    press(K_ENTER);assert(st.page==1);
    for(i=0;i<20;i++)assert(sw_step(0));
    press(K_B);assert(st.mode==M_MENU);
    for(i=0;i<40;i++)assert(sw_step(0));
    assert(hud_ready());
    // Original learned techniques: Stasis is an area, Night Sword restores HP.
    sw_init(1);st.unit[2].hp=50;
    {u8 a=known(2,0);u16 hp=st.unit[2].hp;skill(2,5,a);assert(st.hit&&st.dmg>0);
        for(i=0;i<80&&st.mode==M_ACT;i++)assert(sw_step(0));
        assert(st.unit[2].hp>hp);
    }
    // Complete adapted tactical encounter, no forced HP/outcome writes.
    sw_init(2);
    for(frames=0;frames<28000&&!st.open_stage;frames++)assert(sw_step(K_A));
    assert(frames<28000&&st.open_stage==4);
    // The aftermath ends in a handoff to the scene player's chapter card, no replay.
    for(i=0;i<2000&&sw_step(i%2?0:K_ENTER);i++);
    assert(i<2000&&st.open_stage==5&&st.open_handoff);
    sw_init(5);assert(!sw_step(0)&&st.open_handoff);   // the chapter loader's test door
    sw_init(0);for(i=0;i<10;i++)assert(sw_step(0));   // the fade in, then SHIFT skips to the fight
    press(K_B);for(i=0;i<20;i++)assert(sw_step(0));assert(!st.open_stage);
    printf("Opening: source stats, story flow, palette, movement/undo, drain, complete battle and ending passed (%u battle frames)\n",frames);
    return 0;
}
