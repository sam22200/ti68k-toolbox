// Final Fantasy Tactics, Magic City Gariland: an isometric tech demo on the Portable Game
// Runtime (README.md). Our own engine; the battlefield's heights and its four pictures come
// from the local disc (tools/extract.py -> map.h, fftv0..3), and so do the units (tools/units.py
// -> units.h); the cursor, shadow and rotation greys are ours (tools/art.py).
#ifndef FFT_H
#define FFT_H
#include "../../runtime/core/rt.h"
#include "core_limits.h"
#include "status_policy.h"
#include "status_lifetime.h"
#include "status_control.h"
#include "action_context.h"
#include "reaction_policy.h"
#include "tactical_grid.h"
// FFT_BATTLE: the campaign's battle program, every chapter (campaign/ENGINE.md M4): the chapter
// comes with the scenario number: 100 + door Orbonne, 200 + door Gariland (below 100: the
// door of FFT_DEFAULT_CHAPTER)
#if defined(FFT_BATTLE) && !defined(FFT_DEFAULT_CHAPTER)
#define FFT_DEFAULT_CHAPTER 0
#endif
#if defined(FFT_BATTLE) && !defined(FFT_NO_REACH_CONTOUR)
#ifndef FFT_REACH_CONTOUR
#define FFT_REACH_CONTOUR
#endif
#endif
#ifdef FFT_BATTLE
#define FFT_ACTOR_BANK                // chapter packages: actor frames from the common ffca/ffcb
#define FFT_TURN_LOWPOLY_NATIVE       // same native160x100 geometry as the reviewed camera build
#endif
#ifdef FFT_BATTLE
#include "common.h"
typedef u16 AbId;
typedef u16 LearnBits;
#else
typedef u8 AbId;
typedef u8 LearnBits;
#endif

typedef struct { u8 c[4]; u8 look[5]; u8 walk, stand, water; } Tile;   // map.h, extract.py (walk 2:
                                                                    // crossed, not stopped on)
// battle.h (tools/battle.py): FFT's numbers for the Gariland battle
typedef struct { u8 growth[5], mult[5], move, jump, cev, row; } Job;   // raw stats HP MP SP PA MA; row:
                                                                    // its JP's career slot
