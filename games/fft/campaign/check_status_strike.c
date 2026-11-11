// Monster status strikes (formula 0x50, evadeable): the original evade draws, accuracy roll
// and status request replayed through the owned cast (campaign/MONSTER_STATUS_STRIKES.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-status-strike/oracle/source.h"
static void prepare_strike(AbId ab)
{
    u8 k,f;Unit *u,*t;prepare_spell(0x13);u=&st.unit[U_RAMZA];t=&st.unit[10];
    for(u->set=0x40;u->set<0xE0;u->set++){                 // the monster set holding it
        for(k=0;k<SET_N;k++)if(SET_ABILITY(u->set,k)==ab)break;
        if(k<SET_N)break;
    }
    CHECK(u->set<0xE0,1);CHECK(k<4,1);u->learned=(LearnBits)1<<k;
    u->job=94;                                             // a monster: only monsters use these (can_use)
    st.unit[8].hp=0;st.unit[8].dc=DC_GONE;st.unit[8].x=255;
    t->team=TEAM_ENEMY;t->hp=t->max_hp=999;t->job=STRIKE_JOB;u->female=t->female=0;
    for(f=0;f<4;f++){t->face=f;if(attack_side(U_RAMZA,10)==SIDE_FRONT)break;}   // the attacker in front
    CHECK(f<4,1);
    memset(st.affinity,0,sizeof st.affinity);
}
static void strike_profiles(void)
{
    u16 i;u8 k;
    for(i=0;i<sizeof source_strike/sizeof source_strike[0];i++){
        const typeof(source_strike[0]) *p=&source_strike[i];Unit *a,*t;u32 seed,expected;CastBatch b;
        prepare_strike(p->ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->zodiac=p->az;t->zodiac=p->tz;
        st.support[U_RAMZA]=p->up?SUPPORT_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->gear==2){t->eq[EQ_ACC]=STRIKE_ACC;t->eq[EQ_SHIELD]=STRIKE_SHIELD;}
        if(!p->gear){u8 f;for(f=0;f<4;f++){t->face=f;if(attack_side(U_RAMZA,10)==SIDE_BACK)break;}CHECK(f<4,1);}   // no evade source
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p->ab;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(b.victim[10].hit,p->forecast);CHECK(rng32==seed,1);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        CHECK(b.victim[10].effect,p->hit?AB_STATUS:AB_NULL);
        if(p->hit){
            u8 byte=(u8)(b.victim[10].amount>>8),mask=(u8)b.victim[10].amount;
            CHECK(byte<5,1);CHECK(mask,p->add[byte<5?byte:0]);
            for(k=0;k<5;k++)if(k!=byte)CHECK(p->add[k],0);
        }
    }
}
// Production: the real cast inflicts the record's status (Blind, Poison, Petrify).
static void strike_production(void)
{
    static const AbId skills[]={0x111,0x11B,0x132};
    static const u8 sid[]={SID_DARK,SID_POISON,SID_PETRIFY};
    u8 i,s;
    for(i=0;i<3;i++){
        u8 hits=0;
        prepare_strike(skills[i]);
        CHECK(abilities[skills[i]].kind,AB_STATUS);CHECK(abilities[skills[i]].flags,AF_CAST|AF_AREA|AF_EVADE);
        CHECK(abilities[skills[i]].range,1);
        for(s=1;s<60&&!hits;s++){
            Unit *t;prepare_strike(skills[i]);t=&st.unit[10];st.unit[U_RAMZA].ma=60;
            memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);rng32=s;
            CHECK(can_use(U_RAMZA,skills[i]),1);
            CHECK(cast_order(U_RAMZA,10,skills[i],1),1);impact();
            if(!(core_status_current(&t->statuses,sid[i]>>3)&(0x80>>(sid[i]&7))))continue;   // evaded
            hits++;CHECK(t->hp,999);
        }
        CHECK(hits,1);
    }
}
// Formula 0x3D (Mind Blast, the monsters' Death Sentence): accuracy without Faith, the random
// choice drawn after it, a blocked choice a miss; never evaded.
static void magical_profiles(void)
{
    u16 i;u8 k;
    for(i=0;i<sizeof source_magical/sizeof source_magical[0];i++){
        const typeof(source_magical[0]) *p=&source_magical[i];Unit *a,*t;u32 seed,expected;CastBatch b;
        prepare_strike(p->ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->zodiac=p->az;t->zodiac=p->tz;a->faith=p->af;t->faith=p->tf;
        st.support[U_RAMZA]=p->up?SUPPORT_MAGIC_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(k=0;k<5;k++)t->statuses.inflicted[k]=source_magical_status[p->mode][k];
        t->eq[EQ_ACC]=STRIKE_ACC;t->eq[EQ_SHIELD]=STRIKE_SHIELD;           // evasive gear: never evaded
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p->ab;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(!!(b.mask&((u16)1<<10)),1);
        CHECK(b.victim[10].hit,p->forecast);CHECK(rng32==seed,1);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        if(p->hit){
            u8 byte=(u8)(b.victim[10].amount>>8),mask=(u8)b.victim[10].amount;
            CHECK(byte<5,1);CHECK(mask,p->add[byte<5?byte:0]);
            for(k=0;k<5;k++)if(k!=byte)CHECK(p->add[k],0);
        }
    }
}
// Production: Mind Blast leaves Confusion or Berserk (both seen over the seeds), the monsters'
// Death Sentence its countdown.
static void magical_production(void)
{
    u8 s,seen=0,sentence=0;
    for(s=1;s<40&&seen!=3;s++){
        Unit *t;prepare_strike(0x120);t=&st.unit[10];st.unit[U_RAMZA].ma=99;
        memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);
        rng32=((u32)s*0x9E3779B1UL)|1;                     // spread: small seeds draw tiny first values
        CHECK(can_use(U_RAMZA,0x120),1);CHECK(cast_order(U_RAMZA,10,0x120,1),1);impact();
        if(core_status_current(&t->statuses,SID_CONFUSION>>3)&(0x80>>(SID_CONFUSION&7)))seen|=1;
        if(core_status_current(&t->statuses,SID_BERSERK>>3)&(0x80>>(SID_BERSERK&7)))seen|=2;
        CHECK(t->hp,999);
    }
    CHECK(seen,3);
    for(s=1;s<40&&!sentence;s++){
        Unit *t;prepare_strike(0x130);t=&st.unit[10];st.unit[U_RAMZA].ma=99;
        memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);rng32=s;
        CHECK(can_use(U_RAMZA,0x130),1);CHECK(cast_order(U_RAMZA,10,0x130,1),1);impact();
        sentence=core_status_current(&t->statuses,4)&1;
    }
    CHECK(sentence,1);CHECK(st.unit[10].status_ct[15],3);
}
// Odd Soundwave: the original removal set per present/innate statuses, through real casts on
// radius 2 around the caster (the caster included); nothing removed is a miss, no draw.
static void wave_production(void)
{
    u16 i;u8 k;
    prepare_strike(0x11F);CHECK(abilities[0x11F].kind,AB_CLEANSE);CHECK(abilities[0x11F].flags,AF_CAST|AF_AREA|AF_SELF);
    CHECK(abilities[0x11F].formula,FORMULA_AREA_CLEANSE);
    for(i=0;i<sizeof source_wave/sizeof source_wave[0];i++){
        const typeof(source_wave[0]) *p=&source_wave[i];Unit *a,*t;u32 seed;s16 mask;
        prepare_strike(0x11F);a=&st.unit[U_RAMZA];t=&st.unit[10];
        memset(&t->statuses,0,sizeof t->statuses);memset(&a->statuses,0,sizeof a->statuses);
        for(k=0;k<5;k++){t->statuses.inflicted[k]=p->current[k];t->statuses.innate[k]=p->innate[k];}
        a->statuses.inflicted[3]=0x20;                                  // the caster's own Protect
        unit_status_refresh(t);unit_status_refresh(a);
        mask=cast_targets(U_RAMZA,UNIT_CELL(a),0x11F,0);
        CHECK(mask>=0,1);CHECK(!!(mask&((u16)1<<10)),1);CHECK(!!(mask&((u16)1<<U_RAMZA)),1);
        CHECK(can_use(U_RAMZA,0x11F),1);seed=rng32;
        CHECK(cast_order(U_RAMZA,UNIT_CELL(a),0x11F,0),1);
        CHECK(W->cast.victim[10].hit,p->hit);impact();CHECK(rng32==seed,1);
        for(k=0;k<5;k++)CHECK(t->statuses.inflicted[k],p->current[k]&(u8)~p->remove[k]);
        CHECK(a->statuses.inflicted[3]&0x20,0);                         // the caster loses its Protect too
    }
}
// Beaking 0x55: the original magical evade draws, accuracy without Faith and PA loss, then a
// production cast: PA 12 -> 10.
static void beaking_trials(void)
{
    u16 i;u8 k,s,hits=0;
    for(i=0;i<sizeof source_beaking/sizeof source_beaking[0];i++){
        const typeof(source_beaking[0]) *p=&source_beaking[i];Unit *a,*t;u32 seed,expected;CastBatch b;
        prepare_strike(0x135);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->zodiac=p->az;t->zodiac=p->tz;a->faith=p->af;t->faith=p->tf;
        st.support[U_RAMZA]=p->up?SUPPORT_MAGIC_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->gear){t->eq[EQ_ACC]=STRIKE_ACC;t->eq[EQ_SHIELD]=STRIKE_SHIELD;}
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=0x135;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(b.victim[10].hit,p->forecast);CHECK(rng32==seed,1);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        CHECK(b.victim[10].effect,p->hit?AB_BREAK:AB_NULL);CHECK(b.victim[10].amount,p->pa);
    }
    CHECK(abilities[0x135].kind,AB_BREAK);CHECK(abilities[0x135].flags,AF_CAST|AF_AREA|AF_EVADE);
    for(s=1;s<60&&!hits;s++){
        Unit *t;prepare_strike(0x135);t=&st.unit[10];st.unit[U_RAMZA].ma=70;t->pa=12;t->ma=11;t->sp=8;
        memset(&t->statuses,0,sizeof t->statuses);unit_status_refresh(t);rng32=((u32)s*0x9E3779B1UL)|1;
        CHECK(can_use(U_RAMZA,0x135),1);CHECK(cast_order(U_RAMZA,10,0x135,1),1);impact();
        if(t->pa==12)continue;
        hits++;CHECK(t->pa,10);CHECK(t->ma,11);CHECK(t->sp,8);
    }
    CHECK(hits,1);
}
// Goblin Punch 0x4F: the original evade draws and accuracy, damage = the caster's missing HP.
static void goblin_trials(void)
{
    u16 i;u8 k;
    for(i=0;i<sizeof source_goblin/sizeof source_goblin[0];i++){
        const typeof(source_goblin[0]) *p=&source_goblin[i];Unit *a,*t;u32 seed,expected;CastBatch b;
        prepare_strike(0x10F);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->zodiac=p->az;t->zodiac=p->tz;a->hp=p->hp;a->max_hp=p->max_hp;
        st.support[U_RAMZA]=p->up?SUPPORT_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->gear==2){t->eq[EQ_ACC]=STRIKE_ACC;t->eq[EQ_SHIELD]=STRIKE_SHIELD;}
        if(!p->gear){u8 f;for(f=0;f<4;f++){t->face=f;if(attack_side(U_RAMZA,10)==SIDE_BACK)break;}CHECK(f<4,1);}
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=0x10F;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(b.victim[10].hit,p->forecast);CHECK(rng32==seed,1);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        if(p->hit){CHECK(b.victim[10].effect,AB_DAMAGE);CHECK(b.victim[10].amount,p->damage);}
    }
    CHECK(abilities[0x10F].kind,AB_DAMAGE);CHECK(abilities[0x10F].flags,AF_CAST|AF_AREA|AF_EVADE);
}
// Self Destruct 0x52: the original damage and Oil per target mode, then a production cast: the
// others lose the caster's missing HP and get Oil, the caster dies; no draw.
static void destruct_trials(void)
{
    u16 i;
    CHECK(abilities[0x115].kind,AB_DAMAGE);CHECK(abilities[0x115].flags,AF_CAST|AF_AREA|AF_SELF);
    for(i=0;i<sizeof source_destruct/sizeof source_destruct[0];i++){
        const typeof(source_destruct[0]) *p=&source_destruct[i];Unit *a,*t;u32 seed;u8 victim=p->mode==3?U_RAMZA:10;u16 hp;
        prepare_strike(0x115);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->hp=p->hp;a->max_hp=p->max_hp;t->hp=t->max_hp=999;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->mode==1)t->statuses.inflicted[2]=0x80;
        if(p->mode==2)t->statuses.immune[2]=0x80;
        unit_status_refresh(t);
        CHECK(can_use(U_RAMZA,0x115),1);seed=rng32;
        CHECK(cast_order(U_RAMZA,UNIT_CELL(a),0x115,0),1);
        CHECK(!!(W->cast.mask&((u16)1<<10)),1);CHECK(!!(W->cast.mask&((u16)1<<U_RAMZA)),1);
        CHECK(W->cast.victim[victim].hit,p->hit);CHECK(W->cast.victim[victim].amount,p->damage);
        CHECK(W->cast.victim[victim].effect&EFFECT_KIND_MASK,AB_DAMAGE);
        CHECK(!!(W->cast.victim[victim].effect&EFFECT_STATUS),p->add[2]!=0);
        hp=st.unit[victim].hp;impact();CHECK(rng32==seed,1);
        CHECK(st.unit[victim].hp,hp>p->damage?hp-p->damage:0);
        CHECK(a->hp,0);                                                  // the caster always dies
        if(p->mode==0)CHECK(core_status_current(&t->statuses,2)&0x80,t->hp?0x80:0);
        if(p->mode==2)CHECK(core_status_current(&t->statuses,2)&0x80,0);
    }
}
// Direct-targeting techniques (flags_4 0x20): Black Ink and Look of Devil (formula 0x50, the
// random-one set drawn after the accuracy), Look of Fright (0x62: Beaking's stages, Brave - Y).
static void direct_profiles(void)
{
    u16 i;u8 k;
    for(i=0;i<sizeof source_direct/sizeof source_direct[0];i++){
        const typeof(source_direct[0]) *p=&source_direct[i];Unit *a,*t;u32 seed,expected;CastBatch b;
        prepare_strike(p->ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->zodiac=p->az;t->zodiac=p->tz;
        st.support[U_RAMZA]=p->up?SUPPORT_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(k=0;k<5;k++)t->statuses.inflicted[k]=source_direct_status[p->mode][k];
        if(p->gear==2){t->eq[EQ_ACC]=STRIKE_ACC;t->eq[EQ_SHIELD]=STRIKE_SHIELD;}
        if(!p->gear){u8 f;for(f=0;f<4;f++){t->face=f;if(attack_side(U_RAMZA,10)==SIDE_BACK)break;}CHECK(f<4,1);}
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p->ab;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(b.victim[10].hit,p->forecast);CHECK(rng32==seed,1);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        CHECK(b.victim[10].effect,p->hit?AB_STATUS:AB_NULL);
        if(p->hit){
            u8 byte=(u8)(b.victim[10].amount>>8),mask=(u8)b.victim[10].amount;
            CHECK(byte<5,1);CHECK(mask,p->add[byte<5?byte:0]);
            for(k=0;k<5;k++)if(k!=byte)CHECK(p->add[k],0);
        }
    }
}
static void fright_profiles(void)
{
    u16 i;u8 k;
    for(i=0;i<sizeof source_fright/sizeof source_fright[0];i++){
        const typeof(source_fright[0]) *p=&source_fright[i];Unit *a,*t;u32 seed,expected;CastBatch b;
        prepare_strike(0x12E);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->zodiac=p->az;t->zodiac=p->tz;a->faith=p->af;t->faith=p->tf;
        st.support[U_RAMZA]=p->up?SUPPORT_MAGIC_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->gear){t->eq[EQ_ACC]=STRIKE_ACC;t->eq[EQ_SHIELD]=STRIKE_SHIELD;}
        unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=0x12E;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(b.victim[10].hit,p->forecast);CHECK(rng32==seed,1);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);
        CHECK(b.victim[10].effect,p->hit?AB_BRAVE:AB_NULL);CHECK(b.victim[10].amount,p->brave);
    }
}
// The trajectory: a flat row, the caster, a free panel, the target. A body on the line takes
// the technique instead; terrain on the line leaves nobody; an empty aimed panel too.
static u8 *direct_surface;   // direct_geometry's stack copy of the columns
static u8 direct_row(AbId ab,u8 gap)
{
    u16 c,i;prepare_strike(ab);
    memcpy(direct_surface,W->surface,(u16)MAP_W*MAP_H*8);W->surface=direct_surface;
    for(i=0;i<(u16)MAP_W*MAP_H;i++){direct_surface[i*8]=4;direct_surface[i*8+1]=0;direct_surface[i*8+3]=0;direct_surface[i*8+4]=0;direct_surface[i*8+7]=128;}
    for(i=0;i<MAP_N;i++)W->hs[i]=8;
    for(c=0;c+4<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+4]){
        for(i=0;i<5;i++)if(!map_tiles[c+i].walk||map_tiles[c+i].stand!=map_tiles[c].stand)break;   // the order's own height rule
        if(i==5)break;
    }
    CHECK(c+4<MAP_N,1);
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA&&i!=10){st.unit[i].hp=0;st.unit[i].dc=DC_GONE;st.unit[i].x=255;}
    unit_place(&st.unit[U_RAMZA],(u8)c);unit_place(&st.unit[10],(u8)(c+gap));
    return (u8)c;
}
static void direct_between(u8 c)
{
    st.unit[9]=st.unit[10];unit_place(&st.unit[9],(u8)(c+1));
}
static void direct_geometry(void)
{
    static const AbId skills[]={0x11E,0x12D,0x12E};
    const u8 *saved=W->surface;u8 saved_hs[MAP_NMAX],columns[2048];u8 i,c,s,hits=0;
    direct_surface=columns;
    memcpy(saved_hs,W->hs,sizeof saved_hs);
    for(i=0;i<3;i++){
        u8 gap=i?3:2;AbId ab=skills[i];
        CHECK(abilities[ab].flags,AF_CAST|AF_AREA|AF_EVADE);CHECK(abilities[ab].range,gap);
        CHECK(cj_range(common,ab)[6]&32,32);
        c=direct_row(ab,gap);
        CHECK(can_use(U_RAMZA,ab),1);
        CHECK(cast_targets(U_RAMZA,(u8)(c+gap),ab,0),(u16)1<<10);          // clear line
        CHECK(cast_targets(U_RAMZA,10,ab,1),(u16)1<<10);
        CHECK(cast_targets(U_RAMZA,(u8)(c+1),ab,0),0);                    // an empty aimed panel
        direct_between(c);
        CHECK(cast_targets(U_RAMZA,(u8)(c+gap),ab,0),(u16)1<<9);          // the body on the line
        CHECK(cast_targets(U_RAMZA,(u8)(c+1),ab,0),(u16)1<<9);
        st.unit[9].hp=0;st.unit[9].dc=DC_GONE;st.unit[9].x=255;
        direct_surface[((u16)(W->grid_row[TILE_Z[c+1]]+TILE_X[c+1])<<3)]=20;
        CHECK(cast_targets(U_RAMZA,(u8)(c+gap),ab,0),0);                  // terrain on the line
        W->surface=saved;memcpy(W->hs,saved_hs,sizeof saved_hs);
    }
    // Production: Black Ink aimed past a body blinds that body, not the aimed unit.
    for(s=1;s<60&&!hits;s++){
        c=direct_row(0x11E,2);direct_between(c);st.unit[U_RAMZA].ma=70;st.unit[9].team=TEAM_ENEMY;
        rng32=((u32)s*0x9E3779B1UL)|1;
        CHECK(cast_order(U_RAMZA,10,0x11E,1),1);CHECK(W->cast.mask,(u16)1<<9);impact();
        CHECK(core_status_current(&st.unit[10].statuses,1)&0x20,0);
        if(core_status_current(&st.unit[9].statuses,1)&0x20)hits++;
        W->surface=saved;memcpy(W->hs,saved_hs,sizeof saved_hs);
    }
    CHECK(hits,1);
}
// Production: Black Ink blinds; Look of Devil leaves each of its five statuses over the seeds;
// Look of Fright lowers Brave 70 -> 60.
static void direct_production(void)
{
    u8 s,seen=0,hits=0;
    for(s=1;s<60&&!hits;s++){
        Unit *t;prepare_strike(0x11E);t=&st.unit[10];st.unit[U_RAMZA].ma=70;rng32=((u32)s*0x9E3779B1UL)|1;
        CHECK(cast_order(U_RAMZA,10,0x11E,1),1);impact();
        if(core_status_current(&t->statuses,1)&0x20)hits++;
        CHECK(t->hp,999);
    }
    CHECK(hits,1);
    for(s=1;s<200&&seen!=31;s++){
        Unit *t;prepare_strike(0x12D);t=&st.unit[10];st.unit[U_RAMZA].ma=70;rng32=((u32)s*0x9E3779B1UL)|1;
        CHECK(can_use(U_RAMZA,0x12D),1);CHECK(cast_order(U_RAMZA,10,0x12D,1),1);impact();
        if(core_status_current(&t->statuses,1)&0x80)seen|=1;
        if(core_status_current(&t->statuses,1)&0x20)seen|=2;
        if(core_status_current(&t->statuses,1)&0x08)seen|=4;
        if(core_status_current(&t->statuses,4)&0x08)seen|=8;
        if(core_status_current(&t->statuses,4)&0x04)seen|=16;
    }
    CHECK(seen,31);
    for(s=1,hits=0;s<60&&!hits;s++){
        Unit *t;prepare_strike(0x12E);t=&st.unit[10];st.unit[U_RAMZA].ma=70;t->brave=70;rng32=((u32)s*0x9E3779B1UL)|1;
        CHECK(abilities[0x12E].kind,AB_BRAVE);
        CHECK(can_use(U_RAMZA,0x12E),1);CHECK(cast_order(U_RAMZA,10,0x12E,1),1);impact();
        if(t->brave==70)continue;
        hits++;CHECK(t->brave,60);
    }
    CHECK(hits,1);
}
// Three programs (the 68000 check size limit): check_status_strike2.c defines STRIKE_PART2,
// check_status_strike3.c STRIKE_PART3.
#ifdef STRIKE_PART3
static void strike_trials(void){direct_profiles();fright_profiles();direct_geometry();direct_production();}
#elif defined(STRIKE_PART2)
static void strike_trials(void){wave_production();beaking_trials();goblin_trials();destruct_trials();}
#else
static void strike_trials(void){strike_profiles();strike_production();magical_profiles();magical_production();}
#endif
#ifndef __m68k__
int main(void){sw_init(61);strike_trials();printf("status_strike %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();strike_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
