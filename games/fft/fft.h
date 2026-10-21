// Final Fantasy Tactics, Magic City Gariland: an isometric tech demo on the Portable Game
// Runtime (README.md). Our own engine; the battlefield's heights and its four pictures come
// from the local disc (tools/extract.py -> map.h, fftv0..3), and so do the units (tools/units.py
// -> units.h); the cursor, shadow and rotation greys are ours (tools/art.py).
#ifndef FFT_H
#define FFT_H
#include "../../runtime/core/rt.h"
#if (defined(FFT_OPENING) || defined(FFT_STORY_GAR)) && !defined(FFT_NO_REACH_CONTOUR)
#ifndef FFT_REACH_CONTOUR
#define FFT_REACH_CONTOUR
#endif
#endif
#if defined(FFT_OPENING) || (defined(FFT_STORY_GAR) && !defined(FFT_RAW_ACTORS))
#define FFT_ACTOR_BANK                // chapter packages: actor frames from the common ffca/ffcb
#endif

typedef struct { u8 c[4]; u8 look[5]; u8 walk, stand, water; } Tile;   // map.h, extract.py (walk 2:
                                                                    // crossed, not stopped on)
// battle.h (tools/battle.py): FFT's numbers for the Gariland battle
typedef struct { u8 growth[5], mult[5], move, jump, cev, row; } Job;   // raw stats HP MP SP PA MA; row:
                                                                    // its JP's career slot
