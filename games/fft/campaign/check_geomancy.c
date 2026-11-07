// All retail terrain attacks: source math/procs and production ownership.
#define CLEANSE_RUNTIME_CHECK
#define FFT_CHECK_NATIVE_ORDINAL // PC retains source lines; native reports assertion ordinal
#include "check_cleanse.c"
#include "x/core-geomancy/oracle/source.h"
static u8 *geomancy_bank;
static void prepare_geomancy(AbId ab)
{
    u16 c,area;u8 surface;Unit *a;
    prepare_cleanse(ab);a=&st.unit[U_RAMZA];st.unit[10].team=TEAM_ENEMY;
    // Leave exactly one victim in the footprint, at distance2 from the caster.
    for(c=0;c<MAP_N;c++)if(map_tiles[c].walk==1&&
        abs((s16)TILE_X[c]-st.unit[10].x)+abs((s16)TILE_Z[c]-st.unit[10].z)==2)break;
    CHECK(c<MAP_N,1);CHECK(unit_place(a,(u8)c),1);
    area=(u16)MAP_W*MAP_H;
    memcpy(geomancy_bank,W->surface-8,8+(area<<3));geomancy_bank[3]='2';
    memset(geomancy_bank+8+(area<<3),0,area<<1);
    for(surface=0;source_terrain[surface]!=ab;surface++){}
    geomancy_bank[8+(area<<3)+((W->grid_row[a->z]+a->x)<<1)]=surface;
    CHECK(battle_bind_surface(geomancy_bank,8+10*area),1);
    CHECK(can_use(U_RAMZA,ab),1);a->pa=13;a->ma=17;
}
static void source_profiles(void)
{
    u16 i;u8 k;CastBatch b;Unit *a,*t;
    for(i=0;i<sizeof source_geomancy/sizeof source_geomancy[0];i++){
        const typeof(source_geomancy[0]) *p=&source_geomancy[i];const u8 *r;u32 expected,seed;
        prepare_geomancy(p->ab);a=&st.unit[U_RAMZA];t=&st.unit[10];r=cj_section(common,CJ_STATUS)+6*cj_range(common,p->ab)[11];
        a->pa=p->pa;a->ma=p->ma;a->faith=p->af;t->faith=p->tf;a->zodiac=p->az;t->zodiac=p->tz;t->female=p->sex;
        st.support[U_RAMZA]=p->up?SUPPORT_MAGIC_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->mode&&p->mode<8){
            static const u8 ids[]={0,27,22,21,16,17,35,28};u8 sid=ids[p->mode];
            t->statuses.inflicted[sid>>3]=(u8)(128>>(sid&7));
        }
        for(k=0;k<5;k++){
            if(p->mode==8)t->statuses.immune[k]=r[k+1];
            if(p->mode==9||p->mode==10)t->statuses.inflicted[k]=r[k+1];
            if(p->mode==10)t->statuses.innate[k]=r[k+1];
        }
        unit_status_refresh(t);
        if(p->affinity)st.affinity[p->affinity==5?U_RAMZA:10][p->affinity==5?4:p->affinity-1]=abilities[p->ab].element;
        st.weather=p->weather;seed=rng32;
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p->ab;
        CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(rng32==seed,1);CHECK(b.victim[10].hit,p->hit?100:0);
        CHECK(b.victim[10].amount,p->hit?p->amount:0);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(b.victim[10].amount,p->hit?p->amount:0);
        CHECK(rng32==expected,1);
        CHECK(!!(b.victim[10].effect&EFFECT_STATUS),p->hit?p->proc:0);
        CHECK(!!(b.victim[10].effect&EFFECT_OIL),p->hit?p->oil:0);
        if(p->hit)CHECK(b.victim[10].effect&EFFECT_KIND_MASK,p->kind);
    }
}
static void geomancy_production(void)
{
    AbId ab;u8 byte,mask,sid,k;u16 hp;u32 seed;Unit *t;
    for(ab=126;ab<=137;ab++){
        const u8 *r;prepare_geomancy(ab);t=&st.unit[10];r=cj_section(common,CJ_STATUS)+6*cj_range(common,ab)[11];
        for(byte=0;!r[byte+1];byte++){}mask=r[byte+1];sid=(u8)(byte<<3);for(k=128;!(mask&k);k>>=1)sid++;
        seed=rng32;CHECK(skill_hit(U_RAMZA,10,ab),100);CHECK(skill_amount(U_RAMZA,10,ab,0),119);CHECK(rng32==seed,1);
        CHECK(skill_cost(U_RAMZA,ab),0);CHECK(abilities[ab].range,5);CHECK(abilities[ab].vert,255);
        rng32=1;ZB(61);CHECK(cast_order(U_RAMZA,10,ab,1),1);ZE(61);
        CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(W->cast.victim[10].amount,119);
        CHECK(W->cast.victim[10].effect,AB_DAMAGE|EFFECT_STATUS);CHECK(t->hp,999);
        // Amount and proc are frozen; impact neither rerolls nor reads caster stats.
        st.unit[U_RAMZA].ma=1;st.unit[U_RAMZA].pa=1;seed=rng32;
        ZB(62);impact();ZE(62);CHECK(rng32==seed,1);CHECK(t->mp,999);
        if(sid==SID_DEAD){CHECK(t->hp,0);CHECK(t->kos,1);CHECK(t->dc,3);}
        else{CHECK(t->hp,880);CHECK(core_status_current(&t->statuses,byte)&mask,mask);
            if(sid>=24)CHECK(t->status_ct[sid-24],cj_status_rules(common)[((u16)sid<<4)+3]);}
        hp=t->hp;impact();CHECK(t->hp,hp);CHECK(st.primary.reward||st.gain,1);
        prepare_geomancy(ab);change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,ab),1);
        change(U_RAMZA,SID_FROG);CHECK(can_use(U_RAMZA,ab),0);
        prepare_geomancy(ab);t=&st.unit[10];t->statuses.immune[byte]=mask;rng32=1;
        CHECK(cast_order(U_RAMZA,10,ab,1),1);CHECK(W->cast.victim[10].effect&EFFECT_STATUS,0);
        impact();CHECK(t->hp,880);CHECK(core_status_current(&t->statuses,byte)&mask,0);
    }
    // Damage and the proc have independent channels; immune status never cancels HP.
    // Sleep/Confusion caused by this blow survive the ordinary damage wake-up.
    prepare_geomancy(132);change(10,SID_CONFUSION);rng32=1;cast_order(U_RAMZA,10,132,1);impact();
    CHECK(core_status_current(&st.unit[10].statuses,1)&16,0);CHECK(core_status_current(&st.unit[10].statuses,4)&16,16);
    prepare_geomancy(127);change(10,SID_FROG);rng32=1;cast_order(U_RAMZA,10,127,1);impact();
    CHECK(core_status_current(&st.unit[10].statuses,2)&2,0);
    prepare_geomancy(126);st.unit[10].hp=1;rng32=1;cast_order(U_RAMZA,10,126,1);impact();
    CHECK(st.unit[10].hp,0);CHECK(core_status_current(&st.unit[10].statuses,4)&8,0);
    prepare_geomancy(132);st.unit[10].statuses.inflicted[0]|=8;unit_status_refresh(&st.unit[10]);
    st.pending[10].state=DEFER_CAST_TILE;st.pending[10].ability=15;st.pending[10].ticks=5;
    rng32=1;cast_order(U_RAMZA,10,132,1);impact();CHECK(st.pending[10].state,DEFER_NONE);
    prepare_geomancy(128);st.unit[10].statuses.inflicted[0]|=8;unit_status_refresh(&st.unit[10]);
    st.pending[10].state=DEFER_CAST_TILE;st.pending[10].ability=15;st.pending[10].ticks=5;
    rng32=1;cast_order(U_RAMZA,10,128,1);impact();CHECK(st.pending[10].state,DEFER_CAST_TILE);
    CHECK(st.unit[10].status_flags&STATUS_CT_FROZEN,STATUS_CT_FROZEN);
}
static void terrain_layers(void)
{
    CoreGrid grid;CoreTilePosition pos[2];Tile tiles[2];u8 rows[1],lookup[512],present[64],x[2],z[2],bank[28];
    u8 layer,surface;AbId ab;u16 area=(u16)MAP_W*MAP_H;const u8 *old_surface;
    prepare_geomancy(126);old_surface=W->surface;
    CHECK(battle_bind_surface(geomancy_bank,8+10*area-1),0);CHECK(W->surface==old_surface,1);
    geomancy_bank[8+(area<<3)]=64;CHECK(battle_bind_surface(geomancy_bank,8+10*area),0);
    geomancy_bank[8+(area<<3)]=0;geomancy_bank[3]='1';CHECK(battle_bind_surface(geomancy_bank,8+(area<<3)),1);
    CHECK(can_use(U_RAMZA,126),0); // no invented terrain on a historical FGS1 binding
    memset(&grid,0,sizeof grid);memset(pos,0,sizeof pos);memset(tiles,0,sizeof tiles);
    memset(rows,0,sizeof rows);memset(lookup,0,sizeof lookup);memset(present,0,sizeof present);memset(x,0,sizeof x);memset(z,0,sizeof z);
    grid.width=grid.height=1;grid.count=2;grid.position=pos;grid.row=rows;grid.lookup=lookup;grid.present=present;
    present[0]=present[32]=1;lookup[256]=1;pos[1].z_layer=32;tiles[0].walk=tiles[1].walk=1;
    CHECK(battle_bind_grid(&grid,tiles,x,z),1);memset(bank,0,sizeof bank);memcpy(bank,"FGS2",4);bank[4]=bank[5]=bank[7]=1;
    CHECK(battle_bind_surface(bank,18),1);
    for(layer=0;layer<2;layer++)for(surface=0;surface<64;surface++){
        CHECK(unit_place(&st.unit[U_RAMZA],layer),1);bank[16+layer]=surface;bank[17-layer]=63-surface;
        for(ab=126;ab<=137;ab++)CHECK(can_use(U_RAMZA,ab),source_terrain[surface]==ab);
    }
    chapter_now=0;reset(); // no stack fixture escapes this test
}
static void geomancy_crowd(void)
{
    u8 i,prepared[FFT_CORE_UNIT_MAX];u16 mask=0,c;u32 seed;
    prepare_geomancy(132);
    // A synthetic capacity fixture: twelve stacked victims, with both camps.
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA){
        st.unit[i]=st.unit[10];st.unit[i].team=i&1?TEAM_PLAYER:TEAM_ENEMY;
        mask|=(u16)1<<i;
    }
    rng32=1;ZB(63);CHECK(cast_order(U_RAMZA,10,132,1),1);ZE(63);CHECK(W->cast.mask,mask);
    for(i=0;i<NUNIT;i++)prepared[i]=W->cast.victim[i].effect;
    seed=rng32;ZB(59);impact();ZE(59);CHECK(rng32==seed,1);CHECK(st.unit[U_RAMZA].hp,999);
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA){
        CHECK(st.unit[i].hp,880);CHECK(!!(core_status_current(&st.unit[i].statuses,4)&16),!!(prepared[i]&EFFECT_STATUS));
    }
    prepare_geomancy(126);seed=rng32;
    for(c=0;c<MAP_N;c++)if(map_tiles[c].walk==1&&
        abs((s16)TILE_X[c]-st.unit[U_RAMZA].x)+abs((s16)TILE_Z[c]-st.unit[U_RAMZA].z)>5)break;
    CHECK(c<MAP_N,1);unit_place(&st.unit[10],(u8)c);CHECK(cast_order(U_RAMZA,10,126,1),0);
    CHECK(st.action.phase,CTX_IDLE);CHECK(rng32==seed,1);
}
static void ai_origins(void)
{
    u16 steps,c;u8 k,found,home;prepare_geomancy(126);st.moved=0;W->ai_k=0;steps=0;
    // Only the starting source tile permits Pitfall in this fixture.
    memset(geomancy_bank+8+((u16)MAP_W*MAP_H<<3),1,(u16)MAP_W*MAP_H<<1);
    geomancy_bank[8+((u16)MAP_W*MAP_H<<3)+((W->grid_row[st.unit[U_RAMZA].z]+st.unit[U_RAMZA].x)<<1)]=0;
    while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==126){found=1;CHECK(W->ai_rank[k].o,UNIT_CELL(&st.unit[U_RAMZA]));}
    CHECK(found,1);
    // A learned terrain action may become usable only after moving. Candidate
    // generation must not discard it from the starting surface alone.
    prepare_geomancy(126);st.moved=0;W->ai_k=0;steps=0;
    memset(geomancy_bank+8+((u16)MAP_W*MAP_H<<3),0,(u16)MAP_W*MAP_H<<1);
    geomancy_bank[8+((u16)MAP_W*MAP_H<<3)+((W->grid_row[st.unit[U_RAMZA].z]+st.unit[U_RAMZA].x)<<1)]=1;
    CHECK(can_use(U_RAMZA,126),0);
    while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==126){
        u8 home=UNIT_CELL(&st.unit[U_RAMZA]);found=1;CHECK(W->ai_rank[k].o!=home,1);
        unit_place(&st.unit[U_RAMZA],W->ai_rank[k].o);CHECK(can_use(U_RAMZA,126),1);unit_place(&st.unit[U_RAMZA],home);
    }
    CHECK(found,1);
    // Ineligible origins consume the same per-update scan budget as centres.
    // A whole map of mismatching terrain must yield instead of scanning it all.
    prepare_geomancy(126);home=UNIT_CELL(&st.unit[U_RAMZA]);
    memset(geomancy_bank+8+((u16)MAP_W*MAP_H<<3),1,(u16)MAP_W*MAP_H<<1);
    W->ab=126;W->ab_k=0;W->ai_no=MAP_N;W->ab_best.pri=AI_BAD;W->ai_k=6;
    for(c=0;c<MAP_N;c++)W->ai_ord[c]=(u8)c;
    CHECK(MAP_N>64,1);ZB(58);CHECK(ai_think(),0);ZE(58);
    CHECK(W->ab_k,64);CHECK(W->ai_k,6);CHECK(UNIT_CELL(&st.unit[U_RAMZA]),home);
}
static void geomancy_trials(void)
{
    geomancy_bank=malloc(8+10*256);if(!geomancy_bank){CHECK(0,1);return;}
#ifndef GEOMANCY_RUNTIME_ONLY
    source_profiles();
#endif
#ifndef GEOMANCY_MATH_ONLY
    geomancy_production();geomancy_crowd();ai_origins();terrain_layers();
#endif
    free(geomancy_bank);geomancy_bank=RT_NULL;
}
#ifndef GEOMANCY_LABEL
#define GEOMANCY_LABEL "geomancy"
#endif
#ifndef __m68k__
int main(void){sw_init(61);geomancy_trials();printf(GEOMANCY_LABEL " %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();geomancy_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
