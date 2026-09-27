// Final Fantasy Alternative remake: state, constants and engine API, shared with the tests.
#ifndef FFA_H
#define FFA_H
#include "../../runtime/core/rt.h"
#include "gfx_ids.h"

// ---------------------------------------------------------------- rooms (tools/rooms.py)
typedef struct { u8 dest, key; s8 ax, ay; } Door;   // dest: room index, 0xFE = leaves part I,
                                                    // 0xFF = out of scope; key: flag, 0 none;
                                                    // arrival cell, -1 = keep that coordinate
typedef struct {
    u8 id, w, h, px, py, frc, ndoor, ntrig, set;    // id = original room number, frc x 10,
                                                     // set = tile set in the ffadat file
    const u8 *cell;                                  // w x h cell codes (below)
    const s16 *trig;                                 // original p x 10
    const Door *door;
    RtTilemap map;
} Room;
#define CELL_WALL 0
#define CELL_FLOOR 1
#define CELL_TRIG 0x40                               // | trigger index
#define CELL_DOOR 0x80                               // | door index
#define ROOM_OUT 0xFE
#define KEY_NEVER 0xFD                               // no original program sets it: locked
extern const Room rooms[];
extern const u8 nroom;
extern const u8 room_index[];                        // original id -> index (255 = none)

// ---------------------------------------------------------------- hero geometry (pixels)
#define TILE 16
#define HB_W 10                                      // feet hitbox, x/y = its top-left corner
#define HB_H 8
#define HB_X0 3                                      // hitbox offset in its arrival cell
#define HB_Y0 6
#define SPR_DX (-3)                                  // hero sprite (16x24) = hitbox + (-3, -16)
#define SPR_DY (-16)
#define SLIDE 6                                      // corner sliding: max overlap nudged away
#define STEP_PX 16                                   // one original step = one tile walked

enum { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT };
enum { M_WALK, M_FADE_OUT, M_FADE_IN, M_TEXT, M_BATTLE, M_END, M_SCRIPT, M_GAMEOVER, M_MENU, M_TITLE };
#define FADE_STEPS 4                                 // 0 = normal .. 3 = white (4 greys)
#define FADE_FRAMES 3                                // frames per fade step

// ---------------------------------------------------------------- story state
#define NFLAG 128                                    // clef[1..120] of the original, same indices
#define STAT 20                                      // stats are fixed point x 20 (+0.25 = +5)
typedef struct {
    u8 lv, speci;                                    // level; the stat chosen to grow more
    u16 hp, hpm, mp, mpm, exp, expt, expn, gils;
    u16 st[6];                                       // strength, vitality, magic, spirit, speed,
                                                     // luck (x STAT): forc def mag defm vit chan
    u16 jl;                                          // limit gauge, 0 .. LIMIT_FULL
    u8 weapon, armor, acc[2], slot[2];               // equipped (armat rows, materia rows)
} Hero;
enum { S_STR, S_DEF, S_MAG, S_MDEF, S_SPD, S_LUCK };
#define LIMIT_FULL (30 * 256)

enum { I_POTION, I_HIPOTION, I_ETHER, I_TETHER, I_XPOTION, I_ELIXIR, I_ANTIDOTE, NITEM };
enum { A_SWORD = 1, A_WRIST = 9, A_BANGLE = 25, NARM = 32 };      // armat rows of the original
enum { MAT_FIRE = 2, MAT_CURE = 3, NMAT = 18 };                   // maglist rows

#define NNPC 6                                       // slot 0 = the hero's scripted target
typedef struct {
    u8 on, spr, dir, anim;
    s16 x, y, tx, ty;                                // hitbox top-left, walk target
} Npc;

typedef struct {
    u8 mode, room, dir, anim, fade, fade_t, next_room, next_door;
    s16 x, y;                                        // hitbox top-left, pixels in the room
    u8 sub;                                          // sub-pixel phase (1.5 px/frame walk)
    u8 lock;                                         // arrows held when entering a room, ignored until released
    u8 walked;                                       // pixels since the last step
    u16 steps, mc, co;                               // steps; encounter counter and threshold
    s16 trig;                                        // trigger being handled (p x 10), 0 none
    u8 cell_in;                                      // code of the cell under the hitbox centre
    u8 flag[NFLAG];
    Hero hero;
    char name[9];
    u8 item[NITEM], own[NARM], mat[NMAT];            // counts, owned equipment, owned materia
    u16 num;                                         // number shown by '\2' (the riddle's devi)
    u16 devi;                                        // riddle number, 5000 + rand(100)
    // dialogue
    u8 dlg_on, dlg_text, dlg_ask, dlg_page, dlg_cur, ans;
    u16 dlg_shown;
    // story script (story.c): event, resume point, timer; actors
    u8 ev, timer, hwalk, shop;                       // shop: 1 shop window, 2 number entry
    u8 digit[4], dpos;                               // number entry (the riddle)
    u8 battle, won;                                  // monster n of the fight, result
    u8 ret_mode;                                     // mode to go back to after the fight
    // battle (battle.c)
    u8 bp, bcur, bsub, bt, qu, bwho, bmsg, bflash;
    u16 mhp, ja, jae;                                // monster HP, ATB gauges (x 256)
    s16 bnum;                                        // number shown (damage / heal)
    u8 bdrop;
    u16 stats_kills, stats_flight;
    u8 menu, mcur, mmsg;                             // main menu (menu.c)
    u8 tstep, tcur, tfade;                           // title and intro (title.c)
    u16 pc;
    s16 arg;
    Npc npc[NNPC];
} Game;
extern Game st;

u8 world_load(void);                                 // tile sets from ffadat, 0 = missing
const RtTilemap *world_tilemap(void);                // the current room's map and tiles
void world_enter(u8 room, s8 cx, s8 cy);             // put the hero in a cell of a room
u8 world_cell(s16 px, s16 py);                       // cell code under a pixel of the room
u8 world_solid(u8 code);
void world_move(s16 dx, s16 dy);                     // collide + slide, may open a door
s16 world_cam(s16 p, s16 view, s16 size);
void fade_planes(u8 level);                          // lighten the visible planes by level

void dialog_open(u8 text, u8 ask);                   // ask: Yes/No on the last page -> st.ans
void dialog_update(void);
void dialog_render(u8 top);

u8 story_trigger(s16 p, u8 examine);                 // start the event of a trigger, 1 if any
u8 story_run(void);                                  // one frame of the current event, 1 = done
void story_room(void);                               // spawn the NPCs of the current room
u8 story_opened(u8 i, s16 *cx, s16 *cy);             // opened chests of the room
void npc_step(void);                                 // move the actors towards their targets
void shop_render(void);
void battle_start(u8 n);                             // n = 0: pick from the room's table
void battle_update(void);
void battle_render(void);
void hero_level_up(void);
u8 has_materia(u8 m);
void auto_equip(void);
void menu_open(void);
u8 menu_update(void);                                // 0 = quit
void menu_render(void);
typedef struct { u32 magic; Game g; } Save;          // the save file: magic + version, then st
#define SAVE_MAGIC 0x46464131UL                      // "FFA1": change it when Game changes
#define SAVE_NAME "ffasav"
u8 game_save(void);
u8 game_saved(Save *s);                              // 1 = a valid save was read into s
void title_open(void);
void title_update(void);
void title_render(void);                               // best gear and materia (until the menu)

#endif