typedef struct { u8 id, hp, mp, pa, ma, sp, move, jump, wp, knife; char name[13]; } Item;
typedef struct {
    u8 gfx, job, type, level, brave, faith;   // type: the generation table (male, female, Ramza)
    u16 day;                                   // birthday 1..365; 0, and Brave / Faith 0: drawn
    u8 x, z, face, team;
    u8 eq[5][2];                               // head, body, accessory, weapon, shield: one of two
    u8 set, learned;                           // skill set; the abilities it knows (0xFF: drawn)
} UnitDef;
// An ability (RE_NOTES.md § Abilities): AB_DAMAGE (1..Y times PA, the zodiac, a knockback by
// Brave), AB_CURE (removes the statuses Y, else misses), AB_WISH (the user loses max HP / 5, the
// target heals twice that), AB_HEAL (Y x 10 HP), AB_REVIVE (a KO unit back with 1..Y HP). Range:
// tiles at most `range` steps away (any path), within `vert` half units of the user's height
// (0xFF: any); AF_SELF: the user's own tile too, AF_EVADE: evaded as an attack, AF_ITEM: from
// the bag (slot `bag`; the enemies' items do not run out, those of their level `lv` or less).
enum { AB_DAMAGE, AB_CURE, AB_WISH, AB_HEAL, AB_REVIVE, AB_DEFEND, AB_RAISE, AB_BREAK };   // AB_DEFEND: the command
enum { AF_SELF = 1, AF_EVADE = 2, AF_ITEM = 4, AF_WEAPON = 8, AF_DRAIN = 16, AF_AREA = 32, AF_CHARGE = 64 };
enum { ST_DEFEND = 1, ST_POISON = 2, ST_DARK = 4, ST_SILENCE = 8 };   // Dead: 0 HP
typedef struct { char name[13]; u8 kind, y, range, vert, flags, bag, rate; u16 jp; u8 lv; } Ability;
#define SET_N 6                                // abilities per skill set at most
typedef struct { char name[12]; u8 abil[SET_N]; } SkillSet;   // abilities[] indices, 0xFF none
#include "art.h"
#if defined(FFT_OPENING) || defined(FFT_STORY_GAR)
// Chapter packages: the per-chapter numbers, tables and offsets come from the chapter's
// descriptor (tools/chapter_desc.py, campaign/ENGINE.md), read at load, not from headers.
#define FFT_CHAPTER
#endif
#ifdef FFT_CHAPTER
#ifdef FFT_OPENING
#include "opening/data/units.h"
#include "opening/data/map.h"
#else
#include "units.h"
#include "map.h"
#endif
#ifdef FFT_RAW_ACTORS
#include "weapons.h"                          // the earlier raw fftu/fftw control (Gariland)
#else
#include "campaign/data/weapons.h"            // blades alone: ffcw (both chapters' copies match)
#endif
#include "ui_common.h"                        // ffcu: the interface both chapters share
#include "chapter.h"
#undef ACTOR_ARMED
#undef ACTOR_OBJECTS
#define NUNIT 11                              // units per battle at most (both chapters: 11)
#define BAG_MAX 5                             // bag slots at most
#define CD(f) RD16(cdesc, CD_##f)
#define NJOB CD(NJOB)
#define NITEM CD(NITEM)
#define NABIL CD(NABIL)
#define NSET CD(NSET)
#define SET_BASIC CD(SET_BASIC)
#define NRSM_PRE CD(NRSM_PRE)
#define DEFEND_JP CD(DEFEND_JP)
#define DEFEND_RATE CD(DEFEND_RATE)
#define NBAG CD(NBAG)
#define ACTOR_ARMED CD(ACTOR_ARMED)
#define ACTOR_OBJECTS CD(ACTOR_OBJECTS)
#define jobs ((const Job *)(igfx + CD(JOBS)))
#define items ((const Item *)(igfx + CD(ITEMS)))
#define abilities ((const Ability *)(igfx + CD(ABILITIES)))
#define skill_sets ((const SkillSet *)(igfx + CD(SKILL_SETS)))
#define unit_defs ((const UnitDef *)(igfx + CD(UNIT_DEFS)))
#define UI_PORTRAITS CD(PORTRAITS)
#define UI_MALE CD(MALE)
#define UI_FEMALE CD(FEMALE)
#define UI_TILES CD(TILES)
#define OPENING_RAW CD(UNIT_RAW)
#define OPENING_NAMES CD(UNIT_NAMES)
#define OPENING_WATER CD(WATER)
#define rsm_pre ((const u8 (*)[2])(cdesc + CD(RSM_PRE)))
#define bag_init (cdesc + CD(BAG_INIT))
#define gen_base ((const u8 (*)[5])(cdesc + CD(GEN_BASE)))
#define gen_var ((const u8 (*)[5])(cdesc + CD(GEN_VAR)))
#define ZODIAC_DAY(k) RD16(cdesc + CD(ZODIAC_DAYS), k)
#define zodiac_compat (cdesc + CD(ZODIAC_COMPAT))
#define opening_zodiac (cdesc + CD(UNIT_ZODIAC))
#define opening_range (cdesc + CD(WEAPON_RANGE))
#define opening_evade (cdesc + CD(SHIELD_EVADE))
#define opening_charge_ticks (cdesc + CD(CHARGE_TICKS))
#else
#include "units.h"
#include "weapons.h"
#include "interface.h"
#include "battle.h"
#include "map.h"
#endif
extern const Tile *map_tiles;           // read from ffti (fft.c)
#ifndef BAG_MAX
#define BAG_MAX NBAG
#endif
#ifndef ZODIAC_DAY
#define ZODIAC_DAY(k) zodiac_days[k]
#endif

// Projection (scene pixels): a tile is a 24x12 diamond, one height unit is 6 px.
#define HW 12                          // half tile width
#define HH 6                           // half tile height
#define HU 6                           // pixels per height unit
#define MAXH 10                        // highest corner of the map (checked by the tests)
// Scene: the whole map for one orientation (fftv0..3 and the buffer the current one is copied to)
#define SC_BYTES 40                    // 320 px
#define SC_W (SC_BYTES * 8)
// Per map: its views' rows (SC_H) once the rows above every view's scenery (SC_TOP) are cut.
#ifdef FFT_OPENING
#define SC_H 192                       // exact mesh bottom 190; no cropped walkable terrain
#elif defined(FFT_STORY_GAR)
#ifndef SC_TOP
#define SC_TOP 16                      // Gariland: tools/campaign_assets.py crops and checks them
#endif
#define SC_H (HH * (MAP_W + MAP_H) + HU * MAXH + 8 - SC_TOP)
#else
#define SC_H (HH * (MAP_W + MAP_H) + HU * MAXH + 8)
#endif
#ifndef SC_TOP
#define SC_TOP 0
#endif
#define SC_OY (HU * MAXH + 4 - SC_TOP) // scene y of the top vertex of view tile (0, 0) at h 0
#define SC_PLANE (SC_BYTES * SC_H)     // one plane

