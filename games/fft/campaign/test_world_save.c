// Migration/metadata carrier shared by PC/sanitizers and a selected68000 run.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include <string.h>
#include "save_types.h"
static u8 image[3993];static u16 image_size;
static u8 read_image(const char *name,void *p,u16 size)
{(void)name;if(size!=image_size)return 0;memcpy(p,image,size);return 1;}
#define rt_load read_image
#include "save.h"
static Save a,b,old;
static u16 checks,failures;static u32 digest=17;
static void check(u32 x,u32 expected,u16 line)
{checks++;digest=(digest<<1|digest>>31)^x;if(x!=expected){failures++;
#ifdef __m68k__
 BENCH_VALUE(line);
#else
 printf("world-save line%u got%lu expected%lu\n",line,(unsigned long)x,(unsigned long)expected);
#endif
}}
#define CHECK(x,y) check((u32)(x),(u32)(y),__LINE__)
static void seed(void)
{
    u8 i;save_init(&a);a.n=16;a.loc=25;a.flags[0]=3;
    for(i=0;i<16;i++){a.unit[i].job=74;a.unit[i].level=(u8)(i+1);a.unit[i].name=i;}
    a.guest_id[0][1]=4;a.guest[0]=a.unit[3];a.guest[0].exp=93;save_raw_set(&a,16,1);
    a.inv[22]=7;a.discovered[9]=64;
}
static void serialize(void){image_size=sizeof a;memcpy(image,&a,image_size);}
static void trials(void)
{
    u8 i;u16 k;
    CHECK(sizeof(Save),3992);CHECK(sizeof(WorldParty),69);
    seed();CHECK(save_world_valid(&a),1);old=a;
    for(i=0;i<5;i++){
        static const u16 lengths[]={SAVE_V2_BYTES,SAVE_V3_BYTES,SAVE_V4_BYTES,SAVE_V5_BYTES,SAVE_V6_BYTES};
        image_size=lengths[i];memcpy(image,&old,image_size);memcpy(image,"FSV2",4);image[3]+=i;
        memset(&b,255,sizeof b);CHECK(save_load(&b),1);CHECK(memcmp(b.magic,SAVE_MAGIC,4),0);
        CHECK(memcmp((u8 *)&b+4,(u8 *)&old+4,image_size-4),0);
        CHECK(b.world.month,1);CHECK(b.world.day,1);CHECK(b.world.era,0);CHECK(save_world_valid(&b),1);
        if(i==4){CHECK(b.guest[0].exp,93);CHECK(save_raw_current(&b,16),1);}
        else CHECK(save_guest_find(&b,4),255);
    }
    seed();a.world.month=3;a.world.day=20;a.world.era=99;
    a.unit[15].type=3;a.unit[15].job=95;a.world.egg[15]=0x32;wb_color_set(&a.world,15,3);
    serialize();CHECK(save_load(&b),1);CHECK(memcmp(&a,&b,sizeof a),0);
    // Invalid current state cannot silently become a legacy save.
    for(i=0;i<8;i++){
        seed();
        if(i==0)a.world.month=0;
        if(i==1){a.world.month=2;a.world.day=29;}
        if(i==2)a.world.era=100;
        if(i==3)a.world.egg[2]=0x20;
        if(i==4){a.n=15;a.world.egg[15]=0x22;}
        if(i==5)memset(a.world_rng,0,4);
        if(i==6)a.world_pad=1;
        if(i==7)a.guest_id[1][1]=4;
        serialize();CHECK(save_load(&b),0);CHECK(b.n,0);CHECK(b.magic[0],0);
    }
    seed();old=a;CHECK(save_dismiss_egg(&a,1),0);CHECK(memcmp(&a,&old,sizeof a),0);
    a.unit[1].type=3;a.unit[1].job=94;a.world.egg[1]=0x22;
    a.unit[2].type=3;a.unit[2].job=96;a.world.egg[2]=0x43;wb_color_set(&a.world,2,2);save_raw_set(&a,2,1);
    old=a;CHECK(save_dismiss_egg(&a,1),1);CHECK(a.n,15);CHECK(a.world.egg[1],0x43);
    CHECK(world_egg_color(&a.world,1),2);CHECK(save_raw_current(&a,1),1);
    CHECK(memcmp(&a.unit[1],&old.unit[2],sizeof(SaveUnit)),0);CHECK(a.world.egg[15],0);
    CHECK(memcmp(a.guest,old.guest,sizeof a.guest),0);CHECK(save_world_valid(&a),1);
    a.guest[0].type=3;a.guest[0].job=95;a.world.egg[16]=0x54;wb_color_set(&a.world,16,1);
    CHECK(save_guest_recruit(&a,4),15);CHECK(a.world.egg[15],0x54);CHECK(a.world.egg[16],0);
    CHECK(world_egg_color(&a.world,15),1);CHECK(a.guest_raw,0);CHECK(save_world_valid(&a),1);
    a.guest_id[2][1]=11;a.guest[2]=old.unit[0];a.world.birthday[18][1]=33;
    CHECK(save_guest_remove(&a,11),1);CHECK(a.world.birthday[18][1],0);CHECK(save_world_valid(&a),1);
    for(k=0;k<sizeof a;k++)digest=(digest<<1|digest>>31)^((u8 *)&a)[k];
}
#ifndef __m68k__
int main(void){trials();printf("world-save %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
