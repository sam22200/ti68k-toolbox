#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yoshi.h"
#include "terrain.h"
#include "art.h"
#include "../../runtime/platform-sw/rt_sw.h"
#include "generated/jump_ref.h"
#include "generated/movement_ref.h"
#include "generated/terrain_ref.h"
#include "generated/actors_ref.h"
#include "generated/damage_ref.h"
#include "generated/coins.h"

static void print_item(u32 value) { printf("value: %lu\n",(unsigned long)value); }

/* Retain the straightforward renderer as an independent pixel oracle for
   the masked-blit optimisation, including scrolling and clipped actors. */
static void check_actor_pixels(void)
{
    static u8 fast[2][RT_PSIZE];
    u16 n;
    if (!yoshi.native || !art_ready()) return;
    memcpy(fast, sw_planes, sizeof(fast));
    /* sw_step increments the clock after drawing; compare the same pose. */
    --rt_frame;
    art_reference = 1;
    terrain_render_reference(yoshi.camx,yoshi.camy);
    items_render();
    art_hero();
    actors_render();
    damage_render();
    items_aim_render();
    art_reference = 0;
    if (yoshi.won) {
        draw_rect(13,30,134,22,C_WHITE); draw_text(25,38,"1-1 SLICE CLEAR",F_SMALL,C_BLACK);
    }
    ++rt_frame;
    /* Compare visible pixels; the reference clips to the LCD, ExtGraph to its physical planes. */
    for (n = 0; n < RT_H; ++n) {
        assert(!memcmp(fast[0] + n * RT_PBYTES, sw_planes[0] + n * RT_PBYTES, RT_W >> 3));
        assert(!memcmp(fast[1] + n * RT_PBYTES, sw_planes[1] + n * RT_PBYTES, RT_W >> 3));
    }
}

static void print_actors(void)
{
    const YoshiInteraction *i = &yoshi.action;
    const YoshiActor *a = i->actors;
    u16 n;
    printf("value: %lu\n", (unsigned long)(((u32)i->mouth << 24) | ((u32)i->length << 16) |
        ((u16)i->timer << 8) | i->up));
    printf("value: %lu\n", (unsigned long)(((u32)i->swallow << 16) | ((u16)i->slot << 8) | i->holding));
    printf("value: %lu\n", (unsigned long)(((u32)i->enabled << 24) | ((u32)i->facing << 16) |
        ((u16)i->blocked << 8) | i->eggs));
    for (n = 0; n < YA_COUNT; ++n, ++a) {
        printf("value: %lu\n", (unsigned long)(((u32)a->x << 16) | a->y));
        printf("value: %lu\n", (unsigned long)(((u32)(u16)a->vx << 16) | ((u16)a->sub << 8) | a->state));
        printf("value: %lu\n", (unsigned long)(((u32)a->home << 16) | (u16)a->vy));
        printf("value: %lu\n", (unsigned long)(((u32)a->defeated << 24) | ((u32)a->awake << 16) | ((u16)a->timer << 8) | a->shot));
    }
}

static void print_state(void)
{
    printf("value: %lu\n", (unsigned long)(((u32)yoshi.x << 16) | yoshi.x_sub));
    printf("value: %lu\n", (unsigned long)(((u32)(u16)yoshi.vx << 16) | (u16)yoshi.vy));
    printf("value: %lu\n", (unsigned long)(((u32)yoshi.y << 16) | yoshi.sub));
    printf("value: %lu\n", (unsigned long)(((u32)yoshi.flutter << 24) |
        ((u32)yoshi.phase_timer << 16) | ((u16)yoshi.cooldown << 8) | yoshi.jump));
    printf("value: %lu\n", (unsigned long)(((u16)yoshi.skid << 8) | yoshi.grounded));
    printf("value: %lu\n", (unsigned long)(((u32)yoshi.camx << 16) | yoshi.camy));
    printf("value: %lu\n", (unsigned long)(((u32)yoshi.angle << 24) |
        ((u32)yoshi.won << 16) | ((u16)yoshi.head_timer << 8) | yoshi.native));
}