typedef struct {
    u8 id, hp, mp, pa, ma, sp, move, jump, wp, knife;
#ifdef FFT_BATTLE
    char name[17];                   // complete source names (longest:15 characters), even record size
#else
    char name[13];
#endif
    u8 reach, sev, pad;
} Item;
// (reach: the weapon's range, sev: a shield's evade: the chapters' shared table, ffcj)
typedef struct {
    u8 gfx, job, type, level, brave, faith;   // type: the generation table (male, female, Ramza)
    u16 day;                                   // birthday 1..365; 0, and Brave / Faith 0: drawn
    u8 x, z, face, team;
    u8 eq[5][2];                               // head/body/accessory/right/left item: one of two
    u8 set;
    LearnBits learned;                         // 0xFFFF on the campaign: drawn
#ifdef FFT_BATTLE
    u16 draw;                                 // source positions drawn at this chapter's start
#endif
} UnitDef;
// An ability (RE_NOTES.md § Abilities): AB_DAMAGE (1..Y times PA, the zodiac, a knockback by
// Brave), AB_CURE (removes the statuses Y, else misses), AB_WISH (the user loses max HP / 5, the
// target heals twice that), AB_HEAL (Y x 10 HP), AB_REVIVE (a KO unit back with 1..Y HP). Range:
// tiles at most `range` steps away (any path), within `vert` half units of the user's height
// (0xFF: any); AF_SELF: the user's own tile too, AF_EVADE: evaded as an attack, AF_ITEM: from
// the bag (slot `bag`; the enemies' items do not run out, those of their level `lv` or less).
enum { AB_DAMAGE, AB_CURE, AB_WISH, AB_HEAL, AB_REVIVE, AB_DEFEND, AB_RAISE, AB_BREAK, AB_NULL, AB_REFLECT, AB_STATUS, AB_MP_DAMAGE, AB_MP_HEAL, AB_CLEANSE, AB_HP_MP_HEAL, AB_HP_DAMAGE_MP_HEAL, AB_DEATH, AB_QUICK, AB_BRAVE, AB_GOLEM, AB_FAITH, AB_SPEED, AB_STEAL_EXP, AB_EQUIP_BREAK, AB_STEAL_EQUIP, AB_STEAL_GIL, AB_THROW_SELECTOR, AB_MATH_SELECTOR, AB_CATCH }; // outcome-only markers
enum { AF_SELF = 1, AF_EVADE = 2, AF_ITEM = 4, AF_WEAPON = 8, AF_DRAIN = 16, AF_AREA = 32, AF_CHARGE = 64, AF_CAST=128 };
#define FORMULA_FORECAST 0x8000U        // Charge query: both hands, potential successful hits
enum { ST_DEFEND = 1, ST_POISON = 2, ST_DARK = 4, ST_SILENCE = 8 };   // Dead: 0 HP
enum { DEFER_NONE, DEFER_CHARGING, DEFER_AIRBORNE, DEFER_PERFORMING, DEFER_CAST_TILE, DEFER_CAST_UNIT };
enum { OPEN_PLAY, OPEN_TITLE, OPEN_CHAPEL, OPEN_GATE, OPEN_AFTER, OPEN_DONE, OPEN_BANTER, OPEN_BEGIN };
#ifdef FFT_BATTLE
typedef struct { u8 kind, y, range, vert, flags, bag, rate, lv; u16 jp; u8 formula, element; } Ability;
#define SET_N 16
#define SKILL_NAME(s) cj_set_name(common, (s))
#define ABIL_NAME(a) cj_battle_name(common, (a))
#define SET_ABILITY(s,k) cj_set_ability(common, (s), (k))
#else
typedef struct { char name[13]; u8 kind, y, range, vert, flags, bag, rate; u16 jp; u8 lv; } Ability;
#define SET_N 6                                // abilities per skill set at most
#define SKILL_NAME(s) (skill_sets[s].name)
#define ABIL_NAME(a) (abilities[a].name)
#define SET_ABILITY(s,k) (skill_sets[s].abil[k])
#endif
typedef struct { char name[12]; u8 abil[SET_N]; } SkillSet;   // abilities[] indices, 0xFF none
#include "art.h"
#ifdef FFT_BATTLE
// Chapter packages: the per-chapter numbers, tables and offsets come from the chapter's
// descriptor (tools/chapter_desc.py, campaign/ENGINE.md), read at load, not from headers.
#define FFT_CHAPTER
#endif
#if defined(FFT_CHAPTER) && !defined(FFT_WATER_DUMP)
#define FFT_WATER_BANK                        // glints read from the descriptor, not computed
#endif
#ifdef FFT_CHAPTER
#ifndef FFT_NAV_SPANS
#define FFT_NAV_SPANS                         // bounded costs1..5; long-link geometry remains separate
#endif
#define FFT_VIEW_SPLIT                        // views: colours into the scene, depths in vbuf
#ifdef FFT_EXPLORATION
#error Perspective inspection is experimental; keep it out of chapter packages.
#endif
#endif
#if defined(FFT_CHAPTER) && defined(__m68k__) && !defined(FFT_NO_REACH_CONTOUR)
#define FFT_TABLES_ARCHIVED                   // constant tables read from the archive tail, not RAM
#endif
#if defined(FFT_CHAPTER) && defined(__m68k__) && !defined(FFT_MODULE)
#define FFT_MODULES                           // parts of the engine in modules (module.h)
#endif
// The parts of the engine kept in modules (module.h): MOD_PART(k) is module k's code (in the
// core when there are no modules; alone in that module's build, FFT_MODULE = k + 1), MOD_CORE
// the rest
#define MOD_AI 0                              // the AI: thinking a step a frame, facing at Wait
#define MOD_TURN 1                            // the rotation animation
#define MOD_SCREENS 2                         // Status and configuration screens
#define MOD_BEATS 3                           // the beats on the battle map (mapbeats.inc)
#define MOD_SAVE 4                            // the campaign save, after a victory (save.inc)
#define MOD_FORMULA 5                          // formula evaluation, also compiled into the AI
#define MOD_AI_SETUP 6                        // candidate preparation/facing, separate from evaluation
#define MOD_RESULTS 7                         // result pages, separate to bound the resident overlay zone
#define MOD_PARTY 16                          // encounter/party initialization, no save writes
#define MOD_ACTION 9                           // primary commit and bounded secondary execution
#define MOD_COMMAND 10                         // command preparation returns before AI continuation
#define MOD_RESPONSE 11                        // cold response packet/stock preparation
#define MOD_TARGETS 12                        // pure ordinary-radial footprint query
#define MOD_AREA_AI 13                        // cached area candidates, separate from scalar AI
#define MOD_AREA_UI 14                        // approved manual area inspection/confirmation
#define MOD_NUMBERS 15                       // shared scalar/area numbers and cold glyph preparation
#define MOD_ORDER 8                          // cold AT forecast; ABI89 balances the archive ranges
#define MOD_PERFORMANCE 17                  // recurring performance commands and mixed effects
#define MOD_MOVEMENT 18                     // cold movement geometry and special destination maps
#define MOD_EQUIPMENT 19                    // cold inventory, derived-stat rebuild and move rewards
#define MOD_COMBAT 20                       // cold command effects and combat responses
#define MOD_NATURE 21                      // discovery, monster capabilities and cold math masks
#define MOD_RESOURCES 26                   // cold immutable chapter bank/reference binding
#define MOD_WEAPON 22                      // weapon impact and origin-independent scalar utility
#define MOD_STATUS 23                      // bitmap commits and explicit status requests
#define MOD_PLANNER 24                     // weapon geometry, recipients, previews and continuation
#define MOD_AI_PREP 25                     // bounded victim/Math/collateral cache preparation
#define MOD_COMPANION 27                   // non-canonical jobs', monsters' and bosses' rules: its own
                                           // heap block, resident while loaded (HW3Patch), never the zone
