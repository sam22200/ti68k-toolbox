#ifndef MINISH_H
#define MINISH_H
#include "../../runtime/core/rt.h"
#define WOODS_W 720
#define WOODS_H 320
typedef struct {
    s32 x,y;
    u8 action,timer,face,age,hp,pose,display_pose,recoil;
    u8 recoil_dir,fade;
} MinishEnemy;
typedef struct {
    s32 x,y;
    u8 life,face,age;
} MinishRock;
typedef struct {
    u16 x,y;
    u8 age,kind,display_pose;
} MinishFx;
typedef struct {
    s32 x, y;                 /* Exact Q8.8 source coordinates. */
    u16 collisions, steps;
    s16 camx, camy;
    u8 direction, facing, moving, ready;
    u8 anim_face, anim_walk, anim_phase, anim_timer;
    u8 pose, display_pose, cover, display_cover, preview;
    u8 attack, last_a, cut_count, display_cut_count;
    u8 cut_flags[7], cut_list[53];
    u16 rng;
    u8 encounters,health,iframes,recoil,recoil_dir,kills,ticks;
    u8 roll,last_b,roll_guard;
    MinishEnemy enemies[2];
    MinishRock rocks[4];
    MinishFx effects[4];
} MinishState;
extern MinishState st;
void minish_place(u16 x, u16 y);
void minish_step(u16 keys);    /* One source update, independent of rendering. */
u8 minish_solid(u16 x, u16 y);
u32 minish_hash(void);
u8 minish_actions_init(void);
u8 minish_cut_cell(u16 cell);
u8 minish_action_step(u16 keys);
void minish_draw_cuts(void);
void minish_draw_sword(s16 *x, s16 *y, u16 *w, u16 *h);
u8 minish_combat_init(void);
void minish_combat_reset(void);
void minish_combat_start(void);
u8 minish_combat_before(u16 keys);
void minish_combat_step(void);
void minish_draw_enemies(u8 front);
void minish_draw_health(void);
void minish_draw_label(void);
u8 minish_effects_init(void);
void minish_effects_reset(void);
void minish_effects_step(void);
void minish_fx_spawn(u16 x,u16 y,u8 kind);
void minish_draw_effects(u8 front);
void minish_draw_fx(u8 pose,u16 x,u16 y,u8 cover);
u8 minish_roll_step(u16 keys);
u16 minish_roll_speed(void);
void minish_draw_death(const MinishEnemy *e);
u8 minish_death_length(void);
u8 minish_fx_pose_count(void);
u8 minish_fx_preview_pose(u8 group);
void minish_canopy(s16 x,s16 y,u16 w,u16 h);
#ifdef MINISH_ZOOM
u16 minish_scaled(u16 x);
u8 minish_zoom_init(void);
void minish_zoom_render(void);
#endif
#endif
