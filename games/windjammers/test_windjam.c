/* Headless mechanics tests against locally generated ORIGINAL execution fixtures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "windjam.h"

typedef struct { u16 port; s16 dx, dy; u16 flipped, expected; } ContactRef;
typedef struct { u16 port, frame, keys; s32 x, y, vx, vy; } MoveRef;
typedef struct { u16 port, keys, release, end, terminal; s32 x1, y1, x2, y2; } ShotStart;
typedef struct { u16 id, frame; s32 x, y, vx, vy; u16 mode, action; } ShotRef;
typedef struct { u16 profile; s32 x, y, after_x, after_y; } WallRef;
typedef struct { u16 winner, frame, award; s32 x, y, y1, y2, after_x, after_y; } GoalRef;
#include "generated/reference.h"
typedef struct { u16 owner, keys; s32 x1, y1, x2, y2; u16 hold1, power1, bonus1, hold2, power2, bonus2; } ActionStart;
typedef struct { u16 id, frame; s32 x1, y1, vx1, vy1; u16 action1;
    s32 x2, y2, vx2, vy2; u16 action2; s32 dx, dy; u16 mode; } ActionRef;
#include "generated/actions.h"
typedef struct { u16 owner, delay, keys;
    s32 x1, y1; u16 hold1, power1, bonus1;
    s32 x2, y2; u16 hold2, power2, bonus2; } HoldStart;
typedef struct { u16 id, frame;
    s32 x1, y1, vx1, vy1; u16 action1, hold1, power1, bonus1;
    s32 x2, y2, vx2, vy2; u16 action2, hold2, power2, bonus2;
    s32 dx, dy; u16 mode; s32 dvx, dvy; } HoldRef;
#include "generated/holds.h"


typedef struct { u16 receiver, delay, held, keys; } TimingStart;
typedef struct { u16 id, frame;
    s32 x1, y1, vx1, vy1; u16 action1, hold1, power1, bonus1, charge1, charged1;
    s32 x2, y2, vx2, vy2; u16 action2, hold2, power2, bonus2, charge2, charged2;
    s32 dx, dy; u16 mode; s32 dvx, dvy, z, vz; } TimingRef;
typedef struct { u16 id, frame, mode; } TimingOutcome;
#include "generated/timing.h"
typedef struct { u16 owner, mode; s32 x1,y1; u16 hold1,power1,bonus1;
    s32 x2,y2; u16 hold2,power2,bonus2; s32 dx,dy; u16 profile,jitter; u32 input; u16 steps; } AdvancedStart;
typedef struct { u16 id,frame; s32 x1,y1,vx1,vy1; u16 action1,hold1,power1,bonus1,charge1,charged1;
    s32 x2,y2,vx2,vy2; u16 action2,hold2,power2,bonus2,charge2,charged2; s32 dx,dy; u16 mode; s32 dvx,dvy,z,vz; } AdvancedRef;
typedef struct { u16 p1,p2; } AdvancedInput;
#include "generated/advanced.h"
typedef struct { u16 owner; s32 x1,y1; u16 hold1,power1,bonus1;
    s32 x2,y2; u16 hold2,power2,bonus2; s32 dx,dy; u32 input; u16 steps;
    u8 h0,h1,h2,h3,h4,h5,h6,h7,h8,j0,j1,j2,j3,j4,j5,j6,j7,j8; } CurveStart;
typedef struct { u16 id,frame; s32 x1,y1,vx1,vy1; u16 action1,hold1,power1,bonus1;
    s32 x2,y2,vx2,vy2; u16 action2,hold2,power2,bonus2; s32 dx,dy; u16 mode;
    s32 dvx,dvy,z,vz; u32 angle; u16 speed; s32 turn; } CurveRef;
#include "generated/curves.h"
typedef struct { u16 port, keys, held; s32 x, y; } DashStart;
typedef struct { u16 id, frame; s32 x, y, vx, vy; } DashRef;
#include "generated/dash.h"

static u16 failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)
#define COUNT(a) (sizeof(a)/sizeof((a)[0]))

static u16 core_keys(u16 mask)
{
    return ((mask & 16) ? K_UP : 0) | ((mask & 32) ? K_DOWN : 0) |
           ((mask & 64) ? K_LEFT : 0) | ((mask & 128) ? K_RIGHT : 0) | ((mask & 1) ? K_A : 0) | ((mask & 256) ? K_B : 0);
}

static void original_dash(void)
{
    u16 i;
    for (i = 0; i < COUNT(dash_ref); i++) {
        const DashRef *r = &dash_ref[i];
        const DashStart *s = &dash_start[r->id];
        WjPlayer *p;
        u16 pad = r->frame >= 4 && r->frame < 4 + s->held ? core_keys(s->keys) | K_A : 0;
        if (!r->frame) {
            wj_reset(s->port ^ 1);
            st.player[s->port].x = s->x; st.player[s->port].y = s->y;
        }
        wj_logic(s->port ? 0 : pad, s->port ? pad : 0);
        p = &st.player[s->port];
        if (p->x != r->x || p->y != r->y || p->vx != r->vx || p->vy != r->vy) {
            printf("dash %u frame %u: xy %ld/%ld expected %ld/%ld v %ld/%ld expected %ld/%ld\n",
                r->id, r->frame, (long)p->x, (long)p->y, (long)r->x, (long)r->y,
                (long)p->vx, (long)p->vy, (long)r->vx, (long)r->vy);
            failures++; return;
        }
    }
    printf("original dash: %lu no-write trials, %lu motion steps, both sides/eight directions\n",
        (unsigned long)COUNT(dash_start), (unsigned long)COUNT(dash_ref));
}

static void guided_charge_and_dash_replay(void)
{
    u16 port, frame;
    for (port = 0; port < 2; port++) {
        u16 prepared = 0, lifted = 0, charged = 0, launched = 0;
        wj_timing_door(port);
        for (frame = 0; frame < 180; frame++) {
            u16 pad = 0;
            if (!prepared && wj_prepare_hint(port)) { pad = K_A; prepared = 1; }
            if (st.disc.mode == WJ_HELD && st.disc.owner == port && st.player[port].charged) pad = K_A;
            wj_logic(port ? 0 : pad, port ? pad : 0);
            lifted |= st.disc.mode == WJ_LIFT;
            charged |= st.player[port].charged;
            launched |= st.disc.mode == (port ? WJ_YOO : WJ_MITA);
        }
        CHECK(prepared && lifted && charged && launched);
    }
    for (port = 0; port < 2; port++) {
        Windjam saved;
        u16 hashes[100], planes[100];
        sw_init(port ? 28 : 27);
        sw_step((port ? K_LEFT : K_RIGHT) | K_A);
        CHECK(st.player[port].dash_age && !st.player[port].ready);
        saved = st;
        for (frame = 0; frame < 100; frame++) {
            sw_step(0); hashes[frame] = wj_hash(); planes[frame] = sw_checksum();
        }
        st = saved;
        for (frame = 0; frame < 100; frame++) {
            sw_step(0); CHECK(hashes[frame] == wj_hash() && planes[frame] == sw_checksum());
        }
        CHECK(!st.player[port].dash_age);
    }
    /* A dash interrupted by an ordinary front catch returns to possession. */
    wj_timing_door(0); st.player[0].dash_age = 3; st.player[0].vx = 65536L;
    st.disc.pending = 4; st.disc.defender = 0;
    wj_logic(0, 0);
    CHECK(!st.player[0].dash_age && st.disc.mode == WJ_HELD && st.player[0].catching);
    printf("guided charge: hint -> timed lift -> full charge -> special, both players; dash replay/recovery/capture passed\n");
}