#define MOD_COUNT 28
#define AI_BAD (-0x7FFF)
#ifdef FFT_MODULE
#define MOD_CORE 0
#define MOD_PART(k) (FFT_MODULE == (k) + 1)
#elif defined(FFT_MODULES)
#define MOD_CORE 1
#define MOD_PART(k) 0
#else
#define MOD_CORE 1
#define MOD_PART(k) 1
#endif
#ifdef FFT_CHAPTER
#include "units.h"                            // (the sprites' numbers: the chapters' data)
// The map's and its views' sizes are the chapter's (descriptor MAP_W, MAP_H, SC_H, SC_TOP), read
// at load; buffers take the largest of every chapter (chapters.h, tools/chapter_desc.py)
#include "chapters.h"
extern u8 map_w, map_h, encounter_units;
extern u16 map_n;
extern s16 sc_h, sc_top;
#define MAP_W map_w
#define MAP_H map_h
#define MAP_N map_n                           // count/endpoints include256; IDs remain bytes
#include "campaign/data/weapons.h"            // blades alone: ffcw (both chapters' copies match)
#include "ui_common.h"                        // ffcu: the interface both chapters share
#include "chapter.h"
#undef ACTOR_ARMED
#undef ACTOR_OBJECTS
#ifdef FFT_CAPACITY_CONTROL11                 // explicit test control; never a package default
#define UNIT_CAPACITY 11
#define NUNIT 11
#else
#define UNIT_CAPACITY FFT_CORE_UNIT_MAX        // bounded storage; encounter may contain fewer
#define NUNIT encounter_units                 // FCD3 validated count, shared by core/modules
#endif
#define BAG_MAX 14                            // Potion..Phoenix Down, source order
#define CD(f) RD16(cdesc, CD_##f)
#define NABIL CJ_EXECUTABLES
#define NSET 224
#define SET_BASIC 5
#define DRAW_BASIC CD(DRAW_BASIC)
#define BAG_SLOT(k) ((cdesc + CD(BAG_ITEM))[k] - 0xF0)
#define NRSM_PRE CD(NRSM_PRE)
#define DEFEND_JP CD(DEFEND_JP)
#define DEFEND_RATE CD(DEFEND_RATE)
#define NBAG CD(NBAG)
#define ACTOR_ARMED CD(ACTOR_ARMED)
#define ACTOR_OBJECTS CD(ACTOR_OBJECTS)
extern const Job *jobs;                       // every chapter's, by FFT job id (ffcj)
extern const Item *items;                     // every chapter's, by FFT item id (ffcj)
extern const char *job_names;                 // 14 bytes a job (ffcj)
extern const u8 *job_sets;                    // a job's skill set, FFT id (ffcj)
extern const u8 *common, *charge_ticks;
#define JOB_NAME(j) (job_names + 14 * (j))
extern const Ability *abilities;
extern const UnitDef *unit_defs;
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
// The chapter's rules (campaign/ENGINE.md M3), in effect when its descriptor has the table:
// fixed signs, a weapon's reach (bows: more than 1), a shield's evade, Charge's clock ticks
#define unit_zodiac (cdesc + CD(UNIT_ZODIAC))
#ifdef FFT_CHAPTER
// A Frog fights bare-handed (source weapon range/strike data): reach 1.
#define UNIT_FROG(u) (((u)->statuses.inflicted[2] | (u)->statuses.innate[2]) & 2)
#define WEAPON_REACH(u) weapon_reach(u)  // hands.inc
#else
#define WEAPON_REACH(u) ((u)->eq[EQ_WEAPON] != 255 ? items[(u)->eq[EQ_WEAPON]].reach : 1)
#endif
#define SHIELD_EV(u) ((u)->eq[EQ_SHIELD] != 255 ? items[(u)->eq[EQ_SHIELD]].sev : 0)
// a sprite's portrait in the chapter's table, whether it is a Chemist's body
#define GFX_PORTRAIT(g) (CD(GFX_PORTRAIT) ? (cdesc + CD(GFX_PORTRAIT))[g] : (g))
#define GFX_CHEMIST(g) ((cdesc + CD(GFX_CHEMIST))[g])
#else
#include "units.h"
#include "weapons.h"
#include "interface.h"
#include "battle.h"
#include "map.h"
#endif
#ifndef UNIT_CAPACITY
#define UNIT_CAPACITY NUNIT                    // historical standalone controls retain their count
#endif
extern const Tile *map_tiles;           // read from ffti (fft.c)
#ifndef BAG_MAX
#define BAG_MAX NBAG
#endif
#ifndef BAG_SLOT
#define BAG_SLOT(k) (k)
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
#ifdef FFT_CHAPTER
#define SC_H sc_h                      // (Orbonne 192, its mesh bottom 190; Gariland cropped)
#define SC_TOP sc_top
#else
#define SC_H (HH * (MAP_W + MAP_H) + HU * MAXH + 8)
#endif
#ifndef SC_TOP
#define SC_TOP 0
#endif
#ifdef FFT_CHAPTER
#define MAP_NMAX FFT_CORE_TILE_MAX
#undef MAP_WMAX
#undef MAP_HMAX
#define MAP_WMAX FFT_CORE_AXIS_MAX
#define MAP_HMAX FFT_CORE_AXIS_MAX
#else
#define MAP_WMAX MAP_W
#define MAP_HMAX MAP_H
#define MAP_N (MAP_W * MAP_H)
#define MAP_NMAX (MAP_W * MAP_H)
#define SC_HMAX SC_H
#define GFX_CHEMIST(g) ((g) >= UG_CHEMIST_M)   // the demo's sprites (units.h)
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
#define AI_HOLD_TICKS 384U             // 1.5 s at 256 Hz, including thinking (the user's pick)
#define AI_HOLD_SKIP 255               // a second plan after acting needs no new turn pause
#ifdef FFT_BATTLE
#define AI_NONE 0xFFFD
#else
#define AI_NONE 0xFD                   // an AI plan without an action
#endif
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
// M_AUTO: Manual / This unit / All allies, reusing the ordinary AI;
// M_CLOCK: no ready actor yet; resume the bounded clock on the next update
enum { M_BROWSE, M_TARGET, M_WALK, M_MENU, M_FACE, M_STATUS, M_AIM, M_ACT, M_ACTS, M_SKILLS, M_PICK, M_END,
       M_ORDER, M_QUIT, M_AUTO, M_CAST_RANGE, M_AREA, M_CLOCK, M_EQUIP };