// Fixed inspection uses direct cuts. FFT_CAMERA_MOTION explicitly opts into the
// rejected smooth/cinematic research paths; it is never implied by FFT_CINEMA alone.
#if defined(FFT_TEXTURE_PREVIEW) && !defined(FFT_CONFIG)
#define TURN_FRAMES 6                  // PC textured turns: twice the previous duration
#else
#define TURN_FRAMES 3                  // existing portable / TI turn path
#endif
#define WALK_FRAMES 7                  // frames per tile when a unit walks (FFT: 14 at 60 Hz)
#define CLIMB_FRAMES 10                // a step up, down or over a tile (FFT: ~21)
#define WATER_ROWS (UNIT_FOOT - HU)    // depth 1: crop at the projected surface, clear edge above it
#define PATH_MAX 16
#define AI_GLIDE 3                     // AI turns: frames per tile of the cursor gliding to the
#define AI_SHOW 16                     // tile it moves to, then its move range shown; the preview
#define AI_PREVIEW 30                  // on its target, then an ability's name (FFT: ~1 s; all
#define AI_NAME 45                     // slowed down for the TI's small screen, the user's pick)
#define AI_HOLD 48                     // a new AI turn: its unit shown 1.5 s, the world moving,
                                       // before its plan (the user's pick)
#define AI_NONE 0xFD                   // an AI plan without an action
#define ORDER_N 6                      // turns shown in the HUD, the active one first
// the death counter (Unit.dc): 3..0 while KO, then the unit is a crystal or a chest on its tile,
// gone once picked up
#define DC_CRYSTAL 0xFD
#define DC_CHEST 0xFE
#define DC_GONE 0xFF
#define DC_OBJ(d) ((d) >= DC_CRYSTAL)          // out of the battle: its clock stopped
#define EXP_LEVEL 100                  // EXP to the next level
#define PICK_FRAMES 30                 // M_PICK: the message about what was picked up
#define MORPH_FRAMES 32                // visible KO-to-loot dissolve; tactics pause meanwhile
// an attack's frames, FFT's at 30 fps (RE_NOTES.md § Pace): the attacker standing, its wind-up
// from ACT_WIND, the blow from ACT_BLOW, the target hit (its HP lost, the number shown) at
// ACT_HIT, its hit pose for HIT_FRAMES, the number rising NUM_RISE pixels and shown NUM_T frames,
// the EXP the user gained from ACT_HIT + GAIN_T, its JP from ACT_HIT + JP_T (GAIN_N frames
// each), the menu back at ACT_END (ACT_GAIN_END after a gain). Reward display runs
// at twice the original pace; reward amounts and the hit animation are unchanged.
#define ACT_WIND 5
#define ACT_BLOW 13
#define ACT_HIT 16
#define HIT_FRAMES 24
#define FLASH_FRAMES 12                 // light 2, white 8, light 2, then normal
#define BLADE_END (ACT_HIT + 3)         // blade stays through the impact, then put away
#define NUM_RISE 8
#define NUM_T 24
#define GAIN_T 29
#define JP_T 40
#define GAIN_N 9
#define ACT_END (ACT_HIT + 28)
#define ACT_GAIN_END (ACT_HIT + 54)

// M_BROWSE: the cursor free (enemies' turns, or the player looking around); M_MENU: the
// player's turn menu; M_TARGET: the reach shown, a tile to pick; M_WALK; M_FACE: the facing
// chosen at Wait; M_STATUS: a unit's numbers; M_AIM: the weapon's or the ability's reach
// shown, a target to pick; M_ACT: the attack or the ability played; M_ACTS: Act's list (Attack,
// the skill set); M_SKILLS: the abilities the unit knows; M_PICK: a crystal or a chest picked
// up; M_END: the battle over, its results; M_ORDER: the turn order ([APPS]); M_QUIT: "Quit?";
// M_AUTO: Manual / This unit / All allies, reusing the ordinary AI
enum { M_BROWSE, M_TARGET, M_WALK, M_MENU, M_FACE, M_STATUS, M_AIM, M_ACT, M_ACTS, M_SKILLS, M_PICK, M_END,
       M_ORDER, M_QUIT, M_AUTO };