static void original_curves(void)
{
    u16 trial; u32 index = 0;
    for (trial = 0; trial < COUNT(curve_start); trial++) {
        const CurveStart *s = &curve_start[trial];
        u16 frame;
        wj_reset(s->owner);
        st.player[0].x=s->x1; st.player[0].y=s->y1; st.player[0].hold=s->hold1; st.player[0].power=s->power1; st.player[0].bonus=s->bonus1;
        st.player[1].x=s->x2; st.player[1].y=s->y2; st.player[1].hold=s->hold2; st.player[1].power=s->power2; st.player[1].bonus=s->bonus2;
        memcpy(st.player[0].history, &s->h0, 9); memcpy(st.player[1].history, &s->j0, 9);
        st.disc.x=s->dx; st.disc.y=s->dy;
        for (frame=0; frame<s->steps; frame++,index++) {
            const CurveRef *r=&curve_ref[index];
            const AdvancedInput *in=&curve_input[s->input+frame];
            u16 port;
            wj_logic(core_keys(in->p1),core_keys(in->p2));
            for (port=0;port<2;port++) {
                WjPlayer *p=&st.player[port];
                if (p->x!=(port?r->x2:r->x1) || p->y!=(port?r->y2:r->y1) ||
                    p->vx!=(port?r->vx2:r->vx1) || p->vy!=(port?r->vy2:r->vy1) ||
                    wj_action(port)!=(port?r->action2:r->action1) ||
                    ((p->throwing || p->catching || (st.disc.mode==WJ_HELD && st.disc.owner==port)) && p->hold!=(port?r->hold2:r->hold1)) ||
                    p->power!=(port?r->power2:r->power1) || p->bonus!=(port?r->bonus2:r->bonus1)) {
                    printf("curve %u frame %u player %u: xy %ld/%ld expected %ld/%ld v %ld/%ld expected %ld/%ld action %u/%u hold %u/%u power %u/%u\n",trial,frame,port,(long)p->x,(long)p->y,(long)(port?r->x2:r->x1),(long)(port?r->y2:r->y1),(long)p->vx,(long)p->vy,(long)(port?r->vx2:r->vx1),(long)(port?r->vy2:r->vy1),wj_action(port),port?r->action2:r->action1,p->hold,port?r->hold2:r->hold1,p->power,port?r->power2:r->power1);
                    failures++; goto next;
                }
            }
            if (st.disc.x!=r->dx || st.disc.y!=r->dy || st.disc.mode!=r->mode || wj_vx()!=r->dvx || wj_vy()!=r->dvy ||
                (st.disc.dynamic && (st.disc.mode==4 || st.disc.mode==6 || st.disc.mode==8) &&
                 (st.disc.angle!=r->angle || st.disc.speed!=r->speed || st.disc.turn!=r->turn))) {
                printf("curve %u frame %u disc: xy %ld/%ld expected %ld/%ld mode %u/%u v %ld/%ld expected %ld/%ld angle %lu/%lu speed %u/%u turn %ld/%ld\n",trial,frame,(long)st.disc.x,(long)st.disc.y,(long)r->dx,(long)r->dy,st.disc.mode,r->mode,(long)wj_vx(),(long)wj_vy(),(long)r->dvx,(long)r->dvy,(unsigned long)st.disc.angle,(unsigned long)r->angle,st.disc.speed,r->speed,(long)st.disc.turn,(long)r->turn);
                failures++; goto next;
            }
        }
        continue;
next:   index+=s->steps-frame;
    }
    printf("original curves: %lu no-write trials, %lu bounded execution steps\n",(unsigned long)COUNT(curve_start),(unsigned long)COUNT(curve_ref));
}