enum { CT_DUE = 254, CT_WAIT = 255 }; // clock results, never unit indices
enum { END_NONE, END_VICTORY, END_DEFEAT };
// an AI unit's turn (st.ai): thinking (a few frames), its move range shown, its target shown,
// its facing shown; between them the walk and the act run as a player's (M_WALK, M_ACT)
enum { AI_OFF, AI_THINK, AI_GO, AI_MOVE, AI_AIM, AI_FACE };   // AI_GO: the plan made, shown from
                                                             // the next frame
typedef struct { s16 pri; AbId ab; u8 hit, o, t, f; } AiAct;   // a ranked action: FFT's score, hit
                                                         // chance, ability (NO_ABIL: Attack), the
                                                         // tile it acts from (f: from its own tile,
                                                         // then to o), the target's (z * MAP_W + x)
#ifdef FFT_BATTLE
#define NO_ABIL 0xFFFF
// Monster basic physical attacks (Choco Attack 265, Tackle, Bite...: common_tables.monster_attack_record)
#define MONSTER_ATTACK(ab) ((u16)((ab)-0x100)<0x70U&&abilities[ab].formula==1)
#define ABIL_DEFEND 0xFFFE
#define ABIL_FROG 29                   // FFT id: the one ability a Frog may use
#else
#define NO_ABIL 0xFF                   // st.abil: the weapon's Attack
#define ABIL_DEFEND 0xFE               // st.abil: Defend (the support ability's command)
#endif
enum { SIDE_FRONT, SIDE_SIDE, SIDE_BACK };   // where an attack comes from, for the target
enum { MN_MOVE, MN_ACT, MN_WAIT, MN_STATUS, MN_AUTO, MN_N };   // the turn menu's entries
enum { TEAM_PLAYER, TEAM_ENEMY, TEAM_GUEST };   // a guest (Delita) fights on its own (AI)
#ifdef FFT_CHAPTER
#define U_RAMZA CD(RAMZA)              // the chapter's (descriptor)
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
    u8 female, set, status;              // campaign: derived ST_* cache; use status adapters