static void print_damage(void)
{
    const YoshiBaby *b=&yoshi.baby;
    printf("value: %lu\n",(unsigned long)(((u32)b->x<<16)|b->y));
    printf("value: %lu\n",(unsigned long)(((u32)(u16)b->vx<<16)|(u16)b->vy));
    printf("value: %lu\n",(unsigned long)(((u32)b->remaining<<16)|b->tick));
    printf("value: %lu\n",(unsigned long)(((u32)b->mode<<24)|((u32)b->phase<<16)|((u16)b->age<<8)|b->invincible));
    printf("value: %lu\n",(unsigned long)(((u32)b->xsub<<24)|((u32)b->ysub<<16)|((u16)b->recoil<<8)|b->recharge));
    printf("value: %lu\n",(unsigned long)(((u32)b->leftward<<24)|((u32)b->enabled<<16)|((u16)b->hits<<8)|b->rescues));
}

static void test_items(void)
{
    unsigned f,n;
    YoshiEgg *e;
    YoshiPoint before[6];
    u32 checksum;
    sw_init(0); draw_clear(); rt_frame=0; art_coin(40,40); checksum=sw_checksum();
    draw_clear(); rt_frame=8; art_coin(40,40); assert(sw_checksum()!=checksum);
    sw_init(61); yoshi.items.idle=98; draw_clear(); art_hero(); checksum=sw_checksum();
    yoshi.items.idle=138; draw_clear(); art_hero(); assert(sw_checksum()!=checksum);
    sw_init(60);
    sw_step(0); assert(yoshi.items.count==1);
    for(f=0;f<20;++f) sw_step(0);
    assert(yoshi.items.count==1); /* Collection is idempotent. */
    for(n=0;n<YI_COINS;++n) items_collect(coin_xy[n][0],coin_xy[n][1],16,16);
    assert(yoshi.items.count==21);
    sw_step(K_ENTER); assert(yoshi.items.count==0);
    for(n=0;n<3;++n) assert(!yoshi.items.collected[n]);
    sw_init(61); sw_step(0); checksum=sw_checksum();
    for(f=0;f<320;++f) { sw_step(0); check_actor_pixels(); }
    assert(yoshi.items.idle==321 && checksum!=sw_checksum());
    /* Stationary eggs must not collapse into Yoshi after the history wraps. */
    for(n=1;n<6;++n) assert(yoshi.items.followers[n-1].x-yoshi.items.followers[n].x>=20);
    memcpy(before,yoshi.items.followers,sizeof(before));
    for(f=0;f<32;++f) sw_step(K_RIGHT|K_A);
    assert(yoshi.items.followers[0].x>before[0].x && yoshi.items.followers[0].y!=before[0].y);
    assert(!yoshi.items.idle);
    /* Aim, cancellation without consumption, launch and enemy hit. */
    sw_init(61); sw_step(K_C); assert(yoshi.items.aim && yoshi.action.eggs==6);
    sw_step(0); sw_step(K_DOWN); assert(!yoshi.items.aim && yoshi.action.eggs==6);
    /* One edge toggles the lock; facing changes cannot rotate a locked shot. */
    sw_step(0); sw_step(K_C);
    for(f=0;f<4;++f) sw_step(0);
    sw_step(K_D); assert(yoshi.items.locked && yoshi.items.phase==4);
    for(f=0;f<20;++f) { sw_step(K_D); check_actor_pixels(); }
    assert(yoshi.items.locked && yoshi.items.phase==4);
    sw_step(0); sw_step(K_D); assert(!yoshi.items.locked);
    sw_step(0); assert(yoshi.items.phase==5);
    sw_step(K_D); sw_step(K_LEFT); assert(yoshi.action.facing);
    sw_step(K_C); assert(yoshi.items.shots[0].vx>0 && !yoshi.items.locked);
    sw_init(61); sw_step(K_C);
    for(f=0;f<80;++f) { sw_step(0); check_actor_pixels(); }
    sw_step(K_D); sw_step(K_DOWN); assert(!yoshi.items.aim && !yoshi.items.locked);
    sw_init(61); sw_step(K_C); sw_step(0); sw_step(K_DOWN);
    sw_step(0); sw_step(K_C);
    for(f=0;f<4;++f) sw_step(0); /* Horizontal reticle. */
    assert(yoshi.items.phase==4);
    { YoshiActor *a=yoshi.action.actors;
      a->x=a->home=432; a->y=1904; a->awake=1; a->state=16; a->timer=100;
    }
    sw_step(K_C); assert(!yoshi.items.aim && yoshi.action.eggs==5 && yoshi.items.throwing);
    for(f=0;f<10 && yoshi.action.actors[0].state;++f) { sw_step(0); check_actor_pixels(); }
    assert(!yoshi.action.actors[0].state && !yoshi.baby.hits);
    /* Left-hand shot; cannot spend the last egg twice or fire an empty reserve. */
    sw_init(61); sw_step(K_LEFT); sw_step(K_C);
    for(f=0;f<4;++f) sw_step(0);
    sw_step(K_C); assert(yoshi.items.shots[0].vx<0);
    yoshi.action.eggs=0; sw_step(0); sw_step(K_C); assert(!yoshi.items.aim);
    /* A real 16px wall reflects a shot, with a three-contact lifetime. */
    sw_init(65); e=yoshi.items.shots;
    e->x=980; e->y=1640; e->vx=2272; e->life=180;
    for(f=0;f<15 && !e->bounces && e->life;++f) sw_step(0);
    assert(e->bounces && e->vx<0);
    /* Repeat real wall contacts; the third ends the projectile. */
    for(n=1;n<3;++n) {
        e->x=980; e->y=1640; e->xs=e->ys=0; e->vx=2272;
        for(f=0;f<15 && e->bounces==n && e->life;++f) sw_step(0);
        assert(e->bounces==n+1 && (n==1?e->life!=0:e->life==0));
    }
    sw_init(61); e=yoshi.items.shots;
    e->x=352; e->y=1912; e->vy=768; e->life=180;
    for(f=0;f<10 && !e->bounces;++f) sw_step(0);
    assert(e->bounces==1 && e->vy<0 && e->life);
    sw_init(0); e=yoshi.items.shots;
    e->x=280; e->y=1856; e->vx=2272; e->life=20;
    sw_step(0); assert(yoshi.items.count==1); /* Eggs can collect coins. */
    sw_init(61);
    for(n=0;n<6;++n) {
        e=yoshi.items.shots+n;
        e->x=32+(n<<5); e->y=1600; e->life=180;
    }
    sw_step(K_C); sw_step(0); sw_step(K_C);
    assert(yoshi.action.eggs==6 && yoshi.items.aim); /* No slot, no consumption. */
    /* Stomp kills without detaching Mario, unlike a lateral hit. */
    sw_init(63);
    for(f=0;f<8 && yoshi.action.actors[0].state;++f) sw_step(0);
    assert(!yoshi.action.actors[0].state && yoshi.vy<0 && !yoshi.baby.hits);
    /* Hold, turn and spit a captured actor, leaving the reserve empty. */
    sw_init(62);
    for(f=0;f<14;++f) { sw_step(f==0?K_B:0); check_actor_pixels(); }
    assert(yoshi.action.holding && !yoshi.action.eggs);
    sw_step(K_C); assert(yoshi.action.holding && !yoshi.items.aim);
    sw_step(K_LEFT); assert(yoshi.action.facing && yoshi.action.holding);
    sw_step(K_B);
    for(f=0;f<10;++f) sw_step(0);
    assert(!yoshi.action.holding && !yoshi.action.eggs && yoshi.action.actors[0].shot);
    assert(yoshi.action.actors[0].vx<0);
    sw_init(62);
    for(f=0;f<14;++f) sw_step(f==0?K_B:0);
    for(f=0;f<30;++f) { sw_step(K_DOWN); check_actor_pixels(); }
    assert(yoshi.action.eggs==1 && !yoshi.action.holding);
    sw_init(66);
    for(f=0;f<80;++f) { sw_step(K_RIGHT); check_actor_pixels(); }
    puts("Items: 21 unique coins, idle, spatial egg trail, aim/cancel/throw/hit/rebound, stomp and hold/turn/spit/conversion passed");
}