static void original_movement(void)
{
    u16 i;
    for (i = 0; i < COUNT(movement_ref); i++) {
        const MoveRef *r = &movement_ref[i];
        WjPlayer *p;
        if (!r->frame) wj_reset(r->port ^ 1);
        /* Isolated neutral integration; these fixtures sample one actor. */
        wj_walk(r->port, r->keys);
        p = &st.player[r->port];
        CHECK(p->x == r->x && p->y == r->y && p->vx == r->vx && p->vy == r->vy);
    }
    printf("original movement: %lu frames, both players/eight directions\n", (unsigned long)COUNT(movement_ref));
}

static void original_advanced(void)
{
    u16 trial;
    u32 index = 0;
    for (trial = 0; trial < COUNT(advanced_start); trial++) {
        const AdvancedStart *s = &advanced_start[trial];
        u16 frame;
        wj_reset(s->owner); st.disc.mode = s->mode; st.disc.profile = s->profile; st.lob_seed = s->jitter;
        st.player[0].x=s->x1; st.player[0].y=s->y1; st.player[0].hold=s->hold1; st.player[0].power=s->power1; st.player[0].bonus=s->bonus1;
        st.player[1].x=s->x2; st.player[1].y=s->y2; st.player[1].hold=s->hold2; st.player[1].power=s->power2; st.player[1].bonus=s->bonus2;
        st.disc.x=s->dx; st.disc.y=s->dy;
        for (frame = 0; frame < s->steps; frame++, index++) {
            const AdvancedRef *r = &advanced_ref[index];
            const AdvancedInput *input = &advanced_input[s->input + frame];
            u16 port;
            wj_logic(core_keys(input->p1), core_keys(input->p2));
            for (port = 0; port < 2; port++) {
                WjPlayer *p = &st.player[port];
                if (p->x != (port?r->x2:r->x1) || p->y != (port?r->y2:r->y1) ||
                    p->vx != (port?r->vx2:r->vx1) || p->vy != (port?r->vy2:r->vy1) ||
                    (wj_action(port) != (port?r->action2:r->action1) &&
                     !(p->strong == WJ_SUPERLOB && ((port?r->action2:r->action1) == 0x100c || (port?r->action2:r->action1) == 0x1010))) ||
                    ((port?r->action2:r->action1) != 0 && (port?r->action2:r->action1) != 0x400 && wj_hold_value(port) != (port?r->hold2:r->hold1)) ||
                    p->power != (port?r->power2:r->power1) || p->bonus != (port?r->bonus2:r->bonus1) ||
                    ((p->charging || p->ready || (st.disc.mode == WJ_HELD && st.disc.owner == port)) &&
                     (p->charge != (port?r->charge2:r->charge1) || p->charged != (port?r->charged2:r->charged1)))) {
                    printf("advanced %u frame %u player %u: xy %ld/%ld != %ld/%ld v %ld/%ld != %ld/%ld action %u/%u hold %u/%u power %u/%u bonus %u/%u\n",trial,frame,port,
                        (long)p->x,(long)p->y,(long)(port?r->x2:r->x1),(long)(port?r->y2:r->y1),(long)p->vx,(long)p->vy,(long)(port?r->vx2:r->vx1),(long)(port?r->vy2:r->vy1),wj_action(port),port?r->action2:r->action1,wj_hold_value(port),port?r->hold2:r->hold1,p->power,port?r->power2:r->power1,p->bonus,port?r->bonus2:r->bonus1);
                    failures++; goto next_trial;
                }
            }
            if (st.disc.x != r->dx || st.disc.y != r->dy || st.disc.mode != r->mode ||
                wj_vx() != r->dvx || wj_vy() != r->dvy || st.disc.z != r->z || st.disc.vz != r->vz) {
                printf("advanced %u frame %u disc: xy %ld/%ld != %ld/%ld mode %u/%u v %ld/%ld != %ld/%ld z %ld/%ld != %ld/%ld\n",trial,frame,
                    (long)st.disc.x,(long)st.disc.y,(long)r->dx,(long)r->dy,st.disc.mode,r->mode,(long)wj_vx(),(long)wj_vy(),(long)r->dvx,(long)r->dvy,(long)st.disc.z,(long)st.disc.vz,(long)r->z,(long)r->vz);
                failures++; goto next_trial;
            }
        }
        continue;
next_trial:
        index += s->steps - frame;
    }
    printf("original advanced: %lu cases, %lu bounded execution steps, lob targets conditioned on observed jitter\n",(unsigned long)COUNT(advanced_start),(unsigned long)COUNT(advanced_ref));
}

static void original_shots(void)
{
    u16 i;
    for (i = 0; i < COUNT(shot_ref); i++) {
        const ShotRef *r = &shot_ref[i];
        const ShotStart *s = &shot_start[r->id];
        u16 keys = r->frame >= 4 && r->frame < 6 ? core_keys(s->keys) : 0;
        if (!r->frame) {
            wj_reset(s->port);
            st.player[0].x = s->x1; st.player[0].y = s->y1;
            st.player[1].x = s->x2; st.player[1].y = s->y2;
            st.disc.x = st.player[s->port].x; st.disc.y = st.player[s->port].y;
        }
        wj_logic(s->port ? 0 : keys, s->port ? keys : 0);
        if (st.disc.x != r->x || st.disc.y != r->y || st.disc.mode != r->mode ||
            wj_action(s->port) != r->action || (r->mode == WJ_FLIGHT && (wj_vx() != r->vx || wj_vy() != r->vy))) {
            printf("shot %u frame %u: xy %ld/%ld expected %ld/%ld mode %u/%u action %u/%u\n",
                   r->id, r->frame, (long)st.disc.x, (long)st.disc.y, (long)r->x, (long)r->y,
                   st.disc.mode, r->mode, wj_action(s->port), r->action);
            failures++; return;
        }
        if (r->frame + 1 == s->end && s->terminal == WJ_HELD) {
            wj_logic(0, 0);
            CHECK(st.disc.mode == WJ_HELD && st.disc.owner == (s->port ^ 1));
        }
    }
    printf("original launches/flight: %lu frames, six shots, through first contact\n", (unsigned long)COUNT(shot_ref));
}