#ifdef FFT_CHAPTER
    CoreStatus statuses;                 // authoritative40-bit inflicted/innate/immunity sets
    u8 status_ct[STATUS_TIMERS];         // source24..39; slot15 is turn-based, not ticked
    u8 status_flags;                     // low2:source context; upper:derived control/evasion cache
#endif
    LearnBits learned;                // skill set, the abilities it knows (bit k: the set's
                                       // k-th), ST_* bits
    u8 eq[EQ_N];                       // index in items[], 0xFF none
#ifdef FFT_CHAPTER
    u8 cell;                           // canonical compact node; uses the former alignment byte
#endif
    u16 hp, max_hp, mp, max_mp;
    u8 pa, ma, sp, ct, move, jump;
    u8 dc;                             // death counter: 3, one less per turn at 0 HP, then DC_*
    u8 defend;                         // Defend is its support ability (a command)
    u8 exp, kos, lv0;                  // EXP to the next level; times KO; level at the start
#ifdef FFT_CHAPTER
    u8 allegiance;                    // former alignment byte: saved Charm camp and permanent invitation
#endif
    u16 jp, tjp;                       // its job's JP: left, earned in all (the job level)
    u16 gexp, gjp;                     // EXP and JP earned in this battle (the results)
    u32 raw[5];                        // FFT's raw stats (24 bits): HP MP SP PA MA
    u16 sq_jp, sq_tjp;                 // drawn with them, kept for the campaign save: another job's
    LearnBits sq_learned;
    u8 rsm;                            // Squire JP (left, in all) and abilities (bit k: SET_BASIC's
                                       // k-th); the reactions and supports learned (bit k: rsm_pre's
                                       // k-th, 0x80 Defend)
    u8 slot;                           // its record in the campaign save, when the battle took the
                                       // party from it (st.party_in; 0xFF: none)
#ifdef FFT_CHAPTER
    u8 war_trophy,money_modifier;       // live source result fields; removed units contribute none
#endif
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
#ifdef FFT_CHAPTER
    u8 cursor_cell;
#endif
    u8 mode, sel;                      // M_*, selected unit
    u8 rep;                            // arrow key repeat
    u8 tick;                           // the units' animation clock (frames)
    u8 path_n, path_i, walk_t;         // walking: path, current step, frame in the step
#ifdef FFT_CHAPTER
    u8 path[PATH_MAX];                  // compact nodes, including layer identity
#else
    u8 path[PATH_MAX][2];