enum { END_NONE, END_VICTORY, END_DEFEAT };
// an AI unit's turn (st.ai): thinking (a few frames), its move range shown, its target shown,
// its facing shown; between them the walk and the act run as a player's (M_WALK, M_ACT)
enum { AI_OFF, AI_THINK, AI_GO, AI_MOVE, AI_AIM, AI_FACE };   // AI_GO: the plan made, shown from
                                                             // the next frame
typedef struct { s16 pri; u8 hit, ab, o, t, f; } AiAct;   // a ranked action: FFT's score, hit
                                                         // chance, ability (NO_ABIL: Attack), the
                                                         // tile it acts from (f: from its own tile,
                                                         // then to o), the target's (z * MAP_W + x)
#define NO_ABIL 0xFF                   // st.abil: the weapon's Attack
#define ABIL_DEFEND 0xFE               // st.abil: Defend (the support ability's command)
enum { SIDE_FRONT, SIDE_SIDE, SIDE_BACK };   // where an attack comes from, for the target
enum { MN_MOVE, MN_ACT, MN_WAIT, MN_STATUS, MN_AUTO, MN_N };   // the turn menu's entries
enum { TEAM_PLAYER, TEAM_ENEMY, TEAM_GUEST };   // a guest (Delita) fights on its own (AI)
#ifdef FFT_OPENING
#define U_RAMZA 0
#else
#define U_RAMZA 6                      // in battle.h's order: Delita, five enemies, Ramza, recruits
#endif
#define U_CHEMIST 8                    // the female Chemist
#define U_HIDE 2                       // the enemy the occlusion scenarios move behind a house
enum { EQ_HEAD, EQ_BODY, EQ_ACC, EQ_WEAPON, EQ_SHIELD, EQ_N };

typedef struct {
    u8 x, z, gfx, team, face;          // face: world +x, -x, +z, -z
    u8 job, level, brave, faith, zodiac;   // job: index in jobs[]; zodiac 0 Aries .. 11 Pisces
    u8 name;                             // generic name index in its gender's disc table
    u8 female, set, learned, status;   // skill set, the abilities it knows (bit k: the set's
                                       // k-th), ST_* bits
    u8 eq[EQ_N];                       // index in items[], 0xFF none
    u16 hp, max_hp, mp, max_mp;
    u8 pa, ma, sp, ct, move, jump;
    u8 dc;                             // death counter: 3, one less per turn at 0 HP, then DC_*
    u8 defend;                         // Defend is its support ability (a command)
    u8 exp, kos, lv0;                  // EXP to the next level; times KO; level at the start
    u16 jp, tjp;                       // its job's JP: left, earned in all (the job level)
    u16 gexp, gjp;                     // EXP and JP earned in this battle (the results)
    u32 raw[5];                        // FFT's raw stats (24 bits): HP MP SP PA MA
} Unit;