static void contacts_and_goals(void)
{
    u16 i;
    for (i = 0; i < COUNT(contact_ref); i++) {
        const ContactRef *r = &contact_ref[i];
        CHECK(wj_contact(r->port, r->dx, r->dy, r->flipped) == r->expected);
    }
    CHECK(wj_goal_zone((119L << 16) + 65535) == 3);
    CHECK(wj_goal_zone(120L << 16) == 5);
    CHECK(wj_goal_zone((167L << 16) + 65535) == 5);
    CHECK(wj_goal_zone(168L << 16) == 3);
    for (i = 0; i < COUNT(wall_ref); i++) {
        const WallRef *r = &wall_ref[i];
        wj_reset(0); st.disc.mode = WJ_FLIGHT; st.disc.profile = r->profile;
        st.disc.x = r->x; st.disc.y = r->y;
        wj_logic(0, 0);
        CHECK(st.disc.x == r->after_x && st.disc.y == r->after_y);
    }
    for (i = 0; i < COUNT(goal_ref); i++) {
        const GoalRef *r = &goal_ref[i];
        u16 f;
        wj_reset(0); st.disc.mode = WJ_FLIGHT; st.disc.profile = r->winner ? 3 : 0;
        st.disc.x = r->x; st.disc.y = r->y;
        st.player[0].y = r->y1; st.player[1].y = r->y2;
        for (f = 0; f <= r->frame; f++) wj_logic(0, 0);
        CHECK(st.points[r->winner] == r->award && st.points[r->winner ^ 1] == 0);
        CHECK(st.disc.x == r->after_x && st.disc.y == r->after_y && st.disc.mode == WJ_GOAL);
    }
    /* No input injection in this rally beyond choosing the scenario/ordinary keys. */
    wj_reset(1); st.player[0].y = 88L << 16;
    for (i = 0; i <= 60; i++) wj_logic(0, i >= 4 && i < 6 ? K_A : 0);
    CHECK(st.points[1] == 5 && st.disc.mode == WJ_GOAL && st.disc.x == 1003520L);
    for (i = 0; i < 90; i++) wj_logic(0, 0);
    CHECK(st.points[1] == 5 && st.disc.mode == WJ_HELD && st.disc.owner == 0);
    st.player[1].y = 80L << 16;
    for (i = 0; i < 100 && st.disc.mode != WJ_GOAL; i++) wj_logic(i < 2 ? K_A : 0, 0);
    CHECK(st.points[0] == 5 && st.points[1] == 5 && st.serve_to == 1);
    for (i = 0; i < 90; i++) wj_logic(0, 0);
    CHECK(st.disc.mode == WJ_HELD && st.disc.owner == 1 && st.points[0] == 5);
    printf("original boundaries: %lu contacts including angles, %lu walls, %lu goals; native loser serve passed\n",
           (unsigned long)COUNT(contact_ref), (unsigned long)COUNT(wall_ref), (unsigned long)COUNT(goal_ref));
}

static void original_actions(void)
{
    u16 i;
    for (i = 0; i < COUNT(action_ref); i++) {
        const ActionRef *r = &action_ref[i];
        const ActionStart *s = &action_start[r->id];
        u16 keys = r->frame >= 4 && r->frame < 6 ? core_keys(s->keys) : 0;
        if (!r->frame) {
            wj_reset(s->owner);
            st.player[0].x = s->x1; st.player[0].y = s->y1;
            st.player[1].x = s->x2; st.player[1].y = s->y2;
            st.player[0].hold = s->hold1; st.player[0].power = s->power1; st.player[0].bonus = s->bonus1;
            st.player[1].hold = s->hold2; st.player[1].power = s->power2; st.player[1].bonus = s->bonus2;
        }
        wj_logic(s->owner ? 0 : keys, s->owner ? keys : 0);
        if (st.player[0].x != r->x1 || st.player[0].y != r->y1 ||
            st.player[0].vx != r->vx1 || st.player[0].vy != r->vy1 || wj_action(0) != r->action1 ||
            st.player[1].x != r->x2 || st.player[1].y != r->y2 ||
            st.player[1].vx != r->vx2 || st.player[1].vy != r->vy2 || wj_action(1) != r->action2 ||
            st.disc.x != r->dx || st.disc.y != r->dy || st.disc.mode != r->mode) {
            printf("action %u frame %u: P1 x/vx/action %ld/%ld/%u expected %ld/%ld/%u; "
                   "P2 x/vx/action %ld/%ld/%u expected %ld/%ld/%u; disc %ld,%ld/%u expected %ld,%ld/%u\n",
                   r->id, r->frame, (long)st.player[0].x, (long)st.player[0].vx, wj_action(0),
                   (long)r->x1, (long)r->vx1, r->action1,
                   (long)st.player[1].x, (long)st.player[1].vx, wj_action(1),
                   (long)r->x2, (long)r->vx2, r->action2,
                   (long)st.disc.x, (long)st.disc.y, st.disc.mode, (long)r->dx, (long)r->dy, r->mode);
            failures++; return;
        }
    }
    printf("original complete actions: %lu frames, six throws/captures/settled holds, both players\n",
           (unsigned long)COUNT(action_ref));
}