#endif
    Unit unit[UNIT_CAPACITY];
    s16 camx, camy;                    // camera, scene pixels of the current orientation
    u8 reach[MAP_NMAX];           // M_TARGET: steps to each tile, 0xFF unreachable
    u8 act;                            // the unit whose turn it is
    u8 turns[ORDER_N];                 // the next turns (FFT's AT list), turns[0] = act
    u8 page;                            // Status: stats (0), equipment / abilities (1)
    u8 status_col, status_row, status_info; // equipment/ability selection, open inspection
    u8 menu, back;                     // the menu's entry; the mode M_STATUS, M_ORDER return to
    u8 quit;                           // M_QUIT's entry: 0 Yes, 1 No
    u8 moved, acted;                   // this turn so far
    u8 mx, mz, mface, wface;           // the active unit before its move (undone), at Wait
#ifdef FFT_CHAPTER
    u8 move_cell, push_cell;
#endif
    u8 atk, tgt, act_t;                // M_ACT: attacker, target, frame
    u8 hit, crit, push, kx, kz;        // its outcome: hit, critical, pushed to (kx, kz)
    u16 dmg;                           // HP taken (the fall's included), healed, or the statuses
                                       // removed (AB_CURE)
    // Tagged scalar payload: simultaneous target MP for both paired HP/MP kinds,
    // otherwise caster HP cost (Wish). The response packet preserves its bytes.
    union {u16 self,effect_mp;};
    AbId abil;
    u8 sub, skill;                     // M_ACTS's and M_SKILLS's entries
    u8 bag[BAG_MAX];                   // the party's items
    u8 gain, lvup, jpg;                // M_ACT: the EXP the user earned (0 none), its level up,
                                       // its JP
    u8 pick, pick_t;                   // M_PICK: the item (items[] index; 0xFF: a crystal), frame
    u8 end;                            // END_*
    u8 ai, ai_t;                       // an AI turn: AI_*, its frame
    AbId ai_ab;
    u8 ai_o, ai_tgt, ai_first;         // its plan: the ability (NO_ABIL, ABIL_DEFEND, AI_NONE), the
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
#ifdef FFT_CHAPTER
    // Deferred actions are separate from the active unit's turn: Charge, Jump,
    // casts (explicit tile/unit tags) and recurring performances.
    // tile is a node or CAST_UNIT's actor slot; origin/weapons belong to Charge.
    struct { AbId ability; u8 state, ticks, tile, origin, weapon, off_weapon; } pending[UNIT_CAPACITY];
    struct { AbId ability; u16 roll; u8 actor, target; } formula;
    CoreStatusDelta status_delta;         // one bounded bitmap request; not an event queue
    u16 status_clock;                    // candidates with mutable tick-based status bits
    u8 open_resolving, open_ct_phase;
    u8 affinity[UNIT_CAPACITY][5];       // derived absorb/nullify/half/weak/strengthen; no Unit stride growth
#endif
#ifdef FFT_CHAPTER
    u8 open_stage, open_beat, open_clock, open_white;
    u16 open_reveal, open_offset, open_seen;
    u8 open_pan, open_fade, open_next, open_handoff;
    u8 found[UNIT_CAPACITY], nfound;
    u8 stolen[UNIT_CAPACITY], nstolen; // equipment the player's units stole (items[] index, save.inc)   // the chests' items picked up (items[] index): the campaign's
    u8 party_in;                       // the player's units came from the campaign save (save.inc)
    u8 reward_ready,reward_page,reward_n,reward_scroll,story_done;
    u8 reward_items[UNIT_CAPACITY];
    u8 scenery;                       // bit0:2px edges, bit1:width choice available in this view bank
    u8 weather;                       // effective source weather0..7; adds2 native State bytes with alignment
    u16 golem[2];                     // source g_battle_team_golem: allies, enemies (battle-only)
    u32 reward_base,reward_bonus;
    u16 scene_hide;                    // presentation only; never destroys KO/reward/save state
#endif
#if defined(FFT_REACH_CONTOUR) && !defined(FFT_CONFIG)
    u8 config, config_row;
#endif
#ifdef FFT_REACH_CONTOUR
    u8 reach_contour;                  // full movement tiles by default; exterior edges when set