static void test_damage(void)
{
    unsigned f;
    u16 remaining;
    sw_init(54);
    sw_step(0);
    assert(yoshi.baby.mode==YB_PENDING && yoshi.baby.invincible==160);
    assert(yoshi.vx==-640 && yoshi.vy==-1162 && yoshi.baby.hits==1);
    for(f=0;f<24;++f) {
        sw_step(0);
        assert(yoshi.baby.x==baby_launch[f].x && yoshi.baby.y==baby_launch[f].y);
        assert(yoshi.baby.vx==baby_launch[f].vx && yoshi.baby.vy==baby_launch[f].vy);
        assert(yoshi.baby.xsub==baby_launch[f].xs && yoshi.baby.ysub==baby_launch[f].ys);
        assert(yoshi.baby.phase==baby_launch[f].phase);
        check_actor_pixels();
    }
    assert(yoshi.baby.hits==1);
    /* Ten real seconds, including clock wrap; do not equate PAL stars to Hz. */
    sw_init(55); rt_frame=13105; damage_reset(); damage_scenario(55);
    for(f=0;f<512;++f) { sw_step(0); assert(yoshi.baby.mode==YB_LOST); }
    assert(yoshi.baby.remaining==5);
    sw_step(0); assert(yoshi.baby.mode==YB_FAILED && !yoshi.baby.remaining);
    { YoshiMovement end=yoshi;
      for(f=0;f<20;++f) sw_step(K_RIGHT|K_A|K_B);
      assert(!memcmp(&end,&yoshi,sizeof(end)));
    }
    sw_step(K_ENTER);
    assert(yoshi.baby.mode==YB_ATTACHED && yoshi.baby.remaining==2560);
    assert(!yoshi.baby.hits && !yoshi.baby.rescues && !yoshi.won);
    /* Touch freezes the clock during the measured 32-update return. */
    sw_init(56); sw_step(0); assert(yoshi.baby.mode==YB_RETURN);
    remaining=yoshi.baby.remaining;
    for(f=0;f<32;++f) { sw_step(0); check_actor_pixels(); assert(yoshi.baby.remaining==remaining); }
    assert(yoshi.baby.mode==YB_ATTACHED && yoshi.baby.rescues==1);
    yoshi.baby.remaining=2000;
    for(f=0;f<12;++f) sw_step(0);
    assert(yoshi.baby.remaining==2026);
    /* Tongue rescue is deliberately out of body-contact range. */
    sw_init(56); yoshi.baby.x=532; yoshi.baby.y=1900; yoshi.baby.vx=yoshi.baby.vy=0;
    for(f=0;f<8 && yoshi.baby.mode==YB_LOST;++f) sw_step(K_B);
    assert(yoshi.baby.mode==YB_RETURN);
    /* A repeat hit preserves remaining time and suppresses tongue/capture. */
    sw_init(54); yoshi.baby.mode=YB_LOST; yoshi.baby.remaining=1500;
    yoshi.baby.x=350; yoshi.baby.y=1780; yoshi.baby.age=80; yoshi.baby.phase=11;
    yoshi.action.length=8; yoshi.action.mouth=1;
    sw_step(0);
    assert(yoshi.baby.invincible==128 && yoshi.baby.remaining==1500);
    assert(!yoshi.action.length && !yoshi.action.mouth);
    /* Falling on the actor is a stomp rather than a side hit. */
    sw_init(54); yoshi.y=1870; yoshi.grounded=0; yoshi.vy=1280;
    sw_step(0);
    assert(yoshi.baby.mode==YB_ATTACHED && !yoshi.action.actors[0].state);
    assert(yoshi.vy==-664);
    /* Raw PC state load keeps its saved reserve, not its old clock anchor. */
    sw_init(55);
    for(f=0;f<100;++f) sw_step(0);
    remaining=yoshi.baby.remaining;
    assert(sw_save_state("x/baby.state")==0);
    for(f=0;f<30;++f) sw_step(0);
    assert(sw_load_state("x/baby.state")==0);
    sw_step(0);
    assert(yoshi.baby.mode==YB_LOST && yoshi.baby.remaining==remaining-5);
    /* New-process load starts its own virtual clock at zero. */
    sw_init(0); assert(sw_load_state("x/baby.state")==0); sw_step(0);
    assert(yoshi.baby.mode==YB_LOST && yoshi.baby.remaining==remaining);
    sw_init(58);
    for(f=0;f<80;++f) { sw_step(K_RIGHT); check_actor_pixels(); }
    puts("Damage: 24 original PAL baby states; exact ten seconds/wrap, rescue/recharge, repeat hits, stomp, defeat/restart and dense pixels passed");
}