static void original_holds(void)
{
    u16 i;
    for (i = 0; i < COUNT(hold_ref); i++) {
        const HoldRef *r = &hold_ref[i];
        const HoldStart *s = &hold_start[r->id];
        u16 keys = s->delay != 65535 && r->frame >= s->delay && r->frame < s->delay + 2 ? core_keys(s->keys) : 0;
        if (!r->frame) {
            wj_reset(s->owner);
            st.player[0].x = s->x1; st.player[0].y = s->y1;
            st.player[0].hold = s->hold1; st.player[0].power = s->power1; st.player[0].bonus = s->bonus1;
            st.player[1].x = s->x2; st.player[1].y = s->y2;
            st.player[1].hold = s->hold2; st.player[1].power = s->power2; st.player[1].bonus = s->bonus2;
        }
        wj_logic(s->owner ? 0 : keys, s->owner ? keys : 0);
#define P(port, suffix) (st.player[port].x == r->x##suffix && st.player[port].y == r->y##suffix && \
    st.player[port].vx == r->vx##suffix && st.player[port].vy == r->vy##suffix && \
    (wj_action(port) == r->action##suffix) && \
    ((r->action##suffix != 0x1000 && r->action##suffix != 0x1004 && r->action##suffix != 0x1400) || \
     st.player[port].hold == r->hold##suffix) && \
    st.player[port].power == r->power##suffix && st.player[port].bonus == r->bonus##suffix)
        if (!P(0, 1) || !P(1, 2) || st.disc.x != r->dx || st.disc.y != r->dy || st.disc.mode != r->mode ||
            (r->mode == WJ_FLIGHT && (wj_vx() != r->dvx || wj_vy() != r->dvy))) {
            printf("hold %u frame %u: P1 %ld,%ld/%ld,%ld a%u h%u p%u b%u expected %ld,%ld/%ld,%ld a%u h%u p%u b%u; "
                   "P2 %ld,%ld/%ld,%ld a%u h%u p%u b%u expected %ld,%ld/%ld,%ld a%u h%u p%u b%u; disc %ld,%ld/%u expected %ld,%ld/%u\n",
                   r->id, r->frame,
                   (long)st.player[0].x, (long)st.player[0].y, (long)st.player[0].vx, (long)st.player[0].vy,
                   wj_action(0), st.player[0].hold, st.player[0].power, st.player[0].bonus,
                   (long)r->x1, (long)r->y1, (long)r->vx1, (long)r->vy1, r->action1, r->hold1, r->power1, r->bonus1,
                   (long)st.player[1].x, (long)st.player[1].y, (long)st.player[1].vx, (long)st.player[1].vy,
                   wj_action(1), st.player[1].hold, st.player[1].power, st.player[1].bonus,
                   (long)r->x2, (long)r->y2, (long)r->vx2, (long)r->vy2, r->action2, r->hold2, r->power2, r->bonus2,
                   (long)st.disc.x, (long)st.disc.y, st.disc.mode, (long)r->dx, (long)r->dy, r->mode);
            failures++; return;
        }
#undef P
    }
    printf("original possession: %lu steps, %lu trials, manual ages/directions and two 600-step automatic rallies\n",
           (unsigned long)COUNT(hold_ref), (unsigned long)COUNT(hold_start));
}

static u16 timing_keys(const TimingStart *s, u16 frame)
{
    return s->delay != 65535 && frame >= s->delay && frame < s->delay + s->held ? core_keys(s->keys) : 0;
}

static void original_timing(void)
{
    u16 i;
    for (i = 0; i < COUNT(timing_ref); i++) {
        const TimingRef *r = &timing_ref[i];
        const TimingStart *s = &timing_start[r->id];
        u16 keys = timing_keys(s, r->frame);
        if (!r->frame) wj_timing_door(s->receiver);
        wj_logic(s->receiver ? 0 : keys, s->receiver ? keys : 0);
#define P(port, suffix) (st.player[port].x == r->x##suffix && st.player[port].y == r->y##suffix && \
    st.player[port].vx == r->vx##suffix && st.player[port].vy == r->vy##suffix && \
    wj_action(port) == r->action##suffix && \
    ((r->action##suffix == 0 || r->action##suffix == 0x400) || wj_hold_value(port) == r->hold##suffix) && \
    st.player[port].power == r->power##suffix && st.player[port].bonus == r->bonus##suffix && \
    st.player[port].charge == r->charge##suffix && st.player[port].charged == r->charged##suffix)
        if (!P(0, 1) || !P(1, 2) || st.disc.x != r->dx || st.disc.y != r->dy ||
            st.disc.mode != r->mode || st.disc.z != r->z || st.disc.vz != r->vz ||
            (r->mode == WJ_FLIGHT && (wj_vx() != r->dvx || wj_vy() != r->dvy))) {
            printf("timing %u frame %u: P1 x/vx/a/hold/power/charge %ld/%ld/%u/%u/%u/%u expected %ld/%ld/%u/%u/%u/%u; "
                "P2 %ld/%ld/%u/%u/%u/%u expected %ld/%ld/%u/%u/%u/%u; disc %ld,%ld/%u z%ld vz%ld expected %ld,%ld/%u z%ld vz%ld\n",
                r->id, r->frame, (long)st.player[0].x, (long)st.player[0].vx, wj_action(0), wj_hold_value(0), st.player[0].power, st.player[0].charge,
                (long)r->x1, (long)r->vx1, r->action1, r->hold1, r->power1, r->charge1,
                (long)st.player[1].x, (long)st.player[1].vx, wj_action(1), wj_hold_value(1), st.player[1].power, st.player[1].charge,
                (long)r->x2, (long)r->vx2, r->action2, r->hold2, r->power2, r->charge2,
                (long)st.disc.x, (long)st.disc.y, st.disc.mode, (long)st.disc.z, (long)st.disc.vz,
                (long)r->dx, (long)r->dy, r->mode, (long)r->z, (long)r->vz);
            failures++; return;
        }
#undef P
    }
    for (i = 0; i < COUNT(timing_outcome); i++) {
        const TimingOutcome *r = &timing_outcome[i];
        const TimingStart *s = &timing_start[r->id];
        u16 frame;
        wj_timing_door(s->receiver);
        for (frame = 0; frame <= r->frame; frame++) {
            u16 keys = timing_keys(s, frame);
            wj_logic(s->receiver ? 0 : keys, s->receiver ? keys : 0);
        }
        CHECK(st.disc.mode == r->mode);
    }
    printf("original timed receptions: %lu equality steps, %lu no-write trials, %lu separate block classifications\n",
           (unsigned long)COUNT(timing_ref), (unsigned long)COUNT(timing_start), (unsigned long)COUNT(timing_outcome));
}

static void runtime_replay(void)
{
    Windjam saved;
    u16 hashes[100], i;
    char path[96];
    sw_init(4);
    for (i = 0; i < 1001; i++) sw_step(0);
    CHECK(st.logic_frame == (u32)(8508UL * 2959UL) / 12800UL);
    CHECK(st.seconds == 0 && st.clock_phase == 0);
    saved = st;
    snprintf(path, sizeof path, "/tmp/windjam-%ld.state", (long)getpid());
    CHECK(sw_save_state(path) == 0);
    for (i = 0; i < 100; i++) { sw_step(0); hashes[i] = wj_hash(); }
    CHECK(sw_load_state(path) == 0);
    CHECK(memcmp(&st, &saved, sizeof st) == 0);
    remove(path);
    for (i = 0; i < 100; i++) { sw_step(0); CHECK(hashes[i] == wj_hash()); }
    CHECK(sw_step(K_ESC) == 0);
    sw_init(0); sw_step(K_ENTER); CHECK(st.points[0] == 0 && st.points[1] == 0);
    CHECK(sw_level(30, 30) == C_WHITE && sw_level(130, 30) == C_WHITE);
    printf("runtime: exact rational cadence, saved-state replay and reset/escape passed\n");
}

static void timing_runtime_replay(void)
{
    u16 scenario, save_frame, frame, hashes[120], planes[120];
    char path[96];
    snprintf(path, sizeof path, "/tmp/windjam-timing-%ld.state", (long)getpid());
    for (scenario = 11; scenario <= 12; scenario++) {
        for (save_frame = 8; save_frame <= 25; save_frame += 17) {
            sw_init(scenario);
            for (frame = 0; frame <= save_frame; frame++) sw_step(frame == 8 ? K_A : 0);
            CHECK(save_frame == 8 ? st.player[scenario - 11].ready : st.player[scenario - 11].charging);
            CHECK(sw_save_state(path) == 0);
            for (frame = 0; frame < COUNT(hashes); frame++) {
                sw_step(0); hashes[frame] = wj_hash(); planes[frame] = sw_checksum();
            }
            CHECK(sw_load_state(path) == 0);
            for (frame = 0; frame < COUNT(hashes); frame++) {
                sw_step(0); CHECK(wj_hash() == hashes[frame]); CHECK(sw_checksum() == planes[frame]);
                if (save_frame == 25 && frame == 10) CHECK(st.player[scenario - 11].charged);
            }
        }
    }
    remove(path);
    /* One draw can contain three logic steps: holding A never retriggers the
     * first capture input or resets the throw's animation age. */
    sw_init(13);
    for (frame = 0; frame < 10; frame++) sw_step(0);
    sw_step(K_A);
    CHECK(st.player[0].throwing && st.player[0].throw_catch);
    save_frame = st.player[0].age;
    sw_step(K_A);
    CHECK(st.player[0].age > save_frame && st.player[0].age <= save_frame + 3);
    printf("timed runtime: ready/charge saves replay states and planes, both sides; scheduler input edge passed\n");
}

static u16 curve_pad(u16 scenario, u16 frame)
{
    u16 vertical = scenario & 1 ? K_UP : K_DOWN;
    u16 horizontal = scenario >= 25 ? K_LEFT : K_RIGHT;
    return frame == 0 ? vertical : frame == 1 ? vertical | horizontal :
        frame == 2 ? horizontal | K_A : 0;
}

static void curve_runtime_replay(void)
{
    u16 scenario, save_frame, frame, hashes[140], planes[140];
    char path[96];
    snprintf(path,sizeof path,"/tmp/windjam-curve-%ld.state",(long)getpid());
    for (scenario=23;scenario<=26;scenario++) {
        for (save_frame=1;save_frame<=14;save_frame+=13) {
            sw_init(scenario);
            for (frame=0;frame<=save_frame;frame++) sw_step(curve_pad(scenario,frame));
            CHECK(save_frame==1 ? st.player[st.human_port].history[0]!=0 :
                st.disc.mode==WJ_CURVE_PLUS || st.disc.mode==WJ_CURVE_MINUS);
            CHECK(sw_save_state(path)==0);
            for (frame=0;frame<COUNT(hashes);frame++) {
                sw_step(curve_pad(scenario,save_frame+1+frame));
                hashes[frame]=wj_hash(); planes[frame]=sw_checksum();
            }
            CHECK(st.points[0]+st.points[1]>0);
            CHECK(sw_load_state(path)==0);
            for (frame=0;frame<COUNT(hashes);frame++) {
                sw_step(curve_pad(scenario,save_frame+1+frame));
                CHECK(wj_hash()==hashes[frame]); CHECK(sw_checksum()==planes[frame]);
            }
        }
    }
    remove(path);
    printf("curved runtime: both arcs/sides, mid-gesture and flight saves replay states/planes through scoring/service\n");
}

static void practice_stress(void)
{
    u16 i, catches = 0;
    game_scenario(4);
    for (i = 0; i < 30000; i++) {
        u16 before = st.disc.mode;
        wj_logic(0, 0);
        if (before == WJ_FLIGHT && st.disc.mode == WJ_HELD) catches++;
        CHECK(st.disc.profile < WJ_TEST_PROFILE_COUNT && st.points[0] <= 99 && st.points[1] <= 99);
        CHECK(st.disc.mode == WJ_HELD || st.disc.mode == WJ_FLIGHT || st.disc.mode == WJ_BLOCK || st.disc.mode == WJ_LIFT || st.disc.mode == WJ_GOAL);
        CHECK((st.player[0].x >> 16) >= 27 && (st.player[0].x >> 16) <= 141);
        CHECK((st.player[1].x >> 16) >= 180 && (st.player[1].x >> 16) <= 292);
        CHECK((st.player[0].y >> 16) >= 76 && (st.player[0].y >> 16) <= 188);
        CHECK((st.player[1].y >> 16) >= 76 && (st.player[1].y >> 16) <= 188);
    }
    CHECK(catches > 5);
    printf("native practice: 30000 steps, %u catches, scores %u/%u, bounds/states valid\n",
           catches, st.points[0], st.points[1]);
}

static void possession_limits(void)
{
    u16 i;
    wj_reset(0); st.player[0].hold = 63;
    wj_logic(0, 0);
    CHECK(st.player[0].throwing && st.player[0].age == 0);
    CHECK(st.player[0].power == 49 && st.player[0].bonus == 0 && st.player[0].hold == 60);
    wj_reset(0); st.player[0].hold = 63;
    wj_logic(K_A, 0);
    CHECK(st.player[0].hold == 59 && st.player[0].power == 48);
    for (i = 0; i < 5; i++) wj_logic(K_A, 0);
    CHECK(st.player[0].age == 5 && st.player[0].power == 48);
    wj_reset(0); st.points[1] = 99; st.player[0].hold = 2;
    wj_logic(K_A, 0);
    CHECK(st.player[0].hold == 0);
    wj_reset(1); st.player[1].hold = 64; st.player[1].power = 0; st.player[1].bonus = 8;
    wj_logic(K_A, 0); /* Idle owner automatically releases even with the other pad active. */
    CHECK(st.player[1].hold == 64 && st.player[1].bonus == 2 && st.player[1].power == 32);
    game_scenario(6); st.player[0].power = 60; st.player[0].bonus = 0;
    for (i = 0; i < 90; i++) wj_logic(0, 0);
    CHECK(st.player[0].power == 4 && st.player[0].bonus == 8 && st.player[0].hold == 0);
    printf("possession limits: exact release edge, held input, clamps and native loser-serve history reset passed\n");
}

static void clock_and_score_limits(void)
{
    u16 i;
    wj_reset(0);
    for (i = 0; i < 59; i++) wj_logic(0, 0);
    CHECK(st.seconds == 30);
    wj_logic(0, 0); CHECK(st.seconds == 29);
    for (i = 60; i < 1776; i++) wj_logic(0, 0);
    CHECK(st.seconds == 0 && st.clock_phase == 0);
    for (i = 0; i < 120; i++) wj_logic(0, 0);
    CHECK(st.seconds == 0 && st.clock_phase == 0);
    game_scenario(6);
    for (i = 0; i < 60; i++) wj_logic(0, 0);
    CHECK(st.seconds == 29 && st.disc.mode == WJ_GOAL);
    game_scenario(3); st.points[1] = 98;
    for (i = 0; i < 10 && st.disc.mode != WJ_GOAL; i++) wj_logic(0, 0);
    CHECK(st.points[1] == 99 && st.disc.mode == WJ_GOAL);
    wj_reset(0); CHECK(st.seconds == 30 && st.points[0] == 0 && st.points[1] == 0);
    printf("HUD state: 30-second cadence, goal-pause clock, zero/reset and two-digit score cap passed\n");
}

static u16 advanced_keys(u16 scenario, u16 frame)
{
    if (scenario < 17) return frame == 2 ? K_B : 0;
    if (frame == 8) return K_A;
    return frame == 57 ? (scenario >= 19 ? K_B : K_A) : 0;
}

static void advanced_runtime(void)
{
    u16 scenario, frame, hashes[140], planes[140], kinds = 0;
    char path[96];
    snprintf(path, sizeof path, "/tmp/windjam-advanced-%ld.state", (long)getpid());
    for (scenario = 15; scenario <= 22; scenario++) {
        u16 save_frame = scenario < 17 ? 10 : scenario < 19 ? 67 : 70;
        sw_init(scenario);
        for (frame = 0; frame <= save_frame; frame++) sw_step(advanced_keys(scenario, frame));
        CHECK(st.disc.mode == (scenario < 17 ? WJ_LOB : scenario < 19 ? (scenario == 17 ? WJ_MITA : WJ_YOO) : WJ_SUPERLOB));
        kinds |= 1 << (scenario - 15);
        CHECK(sw_save_state(path) == 0);
        for (frame = 0; frame < COUNT(hashes); frame++) { sw_step(0); hashes[frame] = wj_hash(); planes[frame] = sw_checksum(); }
        CHECK(sw_load_state(path) == 0);
        for (frame = 0; frame < COUNT(hashes); frame++) { sw_step(0); CHECK(wj_hash() == hashes[frame] && sw_checksum() == planes[frame]); }
    }
    CHECK(kinds == 255); remove(path);
    /* Landing awards are two points even at the two-digit score ceiling. */
    wj_reset(0); st.disc.mode = WJ_LAND; st.disc.grace = 1; st.disc.x = 230L << 16; st.points[0] = 98;
    wj_logic(0,0); CHECK(st.last_award == 2 && st.points[0] == 99 && st.disc.mode == WJ_GOAL);
    for (frame = 0; frame < 90; frame++) wj_logic(0,0);
    CHECK(st.disc.mode == WJ_HELD && !st.disc.z && !st.disc.vz && !st.player[0].strong && !st.player[1].charged);
    /* Complete airborne miss -> ground acceleration -> goal -> loser service. */
    for (scenario = 21; scenario <= 22; scenario++) {
        u16 bounced = 0, scored = 0, served = 0;
        sw_init(scenario);
        for (frame = 0; frame < 220; frame++) {
            sw_step(advanced_keys(scenario, frame));
            bounced |= st.disc.mode == WJ_BOUNCE;
            scored |= st.disc.mode == WJ_GOAL;
            if (scored && st.disc.mode == WJ_HELD) served = 1;
        }
        CHECK(bounced && scored && served);
    }
    printf("advanced runtime: eight airborne/special save replays (states and planes); two-point cap, missed-lob bounce/goal/serve passed\n");
}

static void flight_effects(void)
{
    static const u16 scenarios[] = {13,14,17,18,21,23};
    static const char *scripts[] = {"keys/timing_return.txt","keys/timing_return.txt",
        "keys/special.txt","keys/special.txt","keys/superlob.txt","keys/curve23.txt"};
    static SwScript input;
    u16 trial, frame;
    char path[96];
    snprintf(path,sizeof path,"/tmp/windjam-effects-%ld.state",(long)getpid());
    for (trial=0;trial<COUNT(scenarios);trial++) {
        u16 seen=0, saved=0, save_frame=0, last_x=0,last_y=0,last_kind=0,last_owner=0;
        CHECK(sw_load_script(&input,scripts[trial])==0);
        sw_init(scenarios[trial]);
        for (frame=0;frame<220;frame++) {
            u16 before, planes;
            sw_step(sw_script_keys(&input,frame));
            if (st.effect_count>1 && st.effect_kind==last_kind && st.effect_owner==last_owner) {
                CHECK(st.effect_x[1]==last_x && st.effect_y[1]==last_y);
                seen=1;
            }
            if (st.disc.mode==WJ_HELD || st.disc.mode==WJ_GOAL) CHECK(!st.effect_count && !st.effect_kind);
            if (st.effect_kind) CHECK(st.effect_kind==(trial<2?1:2));
            last_x=st.effect_x[0]; last_y=st.effect_y[0]; last_kind=st.effect_kind; last_owner=st.effect_owner;
            before=wj_hash(); planes=sw_checksum();
            game_render(); CHECK(wj_hash()==before && sw_checksum()==planes);
            if (!saved && st.effect_count==4) {
                CHECK(sw_save_state(path)==0); saved=1; save_frame=frame;
            }
        }
        CHECK(trial==5 ? !seen && !saved : seen && saved);
        if (saved) {
            u16 hashes[40], planes[40], i;
            CHECK(sw_load_state(path)==0);
            CHECK(sw_load_script(&input,scripts[trial])==0);
            for (i=0;i<40;i++) {
                sw_step(sw_script_keys(&input,save_frame+1+i)); hashes[i]=wj_hash(); planes[i]=sw_checksum();
            }
            CHECK(sw_load_state(path)==0);
            CHECK(sw_load_script(&input,scripts[trial])==0);
            for (i=0;i<40;i++) {
                sw_step(sw_script_keys(&input,save_frame+1+i));
                CHECK(hashes[i]==wj_hash() && planes[i]==sw_checksum());
            }
        }
    }
    remove(path);
    printf("flight effects: both powerful returns, charged flights/bounce, actual path history, capture clearing, pure renders and saved-state replay passed\n");
}

int main(int argc, char **argv)
{
    /* Select an actual full trail for PC/TI active-frame screen comparisons. */
    if (argc==5 && !strcmp(argv[1],"--effect-frame")) {
        static SwScript input;
        u16 frame, scenario=atoi(argv[3]), kind=atoi(argv[4]);
        if (sw_load_script(&input,argv[2])) return 2;
        sw_init(scenario);
        for (frame=0;frame<220;frame++) {
            sw_step(sw_script_keys(&input,frame));
            if (st.effect_count==4 && st.effect_kind==kind) {
                printf("%u %04X\n",frame+1,sw_checksum()); return 0;
            }
        }
        return 3;
    }
    /* --hash is consumed by the PC/TI validator, one explicit field hash/frame. */
    if (argc == 5 && !strcmp(argv[1], "--hash")) {
        static SwScript script;
        u16 n = atoi(argv[3]), scenario = atoi(argv[4]), i;
        if (sw_load_script(&script, argv[2]) != 0) return 2;
        sw_init(scenario);
        for (i = 0; i < n; i++) { sw_step(sw_script_keys(&script, i)); printf("%u\n", wj_hash()); }
        return 0;
    }
    original_movement(); original_shots(); original_actions(); original_holds(); original_timing(); contacts_and_goals(); runtime_replay(); timing_runtime_replay(); practice_stress();
    clock_and_score_limits(); possession_limits(); original_advanced(); original_curves(); original_dash(); advanced_runtime(); curve_runtime_replay(); guided_charge_and_dash_replay();
    flight_effects();
    printf(failures ? "%u failures\n" : "all tests passed\n", failures);
    return failures != 0;
}