#endif
    u8 banner, banner_t;               // BN_*: the battle's objective, its outcome; its frame
    u8 dbg;                            // DBG_* (debug_battle.inc; fftmem.py set): the former
                                       // alignment byte before morph_mask, State size unchanged
    u16 morph_mask;                    // converted units awaiting their visible transformation
    u8 morph_t;                        // current lowest set bit's animation frame, 0..31
#ifdef FFT_CHAPTER
    CoreAction action;                // transient root/impact/response ownership; never saved
    u16 reaction[UNIT_CAPACITY];       // equipped source IDs; unsupported families stay inert
    u8 movement[UNIT_CAPACITY];         // source movement ID486..509 encoded1..24; zero:none
    u16 support[UNIT_CAPACITY];        // equipped source IDs, separate from innate job traits
    u8 hand,effect_kind;               // low5: category; status/return/Oil flags above it
    ActionPacket primary;             // primary presentation packet while a response runs
#endif
} State;
enum { BN_NONE, BN_START, BN_VICTORY, BN_DEFEAT };
#define DBG_GOD 1                      // allied blows kill, enemy actions on allies miss
#define DBG_FAST 2                     // AI turns run as with [2nd] held, without the 1.5 s hold
#ifdef FFT_CHAPTER
#include "campaign/save_types.h"
#define BATTLE_STORAGE_BYTES (sizeof(State)+sizeof(Save))
extern State *battle_state;
#define st (*battle_state)
#else
extern State st;
#endif
u8 forecast_text(u8 a, u8 t, AbId ab, char *s);   // the prediction's line (fft.c)

