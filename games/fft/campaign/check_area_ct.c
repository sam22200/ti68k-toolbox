// Owned ordinary area packets through real command/CT/impact/response dispatch.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#define AREA_AB 0x110
static u32 area_hash(void)
{
    const u8 *p=(const u8 *)&st;u16 n;u32 h=17;
    for(n=0;n<sizeof st;n++)h=(h<<1|h>>31)^*p++;
    return h;
}
static void area_prepare(void)
{
    u8 i;arena();
    for(i=0;i<NUNIT;i++){
        Unit *u=&st.unit[i];memset(&u->statuses,0,sizeof u->statuses);
        memset(st.affinity[i],0,5);st.reaction[i]=st.support[i]=0;
        memset(u->eq,255,sizeof u->eq);u->status_flags=0;
        u->ma=5;u->faith=70;u->zodiac=0;u->mp=14;u->max_mp=999;
        u->set=SET_BASIC;u->hp=u->max_hp=999;u->dc=3;
        if(i!=U_RAMZA&&i!=10&&i!=8){u->x=u->z=255;u->hp=0;u->dc=DC_GONE;}
        unit_status_refresh(u);
    }
    unit_place(&st.unit[8],third);st.unit[8].team=TEAM_ENEMY;
    st.weather=0;st.acted=st.moved=st.open_resolving=st.open_ct_phase=0;
    CHECK(W->cast.phase,CAST_BATCH_EMPTY);CHECK(sizeof W->cast,60);
}
static void area_cure(u8 who,u8 sid)
{
    CoreStatusDelta d;memset(&d,0,sizeof d);d.remove[sid>>3]=0x80>>(sid&7);
    battle_status_apply(who,&d,1);
}
static u8 area_far(void)
{
    u16 c;for(c=0;c<MAP_N;c++)if(map_tiles[c].walk==1){
        s16 dx=(s16)TILE_X[c]-st.unit[U_RAMZA].x,dz=(s16)TILE_Z[c]-st.unit[U_RAMZA].z;
        u16 next=node_at(TILE_X[c]+1,TILE_Z[c],0);
        if((dx<0?-dx:dx)+(dz<0?-dz:dz)>5&&next<MAP_N&&map_tiles[next].walk==1)return (u8)c;
    }
    CHECK(0,1);return third;
}
static void area_immediate(void)
{
    CastBatch copy;u32 seed,hash;u8 k;u16 hp[3],amount[3];const u8 ids[3]={U_RAMZA,8,10};
    area_prepare();st.unit[U_RAMZA].mp=7;st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    ZB(61);CHECK(cast_order(U_RAMZA,10,AREA_AB,1),1);ZE(61);
    CHECK(st.unit[U_RAMZA].mp,0);CHECK(st.action.phase,CTX_ROOT);CHECK(W->cast.phase,CAST_BATCH_READY);
    CHECK(W->cast.mask,((u16)1<<U_RAMZA)|((u16)1<<8)|((u16)1<<10));
    for(k=0;k<3;k++){hp[k]=st.unit[ids[k]].hp;amount[k]=W->cast.victim[ids[k]].amount;CHECK(hp[k],999);CHECK(amount[k],34);}
    copy=W->cast;hash=area_hash();seed=rng32;
    CHECK(cast_order(U_RAMZA,10,AREA_AB,1),0);CHECK(area_hash()==hash,1);CHECK(rng32==seed,1);
    CHECK(charge_resolve(),1);CHECK(memcmp(&copy,&W->cast,60)==0,1);
    // Frame updates pause CT and retain the owned packet throughout wind-up.
    for(k=0;k<3;k++)game_update();CHECK(st.act_t,3);CHECK(st.unit[10].hp,999);
    CHECK(memcmp(&copy,&W->cast,60)==0,1);
    make_forecast(U_RAMZA,10,AREA_AB);cast_targets(U_RAMZA,second,AREA_AB,0);turn_order();
    CHECK(memcmp(&copy,&W->cast,60)==0,1);CHECK(rng32==seed,1);
    battle_weather(3,0);st.unit[U_RAMZA].ma=1;
    ZB(62);impact();ZE(62);CHECK(W->cast.phase,CAST_BATCH_APPLIED);
    for(k=0;k<3;k++)CHECK(st.unit[ids[k]].hp,hp[k]-amount[k]);
    hash=area_hash();impact();CHECK(st.unit[10].hp,965);CHECK(st.unit[U_RAMZA].mp,0);CHECK(rng32==seed,1);
    end_anim();end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
    CHECK(st.unit[U_RAMZA].mp,0);
}
static void area_binding(void)
{
    u8 follow,far,near,k;
    for(follow=0;follow<2;follow++){
        area_prepare();far=area_far();near=(u8)node_at(TILE_X[far]+1,TILE_Z[far],0);
        CHECK(cast_order(U_RAMZA,follow?10:second,AREA_AB,follow),2);
        CHECK(st.unit[U_RAMZA].mp,14);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
        unit_place(&st.unit[10],far);if(follow)unit_place(&st.unit[8],near);
        st.pending[U_RAMZA].ticks=0;ZB(63);CHECK(charge_resolve(),1);ZE(63);
        CHECK(W->cast.ref,follow?10:second);CHECK(W->cast.follow,follow);
        CHECK(st.unit[U_RAMZA].mp,7);CHECK(st.open_resolving,1);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
        CHECK(W->cast.mask,((u16)1<<8)|((u16)1<<(follow?10:U_RAMZA)));
        impact();CHECK(st.unit[8].hp,965);CHECK(st.unit[10].hp,follow?965:999);
        CHECK(st.unit[U_RAMZA].hp,follow?999:965);
        for(k=0;st.action.phase!=CTX_IDLE&&k<8;k++)end_anim();
        CHECK(W->cast.phase,CAST_BATCH_EMPTY);CHECK(st.action.phase,CTX_IDLE);
    }
}
static void area_absent_centres(void)
{
    u8 mode,k;u32 seed;
    for(mode=0;mode<3;mode++){
        area_prepare();CHECK(cast_order(U_RAMZA,10,AREA_AB,1),2);
        if(mode==0){st.unit[10].hp=0;st.unit[10].dc=DC_CRYSTAL;}
        else if(mode==1){st.unit[10].statuses.inflicted[SID_JUMP>>3]|=0x80>>(SID_JUMP&7);unit_status_refresh(&st.unit[10]);}
        else{st.unit[10].hp=0;st.unit[10].dc=DC_GONE;st.unit[10].x=st.unit[10].z=255;}
        st.pending[U_RAMZA].ticks=0;seed=rng32;CHECK(charge_resolve(),1);
        CHECK(st.unit[U_RAMZA].mp,7);CHECK(W->cast.mask,mode==2?0:((u16)1<<U_RAMZA)|((u16)1<<8));
        CHECK(W->cast.victim[10].hit,0);if(mode==2)CHECK(rng32==seed,1);
        impact();CHECK(st.unit[8].hp,mode==2?999:965);
        for(k=0;st.action.phase!=CTX_IDLE&&k<8;k++)end_anim();
        CHECK(W->cast.phase,CAST_BATCH_EMPTY);if(mode==2)CHECK(st.unit[U_RAMZA].gexp,0);
    }
}
static void area_interruptions(void)
{
    u8 mode;u32 seed,hash;CastBatch copy;
    for(mode=0;mode<5;mode++){
        area_prepare();CHECK(cast_order(U_RAMZA,10,AREA_AB,1),2);seed=rng32;
        if(mode==4)st.unit[U_RAMZA].mp=6;
        else change(U_RAMZA,mode==0?SID_SILENCE:mode==1?SID_DONT_ACT:mode==2?SID_SLEEP:SID_STOP);
        st.pending[U_RAMZA].ticks=0;
        if(mode==3){
            CHECK(pending_next(),255);CHECK(st.pending[U_RAMZA].state,DEFER_CAST_UNIT);
            CHECK(st.unit[U_RAMZA].mp,14);area_cure(U_RAMZA,SID_STOP);CHECK(charge_resolve(),1);
            CHECK(W->cast.phase,CAST_BATCH_READY);continue;
        }
        CHECK(charge_resolve(),0);CHECK(st.unit[U_RAMZA].mp,mode==4?6:14);
        CHECK(W->cast.phase,CAST_BATCH_EMPTY);CHECK(rng32==seed,1);
    }
    for(mode=0;mode<6;mode++){
        area_prepare();copy=W->cast;seed=rng32;hash=area_hash();
        CHECK(cast_order(mode==0?13:U_RAMZA,mode==1?NUNIT:10,
            mode==2?0x1e:mode>=3?AREA_AB+mode-2:AREA_AB,1),0);
        CHECK(area_hash()==hash,1);CHECK(rng32==seed,1);CHECK(memcmp(&copy,&W->cast,60)==0,1);
    }
    {const u8 *surface;area_prepare();surface=W->surface;W->surface=RT_NULL;
     hash=area_hash();seed=rng32;CHECK(cast_order(U_RAMZA,10,AREA_AB,1),0);
     CHECK(area_hash()==hash,1);CHECK(rng32==seed,1);W->surface=surface;}

}
static void area_clock(void)
{
    u8 i;u32 seed;area_prepare();CHECK(cast_order(U_RAMZA,10,AREA_AB,1),2);seed=rng32;
    game_update();CHECK(st.pending[U_RAMZA].ticks,3);CHECK(rng32==seed,1);
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}
    st.unit[10].sp=50;
    for(i=0;i<3;i++){
        st.unit[10].ct=50;CHECK(ct_next(),i==2?254:10);
        CHECK(st.pending[U_RAMZA].ticks,2-i);CHECK(st.unit[U_RAMZA].mp,14);
    }
    CHECK(charge_resolve(),1);CHECK(W->cast.phase,CAST_BATCH_READY);CHECK(st.unit[U_RAMZA].mp,7);
}
static void area_responses(void)
{
    CastBatch copy;u8 k;area_prepare();
    CHECK(cast_order(U_RAMZA,10,AREA_AB,1),2);CHECK(cast_order(10,U_RAMZA,AREA_AB,1),2);
    st.pending[U_RAMZA].ticks=st.pending[10].ticks=0;st.open_ct_phase=1;
    st.reaction[10]=REACTION_POTION;st.unit[10].brave=100;
    turn_start();CHECK(st.action.root,U_RAMZA);CHECK(W->cast.phase,CAST_BATCH_READY);
    impact();copy=W->cast;end_anim();CHECK(st.primary.running,1);CHECK(st.action.actor,10);
    CHECK(st.unit[10].mp,14);CHECK(st.pending[10].state,DEFER_CAST_UNIT);
    CHECK(charge_resolve(),1);CHECK(memcmp(&copy,&W->cast,60)==0,1);
    impact();CHECK(W->cast.phase,CAST_BATCH_APPLIED);end_anim();
    for(k=0;st.action.root!=10&&k<8;k++)end_anim();
    CHECK(st.action.root,10);CHECK(st.action.phase,CTX_ROOT);CHECK(W->cast.phase,CAST_BATCH_READY);
    CHECK(W->cast.actor,10);CHECK(st.unit[U_RAMZA].mp,7);CHECK(st.unit[10].mp,7);
    CHECK(st.open_ct_phase,1);
}
static void area_wave(void)
{
    Unit prototype;u16 cell=0;u8 i,k;area_prepare();prototype=st.unit[U_RAMZA];
    for(i=0;i<NUNIT;i++){
        while(cell<MAP_N&&map_tiles[cell].walk!=1)cell++;
        CHECK(cell<MAP_N,1);st.unit[i]=prototype;unit_place(&st.unit[i],(u8)cell++);
        st.unit[i].mp=7;st.unit[i].ct=i==12?50:0;st.unit[i].sp=i==12?50:1;
        st.unit[i].team=i==U_RAMZA?TEAM_PLAYER:TEAM_ENEMY;st.support[i]=st.reaction[i]=0;
        CHECK(cast_order(i,i,AREA_AB,1),2);st.pending[i].ticks=1;
    }
    turn_start();
    for(i=0;i<NUNIT;i++){
        CHECK(st.action.root,i);CHECK(W->cast.actor,i);CHECK(W->cast.phase,CAST_BATCH_READY);
        CHECK(st.unit[i].mp,0);CHECK(st.open_ct_phase,1);impact();
        for(k=0;st.action.root==i&&st.action.phase!=CTX_IDLE&&k<8;k++)end_anim();
        CHECK(k<8,1);
    }
    CHECK(st.action.phase,CTX_IDLE);CHECK(W->cast.phase,CAST_BATCH_EMPTY);CHECK(st.act,12);
    for(i=0;i<NUNIT;i++)CHECK(st.pending[i].state,DEFER_NONE);
}
static void area_restart(void)
{
    u8 phase;CHECK(W!=RT_NULL,1);if(!W)return;
    for(phase=CAST_BATCH_READY;phase<=CAST_BATCH_APPLIED;phase++){
        W->cast.phase=phase;st.action.phase=phase==CAST_BATCH_READY?CTX_ROOT:CTX_DRAIN;
        game_scenario(105);CHECK(W->cast.phase,CAST_BATCH_EMPTY);CHECK(st.action.phase,CTX_IDLE);
    }
}
static void area_trials(void){area_immediate();area_binding();area_absent_centres();area_interruptions();area_clock();area_responses();area_wave();}
#ifndef __m68k__
int main(int argc,char **argv){(void)argv;sw_init(argc>1?105:61);if(argc>1)area_restart();else area_trials();printf("%s %08lX failures %u checks %u\n",argc>1?"area_restart":"area_ct",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();if(chapter_now==1)area_restart();else area_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
