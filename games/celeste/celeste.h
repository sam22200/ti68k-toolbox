// Celeste Classic (PICO-8, Matt Thorson + Noel Berry) on the Portable Game Runtime: state and
// constants, shared with the trace tests. The logic is the cart's code.lua translated function
// by function, bit-exact in 16.16 (p8num.h); see README.md for the scope and the decisions.
#ifndef CELESTE_H
#define CELESTE_H
#include "../../runtime/core/rt.h"
#include "p8num.h"

#define K_JUMP K_A                     // [2nd]   = PICO-8 O (btn 4)
#define K_DASH K_B                     // [shift] = PICO-8 X (btn 5)

#define MAXOBJ 24                      // pool; room 0 peaks at about a dozen (smoke included)
#define ROOM_X 16                      // the 128-pixel room on the 160-pixel screen

// object types (the cart's type tables); only the ones room 0 needs exist in this slice
enum { T_NONE, T_SPAWN, T_PLAYER, T_SMOKE, T_TITLE, T_FAKEWALL, T_FRUIT, T_LIFEUP,
       T_PLATFORM, T_FALLFLOOR };
enum { M_PLAY, M_END };                // playing; "end of demo" screen (exit at the top)

typedef struct {
    u8 type, slot, collideable, solids, flipx, flipy;   // slot: pool index (no pointer division)
    s8 hbx, hby, hbw, hbh;             // hitbox (integers in the cart)
    s16 x, y;                          // whole pixels: the cart moves objects pixel by pixel,
    u16 yf;                            // the fractions stay in rem; only the fruit's bobbing y
                                       // (and the "1000" born there) has one: yf, the low 16 bits of the 16.16 y
    fix spdx, spdy, remx, remy, spr;
    // player
    u8 p_jump, p_dash, was_on_ground;
    s8 grace, jbuffer, djump, dash_time;
    fix dash_effect_time, dtx, dty, dax, day, spr_off;   // dash_effect_time never stops falling
    // player_spawn (state, delay, target y), room_title (delay), lifeup (delay = duration)
    u8 state;
    s16 delay, ty;
    // fruit
    fix start;
    u8 off40;                          // fruit off % 40: sin(off / 40) repeats every 40
} Obj;

typedef struct { s16 x, y, sx, sy; s8 t; } DeadP;        // cosmetic, 12.4 pixels

typedef struct {
    Obj pool[MAXOBJ];
    u8 used[MAXOBJ];                   // 0 free, 1 in the list, 2 deleted this frame
    u8 order[MAXOBJ];                  // the cart's objects list (pool indices, in order)
    u8 nobj;
    u8 ntype[T_FALLFLOOR + 1];         // objects per type in the list: collide() skips absent types
    u8 mode;
    u8 room_x, room_y;
    u8 freeze, shake, will_restart, has_dashed, has_key, pause_player;
    s8 delay_restart, sfx_timer;
    s16 deaths;
    s8 max_djump;
    u8 frames, seconds;
    s16 minutes;
    u32 got_fruit;                     // bit i = got_fruit[1 + i]
    u8 nsfx;
    s8 sfx_ev[8];                      // sfx() calls this frame (the trace's __sfx)
    u8 overflow, unsupported;          // pool full; something outside the slice
    // cosmetic state: not traced, its own random numbers (rt_rand)
    s8 cam_y;                          // vertical screen shake (decision Q11)
    DeadP dead[8];
    u8 ndead;
    s16 hair[5][2];                    // hair nodes, 12.4 pixels
    s16 view_y;                        // top row of the room shown (160x100 view camera)
} Celeste;

extern Celeste S;
Obj *init_object(u8 type, s16 x, s16 y);
void destroy_object(Obj *o);
typedef void (*ObjFn)(Obj *);
void for_all(ObjFn f);                 // PICO-8 all()/foreach() over the objects list
u32 state_hash(void);                  // gameplay state hash (TI = PC check)
#define OBJ_Y(o) (FIX((o)->y) + (o)->yf)   // the cart's y as a 16.16 number

#endif