typedef struct {
    u8 rot;                            // orientation 0..3 (F5 +1, F1 -1)
    u8 want, prot;                     // the orientation to turn to; the player's (F1 / F5), the
                                       // view back on the player's turns after an AI turned it
    u8 over;                           // [MODE]: the reach tiles drawn over the scenery in front
                                       // of them (else hidden by it but for dashes, as in FFT)
    s8 turn;                           // rotation in progress: +1 / -1, 0 none
    u8 turn_t;                         // its frame, 1..TURN_FRAMES
    u8 cx, cz;                         // cursor, world tile
    u8 mode, sel;                      // M_*, selected unit
    u8 rep;                            // arrow key repeat
    u8 tick;                           // the units' animation clock (frames)
    u8 path_n, path_i, walk_t;         // walking: path, current step, frame in the step
    u8 path[PATH_MAX][2];
    Unit unit[NUNIT];
    s16 camx, camy;                    // camera, scene pixels of the current orientation
    u8 reach[MAP_W * MAP_H];           // M_TARGET: steps to each tile, 0xFF unreachable
    u8 act;                            // the unit whose turn it is
    u8 turns[ORDER_N];                 // the next turns (FFT's AT list), turns[0] = act
    u8 page;                            // Status: stats (0), equipment / abilities (1)
    u8 status_col, status_row, status_info; // equipment/ability selection, open inspection
    u8 menu, back;                     // the menu's entry; the mode M_STATUS, M_ORDER return to
    u8 quit;                           // M_QUIT's entry: 0 Yes, 1 No
    u8 moved, acted;                   // this turn so far
    u8 mx, mz, mface, wface;           // the active unit before its move (undone), at Wait
    u8 atk, tgt, act_t;                // M_ACT: attacker, target, frame
    u8 hit, crit, push, kx, kz;        // its outcome: hit, critical, pushed to (kx, kz)
    u16 dmg;                           // HP taken (the fall's included), healed, or the statuses
                                       // removed (AB_CURE)
    u16 self;                          // HP the user loses (Wish)
    u8 abil, sub, skill;               // the ability (NO_ABIL: Attack); M_ACTS's and M_SKILLS's entries
    u8 bag[BAG_MAX];                   // the party's items
    u8 gain, lvup, jpg;                // M_ACT: the EXP the user earned (0 none), its level up,
                                       // its JP
    u8 pick, pick_t;                   // M_PICK: the item (items[] index; 0xFF: a crystal), frame
    u8 end;                            // END_*
    u8 ai, ai_t;                       // an AI turn: AI_*, its frame
    u8 ai_ab, ai_o, ai_tgt, ai_first;  // its plan: the ability (NO_ABIL, ABIL_DEFEND, AI_NONE), the
                                       // tile to act from, the target's tile, to act before moving
    u8 ai_view;                        // the view it acts in (0xFF: the current one)
    u8 ai_all;                         // the AI plays the player's units too (a test scenario)
    u16 auto_units;                    // player-unit bits delegated to the ordinary AI
    u8 ai_nosup;                       // FFT's sticky flag: the enemies have lost their support
                                       // (fewer than two, or all Critical): no more retreats
    u8 deny;                           // [2nd] on a tile that cannot be picked: the cursor blinks
    u8 rounds;                         // Ramza's turns so far (the results)
#ifdef FFT_EXPLORATION
    u8 perspective, pyaw, pbase, pdist; // exploration camera: 0 off, 1 fixed, 2 lateral, 3 orbit
    s8 ppitch, pside;
    u8 psolid;                         // fixed inspection: 0 actual-pixel auto, 1 solid, 2 dotted
#ifdef FFT_FIXED
    u8 ptrans, ptrans_t;               // 0 idle, 1 enter, 2 exit, 3 close/wide motion
    u8 pzoom, pfrom;                   // remembered zoom (0 close, 1 wide), transient zoom source
#ifdef FFT_CONFIG
    u8 config, config_row, pwire; // pwire: filled untextured mesh, otherwise textures
#endif
#ifdef FFT_CINEMA
    u8 cphase, cframe, cenabled;      // attack camera: enter, action, return; optional at [7]
    u8 cax, caz, cah, cyaw, cdist, ceye, cpitch;
    s8 cside;
    s16 ciso_x, ciso_y;
    u8 ciso_rot;
#endif
#endif
#endif
#ifdef FFT_OPENING
    // Deferred actions are separate from the active unit's turn. Only Archer
    // Charge is currently exposed; Jump and performances need their own rules.
    struct { u8 state, ticks, ability, tile, origin, weapon; } pending[NUNIT];
    u8 open_resolving, open_ct_phase;
    u8 open_stage, open_beat, open_clock, open_white;
    u16 open_reveal, open_offset, open_seen;
    u8 open_pan, open_fade, open_next, open_handoff;
#endif
#if defined(FFT_REACH_CONTOUR) && !defined(FFT_CONFIG)
    u8 config, config_row;
#endif
#ifdef FFT_REACH_CONTOUR
    u8 reach_contour;                  // full movement tiles by default; exterior edges when set
#endif
    u8 banner, banner_t;               // BN_*: the battle's objective, its outcome; its frame
    u16 morph_mask;                    // converted units awaiting their visible transformation
    u8 morph_t;                        // current lowest set bit's animation frame, 0..31
} State;
enum { BN_NONE, BN_START, BN_VICTORY, BN_DEFEAT };
extern State st;
u8 forecast_text(u8 a, u8 t, u8 ab, char *s);   // the prediction's line (fft.c)

