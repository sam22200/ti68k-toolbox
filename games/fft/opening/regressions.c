// User-reported combat/story regressions, through the actual shared engine.
#include "opening.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    static const u8 ramza_dir[4]={1,0,2,3};
    u8 i;u16 hp;char text[32];
    if(access("ffoa.bin",R_OK))assert(!chdir("data"));
    sw_init(1);
    // Source tactical record faces -z. The stale scene actor angle was +x.
    assert(st.unit[U_RAMZA].face==3);
    for(i=0;i<4;i++)assert(unit_direction(i,U_RAMZA)==ramza_dir[i]);
    assert(skill_amount(9,9,9,0)==30); // named Potion, never 100 HP
    forecast_text(0,5,1,text);assert(!strstr(text,"-20"));
    // Charge commits a fixed tile, consumes no HP immediately and leaves its user present.
    st.ai=0;st.act=6;hp=st.unit[0].hp;skill(6,0,7);
    assert(st.mode!=M_ACT&&st.pending[6].ticks==4&&st.unit[0].hp==hp);
    assert(unit_at(st.unit[6].x,st.unit[6].z)==7);
    st.unit[0].x++; // Original target leaves: resolution must never follow it.
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}
    turn_start();assert(st.unit[0].hp==hp&&!st.pending[6].state);
    // It hits a replacement occupant, not the unit originally selected.
    sw_init(1);st.ai=0;st.act=6;skill(6,0,8);
    assert(st.pending[6].ticks==5);
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}
    turn_start();assert(st.open_resolving&&st.mode==M_ACT&&st.tgt==0);
    hp=st.unit[0].hp;st.hit=1;st.act_t=ACT_HIT;attack_step();assert(st.unit[0].hp<hp);
    // Interrupted by death or displacement, without gaining an extra action.
    sw_init(1);st.ai=0;st.act=6;skill(6,0,7);st.unit[6].hp=0;
    assert(pending_next()==255&&!st.pending[6].state);
    sw_init(1);st.ai=0;st.act=6;skill(6,0,7);st.unit[6].x++;
    assert(pending_next()==255&&!st.pending[6].state);
    // A different unit can take its ordinary turn while the archer is still charging.
    sw_init(1);st.ai=0;st.act=6;skill(6,0,7);
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}
    st.unit[0].ct=100;turn_start();assert(st.act==0&&st.pending[6].ticks==4);
    // Text confirmation changes the page only; it does not rewind actor coordinates.
    sw_init(3);assert(sw_step(0));
    {s16 x=W->feet[2].x,y=W->feet[2].y;st.open_reveal=W->dialog_total;W->dialog_drawn=W->dialog_total;
        assert(sw_step(K_ENTER));assert(W->feet[2].x==x&&W->feet[2].y==y);}
    puts("Regressions: Potion 30 HP, one-value preview, CT charges/tile lock/interruption, stable scene confirmed");
    return 0;
}
