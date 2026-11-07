// Area AI: source accounting, actual planner, empty centres and native quanta.
#define FORECAST_CHECK
#include "check_batch.c"
#include "x/core-area-ai/oracle/source.h"
#ifdef FFT_MODULES
#include "../ai_evaluation.inc"
#include "../area_ai_reduce.inc"
#endif
static u16 formula_calls,cache_origins,max_steps;
#ifdef FFT_MODULES
static u32 general_peak;static u8 general_peak_stage;
static u16 counted_amount(u8 a,u8 t,u16 spec){formula_calls++;return magic_amount(a,t,spec);}
#endif
static void source_scores(void)
{
    const u8 *src;u16 bytes,n,k;AreaAi q;AiAct result;Unit u=st.unit[0];
    src=rt_file("fai00",&bytes);CHECK(src!=RT_NULL,1);if(!src)return;
    CHECK(RD16(src,2),12);
    for(n=0;n<12;n++){
        const u8 *p=src+6+164*n;u16 selected=RD16(p,0),counted;u8 ok;
        memset(&q,0,sizeof q);
        for(k=0;k<13;k++){
            const u8 *v=p+8+12*k;s16 d;
            u.max_hp=RD16(v,1);u.status=0;
            d=worth(&u,RD16(v,2),0)-worth(&u,RD16(v,0),0);
            q.gain[k]=(RD16(v,3)?-d:d)-(RD16(v,3)?0:1);q.hit[k]=RD16(v,4);
            if(RD16(v,5))q.useful|=(u16)1<<k;
        }
        ok=area_ai_reduce(&q,selected,13,&result,&counted);CHECK(ok,RD16(p,1));
        if(ok){CHECK(result.pri,RD16(p,2));CHECK(result.hit,RD16(p,3));}
    }
    for(n=0;n<sizeof source_area_rank/sizeof source_area_rank[0];n++){
        const u16 *p=source_area_rank[n];AiAct a,b;
        a.pri=p[0];a.hit=p[1];a.o=0;b.pri=p[3];b.hit=p[4];b.o=1;
        W->ai_prox[0]=p[2];W->ai_prox[1]=p[5];CHECK(ai_better(a.pri,a.hit,a.o,&b),p[6]);
    }
}
static void flat(u8 w,u8 h,u8 upper,u8 reverse)
{
    u16 area=(u16)w*h,count=area*(upper?2:1),i;u8 layer;
    memset(f,0,sizeof *f);f->grid.count=count;f->grid.width=w;f->grid.height=h;
    f->grid.position=f->pos;f->grid.row=f->row;f->grid.lookup=f->lookup;f->grid.present=f->present;
    for(i=0;i<h;i++)f->row[i]=i*w;
    memcpy(f->surface,"FGS1",4);f->surface[4]=w;f->surface[5]=h;f->surface[6]=area>>8;f->surface[7]=area;
    for(i=0;i<area;i++){f->surface[8+8*i+4]=4;f->surface[8+8*i+7]=upper?1:128;}
    for(layer=0;layer<(upper?2:1);layer++)for(i=0;i<area;i++){
        u16 id=(u16)layer*area+i,address=((u16)layer<<8)+i;u8 x=i%w,z=i/w;
        if(reverse)id=count-1-id;
        f->lookup[address]=(u8)id;f->present[address>>3]|=(u8)(1U<<(address&7));
        f->pos[id].x=f->x[id]=x;f->pos[id].z_layer=z|(layer<<5);f->z[id]=z;
        f->tiles[id].walk=1;f->tiles[id].stand=layer?8:0;
        f->tiles[id].c[0]=f->tiles[id].c[1]=f->tiles[id].c[2]=f->tiles[id].c[3]=layer?4:0;
    }
    CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),1);
    CHECK(battle_bind_surface(f->surface,8+8*area),1);
}
static void put_unit(u8 id,u8 x,u8 z,u8 layer,u8 team)
{
    Unit *u=&st.unit[id];CHECK(unit_place(u,(u8)node_at(x,z,layer)),1);
    u->hp=u->max_hp=100;u->dc=3;u->team=team;
}
static void fixture(u8 reverse,u8 large)
{
    u8 i,k;Unit seed;
    CHECK(chapter_load(1),1);reset();seed=st.unit[U_RAMZA];
    flat(large?16:5,large?8:5,large,reverse);
    for(i=0;i<NUNIT;i++){
        st.unit[i]=seed;st.unit[i].hp=0;st.unit[i].x=st.unit[i].z=255;st.unit[i].dc=DC_GONE;
        st.unit[i].sp=5;st.unit[i].pa=0;st.unit[i].ma=5;st.unit[i].faith=100;
        st.unit[i].zodiac=0;st.unit[i].move=3;st.unit[i].learned=0;st.unit[i].defend=0;
        st.unit[i].mp=st.unit[i].max_mp=100;memset(st.unit[i].eq,255,sizeof st.unit[i].eq);
        memset(&st.unit[i].statuses,0,sizeof st.unit[i].statuses);st.unit[i].status=0;
        memset(st.affinity[i],0,5);st.reaction[i]=st.support[i]=0;
    }
    put_unit(0,large?14:2,large?7:4,0,TEAM_PLAYER);
    put_unit(1,large?15:1,large?7:1,large?1:0,TEAM_ENEMY);
    put_unit(2,large?13:3,large?7:1,large?1:0,TEAM_ENEMY);
    if(large)for(i=3;i<NUNIT;i++)put_unit(i,(u8)(9+(i%6)),(u8)(3+i/6),i&1,TEAM_ENEMY);
    st.unit[0].set=SET_BASIC;
    for(k=0;k<SET_N;k++)if(SET_ABILITY(SET_BASIC,k)==0x93)break;
    CHECK(k<SET_N,1);st.unit[0].learned=(LearnBits)1<<k;
    st.act=0;st.moved=1;st.acted=0;st.ai=AI_THINK;st.banner=st.turn=0;
    st.mode=M_BROWSE;W->ai_k=0;
}
static void plan(u8 invalidate,u8 count_math)
{
    Unit before[UNIT_CAPACITY];u16 steps=0;u8 done=0,restarted=0;
    memcpy(before,st.unit,sizeof before);
    while(!done&&steps<4096){
        u32 seed=rng32;u8 area=W->ai_k>=4;
#ifdef FFT_MODULES
        u8 stage=W->ai_k;u32 began=BENCH_CYCLES;
#endif
        if(invalidate&&!restarted&&W->ai_k==5){
            if(invalidate==1){CHECK(battle_weather(3,0),1);CHECK(W->ai_k,0);}
            else{
                CastBatch peek;memset(&peek,0,sizeof peek);peek.ability=0x93;peek.ref=UNIT_CELL(&st.unit[st.act]);
                CHECK(cast_forecast(&peek),1);CHECK(W->ai_k,5);
            }
            restarted=1;
        }
#ifdef FFT_MODULES
        if(count_math&&W->ai_k>=4){
            // Separate cache-count trial: preload excluded from timed quanta.
            mod_call(MOD_AI_PREP,255,0);mod_context(MOD_AI_PREP)->magic_amount=counted_amount;
        }
#endif
        if(!count_math){
            if(W->ai_k>=4){ZB(53);done=ai_think();ZE(53);}
            else{ZB(52);done=ai_think();ZE(52);}
        }else done=ai_think();
#ifdef FFT_MODULES
        if(!count_math&&stage<4&&(u32)(BENCH_CYCLES-began)>general_peak){
            general_peak=BENCH_CYCLES-began;general_peak_stage=stage;
        }
#endif
        if(area&&invalidate!=1)CHECK(rng32==seed,1);
        CHECK(memcmp(before,st.unit,sizeof before)==0,1);steps++;
    }
    CHECK(done,1);CHECK(steps<4096,1);if(steps>max_steps)max_steps=steps;
    if(count_math)cache_origins=W->ai_no;
    if(invalidate)CHECK(restarted,1);
}
static void centres(void)
{
    u8 reverse;
    for(reverse=0;reverse<2;reverse++){
        fixture(reverse,0);plan(0,0);CHECK(st.ai_ab,0x93);
        CHECK(TILE_X[st.ai_tgt],2);CHECK(TILE_Z[st.ai_tgt],1);CHECK(unit_at_cell(st.ai_tgt),0);
        CHECK(W->ab_best.pri,180);CHECK(W->ab_best.hit,100);
        fixture(reverse,0);put_unit(3,2,1,0,TEAM_PLAYER);plan(0,0);
        CHECK(st.ai_ab,0x93);CHECK(TILE_X[st.ai_tgt],1);CHECK(TILE_Z[st.ai_tgt],2);
        fixture(reverse,0);put_unit(3,2,1,0,TEAM_PLAYER);st.unit[3].hp=20;st.affinity[3][0]=128;
        plan(0,0);CHECK(st.ai_ab,0x93);CHECK(TILE_X[st.ai_tgt],2);CHECK(TILE_Z[st.ai_tgt],1);
        fixture(reverse,0);st.affinity[1][0]=128;st.unit[1].hp=20;plan(0,0);
        CHECK(st.ai_ab,0x93);CHECK(TILE_X[st.ai_tgt],3);CHECK(TILE_Z[st.ai_tgt],1);
    }
    fixture(0,0);st.affinity[1][1]=st.affinity[2][1]=128;plan(0,0);CHECK(st.ai_ab,AI_NONE);
    fixture(0,0);st.unit[0].mp=6;plan(0,0);CHECK(st.ai_ab,AI_NONE);CHECK(st.unit[0].mp,6);
    fixture(0,0);st.unit[0].learned=4;plan(0,0);CHECK(st.ai_ab,AI_NONE); // unsupported trajectory policy
    fixture(0,0);plan(1,0);CHECK(st.ai_ab,0x93);CHECK(W->ab_best.pri,134);
    fixture(0,0);plan(2,0);CHECK(st.ai_ab,0x93);CHECK(W->ab_best.pri,180);
    fixture(0,0);plan(0,1);CHECK(st.ai_ab,0x93);
}
static void empty_execution(void)
{
    fixture(0,0);plan(0,0);st.support[0]=SUPPORT_NON_CHARGE;
    CHECK(cursor_place(st.ai_tgt),1);W->hud_ok=1;st.ai=AI_AIM;st.ai_t=AI_PREVIEW+AI_NAME;
    ai_step();CHECK(st.mode,M_ACT);CHECK(st.acted,1);CHECK(st.unit[0].mp,93);
    CHECK(W->cast.phase,CAST_BATCH_READY);CHECK(W->cast.follow,0);CHECK(W->cast.mask,6);
    impact();CHECK(st.unit[1].hp,30);CHECK(st.unit[2].hp,30);
}
static void legacy_area_flag(void)
{
    // Enabled weapon skills can carry AREA without being an incantation.
    fixture(0,0);W->ai_list[0]=0x9b;W->ai_nab=1;W->ai_a=0;W->ai_self=0;W->ai_k=3;
#ifdef FFT_MODULES
    mod_call(MOD_AI,1,0);
#else
    ai_ability(0,0x9b,0);
#endif
    CHECK(W->ai_k,3);
    st.ai_ab=0x9b;st.ai_tgt=UNIT_CELL(&st.unit[1]);cursor_place(st.ai_tgt);
    st.ai=AI_AIM;st.ai_t=AI_PREVIEW+AI_NAME;W->hud_ok=1;
    ai_step();CHECK(st.mode,M_ACT);CHECK(st.acted,1);CHECK(st.unit[0].mp,100);
}
static void rejected_execution(void)
{
    fixture(0,0);plan(0,0);st.unit[0].mp=6;cursor_place(st.ai_tgt);
    st.ai=AI_AIM;st.ai_t=AI_PREVIEW+AI_NAME;W->hud_ok=1;
    ai_step();CHECK(st.ai,AI_THINK);CHECK(W->ai_k,0);CHECK(st.acted,0);
    CHECK(st.unit[0].mp,6);CHECK(st.action.phase,CTX_IDLE);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
}
static void stress(void)
{
    u8 k;
    fixture(0,1);st.moved=0;plan(0,0);CHECK(st.ai_ab,0x93);CHECK(MAP_N,256);
    CHECK(TILE_X[st.ai_tgt]<MAP_W,1);CHECK(TILE_Z[st.ai_tgt]<MAP_H,1);
    fixture(1,1);plan(0,0);CHECK(st.ai_ab,0x93);CHECK(MAP_N,256);
    fixture(0,1);st.moved=0;
    for(k=0;k<SET_N;k++)if(SET_ABILITY(SET_BASIC,k)==0x95)break;
    CHECK(k<SET_N,1);st.unit[0].learned=(LearnBits)1<<k;
    plan(0,0);CHECK(st.ai_ab,0x95);CHECK(max_steps<512,1); // bounded graph/victim preparation now yields too
}
static void area_trials(void)
{
    source_scores();f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
    centres();empty_execution();legacy_area_flag();rejected_execution();stress();CHECK(chapter_load(1),1);reset();free(f);
    CHECK(sizeof(AreaAi)<=256,1);
}
#ifndef AREA_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);area_trials();printf("area_ai %08lX failures %u checks %u max_steps %u\n",(unsigned long)digest,failures,checks,max_steps);return failures!=0;}
#else
void _main(void){reaction_runtime_main();area_trials();BENCH_VALUE(general_peak_stage);BENCH_VALUE(formula_calls);BENCH_VALUE(cache_origins);BENCH_VALUE(max_steps);BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif

#endif // AREA_UI_CHECK
