// Player-facing configuration: input routing, paused battle, low-poly turns and cache return.
#include <assert.h>
#include <stdio.h>
#include "fft.c"
#include "../../runtime/platform-sw/rt_sw.h"

static void press(u32 keys) { assert(sw_step(keys)); assert(sw_step(0)); }
static void unchanged(const State *before, u32 rng)
{
    assert(!memcmp(before->unit,st.unit,sizeof st.unit));
    assert(!memcmp(before->bag,st.bag,sizeof st.bag));
    assert(st.mode==before->mode && st.act==before->act && st.sel==before->sel);
    assert(st.cx==before->cx && st.cz==before->cz && st.tick==before->tick && rng32==rng);
}
int main(int argc,char **argv)
{
    State before; u32 rng; u16 solid,wire,low,initial_wide; unsigned i;
    sw_init(65); assert(W && pbank);
    assert(!st.config && st.pzoom && !st.pwire && st.psolid==1);
    press(K_C); assert(st.perspective && st.pdist==80 && st.ppitch==6 && !st.ptrans);
    st.tick=0; game_render(); initial_wide=sw_checksum();
    press(K_B); sw_init(65);
    before=st; rng=rng32;
    assert(sw_step(sw_parse_keys("CATALOG"))); assert(st.config && st.mode==M_MENU);
    for(i=0;i<8;i++) assert(sw_step(K_CATALOG|K_APPS));
    unchanged(&before,rng); // holding the key neither closes nor opens the AT list
    assert(sw_step(0));
    if(argc>1) assert(!sw_write_png(argv[1],4));
    press(K_ENTER); assert(!st.pzoom);
    press(K_ENTER); assert(st.pzoom);
    press(K_A); assert(!st.pzoom);
    press(K_DOWN); press(K_RIGHT); assert(st.pwire);
    unchanged(&before,rng);
    press(K_ESC); assert(!st.config && st.mode==before.mode);
    press(K_C); assert(st.perspective && st.pdist==40 && !st.ptrans && st.pwire);
    st.tick=0; game_render(); wire=sw_checksum();
    // Change both perspective preferences while inspection is paused.
    before=st; rng=rng32; press(sw_parse_keys("CATALOG"));
    press(K_ENTER); assert(st.pzoom && st.pdist==80 && st.ppitch==6);
    press(K_DOWN); press(K_ENTER); assert(!st.pwire && !pvalid);
    unchanged(&before,rng);
    press(sw_parse_keys("CATALOG")); assert(!st.config && st.perspective && !st.ptrans);
    st.tick=0; game_render(); solid=sw_checksum(); assert(solid!=wire);
    if(argc>2) assert(!sw_write_png(argv[2],4));
    press(K_D); assert(st.pwire); st.tick=0; game_render(); wire=sw_checksum(); assert(wire!=solid);
    if(argc>3) assert(!sw_write_png(argv[3],4));
    press(K_D); st.tick=0; game_render(); assert(sw_checksum()==solid);
    press(K_B); assert(!st.perspective); press(K_C); assert(st.pdist==80 && !st.pwire);
    press(K_B); press(K_APPS); assert(st.mode==M_ORDER && !st.config); press(K_APPS);
    // F1/F5 retain real intermediate frames; no hold/switch replacement.
    st.turn=1; st.turn_t=0; render_turn(); low=sw_checksum();
    st.turn_t=1; render_turn(); assert(sw_checksum()!=low);
    st.turn_t=0; render_turn(); assert(sw_checksum()==low); st.turn=0;
    // The full tactical renderer, not just the compact fixture, schedules idle work.
    sw_init(65);
    for(i=0;i<3000 && pidle->stage!=PIDLE_READY;i++) assert(sw_step(0));
    assert(i<3000 && !st.perspective && !st.config);
    printf("full ISO menu geometry prepared after %u idle frames\n",i);
    before=st; rng=rng32;
    press(sw_parse_keys("CATALOG"));
    for(i=0;i<40;i++) assert(sw_step(0));
    unchanged(&before,rng);
    press(K_ESC); press(K_C); assert(st.perspective && pidle->stage==PIDLE_READY);
    st.tick=0; game_render(); assert(sw_checksum()==initial_wide);
    press(K_B);
    // A requested view finishes a partial job instead of discarding its geometry.
    sw_init(65); press(sw_parse_keys("CATALOG"));
    for(i=0;i<100;i++) assert(sw_step(0));
    assert(pidle->stage>0 && pidle->stage<PIDLE_READY);
    press(K_ESC); press(K_C); assert(pidle->stage==PIDLE_READY);
    press(K_B);
    // A preference change replaces a partial job before any stale result is exposed.
    sw_init(65); press(sw_parse_keys("CATALOG"));
    for(i=0;i<40;i++) assert(sw_step(0));
    assert(pidle->stage>0); press(K_ENTER); assert(!st.pzoom);
    for(i=0;i<40;i++) assert(sw_step(0));
    assert(!pidle->descriptor[3] && pidle->stage<PIDLE_READY);
    press(K_ESC);
    // Status and turn depth overwrite the borrowed vertex scratch: cancel partial jobs.
    sw_init(65); press(sw_parse_keys("CATALOG"));
    for(i=0;i<60;i++) assert(sw_step(0));
    assert(pidle->stage>0 && pidle->stage<PIDLE_READY);
    press(K_ESC); st.mode=M_STATUS; game_render(); assert(!pidle->stage);
    st.mode=M_MENU; press(sw_parse_keys("CATALOG"));
    for(i=0;i<60;i++) assert(sw_step(0));
    assert(pidle->stage>0 && pidle->stage<PIDLE_READY);
    press(K_ESC); press(K_F5); assert(!pidle->stage);
    // Safe opening cannot interrupt an attack, animation or an AI turn.
    st.mode=M_ACT; st.act_t=0; st.ai=AI_OFF;
    rt_prev=0; rt_keys=K_CATALOG|K_APPS; assert(!config_update() && !st.config);
    st.mode=M_MENU; st.turn=1;
    rt_prev=0; rt_keys=K_CATALOG|K_APPS; assert(!config_update() && !st.config);
    st.turn=0; st.ai=AI_THINK;
    rt_prev=0; rt_keys=K_CATALOG|K_APPS; assert(!config_update() && !st.config);
    puts("configuration: two perspective options, paused tactics/RNG, APPS routing, close/wide, textured/filled-mesh cache, animated low-poly turns passed");
    return 0;
}
