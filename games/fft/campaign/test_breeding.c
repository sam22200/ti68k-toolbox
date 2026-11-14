// Source-read boundaries, not shipping worldmap/FSV7 integration.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include "save_types.h"
#include "../base.inc"
#include "breeding.h"
#include "../raw_stats.h"
#include "x/breeding/fixture.h"
static Save save,before;
static WorldParty world,prior;
static u8 scratch[WB_RULE_BYTES+1];
static u16 checks,failures,draw_count,draw_at;
static u32 digest=23;
static const u16 *draws;
static u16 first_roll,fallback;
static void check(u32 actual,u32 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^actual;
    if(actual!=expected){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("breeding line%u: %lu expected%lu\n",line,(unsigned long)actual,(unsigned long)expected);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static u16 draw(void *ctx)
{
    (void)ctx;
    if(!draw_count++)return first_roll;
    return draws?draws[draw_at++]:fallback;
}
static void seed(u8 n)
{
    u8 i;memset(&save,0,sizeof save);memcpy(save.magic,SAVE_MAGIC,4);save.n=n;
    for(i=0;i<n;i++){save.unit[i].level=7;save.unit[i].job=74;save.unit[i].name=i;}
    memset(&world,0,sizeof world);world.month=1;world.day=1;world.era=1;
    draw_count=draw_at=0;draws=RT_NULL;first_roll=fallback=255;
}
static void parent(u8 slot,u8 job)
{SaveUnit *u=wb_unit(&save,slot);u->type=3;u->job=job;u->level=42;u->name=250;}
static void unchanged(void)
{CHECK(memcmp(&save,&before,sizeof save),0);CHECK(memcmp(&world,&prior,sizeof world),0);CHECK(draw_count,0);}
static void trials(void)
{
    u8 i;u16 k;
    CHECK(sizeof(Save),3918);CHECK(sizeof(WorldParty),69);
    CHECK(world_rules_valid(rules,sizeof rules),1);
    memcpy(scratch+1,rules,sizeof rules);CHECK(world_rules_valid(scratch+1,sizeof rules),1);
    for(i=0;i<7;i++){
        static const u16 offsets[]={0,4,6,8,20,59,261};
        memcpy(scratch,rules,sizeof rules);scratch[offsets[i]]=255;
        CHECK(world_rules_valid(scratch,sizeof rules),0);
    }
    CHECK(world_rules_valid(rules,sizeof rules-1),0);
    memcpy(scratch,rules,sizeof rules);scratch[262]++;
    CHECK(world_rules_valid(scratch,sizeof rules),0);
    seed(1);before=save;
    CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);
    CHECK(world.day,2);CHECK(draw_count,0);CHECK(memcmp(&save,&before,sizeof save),0);
    world.month=2;world.day=28;CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);
    CHECK(world.month,3);CHECK(world.day,1);
    world.month=12;world.day=31;CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);
    CHECK(world.month,1);CHECK(world.day,1);CHECK(world.era,1);
    world.month=3;world.day=20;CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);CHECK(world.era,2);
    world.day=20;world.era=99;world_party_day(&world,&save,rules,draw,RT_NULL);CHECK(world.era,99);
    // Rarity/no-birth edges: source draws once on failure, four on full success.
    for(i=0;i<8;i++){
        static const u8 rolls[]={0,5,6,15,16,31,32,255};
        seed(16);parent(1,95);first_roll=rolls[i];before=save;
        CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),i<6?WB_FULL:WB_DAY);
        CHECK(draw_count,i<6?4:1);CHECK(save.n,16);CHECK(memcmp(&save,&before,sizeof save),0);
    }
    // Eight complete source-model children, including every serialized stat.
    for(i=0;i<sizeof cases/sizeof cases[0];i++){
        seed(2);parent(1,cases[i].parent);before=save;
        draws=cases[i].draws;first_roll=cases[i].roll;
        CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_LAID);
        CHECK(draw_count,cases[i].count+1);CHECK(save.n,3);
        CHECK(memcmp(&save.unit[2],cases[i].unit,sizeof(SaveUnit)),0);
        CHECK(world.egg[2],cases[i].days*17);CHECK(world_egg_color(&world,2),cases[i].color);
        CHECK(save_raw_current(&save,2),0);
        CHECK(memcmp(save.unit,before.unit,2*sizeof(SaveUnit)),0);
        CHECK(memcmp(save.guest,before.guest,sizeof save.guest),0);
        CHECK(world_party_valid(&world,&save,rules),1);
        // A cold byte round-trip restores the actual generated child. Hatching
        // changes only zodiac and metadata, never seeds/name/level/Brave/Faith.
        memcpy(&before,&save,sizeof save);memcpy(&prior,&world,sizeof world);
        memset(&save,0,sizeof save);memset(&world,0,sizeof world);
        memcpy(&save,&before,sizeof save);memcpy(&world,&prior,sizeof world);
        draws=RT_NULL;first_roll=fallback=255;draw_count=0;
        for(k=0;k<cases[i].days;k++)CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);
        CHECK(world.egg[2],0);CHECK(wb_word(world.birthday[2]),2+cases[i].days);
        CHECK(world_egg_color(&world,2),cases[i].color);CHECK(save.unit[2].zodiac,9);
        before.unit[2].zodiac=save.unit[2].zodiac;
        CHECK(memcmp(&save,&before,sizeof save),0);
    }
    // A hatch cannot breed or donate a level on that same day's scan. The next
    // day it can; there is no minimum mating pair requirement.
    seed(1);parent(0,94);world.egg[0]=0x21;
    CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);CHECK(draw_count,0);CHECK(world.egg[0],0);
    before=save;first_roll=32;
    CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);CHECK(draw_count,1);CHECK(save.n,1);
    seed(1);parent(0,94);world.egg[0]=0x21;world.month=3;world.day=20;
    CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);
    CHECK(wb_word(world.birthday[0]),80);CHECK(save.unit[0].zodiac,0);CHECK(world.era,2);
    // An existing egg can hatch while a full party rejects that day's birth.
    seed(16);parent(1,95);parent(2,96);world.egg[2]=0x21;first_roll=0;
    CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_FULL);CHECK(world.egg[2],0);CHECK(draw_count,4);
    // A lone persistent guest is a valid parent and level donor, but the egg
    // still consumes an ordinary slot. Newly hatched guest is excluded today.
    seed(0);save.guest_id[3][1]=4;parent(19,94);before=save;first_roll=0;fallback=12345;
    CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_LAID);CHECK(save.n,1);CHECK(save.unit[0].level,42);
    CHECK(memcmp(save.guest,before.guest,sizeof save.guest),0);CHECK(world_party_valid(&world,&save,rules),1);
    seed(1);save.guest_id[0][1]=4;parent(16,94);world.egg[16]=0x21;
    CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_DAY);CHECK(draw_count,0);
    CHECK(wb_word(world.birthday[16]),2);
    // Name collision retries only generation; it does not redraw offspring,
    // timer or stats. Zero draw -> existing monster name0, then fallback changes.
    seed(2);parent(1,95);save.unit[1].name=0;first_roll=0;
    {static const u16 retry[]={0,0,2,0,0,0,0,0,0,0,0,0,32767,1};draws=retry;
     CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_LAID);CHECK(save.unit[2].name,254);CHECK(draw_count,15);}
    // Invalid state rejects before date/RNG/party writes; no underflowed egg.
    for(i=0;i<8;i++){
        seed(2);parent(1,95);
        if(i==0)world.month=0;
        if(i==1)world.day=32;
        if(i==2)world.era=100;
        if(i==3)world.egg[1]=0x20;
        if(i==4)world.egg[1]=0x23;
        if(i==5)world.egg[2]=0x22;
        if(i==6)save.guest_reserved=1;
        if(i==7)save.unit[1].level=0;
        before=save;prior=world;CHECK(world_party_day(&world,&save,rules,draw,RT_NULL),WB_INVALID);unchanged();
    }
    // Side metadata follows compaction/guest-to-player moves, including color.
    seed(2);world.egg[19]=0x32;world.birthday[19][1]=0;wb_color_set(&world,19,3);
    world_party_move_slot(&world,1,19);CHECK(world.egg[1],0x32);CHECK(world.egg[19],0);
    CHECK(world_egg_color(&world,1),3);CHECK(world_egg_color(&world,19),0);
    prior=world;world_party_move_slot(&world,1,1);world_party_move_slot(&world,20,1);
    CHECK(memcmp(&world,&prior,sizeof world),0);world_party_clear_slot(&world,1);CHECK(world.egg[1],0);
    // Existing seed representation agrees with step-by-step source growth on
    // a concrete source monster seed; it need not run this loop during travel.
    {u32 value=35UL<<14,stepped=value;for(i=1;i<42;i++)stepped=core_raw_level(stepped,6,i,0);
     CHECK(core_raw_grown(value,6,42),stepped);}
    for(k=0;k<sizeof save;k++)digest=(digest<<1|digest>>31)^((const u8 *)&save)[k];
    for(k=0;k<sizeof world;k++)digest=(digest<<1|digest>>31)^((const u8 *)&world)[k];
}
#ifndef __m68k__
int main(void){trials();printf("breeding %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