// rotation of world tile (x, z) into view tile (u, v) and back, view size
void view_of(u8 rot, s16 x, s16 z, s16 *u, s16 *v);
void world_of(u8 rot, s16 u, s16 v, s16 *x, s16 *z);
u8 view_w(u8 rot);
u8 view_h(u8 rot);
// corner c (0..3: view (0,0) (1,0) (1,1) (0,1)) of view tile (u, v): its height
u8 corner_h(u8 rot, s16 u, s16 v, u8 c);
const Tile *tile_at(s16 x, s16 z);     // world tile or RT_NULL outside
u16 node_at(s16 x,s16 z,u8 layer);     // temporary256 means absent; node255 remains valid
u8 unit_place(Unit *u,u8 cell);
u8 cursor_place(u8 cell);
u8 unit_at_cell(u8 cell);
#ifdef FFT_CHAPTER
#define UNIT_CELL(u) ((u)->cell)
#define CURSOR_CELL (st.cursor_cell)
#define PATH_CELL(i) (st.path[i])
#define PATH_X(i) (TILE_X[PATH_CELL(i)])
#define PATH_Z(i) (TILE_Z[PATH_CELL(i)])
#else
#define UNIT_CELL(u) ((u8)((u)->z*MAP_W+(u)->x))
#define CURSOR_CELL ((u8)(st.cz*MAP_W+st.cx))
#define PATH_CELL(i) ((u8)(st.path[i][1]*MAP_W+st.path[i][0]))
#define PATH_X(i) (st.path[i][0])
#define PATH_Z(i) (st.path[i][1])
#endif
s16 scene_x(u8 rot, s16 u, s16 v);     // scene position of view corner (u, v) at height 0
s16 scene_y(s16 u, s16 v);
u8 unit_at(s16 x, s16 z);              // unit index + 1, 0 = none
const char *unit_name(u8 i);            // fixed heroes, a disc name for each generic unit
void battle_units(void);               // the battle's units drawn and computed (rt_rand)
void unit_stats(Unit *u);              // FFT's stats from the raw ones, the job, the equipment
u8 ct_next(void);                      // FFT's clock to the next turn: its unit
u8 ct_end(u8 who, u8 moved, u8 acted);     // its CT when the turn ends, then upkeep: 1 a KO
void turn_order(void);                 // st.turns from the CTs
void turn_end(u8 moved, u8 acted);     // the active unit's turn over, the next one starts
void compute_reach(u8 who);            // st.reach from unit who (BFS, Move/Jump)
u8 unit_in_water(u8 who);              // grounded in depth 1, including a walking step's position
u8 make_path(u8 who, u8 tx, u8 tz);    // st.path to a reachable tile, its length
u8 make_path_cell(u8 who,u8 cell);
#ifdef FFT_CHAPTER
u8 battle_line_clear(u8 actor,u8 target);
u8 battle_bind_surface(const u8 *bank,u16 bytes);
u8 battle_bind_grid(const CoreGrid *grid,const Tile *tiles,const u8 *x,const u8 *z);
#endif
// FFT's Attack (RE_NOTES.md § Attack): the tiles a weapon reaches (st.reach 1, else 0xFF), the
// side an attack comes from, the hit chance and the damage (crit: XA + r * XA / 32768, r 0..32767)
void attack_range(u8 who);
u8 attack_side(u8 a, u8 t);
u8 hit_chance(u8 a, u8 t);
u16 attack_damage(u8 a, u8 t, u8 crit, u16 r);
#ifdef FFT_MODULE
static u16 attack_prediction(u8 a,u8 t);
#else
u16 attack_prediction(u8 a,u8 t);
#endif
#ifdef FFT_CHAPTER
#include "cast_batch.h"
#include "area_ai.h"
#include "area_ui.h"
#include "area_impact.h"
u8 battle_weather(u8 strength,u8 flags); // validated boundary; raw0..4, map flags snow/indoors
enum { TARGETS_INVALID=-1,TARGETS_UNSUPPORTED=-2 };
s16 cast_targets(u8 actor,u8 ref,AbId ab,u8 follow); // footprint only; no command eligibility or effects
u8 cast_batch(u8 operation,CastBatch *batch); // gated effect-stage prepare/commit; caller retains storage
u8 cast_forecast(CastBatch *batch);    // pure current effect set, no cost/RNG/command validation
#ifdef FFT_MODULE
static u16 attack_prediction(u8 a,u8 t);
#else
u16 attack_prediction(u8 a,u8 t);
#endif       // shared no-critical primary+offhand forecast
u16 charge_query(u8 a,u8 t,AbId ab,u8 flags,u16 roll); // flags:hand|awake<<1|critical<<2
u8 skill_cost(u8 who,AbId ab);           // pure effective MP cost; no image loads
u16 magic_amount(u8 a,u8 t,u16 spec);    // raw source halfword; element<<8 | power (<128), bit7: no Faith (0x4E)
u16 magic_heal(u8 a,u8 t,AbId ab,u8 query); // pure amount/chance/kind/status; Geomancy4: terrain ability
u8 skill_kind(u8 a,u8 t,AbId ab);        // pure live damage/healing/null outcome
u8 cast_order(u8 who,u8 target,AbId ab,u8 follow); //0 rejected,1 immediate/empty,2 armed
#endif
void attack(u8 a, u8 t);               // rolled and started (M_ACT), its outcome in st
#ifdef FFT_CHAPTER
u8 attack_sequence(u8 a,u8 t,u8 count); // bounded engine API; no Two Swords producer yet
#endif
// A unit's k-th known action (AbId, NO_ABIL when absent), and whether it can use it.
// now (an item left), its reach (st.reach 1), the hit chance, the amount (damage or HP healed;
// r: the random roll 0..32767 of Dash, Throw Stone and Phoenix Down), the ability used
AbId known(u8 who, u8 k);               // k255 queries count in one pass
u8 can_use(u8 who, AbId ab);
void skill_range(u8 who, AbId ab);
u8 skill_hit(u8 a, u8 t, AbId ab);
u16 skill_amount(u8 a, u8 t, AbId ab, u16 r);
void skill(u8 a, u8 t, AbId ab);
void defend(u8 a);                     // Defend: Defending until its next turn
u8 act_kind(void);                     // st.abil's AB_*
// the end of the battle (RE_NOTES.md § KO and the end): EXP and JP for an action that did
// something (kill: it knocked the target out), a level up; the outcome (END_*); what a unit
// stepping on a crystal or a chest gets
void rewards(u8 a, u8 t, u8 kill);
#ifdef FFT_CHAPTER
// Resident-only entry; overlays use their local rules. Returns accepted-mask sum
// (zero:no effect), or STATUS_REJECTED. Targets must be living/deployed;
// removal-only requests cannot add a status. KO/revival has its own lifecycle.
u16 battle_status_apply(u8 target,const CoreStatusDelta *delta,u8 removal_only);
#endif
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