static void check_jump(const JumpSample *reference, unsigned hold)
{
    unsigned frame;
    sw_init(0);
    for (frame = 0; frame < 90; frame++) {
        sw_step(frame < hold ? K_A : 0);
        if (yoshi.y != reference[frame].y || yoshi.sub != reference[frame].sub ||
            yoshi.vy != reference[frame].vy) {
            printf("frame %u hold %u: got %u/%u/%d expected %u/%u/%d\n", frame, hold,
                   yoshi.y, yoshi.sub, yoshi.vy, reference[frame].y,
                   reference[frame].sub, reference[frame].vy);
            assert(0);
        }
    }
    assert(yoshi.grounded);
}

int main(int argc, char **argv)
{
    unsigned n, frame, total = 0;
    sw_init(0);
    assert(terrain_ready() && art_ready());
    if(argc==3 && !strcmp(argv[1],"--play-stomp")) {
        FILE *keys=fopen(argv[2],"w"); u32 previous=0xffffffffUL;
        assert(keys); sw_init(68);
        for(frame=0;frame<100;++frame) {
            s16 delta=(s16)yoshi.action.actors[0].x-(s16)yoshi.x;
            /* A small lead term brakes horizontal momentum above the actor. */
            s16 lead=delta-(yoshi.vx>>5);
            u32 buttons=frame<20?K_A:0;
            if(yoshi.action.actors[0].state) {
                if(lead>2)buttons|=K_RIGHT;
                else if(lead<-2)buttons|=K_LEFT;
            }
            if(buttons!=previous) {
                fprintf(keys,"%u%s%s%s\n",frame,buttons&K_A?" A":"",buttons&K_RIGHT?" RIGHT":"",buttons&K_LEFT?" LEFT":"");
                previous=buttons;
            }
            sw_step(buttons);
        }
        fprintf(keys,"100\n");fclose(keys);
        printf("Jump/stomp replay: actor state%u hits%u x%u y%u\n",yoshi.action.actors[0].state,yoshi.baby.hits,yoshi.x,yoshi.y);
        assert(!yoshi.action.actors[0].state && !yoshi.baby.hits);
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--play-actors")) {
        FILE *keys = fopen(argv[2], "w");
        u32 previous = 0xffffffffUL;
        assert(keys);
        sw_init(0);
        for (frame = 0; frame < 1800 && !yoshi.won && yoshi.baby.mode!=YB_FAILED; ++frame) {
            const YoshiActor *target = yoshi.action.actors + (yoshi.action.eggs ? 1 : 0);
            u32 buttons = K_RIGHT | (frame % 100 != 99 ? K_A : 0);
            if (yoshi.baby.mode==YB_LOST) {
                s16 delta=(s16)yoshi.baby.x-(s16)yoshi.x;
                buttons=delta>8?K_RIGHT:delta<-8?K_LEFT:0;
                if (yoshi.baby.y+8<yoshi.y && frame%80!=79) buttons|=K_A;
                if (delta>-56 && delta<56) buttons|=K_B|K_UP;
            } else if (yoshi.action.holding) buttons = K_DOWN;
            else if (yoshi.action.mouth) buttons = K_B;
            else if (yoshi.action.eggs < 2 && target->state == 16 &&
                     (s16)target->x - (s16)yoshi.x > 0 &&
                     (s16)target->x - (s16)yoshi.x <= 52) {
                buttons = yoshi.grounded && !(rt_keys & K_B) ? K_B : 0;
            }
            if (buttons != previous) {
                fprintf(keys, "%u%s%s%s%s%s%s\n", frame, buttons & K_RIGHT ? " RIGHT" : "",
                    buttons & K_B ? " B" : "", buttons & K_DOWN ? " DOWN" : "",
                    buttons & K_A ? " A" : "", buttons & K_LEFT ? " LEFT" : "",
                    buttons & K_UP ? " UP" : "");
                previous = buttons;
            }
            sw_step(buttons);
        }
        fprintf(keys, "%u\n", frame);
        fclose(keys);
        printf("Actor traversal: %u frames, X%u/Y%u, %u eggs, won=%u, hits=%u/rescues=%u\n", frame,
            yoshi.x,yoshi.y,yoshi.action.eggs,yoshi.won,yoshi.baby.hits,yoshi.baby.rescues);
        assert(yoshi.won && yoshi.action.eggs >= 2);
        return 0;
    }
    if (argc == 5 && (!strcmp(argv[1], "--trace") || !strcmp(argv[1], "--actortrace") || !strcmp(argv[1], "--damagetrace") || !strcmp(argv[1],"--itemtrace"))) {
        static SwScript script;
        unsigned frames = (unsigned)atoi(argv[3]);
        assert(sw_load_script(&script, argv[2]) == 0);
        sw_init((u16)atoi(argv[4]));
        for (frame = 0; frame < frames; ++frame) {
            sw_step(sw_script_keys(&script, frame));
            if(!strcmp(argv[1],"--itemtrace")) items_trace(print_item);
            print_state();
            if (strcmp(argv[1], "--trace")) print_actors();
            if (!strcmp(argv[1], "--damagetrace") || !strcmp(argv[1],"--itemtrace")) print_damage();
        }
        printf("value: %u\n", frames);
        return 0;
    }
    test_items();
    test_damage();
    for (n = 0; n < sizeof(actor_cases) / sizeof(actor_cases[0]); ++n) {
        const ActorCase *c = actor_cases + n;
        static SwScript script;
        char path[256];
        sprintf(path, "../../sources/yoshi_snes/actors/%s.txt", c->name);
        assert(sw_load_script(&script, path) == 0);
        sw_init(c->scenario);
        for (frame = 0; frame < c->count; ++frame) {
            const ActorSample *r = c->samples + frame;
            const YoshiInteraction *i = &yoshi.action;
            sw_step(sw_script_keys(&script, frame));
            check_actor_pixels();
            if (i->mouth != r->mouth || i->length != r->length || i->timer != r->timer ||
                i->up != r->up || i->slot != r->slot || i->holding != r->holding ||
                i->swallow != r->swallow || i->eggs != r->eggs) {
                printf("%s frame %u: mouth/length/timer/up %u/%u/%u/%u vs %u/%u/%u/%u; "
                       "slot/holding/swallow/eggs %u/%u/%u/%u vs %u/%u/%u/%u\n", c->name, frame,
                       i->mouth,i->length,i->timer,i->up,r->mouth,r->length,r->timer,r->up,
                       i->slot,i->holding,i->swallow,i->eggs,r->slot,r->holding,r->swallow,r->eggs);
                fflush(stdout); assert(0);
            }
        }
    }
    printf("Actors: %u original PAL tongue/ingestion timelines match\n", n);
    {
        static SwScript script;
        assert(sw_load_script(&script, "keys/actors.txt") == 0);
        sw_init(0);
        for (frame = 0; frame < 1300; ++frame) {
            sw_step(sw_script_keys(&script, frame));
            check_actor_pixels();
        }
        assert(yoshi.won && yoshi.action.eggs == 2 && yoshi.baby.hits==2 && yoshi.baby.rescues==2);
    }
    sw_init(53);
    for (frame = 0; frame < 80; ++frame) { sw_step(K_RIGHT); check_actor_pixels(); }
    puts("ROM scenery and masked sprites/HUD: original actions, damage route and dense scenes equal independent pixel renderer");
    /* Capacity/restart checks use one real capture, then an injected full reserve. */
    sw_init(52);
    for (frame = 0; frame < 4; ++frame) sw_step(frame == 0 ? K_B : 0);
    assert(yoshi.action.holding && yoshi.action.actors[0].state == 8);
    yoshi.action.eggs = 6;
    for (frame = 0; frame < 30; ++frame) sw_step(K_DOWN);
    assert(yoshi.action.eggs == 6 && !yoshi.action.holding && !yoshi.action.actors[0].state);
    sw_step(K_ENTER);
    assert(!yoshi.action.eggs && !yoshi.action.slot && !yoshi.action.holding);
    assert(yoshi.action.actors[0].state == 16 && !yoshi.action.actors[0].awake);
    sw_init(50);
    yoshi.x = 988; yoshi.y = 1634; yoshi.grounded = 0;
    sw_step(K_B);
    assert(yoshi.action.blocked && yoshi.action.mouth == 2);
    for (frame = 0; frame < 20; ++frame) sw_step(0);
    assert(!yoshi.action.mouth && !yoshi.action.length);
    for (n = 0; n < sizeof(drops) / sizeof(drops[0]); ++n) {
        const DropCase *c = &drops[n];
        sw_init(20 + n);
        assert(terrain_ready());
        assert(yoshi.x == c->x && yoshi.y == c->y);
        for (frame = 0; frame < c->count; ++frame) {
            const DropSample *r = &c->samples[frame];
            sw_step(0);
            if (yoshi.y != r->y || yoshi.sub != r->sub || yoshi.vy != r->vy ||
                yoshi.jump != r->jump || yoshi.angle != r->angle) {
                printf("drop %u frame %u got %u/%u/%d/%u/%u expected %u/%u/%d/%u/%u\n",
                       c->x, frame, yoshi.y, yoshi.sub, yoshi.vy, yoshi.jump, yoshi.angle,
                       r->y, r->sub, r->vy, r->jump, r->angle);
                fflush(stdout); assert(0);
            }
        }
        assert(yoshi.grounded);
    }
    printf("Terrain: %u original PAL drop timelines match exactly\n", n);
    for (n = 0; n < 2; ++n) {
        const DropSample *reference = n ? interaction_ceiling : interaction_one_way;
        unsigned count = n ? sizeof(interaction_ceiling) / sizeof(DropSample) :
                             sizeof(interaction_one_way) / sizeof(DropSample);
        sw_init(12 + n);
        for (frame = 0; frame < count; ++frame) {
            const DropSample *r = reference + frame;
            sw_step(frame < 35 ? K_A : 0);
            if (yoshi.y != r->y || yoshi.sub != r->sub || yoshi.vy != r->vy ||
                yoshi.jump != r->jump || yoshi.flutter != r->flutter ||
                yoshi.phase_timer != r->timer || yoshi.cooldown != r->cooldown ||
                yoshi.head_timer != r->head) {
                printf("interaction %u frame %u: Y %u/%u sub %u/%u VY %d/%d jump %u/%u "
                       "flutter %u/%u timer %u/%u cooldown %u/%u head %u/%u\n", n, frame,
                       yoshi.y, r->y, yoshi.sub, r->sub, yoshi.vy, r->vy, yoshi.jump, r->jump,
                       yoshi.flutter, r->flutter, yoshi.phase_timer, r->timer,
                       yoshi.cooldown, r->cooldown, yoshi.head_timer, r->head);
                fflush(stdout); assert(0);
            }
        }
        assert(yoshi.grounded);
    }
    puts("Original PAL: one-way ascent/landing and ceiling-impact timelines match exactly");
    sw_init(0);
    yoshi.x = 992; yoshi.y = 1634;
    yoshi.grounded = 0; yoshi.jump = 6; yoshi.vx = 960;
    sw_step(K_RIGHT);
    assert(yoshi.x == 992); /* Actual isolated block's left wall at X1008. */
    sw_init(0);
    yoshi.y = YT_BOTTOM - 1; yoshi.grounded = 0; yoshi.jump = 8; yoshi.vy = 1280;
    sw_step(0);
    assert(yoshi.x == 119 && yoshi.y == 1904 && yoshi.grounded);
    {
        static SwScript script;
        u16 previous = 119, landed_slopes = 0;
        assert(sw_load_script(&script, "keys/terrain.txt") == 0);
        sw_init(59); /* Controlled terrain-only route; full gameplay is checked above. */
        for (frame = 0; frame < 600; ++frame) {
            sw_step(sw_script_keys(&script, frame));
            assert(yoshi.x >= previous && yoshi.y < YT_BOTTOM);
            assert(yoshi.camx <= 480 && yoshi.camy <= 156);
            if (yoshi.grounded && yoshi.angle) ++landed_slopes;
            previous = yoshi.x;
        }
        assert(yoshi.won && yoshi.x == YT_END && yoshi.grounded && landed_slopes);
        { YoshiMovement end = yoshi;
          sw_step(K_RIGHT | K_A);
          assert(!memcmp(&end, &yoshi, sizeof(end)));
        }
        sw_step(K_ENTER);
        assert(!yoshi.won && yoshi.x == 119 && yoshi.y == 1904);
        assert(sw_step(K_ESC) == 0);
        puts("Native terrain: five-screen traversal, camera, frozen finish and restart passed");
    }
    for (n = 0; n < sizeof(movement_cases) / sizeof(movement_cases[0]); ++n) {
        const MovementCase *c = &movement_cases[n];
        sw_init(10);
        for (frame = 0; frame < c->frames; ++frame) {
            const MovementSample *r = &c->samples[frame];
            sw_step(c->keys[frame]);
            if (yoshi.x != r->x || yoshi.x_sub != r->x_sub || yoshi.y != r->y ||
                yoshi.sub != r->sub || yoshi.vx != r->vx || yoshi.vy != r->vy ||
                yoshi.flutter != r->flutter || yoshi.phase_timer != r->timer ||
                yoshi.cooldown != r->cooldown || yoshi.jump != r->jump ||
                yoshi.skid != r->skid || yoshi.grounded != (r->jump == 0)) {
                printf("%s frame %u: got %u/%u %u/%u vx/vy %d/%d f/t/c/j/s %u/%u/%u/%u/%u; "
                       "expected %u/%u %u/%u %d/%d %u/%u/%u/%u/%u\n", c->name, frame,
                       yoshi.x, yoshi.x_sub, yoshi.y, yoshi.sub, yoshi.vx, yoshi.vy,
                       yoshi.flutter, yoshi.phase_timer, yoshi.cooldown, yoshi.jump, yoshi.skid,
                       r->x, r->x_sub, r->y, r->sub, r->vx, r->vy,
                       r->flutter, r->timer, r->cooldown, r->jump, r->skid);
                fflush(stdout);
                assert(0);
            }
            ++total;
        }
    }
    check_jump(jump_tap, 1);
    check_jump(jump_hold, 35);
    sw_init(11);
    for (frame = 0; frame < 400; ++frame) sw_step(K_RIGHT);
    assert(yoshi.x == 1264 && yoshi.y == 1904);
    for (frame = 0; frame < 800; ++frame) sw_step(K_LEFT);
    assert(yoshi.x == 16 && yoshi.y == 1904);
    sw_init(0);
    sw_step(K_A);
    sw_step(0);
    sw_step(K_ENTER);
    assert(yoshi.y == 1904 && yoshi.vy == 0 && yoshi.grounded);
    assert(sw_step(K_ESC) == 0);
    printf("Yoshi movement: %u controlled PAL states + 180 ordinary jump frames match; restart/quit passed\n", total);
    return 0;
}