// rotation of world tile (x, z) into view tile (u, v) and back, view size
void view_of(u8 rot, s16 x, s16 z, s16 *u, s16 *v);
void world_of(u8 rot, s16 u, s16 v, s16 *x, s16 *z);
u8 view_w(u8 rot);
u8 view_h(u8 rot);
// corner c (0..3: view (0,0) (1,0) (1,1) (0,1)) of view tile (u, v): its height
u8 corner_h(u8 rot, s16 u, s16 v, u8 c);
const Tile *tile_at(s16 x, s16 z);     // world tile or RT_NULL outside
s16 scene_x(u8 rot, s16 u, s16 v);     // scene position of view corner (u, v) at height 0
s16 scene_y(s16 u, s16 v);
u8 unit_at(s16 x, s16 z);              // unit index + 1, 0 = none
const char *unit_name(u8 i);            // fixed heroes, a disc name for each generic unit
void battle_units(void);               // the battle's units drawn and computed (rt_rand)
void unit_stats(Unit *u);              // FFT's stats from the raw ones, the job, the equipment
u8 ct_next(void);                      // FFT's clock to the next turn: its unit
void ct_end(u8 who, u8 moved, u8 acted);   // its CT when the turn ends
void turn_order(void);                 // st.turns from the CTs
void turn_end(u8 moved, u8 acted);     // the active unit's turn over, the next one starts
void compute_reach(u8 who);            // st.reach from unit who (BFS, Move/Jump)
u8 unit_in_water(u8 who);              // grounded in depth 1, including a walking step's position
u8 make_path(u8 who, u8 tx, u8 tz);    // st.path to a reachable tile, its length
// FFT's Attack (RE_NOTES.md § Attack): the tiles a weapon reaches (st.reach 1, else 0xFF), the
// side an attack comes from, the hit chance and the damage (crit: XA + r * XA / 32768, r 0..32767)
void attack_range(u8 who);
u8 attack_side(u8 a, u8 t);
u8 hit_chance(u8 a, u8 t);
u16 attack_damage(u8 a, u8 t, u8 crit, u16 r);
void attack(u8 a, u8 t);               // rolled and started (M_ACT), its outcome in st
// the abilities: a unit's k-th known one (abilities[] index, 0xFF none), whether it can use it
// now (an item left), its reach (st.reach 1), the hit chance, the amount (damage or HP healed;
// r: the random roll 0..32767 of Dash, Throw Stone and Phoenix Down), the ability used
u8 known(u8 who, u8 k);
u8 can_use(u8 who, u8 ab);
void skill_range(u8 who, u8 ab);
u8 skill_hit(u8 a, u8 t, u8 ab);
u16 skill_amount(u8 a, u8 t, u8 ab, u16 r);
void skill(u8 a, u8 t, u8 ab);
void defend(u8 a);                     // Defend: Defending until its next turn
u8 act_kind(void);                     // st.abil's AB_*
// the end of the battle (RE_NOTES.md § KO and the end): EXP and JP for an action that did
// something (kill: it knocked the target out), a level up; the outcome (END_*); what a unit
// stepping on a crystal or a chest gets
void rewards(u8 a, u8 t, u8 kill);
u8 job_level(const Unit *u);
u8 outcome(void);
void pick_up(u8 who, u8 obj);
// FFT's AI (RE_NOTES.md § Enemy AI): unit a's plan made at once (st.ai_ab, st.ai_o, st.ai_tgt,
// st.ai_first; acted: after acting, where to go), the actions it ranked, the tile it falls back
// to, whether the picture stands still (an AI turn thinks then), the facing it takes at Wait
void ai_plan(u8 a, u8 acted);
const AiAct *ai_ranked(u8 *n);
u8 ai_fallback(void);
u8 ai_frozen(void);
#ifndef __m68k__
extern u32 bg_used, bg_bad;            // frames drawn from the screen's copy of the scene, and
#endif                                  // those that differ from the view's (tests)
u8 ai_facing(u8 a);
u8 view_hides(u8 rot, u8 i);           // rows of unit i's sprite view rot hides (4 columns, 0..88):
#ifndef __m68k__                        // from the map's heights; from the current view's depth
u8 view_hides_now(u8 i);
u8 hud_ready(void);                     // every panel and menu due this frame drawn
#endif
u16 scene_checksum(void);              // the scene buffer (tests)
extern u8 *scene_l, *scene_d;          // the scene buffer planes (SC_BYTES x SC_H)
extern u8 scene_rot;                   // orientation it holds (0xFF: stale)

#endif
