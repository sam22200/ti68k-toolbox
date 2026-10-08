#ifndef WINDJAM_H
#define WINDJAM_H
#include "../../runtime/core/rt.h"

enum { WJ_HELD = 2, WJ_FLIGHT = 4, WJ_CURVE_PLUS = 6, WJ_CURVE_MINUS = 8,
       WJ_BLOCK = 10, WJ_LIFT = 14, WJ_GOAL = 18,
       WJ_LOB = 22, WJ_DRAG = 24, WJ_LAND = 28, WJ_SUPERLOB = 38,
       WJ_BOUNCE = 40, WJ_YOO = 42, WJ_MITA = 44 };
typedef struct {
    s32 x, y, vx, vy;
    u8 throwing, age, aim, throw_delay, throw_catch, lock, hold, power, bonus;
    u8 catching, catch_age, recoil_profile, recoil_reverse, recoil_aim;
    u8 art_action, art_direction, art_tick;
    u8 ready, ready_age, charging, charge, charged;
    u16 ready_counter;
    u8 throw_kind, counter_kind, strong, boundary, dragged, air_charge, y_boundary;
    u8 history[9], curve_aim, recoil_dynamic, recoil_angle, facing;
    u16 recoil_speed;
    u8 dash_age, dash_wall;
} WjPlayer;
typedef struct {
    s32 x, y, z, vz, free_vx, free_vy;
    u8 mode, owner, profile, reverse, pending, defender, grace;
    u32 angle;
    s32 turn;
    u16 speed, anchor;
    u16 target_x, target_y;
    u8 wave, wall_side, catch_angle;
    u8 dynamic;
} WjDisc;
typedef struct {
    WjPlayer player[2];
    WjDisc disc;
    u16 keys[2], points[2], logic_frame, draw_frame, phase, goal_age, clock_phase;
    u8 bots, serve_to, shot_number, last_award, seconds, human_port;
    u8 lob_seed;
    u8 effect_x[4], effect_y[4], effect_count, effect_kind, effect_owner;
} Windjam;
extern Windjam st;

void wj_reset(u8 owner);
void wj_walk(u16 port, u16 keys);
void wj_logic(u16 p1, u16 p2);  /* one original-rate step, two independent pads */
u16 wj_contact(u16 port, s16 dx, s16 dy, u8 flipped);
u8 wj_goal_zone(s32 y);
u16 wj_action(u16 port);
s32 wj_vx(void);
s32 wj_vy(void);
u16 wj_hash(void);             /* per-field, independent of byte order/padding */
void wj_timing_door(u16 receiver);
u16 wj_hold_value(u16 port);
u8 wj_prepare_hint(u16 port); /* aligned incoming shot, a few steps before contact */
u8 wj_disc_effect(void);      /* 0 none, 1 powerful flight, 2 charged special */
void wj_effect_tick(void);    /* one LCD-history sample per draw, never per render */
void wj_art_tick(void);        /* native cosmetic clock, once per logic step */
u8 wj_art_init(void);
#endif
