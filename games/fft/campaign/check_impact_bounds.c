// Bounded thirteen-glyph preparation, compact node255, both floors and signed rows.
#define AREA_UI_CHECK
#include "check_area_ai.c"
#include "area_impact_pixels.inc"
static u32 image_hash;
static void impact_bounds_trials(void)
{
    u8 i;u32 seed;CastBatch packet;
    f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
    fixture(0,1);st.ai=AI_OFF;st.support[0]=SUPPORT_NON_CHARGE;
    CHECK(cast_order(0,255,18,0),1);CHECK(MAP_N,256);
    CHECK(W->area_impact.centre,255);CHECK(W->cast.mask,8191);
    CHECK(sizeof(AreaImpact),412);CHECK(W->area_impact.ready,0);
    W->cast.victim[1].hit=0;W->cast.victim[1].effect=AB_NULL;W->cast.victim[1].amount=0;
    W->cast.victim[2].effect=AB_HEAL;W->cast.victim[2].amount=10;
    W->cast.victim[3].effect=AB_DAMAGE;W->cast.victim[3].amount=(u16)-10;
    W->cast.victim[4].effect=AB_HEAL;W->cast.victim[4].amount=(u16)-32768;
    W->cast.victim[5].effect=AB_DAMAGE|EFFECT_OIL;
    st.unit[5].statuses.inflicted[2]|=128;unit_status_refresh(&st.unit[5]);
    packet=W->cast;seed=rng32;
    for(i=0;i<ACT_HIT;i++){
        ZB(59);attack_step();ZE(59);
        CHECK(W->cast.phase,CAST_BATCH_READY);CHECK(memcmp(&packet,&W->cast,sizeof packet)==0,1);
        CHECK(rng32==seed,1);CHECK(st.unit[0].hp,100);
    }
    CHECK(W->area_impact.ready,8191);CHECK(W->area_impact.oil_ready,1);
    CHECK(W->area_impact.next,NUNIT+1);
    for(i=0;i<NUNIT;i++)CHECK(W->area_impact.width[i]>0&&W->area_impact.width[i]<=32,1);
    attack_step();CHECK(W->cast.phase,CAST_BATCH_APPLIED);CHECK(st.unit[1].hp,100);
    CHECK(st.unit[3].hp,100);CHECK(st.unit[4].hp,0);
    CHECK(core_status_current(&st.unit[5].statuses,2)&128,0);
    {Feet focus=unit_feet(st.rot,1);st.camx=focus.x-80;st.camy=focus.y-60;}
    draw_clear();image_hash=impact_picture_hash();
#ifdef FFT_MODULES
    ZB(60);mod_call(MOD_AREA_UI,AREA_RESULTS,0);ZE(60);
#else
    numbers_draw();
#endif
    CHECK(impact_picture_hash()!=image_hash,1);image_hash=impact_picture_hash();CHECK(rng32==seed,1);
    // A secondary area response owns its own FX; stale parent packets do not.
    st.action.phase=CTX_SECONDARY;CHECK(area_impact_root(),1);
    st.atk++;CHECK(area_impact_root(),0);st.atk--;
    st.abil++;CHECK(area_impact_root(),0);st.abil--;
    st.action.phase=CTX_DRAIN;CHECK(area_impact_root(),0);
    for(i=0;i<3;i++){
        u16 cell;fixture(0,0);st.ai=AI_OFF;cell=UNIT_CELL(&st.unit[1]);
        CHECK(cast_order(0,1,16,1),2);
        if(i==0){st.unit[1].hp=0;st.unit[1].dc=DC_CRYSTAL;}
        else if(i==1){st.unit[1].statuses.inflicted[SID_JUMP>>3]|=0x80>>(SID_JUMP&7);unit_status_refresh(&st.unit[1]);}
        else{st.unit[1].x=st.unit[1].z=255;st.unit[1].dc=DC_GONE;}
        st.pending[0].ticks=0;CHECK(charge_resolve(),1);
        CHECK(W->area_impact.centre,i==2?MAP_N:cell);
    }
    fixture(0,0);st.ai=AI_OFF;memset(&W->cast,0,sizeof W->cast);
    W->cast.actor=0;W->cast.ability=16;W->cast.ref=UNIT_CELL(&st.unit[1]);
    CHECK(cast_batch(CAST_BATCH_PREPARE,&W->cast),1);
    CHECK(W->area_impact.centre,UNIT_CELL(&st.unit[1]));CHECK(W->area_impact.ready,0);
    for(i=0;i<ACT_HIT;i++)attack_step();CHECK(W->area_impact.ready,W->cast.mask);
    CHECK(chapter_load(1),1);reset();free(f);f=RT_NULL;
}
#ifndef __m68k__
int main(void){sw_init(61);impact_bounds_trials();printf("impact_bounds %08lX picture %08lX failures %u checks %u\n",(unsigned long)digest,(unsigned long)image_hash,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();impact_bounds_trials();BENCH_VALUE(digest);BENCH_VALUE(image_hash);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
