#include "mmx.h"
#include "terrain.h"
#include "../../runtime/platform-sw/rt_sw.h"
#include "generated/motion_ref.h"
#include "generated/combat_ref.h"
#include "generated/combined_ref.h"
#include "generated/enemy_ref.h"
#include "generated/damage_ref.h"
#include "generated/capacity_ref.h"
#include "generated/wall_ref.h"
#include "generated/traversal_ref.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static u16 buttons(const char *name,unsigned f)
{
    if(!strcmp(name,"right"))return K_RIGHT;
    if(!strcmp(name,"left"))return K_LEFT;
    if(!strcmp(name,"release"))return f<35?K_RIGHT:0;
    if(!strcmp(name,"turn"))return f<35?K_RIGHT:f<60?K_LEFT:0;
    if(!strcmp(name,"jump_1"))return f<1?K_A:0;
    if(!strcmp(name,"jump_10"))return f<10?K_A:0;
    if(!strcmp(name,"jump_held"))return f<60?K_A:0;
    if(!strcmp(name,"run_jump"))return (f<60?K_RIGHT:0)|(f<35?K_A:0);
    if(!strcmp(name,"run_then_jump"))return (f<70?K_RIGHT:0)|(f>=20 && f<45?K_A:0);
    if(!strcmp(name,"wall_slide"))return K_RIGHT;
    if(!strcmp(name,"wall_kick_short"))return K_RIGHT|(f<2?K_A:0);
    if(!strcmp(name,"wall_kick_held"))return K_RIGHT|(f<35?K_A:0);
    return 0;
}
static void check(const char *name,const MotionSample *samples,unsigned count)
{
    unsigned f;
    sw_init(!strcmp(name,"hurt")?7:strncmp(name,"wall_",5)?1:2);mmx.enabled=0;
    for(f=0;f<count;++f) {
        const MotionSample *p=samples+f;
        mmx_step(buttons(name,f));
        if(mmx.x!=p->x || mmx.y!=p->y || mmx.vx!=p->vx || mmx.vy!=p->vy ||
           mmx.xs!=p->xs || mmx.ys!=p->ys || mmx.state!=p->state) {
            printf("%s frame%u got %u/%u %d/%d %u/%u state%u expected %u/%u %d/%d %u/%u state%u\n",name,f,
                mmx.x,mmx.y,mmx.vx,mmx.vy,mmx.xs,mmx.ys,mmx.state,
                p->x,p->y,p->vx,p->vy,p->xs,p->ys,p->state);fflush(stdout);assert(0);
        }
    }
}
static void projectile(unsigned hold,const ShotSample *samples)
{
    unsigned f;
    sw_init(1);
    if(hold>1)for(f=0;f<hold;++f)mmx_step(K_B);
    for(f=0;f<24;++f) {
        const ShotSample *r=samples+f;
        const MmxShot *p=mmx.shots;
        mmx_step(hold==1 && !f?K_B:0);
        if(!p->active || p->x!=r->x || p->y!=r->y || p->vx!=r->vx || p->xs!=r->xs || p->kind!=r->kind) {
            printf("hold%u frame%u shot got %u/%u vx%d xs%u kind%u expected %u/%u vx%d xs%u kind%u\n",
                   hold,f,p->x,p->y,p->vx,p->xs,p->kind,r->x,r->y,r->vx,r->xs,r->kind);
            fflush(stdout);assert(0);
        }
    }
}
static void combined(const char *name,const ComboSample *samples,unsigned count,
                     const ComboKeys *keys,unsigned nkeys,unsigned frames,unsigned precharge,unsigned scenario)
{
    unsigned f,k=0,q=0;u16 held=0;
    sw_init(scenario);
    for(f=0;f<precharge;++f)mmx_step(K_B);
    for(f=0;f<frames;++f) {
        if(k<nkeys && keys[k].frame==f)held=keys[k++].keys;
        mmx_step(held);
        while(q<count && samples[q].frame==f) {
            const ComboSample *r=samples+q++;
            const MmxShot *p=mmx.shots+r->slot;
            s16 dy=(s16)p->y-r->y;
            if(!p->active || p->x!=r->x || dy>r->ytol || dy<-(s16)r->ytol ||
               p->vx!=r->vx || p->kind!=r->kind || (r->checkxs && p->xs!=r->xs)) {
                printf("%s frame%u shot%u got %u/%u vx%d kind%u active%u xs%u expected %u/%u vx%d kind%u xs%u\n",
                       name,f,r->slot,p->x,p->y,p->vx,p->kind,p->active,p->xs,r->x,r->y,r->vx,r->kind,r->xs);
                fflush(stdout);assert(0);
            }
        }
    }
    assert(q==count);
}
static void combat(void)
{
    unsigned f,n;
    for(n=30;n<=101;++n) {
        sw_init(1);for(f=0;f<n;++f)mmx_step(K_B);mmx_step(0);
        if(n==30)assert(!mmx.shots[0].active || mmx.shots[0].kind==0);
        else { unsigned i;for(i=0;i<3;++i)if(mmx.shots[i].active && mmx.shots[i].kind==(n>=101?3:1))break;assert(i<3); }
    }
    sw_init(0);
    for(f=0;f<350;++f)mmx_step(f<287?K_RIGHT:0);
    assert(mmx.hp==14 && mmx.state==MX_IDLE && mmx.enemy.hp==2);
    /* Traverse the actual map, charge before activation and jump the gap. */
    sw_init(0);
    for(f=0;f<680;++f)mmx_step(K_RIGHT | (f>=140 && f<241?K_B:0) | (f>=450 && f<480?K_A:0));
    assert(mmx.won && mmx.hp==16 && mmx.kills==1);
    sw_init(5);
    for(f=0;f<80;++f)assert(sw_step(f==0 || f==16?K_B:0));
    assert(mmx.kills==1 && mmx.hits==2 && mmx.hp==14);
    sw_init(1);rt_keys=K_A;
    assert(sw_step(K_A));assert(mmx.state==MX_IDLE && mmx.grounded);
    assert(sw_step(0));assert(sw_step(K_A));assert(mmx.state==MX_RISE && !mmx.grounded);
    sw_init(2);mmx.hp=2;mmx.enemy.active=mmx.enemy.spawned=1;mmx.enemy.phase=2;
    mmx.enemy.hp=2;mmx.enemy.x=mmx.x+8;mmx.enemy.y=mmx.y;mmx_step(0);
    assert(mmx.state==MX_DEAD && !mmx.hp);game_scenario(0);assert(mmx.hp==16 && !mmx.enemy.spawned);
}
static void culling(void)
{
    unsigned scenario;
    /* Source one-coordinate probes establish [-32,288) and the distinct
       normal/charged removal phases; target width extends the right bound. */
    for(scenario=110;scenario<=115;++scenario) {
        const MmxShot *p;
        sw_init(scenario);p=mmx.shots;mmx_step(0);
        assert(p->active);
        assert((s16)p->x==(scenario<113?(p->kind==3?-40:p->kind?-38:-32):
                                      (p->kind==3?359:p->kind?357:351)));
        mmx_step(0);assert(!p->active);
    }
}
static void enemy(const char *name,const EnemySample *samples,unsigned count,
                  const ComboKeys *keys,unsigned nkeys,unsigned frames,unsigned scenario)
{
    unsigned f,k=0,q=0;u16 held=0;
    sw_init(scenario);
    for(f=0;f<frames;++f) {
        if(k<nkeys && keys[k].frame==f)held=keys[k++].keys;
        mmx_step(held);
        if(q<count && samples[q].frame==f) {
            const EnemySample *r=samples+q++;
            const MmxEnemy *e=&mmx.enemy;
            if(!!e->active!=r->active || mmx.hp!=r->player_hp ||
               (r->active && (e->x!=r->x || e->y!=r->y || e->xs!=r->xs || e->vx!=r->vx ||
                              e->hp!=r->hp || e->phase!=r->phase || e->fuse!=r->fuse))) {
                printf("%s frame%u enemy got active%u x%u/y%u xs%u vx%d hp%u phase%u fuse%u heroHP%u expected active%u x%u/y%u xs%u vx%d hp%u phase%u fuse%u heroHP%u\n",name,f,
                    e->active,e->x,e->y,e->xs,e->vx,e->hp,e->phase,e->fuse,mmx.hp,
                    r->active,r->x,r->y,r->xs,r->vx,r->hp,r->phase,r->fuse,r->player_hp);
                fflush(stdout);assert(0);
            }
        }
    }
    assert(q==count);
}
static void edge_sample(const EdgeSample *r)
{
    const MotionSample *p=&r->motion;
    if(mmx.x!=p->x || mmx.y!=p->y || mmx.vx!=p->vx || mmx.vy!=p->vy ||
       mmx.xs!=p->xs || mmx.ys!=p->ys || mmx.state!=p->state || mmx.wall!=r->wall ||
       (mmx.state!=MX_SLIDE && mmx.state!=MX_KICK && mmx.facing!=r->facing)) {
        /* Wall art banks are grouped by wall side; effective gun/pose direction
           is selected by wallage. That bank orientation is not source facing. */
        printf("Edge tick%u got %u/%u vx%d vy%d xs%u ys%u state%u wall%u face%u expected %u/%u vx%d vy%d xs%u ys%u state%u wall%u face%u\n",mmx.tick,
               mmx.x,mmx.y,mmx.vx,mmx.vy,mmx.xs,mmx.ys,mmx.state,mmx.wall,mmx.facing,
               p->x,p->y,p->vx,p->vy,p->xs,p->ys,p->state,r->wall,r->facing);
        fflush(stdout);assert(0);
    }
}
static void ledge_case(const EdgeSample *samples,unsigned count,const ComboKeys *keys,unsigned nkeys,unsigned scenario)
{
    unsigned f,k=0;u16 held=0;sw_init(scenario);
    for(f=0;f<count;++f) {
        if(k<nkeys && keys[k].frame==f)held=keys[k++].keys;
        mmx_step(held);edge_sample(samples+f);
    }
}
static void edge_checks(void)
{
    unsigned i;
#define LEDGE(n,S) ledge_case(ledge_##n,sizeof(ledge_##n)/sizeof(ledge_##n[0]),ledge_keys_##n,sizeof(ledge_keys_##n)/sizeof(ledge_keys_##n[0]),S);
    LEDGE_CASES(LEDGE)
#undef LEDGE
    for(i=0;i<16;++i) { sw_init(126+i);mmx_step(K_RIGHT);edge_sample(first_clamp+i); }
}
static void traversal(const char *name,const TraverseSample *samples,unsigned count,
                      const ComboKeys *keys,unsigned nkeys)
{
    unsigned f,k=0;u16 held=0;
    sw_init(0);
    for(f=0;f<count;++f) {
        const TraverseSample *r=samples+f;const MotionSample *p=&r->motion;
        const MmxEnemy *e=&mmx.enemy;u8 tier,births=0,i;
        if(k<nkeys && keys[k].frame==f)held=keys[k++].keys;
        mmx_step(held);tier=mmx.charge<31?0:mmx.charge<101?3:2;
        for(i=0;i<3;++i)if(mmx.shots[i].active && !mmx.shots[i].age)births|=1<<i;
        if(mmx.x!=p->x || mmx.y!=p->y || mmx.vx!=p->vx || mmx.vy!=p->vy ||
           mmx.xs!=p->xs || mmx.ys!=p->ys || mmx.state!=p->state || mmx.hp!=r->hp ||
           (r->charging?(!mmx.charge || tier!=r->tier):mmx.charge!=0) || births!=r->births ||
           !!e->active!=r->active || (r->active && (e->x!=r->ex || e->y!=r->ey ||
           e->vx!=r->evx || e->xs!=r->exs || e->hp!=r->ehp || e->phase!=r->phase || e->fuse!=r->fuse))) {
            printf("%s logic%u route mismatch: hero%u/%u state%u HP%u roller%u/%u phase%u HP%u\n",
                   name,f,mmx.x,mmx.y,mmx.state,mmx.hp,e->x,e->y,e->phase,e->hp);
            fflush(stdout);assert(0);
        }
    }
    assert(mmx.won && mmx.kills==1 && mmx.hp==16);
}
static void damage(const char *name,const DamageSample *samples,unsigned count,
                   const ComboKeys *keys,unsigned nkeys,unsigned scenario)
{
    unsigned f,k=0,i;u16 held=0;
    sw_init(scenario);
    for(f=0;f<count;++f) {
        const DamageSample *r=samples+f;u8 tier;
        if(k<nkeys && keys[k].frame==f)held=keys[k++].keys;
        mmx_step(held);tier=mmx.charge<31?0:mmx.charge<101?3:2;
        if(mmx.x!=r->x || mmx.y!=r->y || mmx.vx!=r->vx || mmx.vy!=r->vy ||
           mmx.xs!=r->xs || mmx.ys!=r->ys || mmx.state!=r->state || mmx.hp!=r->hp ||
           (r->charging?(!mmx.charge || tier!=r->tier):mmx.charge!=0)) {
            printf("%s frame%u contact/charge mismatch: state%u HP%u charge%u\n",name,f,mmx.state,mmx.hp,mmx.charge);
            fflush(stdout);assert(0);
        }
        for(i=0;i<3;++i) {
            const MmxShot *p=mmx.shots+i;const DamageShot *shot=r->shots+i;
            if((p->active && !p->age)!=shot->born || (shot->active &&
               (!p->active || p->x!=shot->x || p->y!=shot->y || p->vx!=shot->vx ||
                p->kind!=shot->kind || (!p->kind && p->xs!=shot->xs)))) {
                printf("%s frame%u shot%u birth/trajectory mismatch\n",name,f,i);
                fflush(stdout);assert(0);
            }
        }
    }
}
static void capacity(const char *name,const CapacitySample *samples,unsigned count,
                     const ComboKeys *keys,unsigned nkeys)
{
    unsigned f,k=0,i;u16 held=0;
    sw_init(101);
    for(f=0;f<count;++f) {
        const CapacitySample *r=samples+f;u8 births=0,tier;
        if(k<nkeys && keys[k].frame==f)held=keys[k++].keys;
        mmx_step(held);tier=mmx.charge<31?0:mmx.charge<101?3:2;
        for(i=0;i<3;++i)if(mmx.shots[i].active && !mmx.shots[i].age)births|=1<<i;
        if(births!=r->births || (r->charging?(!mmx.charge || tier!=r->tier):mmx.charge!=0)) {
            printf("%s frame%u birth/charge mismatch: mask%u charge%u\n",name,f,births,mmx.charge);
            fflush(stdout);assert(0);
        }
    }
}
static void wall_probes_check(void)
{
    unsigned n,f,i;
    for(n=0;n<sizeof(wall_probes)/sizeof(wall_probes[0]);++n) {
        const WallProbe *r=wall_probes+n;const MmxShot *shot=0;
        sw_init(102);
        for(f=0;f<=r->frame;++f)
            mmx_step(K_RIGHT | (r->kick && f>=10?K_A:0) | (f==r->frame || f==r->earlier?K_B:0));
        for(i=0;i<3;++i)if(mmx.shots[i].active && !mmx.shots[i].age) {
            assert(!shot);shot=mmx.shots+i;
        }
        if(mmx.x!=r->x || mmx.y!=r->y || mmx.vx!=r->vx || !!shot!=r->active ||
           (shot && (shot->x!=r->shotx || shot->y!=r->shoty))) {
            printf("wall probe%u frame%u prior%u kick%u got hero%u/%u vx%d shot%s %u/%u expected hero%u/%u vx%d active%u %u/%u\n",n,r->frame,r->earlier,r->kick,
                mmx.x,mmx.y,mmx.vx,shot?"yes":"no",shot?shot->x:0,shot?shot->y:0,
                r->x,r->y,r->vx,r->active,r->shotx,r->shoty);fflush(stdout);assert(0);
        }
    }
}
static void wall_fraction(unsigned scenario,const ShotSample *samples)
{
    unsigned f;sw_init(scenario);
    for(f=0;f<8;++f) {
        const ShotSample *r=samples+f;const MmxShot *p=mmx.shots;
        mmx_step(!f?K_B:0);
        assert(p->active && p->x==r->x && p->y==r->y && p->vx==r->vx && p->xs==r->xs && p->kind==r->kind);
    }
}
static int trace(int argc,char **argv)
{
    static SwScript script;
    unsigned f,n,count=(unsigned)atoi(argv[3]);u16 words[MMX_WORDS];
    sw_init((u16)atoi(argv[2]));assert(sw_load_script(&script,argv[4])==0);
    for(f=0;f<count;++f) {
        assert(sw_step(sw_script_keys(&script,f)));mmx_export(words);
        for(n=0;n<MMX_WORDS;++n)printf("%u ",words[n]);
        printf("|%04X\n",sw_checksum());
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc==5 && !strcmp(argv[1],"--trace"))return trace(argc,argv);
    sw_init(0);assert(terrain_ready());
#define CHECK(n) check(#n,ref_##n,sizeof(ref_##n)/sizeof(ref_##n[0]))
    CHECK(idle);CHECK(right);CHECK(left);CHECK(release);CHECK(turn);
    CHECK(jump_1);CHECK(jump_10);CHECK(jump_held);CHECK(run_jump);CHECK(run_then_jump);
    CHECK(wall_slide);CHECK(wall_kick_short);CHECK(wall_kick_held);
    CHECK(hurt);
#undef CHECK
    puts("1230 original USA movement/recoil states match portable C");
    projectile(1,ref_shot_1);projectile(45,ref_shot_45);projectile(110,ref_shot_110);
#define COMBO(n,N) combined(#n,ref_combo_##n,sizeof(ref_combo_##n)/sizeof(ref_combo_##n[0]),keys_##n,sizeof(keys_##n)/sizeof(keys_##n[0]),COMBO_##N##_FRAMES,COMBO_##N##_PRECHARGE,COMBO_##N##_SCENARIO)
    COMBO(run_tap,RUN_TAP);COMBO(jump_tap,JUMP_TAP);COMBO(run_charge,RUN_CHARGE);
    COMBO(run_medium,RUN_MEDIUM);COMBO(run_charge_turn,RUN_CHARGE_TURN);
    COMBO(jump_charge,JUMP_CHARGE);COMBO(wall_tap,WALL_TAP);
    COMBO(jump_late_charge,JUMP_LATE_CHARGE);
    COMBO(left_tap,LEFT_TAP);COMBO(left_medium,LEFT_MEDIUM);COMBO(left_charge,LEFT_CHARGE);
    COMBO(run_stop_tap,RUN_STOP_TAP);COMBO(run_stop_medium,RUN_STOP_MEDIUM);
    COMBO(run_stop_charge,RUN_STOP_CHARGE);
    COMBO(run_jump_tap,RUN_JUMP_TAP);COMBO(run_jump_medium,RUN_JUMP_MEDIUM);
    COMBO(run_jump_large,RUN_JUMP_LARGE);COMBO(ledge_tap,LEDGE_TAP);
    COMBO(ledge_stop_tap,LEDGE_STOP_TAP);COMBO(ledge_medium,LEDGE_MEDIUM);
    COMBO(ledge_large,LEDGE_LARGE);
#undef COMBO
    puts("Original combined-shot samples pass; run bob flattened within one source pixel");
#define ENEMY(n,N) enemy(#n,enemy_##n,sizeof(enemy_##n)/sizeof(enemy_##n[0]),enemy_keys_##n,sizeof(enemy_keys_##n)/sizeof(enemy_keys_##n[0]),ENEMY_##N##_FRAMES,ENEMY_##N##_SCENARIO)
    ENEMY(no_fire,NO_FIRE);ENEMY(single_tap,SINGLE_TAP);ENEMY(double_tap,DOUBLE_TAP);
    ENEMY(spaced_tap,SPACED_TAP);ENEMY(charged_release,CHARGED_RELEASE);
    ENEMY(medium_release,MEDIUM_RELEASE);ENEMY(behind_double,BEHIND_DOUBLE);
#undef ENEMY
    puts("Natural source roller timelines, HP, braking, death and player damage match");
#define DAMAGE(n,N) damage(#n,damage_##n,sizeof(damage_##n)/sizeof(damage_##n[0]),damage_keys_##n,sizeof(damage_keys_##n)/sizeof(damage_keys_##n[0]),DAMAGE_##N##_SCENARIO)
    DAMAGE(hurt_hold_shoot,HURT_HOLD_SHOOT);DAMAGE(hurt_tap_shoot,HURT_TAP_SHOOT);
    DAMAGE(hurt_release_early,HURT_RELEASE_EARLY);DAMAGE(hurt_charge_medium,HURT_CHARGE_MEDIUM);
    DAMAGE(hurt_release_on_recovery,HURT_RELEASE_ON_RECOVERY);DAMAGE(hurt_press_after_recovery,HURT_PRESS_AFTER_RECOVERY);
    DAMAGE(hurt_run,HURT_RUN);DAMAGE(hurt_jump_held,HURT_JUMP_HELD);
    DAMAGE(hurt_jump_repress,HURT_JUMP_REPRESS);DAMAGE(contact_hold_charge,CONTACT_HOLD_CHARGE);
    DAMAGE(contact_release_charge,CONTACT_RELEASE_CHARGE);DAMAGE(contact_jump_shoot,CONTACT_JUMP_SHOOT);
#undef DAMAGE
    puts("1320 natural contact/recovery states, charge tiers and188 projectile samples match");
#define CAPACITY(n,N) do { combined("capacity_"#n,capacity_##n,sizeof(capacity_##n)/sizeof(capacity_##n[0]),capacity_keys_##n,sizeof(capacity_keys_##n)/sizeof(capacity_keys_##n[0]),CAPACITY_##N##_FRAMES,0,101); capacity(#n,capacity_state_##n,CAPACITY_##N##_FRAMES,capacity_keys_##n,sizeof(capacity_keys_##n)/sizeof(capacity_keys_##n[0])); } while(0)
    CAPACITY(rapid,RAPID);CAPACITY(three_medium,THREE_MEDIUM);CAPACITY(three_large,THREE_LARGE);
#undef CAPACITY
    sw_init(108);mmx_step(0);assert(!mmx.charge);
    for(unsigned i=0;i<3;++i)assert(mmx.shots[i].active && mmx.shots[i].age>0);
    puts("Original three-slot cap and charge/repress projectile cases pass");
#define WALL(n,S) damage("wall_"#n,wall_##n,sizeof(wall_##n)/sizeof(wall_##n[0]),wall_keys_##n,sizeof(wall_keys_##n)/sizeof(wall_keys_##n[0]),S);
    WALL_CASES(WALL)
#undef WALL
    wall_probes_check();wall_fraction(101,wall_fraction_0);wall_fraction(121,wall_fraction_64);wall_fraction(122,wall_fraction_192);
    puts("Original wall-fire movement, charge/births, trajectories, adjacent muzzles and normal slot fractions match");
#define TRAVERSE(n) do { traversal(#n,traverse_##n,sizeof(traverse_##n)/sizeof(traverse_##n[0]),traverse_keys_##n,sizeof(traverse_keys_##n)/sizeof(traverse_keys_##n[0])); combined(#n,traverse_shots_##n,sizeof(traverse_shots_##n)/sizeof(traverse_shots_##n[0]),traverse_keys_##n,sizeof(traverse_keys_##n)/sizeof(traverse_keys_##n[0]),sizeof(traverse_##n)/sizeof(traverse_##n[0]),0,0); } while(0)
    TRAVERSE(gap_jump);TRAVERSE(wall_recovery);
    edge_checks();
#undef TRAVERSE
    puts("Original long routes, charge, natural roller spawn/defeat and gap/wall recovery match after explicit source lag alignment");
    combat();culling();puts("72 original projectile states, charge boundaries, damage/death/retry, screen edges and complete traversal pass");
    return 0;
}
