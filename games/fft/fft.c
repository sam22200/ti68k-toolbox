// Final Fantasy Tactics, Magic City Gariland: isometric tech demo (README.md for the design).
// World tiles (x, z) are rotated into view tiles (u, v) for each of the four orientations; the
// scene of each orientation is the game's own textured map drawn on the PC (tools/extract.py)
// with the depth of its pixels, four ZX0-packed data files read in place. The current one is
// unpacked (during the turn that reaches it) and copied into a buffer larger than the screen
// (the reach rings are drawn there); every frame copies the camera's window and draws the
// units through a cover mask made from that depth.
// The rotation itself is animated with polygons projected at intermediate angles, each face in
// the mean grey of its pixels in the views (map.h).
#include "fft.h"
#include "ct_recip.h"
#include <stdlib.h>
#include <string.h>
#ifdef FFT_REACH_CONTOUR
#include "contour.h"
#if MOD_CORE
static const u8 *contour_gfx;            // masks appended to the existing first view bank
#endif
#endif
#if defined(__m68k__) && defined(FFT_CONFIG) && !defined(RT_CYCLES)
#include <alloc.h>
#endif

#include "base.inc"

#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#include "contour_art.inc"
#define ct_recip CONTOUR_REF_ct_recip           // archived; the headers' copies go unused
#define weapon_x CONTOUR_REF_weapon_x
#endif

#ifdef RT_CYCLES                        // zones under ti-cycles: 3 view copy, 4 units, 5 cursor
#include "../../tools/m68kbench/bench.h" // and HUD, 6 scene, 7 rotation frame, 8 view unpacking,
                                         // 9 an AI unit's thinking (a frame's step), 10 the new reach rings,
                                         // 11 a menu part, 12 a panel part, 13 the prediction,
                                         // 14 a banner's glyph
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif

#if defined(FFT_MEASURE) && defined(RT_CYCLES)
static u32 frame_begin;
static u8 frame_stall;
#ifdef FFT_CINEMA
#define FRAME_SHOT() BENCH_SHOT(rt_dark)
#else
#define FRAME_SHOT()
#endif
#ifdef FFT_OPENING
#define FRAME_MODE() (st.open_stage ? st.open_stage + 32 : st.mode)
#else
#define FRAME_MODE() st.mode
#endif
#define FRAME_BEGIN() (frame_begin = BENCH_CYCLES, frame_stall = st.turn != 0)
// Keep profiling below the AMS 64K variable ceiling. All render exits share
// the marker code instead of inlining it repeatedly; production is unaffected.
static void __attribute__((noinline)) frame_end(void)
{
    BENCH_VALUE(((u32)frame_stall << 31) | ((u32)FRAME_MODE() << 24) | ((u32)st.act_t << 16) | (u16)rt_frame);
    BENCH_VALUE(BENCH_CYCLES-frame_begin);FRAME_SHOT();
}
#define FRAME_END() frame_end()
#else
#define FRAME_BEGIN()
#define FRAME_END()
#endif

#if defined(FFT_OPENING) || defined(FFT_STORY_GAR)
#define AB_JP(b) RD16((const u8 *)(b)+20,0)
#else
#define AB_JP(b) ((b)->jp)
#endif

#if MOD_CORE                            // a module reaches them through its context (module.h)
State st;
u8 *scene_l, *scene_d;
u8 scene_rot;
#endif
// the four views (fftv0..3, ZX0, read in place), unpacked one at a time into vbuf: light and
// dark planes, then twice (the frontmost pixels of each byte, then the next ones) their view
// diagonal and their mask (SC_PLANE each)
#ifdef FFT_VIEW_SPLIT                   // the colours live in the scene buffer only
#define VIEW_BYTES (4UL * SC_PLANE)
#define VDEPTH vbuf                     // depth, mask, depth, mask
#else
#define VDEPTH (vbuf + 2 * SC_PLANE)
#define VIEW_BYTES (6UL * SC_PLANE)          // 52,320 (Gariland 48,480): above a 16-bit int
#endif
#if MOD_CORE
static const u8 *views[4];
static const u8 *ugfx;                  // the units' frames (fftu, units.h), read in place
static const u8 *igfx;                  // portraits, names, marker and text (ffti)
#endif
#ifdef FFT_ACTOR_BANK
#define ACTOR_SLOTS (NUNIT + 2)          // units, an object, a blade
#define ACTOR_UNITS NUNIT
#define ACTOR_KEY(logical) RD16(ugfx, logical)   // ffoa/ffga: logical frame -> bank frame
#include "actors.h"
#endif
#ifdef FFT_CHAPTER
#if MOD_CORE
static const u8 *cdesc;                 // the chapter's descriptor (ffod, ffgd: chapter.h)
const Job *jobs; const Item *items; const Ability *abilities;   // its tables in igfx
const SkillSet *skill_sets; const UnitDef *unit_defs;
static const u8 *cgfx;                  // the interface both chapters share (ffcu, ui_common.h)
#endif
#define UI_SHARED(s) (cgfx + UIC_##s)
#else
#define UI_SHARED(s) (igfx + UI_##s)
#endif
#if MOD_CORE
const Tile *map_tiles;                  // in ffti too: the map's tiles (extract.py) and the sloped
static const PlaneWord *sl_l, *sl_m, *sc_l, *sc_m;   // markers' planes (art.py), out of the program
#endif
#define sl_d sl_l                       // black and white: one plane for both
#define sc_d sc_l
#ifndef __m68k__
static u32 slope_pc[4 * SLOPE_ROWS];    // the PC's native copy (ffti is big-endian)
#endif
#if MOD_CORE
static const u8 *wgfx;                  // armed body poses and blades (fftw), read in place
static u8 *vbuf, vbuf_rot;
#endif

#define BK_N 128                        // FFT_VIEW_SPLIT: rows kept (a reach touches up to 126)
// Work memory, allocated with the scene (globals and statics are stored in the program file
// on the TI: -mno-bss, and the TI-89 refuses programs above 24 KB)
#define TW (RT_W / 2)                   // the rotation frames: half resolution
#define TH (RT_H / 2)
#define COVER_H UNIT_SH                // the shadow lies inside the sprite's box
#ifdef FFT_OPENING
#define GLINTS 64                      // palette-animated water, no traversable canal immersion
#else
#define GLINTS 250                      // glints per phase at most
#endif
#define HUD_ROWS 14                     // scratch rows for outlined action numbers (8-13)
#define PANEL_ROWS 29                   // portrait sets the height; level overlays its lower corner
#define PANEL_Y (RT_H - PANEL_ROWS)
#define FC_ROWS 9                       // the prediction: a white box, black frame, small font
#define PANEL_GAUGE_W 40                // MP + gap + 999/999 in the existing small font
#define TARGET_STATS_X (61 - PANEL_GAUGE_W - 2) // rounded gauge ends against the target portrait
#define WIN_ROWS 47                     // bottom-right menu / preview: tab plus six lines
typedef struct {                        // a unit's cover mask, and its covered rows r0..r1
    s16 x, y;                           // among 2 .. UNIT_SH - 3 (r0 > r1: none)
    u8 depth, rot, r0, r1, wet;
    u16 cm[COVER_H];
} Cover;
typedef struct { s16 x, y; u8 depth; } Feet;   // a unit's feet (unit_feet)
typedef struct { u8 *l, *d; s16 stride, ox, oy, x0, x1, y0, y1; } Dst;   // a drawing target
typedef struct {
#ifdef FFT_ACTOR_BANK
    ACTOR_FIELDS
#endif
#ifdef FFT_OPENING
    char dialog[6][48];
    u8 dialog_n, dialog_w, dialog_h, dialog_next;
    u16 dialog_total, dialog_key, dialog_drawn;
    u8 dialog_white, dialog_part, dialog_face;
#endif
    Cover cover[NUNIT];                 // per unit, for its feet, its depth and the view
    u32 wcover[WEAPON_SH];              // the blade's cover, kept while its anchor stands
    s16 wx, wy;
    u8 wrot, wdepth;
#ifndef FFT_COMPACT_UI
    u8 blade_k[3];                      // one unique frame + 1 per swing phase; zero invalid
    u32 blade[3][3][WEAPON_SH];         // three decoded blades, big-endian bytes on both hosts
#endif
    Feet feet[NUNIT];                   // its feet (unit_feet) for its tile and the
    u16 feet_k[NUNIT];                  // view (x | z << 8 | rot << 12 | 0x8000; 0: none)
#ifdef FFT_TURN_LOWPOLY_NATIVE
    s16 span_l[RT_H + 8], span_r[RT_H + 8];
#else
    s16 span_l[TH + 8], span_r[TH + 8];
#endif
    u8 from[MAP_W * MAP_H];
    u8 half[2][TW / 8 * TH];
#ifndef FFT_TABLES_ARCHIVED
    u16 dbl[256];                       // byte -> its bits doubled
#endif
    u8 order[MAP_W * MAP_H], count[256];
#ifdef FFT_WATER_BANK
    const u8 *puddle[4];                // archived even-x/2,y pairs; no decoded glint arrays
#else
    struct { s16 x, y; } glint[4][GLINTS];  // the water's glints of each phase (scene pixels)
#endif
    u8 glints[4];
    s16 key[MAP_W * MAP_H], gx[MAP_W + 1][MAP_H + 1], gy[MAP_W + 1][MAP_H + 1];
#ifdef FFT_COMPACT_UI
    // No backing tactical panels in the bounded camera renderer. Its full-UI render
    // entry point is excluded below; legacy panel helpers are never called.
    u16 (*hud)[HUD_ROWS * RT_PBYTES / 2];
    u16 (*win)[WIN_ROWS * RT_PBYTES / 2];
#else
    u16 hud[2][HUD_ROWS * RT_PBYTES / 2];   // outlined panel text (0-6) / action numbers (7-13)
    u16 win[2][WIN_ROWS * RT_PBYTES / 2];   // bottom-right menu / preview cache
#endif
    u8 win_k[3], win_w, win_h, win_n, win_m;   // mode, moved | acted << 1 (M_ACTS: no ability
                                        // known; M_ACT: the ability), unit;
                                        // its width in words, its rows; its part to draw next
                                        // its parts (the box, a line each; win_n == win_m: drawn)
#ifdef FFT_COMPACT_UI
    u16 (*forecast)[FC_ROWS * RT_PBYTES / 2];
#else
    u16 forecast[3][FC_ROWS * RT_PBYTES / 2]; // the prediction's box, 160 wide, above the arrow
#endif
    u16 pan_k[2][6], pan_fk[2][6];      // unit, HP, MP, level, maxima: being made, shown
    u16 fc_key;                         // the forecast made (target | ability << 8)
    u8 pan_n[2], pan_f[2], pan_ok[2];   // parts made (cutout portrait, name, HP, MP, level), the
                                        // buffer shown, one shown
    u8 tag_w[2], tag_ok;                // the tags' widths, made
    u8 hud_ok;                          // every panel and menu due this frame drawn (tests)
    u8 arrow[4][2][8];
#if defined(FFT_OPENING) || defined(FFT_STORY_GAR)
    // Status/dialogue storage aliases the idle tactical portrait cache below.
#elif defined(FFT_COMPACT_UI)
    u16 status[2][800];                 // fixed inspection FWork only; no full-screen Status
#else
    u16 status[2][RT_PSIZE / 2];        // full-screen Status, assembled a line per frame
#endif
    u8 status_i, status_page, status_n;
    u8 status_col, status_row, status_info;
    s16 ring_y0, ring_y1;               // the scene rows the reach rings are on
    s16 drop_y0, drop_y1;               // the rows still to put back (rings going)
    u8 from_who;                        // whose reach W->from holds the steps of (+ 1; 0: none)
    u8 ring_i;                          // next reach tile; MAP_W * MAP_H: all drawn
    u8 ring_bits[(MAP_W * MAP_H + 7) / 8], ring_style; // last completed visual reach, not its costs
    u32 cur_m[24];                      // the cursor's mask, dashed where the view is in front
    u16 cur_k;                          // of it, for this tile | rot << 8 | 0x8000 (0: none)
    u32 num[3][2][7];                   // an attack's number (or Miss), the EXP and the JP its user
    u8 num_w[3], num_ok;                // earned: black, its white outline; their widths; drawn (bits)
    u8 end_n;                           // M_END: the results' next line to draw
    u8 bn_end, bn_t0[2];                // banner end and each line's first reveal frame
    u16 morph_g[4][UNIT_SH];           // one animated KO/object frame, shared across draws
    u8 dcg[4][2][7];                    // the death counter's digits: black, mask (8 wide)
    // the movement spread (spread_start, spread_go): its tiles to go on from by cost & 3, where it
    // stands; the mover's enemies' tiles; each tile's x, z, standing height, height out of it
    // (water: its surface), 1 walkable | 2 water; the moves out of each tile (edges()), for Jump ej
    u8 sq[4][MAP_W * MAP_H], sq_n[4];
    u8 sp_d, sp_i, sp_limit, sp_out, sp_left, *sp_dist, *sp_from;
    const u8 *sp_need;
    u8 blk[MAP_W * MAP_H];
#ifndef FFT_TABLES_ARCHIVED
    u8 tx[MAP_W * MAP_H], tz[MAP_W * MAP_H];
#endif
    u8 hs[MAP_W * MAP_H], hl[MAP_W * MAP_H], hw[MAP_W * MAP_H];
    u8 eg[MAP_W * MAP_H][16], en[MAP_W * MAP_H], ej, ej_pad;
#ifdef FFT_OPENING
    u16 top[MAP_W * MAP_H];             // map-56 towers exceed 255 source height units
#else
    u8 top[MAP_W * MAP_H];              // a tile's column for a projectile (FFT: its height x 12);
#endif
#ifndef FFT_TABLES_ARCHIVED
    u8 col[MAP_W * 28], row[MAP_H * 28];    // world x, z -> the tile's column, z * MAP_W
#endif
    // the AI (fft.c § the AI): per tile the sets (1: can stop there, 2: to act from), the danger,
    // the steps from the focus, what stepping there gives; the tiles to act from
    // in map order and safest first; the ranking; the usable abilities, the unit's thinking step
    u8 ai_m[MAP_W * MAP_H], ai_prox[MAP_W * MAP_H], ai_dist[MAP_W * MAP_H];
    u8 ai_tt[NUNIT], ai_tu[NUNIT];      // an ability's targets (but the actor): their tiles, units
    u8 ab, ab_k, ab_n, ab_all, ai_traj; // the ability being tried: its next origin, its targets,
    u16 ab_done;                        // no target used up, a trajectory just checked; the
    AiAct ab_best;                      // targets used up, its best so far
    s16 ai_gain[MAP_W * MAP_H], ai_key[MAP_W * MAP_H];   // what stepping there gives, its key
    u8 ai_grid[MAP_W * MAP_H], ai_ord[MAP_W * MAP_H], ai_no;
    AiAct ai_rank[8];
    u8 ai_n, ai_list[1 + SET_N], ai_nab, ai_k, ai_a, ai_self, ai_focus, ai_c0, ai_c1, ai_retreat;
    u8 ai_er, ai_ar, ai_ranged;         // its shortest reach on an enemy, an ally; a ranged ability
    u16 ai_use;                         // the units the ability is worth using on (bits)
    s16 ai_base;                        // the fallback tile's score: to beat
    u8 frz, frz_n, hold;                // a unit thinking: the picture stands still (no longer
    u32 frz_h;                          // drawn), its frames so far, one more frame not drawn,
    u32 bg[2][RT_H * RT_W / 32];        // the scene as the screen shows it (copy_view's), from
    s16 bg_x, bg_y;                     // this camera and scene version (bg_ok): copied to the
    u16 bg_ver;                         // screen without shifts while the camera stands still;
    u8 bg_ok, bg_part, bg_drawn;        // valid cache, next quarter, actually used this frame
    u8 drop_wait, hud_wait, bg_wait;    // frames the rings going, a HUD part, the copy, the cursor's
    u8 cur_wait;                        // dashes waited for room
    s16 left, upd_k;                    // the frame's budget left for optional work, the update's
                                        // heavy work this frame (k cycles)
                                        // the last one's checksum
    // last: the big HUD caches (the fields above keep 16-bit offsets)
#ifdef FFT_COMPACT_UI
    u16 (*pan)[2][3][PANEL_ROWS * RT_PBYTES / 2];
    u16 (*tag)[3][7 * RT_PBYTES / 2];
#else
#if defined(FFT_OPENING) || defined(FFT_STORY_GAR)
    union {
        u16 status[2][RT_H * RT_PBYTES / 2];
        u16 pan[2][2][3][PANEL_ROWS * RT_PBYTES / 2];
    };
#else
    u16 pan[2][2][3][PANEL_ROWS * RT_PBYTES / 2]; // per side, two 80-wide planes and opacity masks:
                                        // the one shown, the one being made (scene shows through)
#endif
    u16 tag[2][3][7 * RT_PBYTES / 2];   // "Enemy" / "Guest" above a portrait, made once
#endif
#ifdef FFT_VIEW_SPLIT
    // The view's colours of the scene rows the reach rings were drawn on, kept before a ring
    // changes them (the colours are no longer kept whole: VIEW_BYTES)
    u8 bk_of[SC_H];                     // a row's slot (0xFF: the row is still the view's)
    u8 bk_row[BK_N], bk_n, bk_over;     // each slot's row; slots used; rings past the backup
    u8 bk[BK_N][2][SC_BYTES];
#endif
} Work;
#if MOD_CORE
static Work *W;
#endif
#if defined(FFT_MODULES) || defined(FFT_MODULE)
#include "module.h"
#endif
static u8 bg_usable(void);

// A frame's budget, in thousands of cycles (ti-cycles): what a frame costs without its optional
// work is reckoned before drawing (the units on screen, their cover masks to make, the copy of
// the scene); the optional work (new rings, a part of the HUD, the screen's copy kept) waits for
// a frame with room for it: it shows a frame or two later, the frame stays within the budget
#define FRAME_K 332
#define BASE_K 65                       // glints, HUD copies, cursor, the units' sort
#ifdef FFT_OPENING
#define UNIT_K 16                       // dry-body shader; pose decoding charged separately
#else
#define UNIT_K 20                       // a unit on screen, including its water check / surface edge
#endif
#define COVER_K 16                      // its cover mask made again
#define BG_K 25                         // the screen's copy of the scene put back
#define VIEW_K 105                      // the view copied (the camera moved)
#define RELOAD_K 800                    // FFT_VIEW_SPLIT: the colours decoded again (16 KB ZX0)
#define RING_K 30                       // a new flat ring (12 rows; a sloped one by its rows): taken
                                        // from the budget, the other optional work waits
#define ROW_K 2                         // a scene row put back (rings going), and on the screen's copy
#define HUD_K 60                        // a part of the HUD made
#define DK_K 80                         // a banner's shade (ti-cycles: up to 80k, its edge rows)
#define DK_SPEED 8                     // the shade's diagonal edge (x + y), pixels a frame
#define DK_N ((RT_W + RT_H) / DK_SPEED + 1)   // its frames, coming then leaving
#define SAVE_K 13                       // a quarter of the screen's copy kept (halves, 26k,
                                        // went over the budget once forced)
#define SAVE_WAIT 2                     // frames the copy waits at most once the camera stops,
#define WAIT_MAX 8                      // the rings going or a HUD part (then done anyway)
#define SUM_K 25                        // an AI unit's picture checked (planes_sum)
#define CUR_K 15                        // the cursor's dashes made again (8-19k)
#define TURN_K 45                       // the update's own heavy work: a turn's start (the clock,
#define AIM_K 35                        // the AT list), an aim (the reach, the preview), a Move's
#define REACH_K 90                      // reach, an AI unit's facing (W->upd_k)
#define FACE_K 50
#define span_l (W->span_l)
#define span_r (W->span_r)
#define bfs_from (W->from)
#define half (W->half)
#ifdef FFT_TABLES_ARCHIVED                 // in the archive tail (tools/contour_assets.py)
#define dbl CONTOUR_REF_dbl
#define TILE_X CONTOUR_REF_tile_x
#define TILE_Z CONTOUR_REF_tile_z
#define TILE_COL CONTOUR_REF_tile_col
#define TILE_ROW CONTOUR_REF_tile_row
#else
#define dbl (W->dbl)
#define TILE_X (W->tx)
#define TILE_Z (W->tz)
#define TILE_COL (W->col)
#define TILE_ROW (W->row)
#endif
#define order (W->order)
#define key (W->key)
#define gx (W->gx)
#define gy (W->gy)
#define HUD_L ((u8 *)W->hud[0])
#define HUD_D ((u8 *)W->hud[1])

#if MOD_CORE                            // a module's build (module.h): only its own part
// ---------------------------------------------------------------- the map in four orientations
u8 view_w(u8 rot) { return rot & 1 ? MAP_H : MAP_W; }
u8 view_h(u8 rot) { return rot & 1 ? MAP_W : MAP_H; }

void view_of(u8 rot, s16 x, s16 z, s16 *u, s16 *v)
{
    switch (rot) {
    case 0: *u = x; *v = z; break;
    case 1: *u = MAP_H - 1 - z; *v = x; break;
    case 2: *u = MAP_W - 1 - x; *v = MAP_H - 1 - z; break;
    default: *u = z; *v = MAP_W - 1 - x; break;
    }
}

void world_of(u8 rot, s16 u, s16 v, s16 *x, s16 *z)
{
    switch (rot) {
    case 0: *x = u; *z = v; break;
    case 1: *x = v; *z = MAP_H - 1 - u; break;
    case 2: *x = MAP_W - 1 - u; *z = MAP_H - 1 - v; break;
    default: *x = MAP_W - 1 - v; *z = u; break;
    }
}

const Tile *tile_at(s16 x, s16 z)
{
    if ((u16)x >= MAP_W || (u16)z >= MAP_H) return RT_NULL;
    return &map_tiles[z * MAP_W + x];
}

// view corner c of a tile is world corner (c - rot) & 3 (both numbered (0,0) (1,0) (1,1) (0,1))
u8 corner_h(u8 rot, s16 u, s16 v, u8 c)
{
    s16 x, z;
    if ((u16)u >= view_w(rot) || (u16)v >= view_h(rot)) return 0;
    world_of(rot, u, v, &x, &z);
    return map_tiles[z * MAP_W + x].c[(c - rot) & 3];
}

s16 scene_x(u8 rot, s16 u, s16 v) {
#ifdef FFT_SOURCE_CAMERA
    return SC_W - (s16)((u - v + view_h(rot)) * HW);
#else
    return (s16)((u - v + view_h(rot)) * HW);
#endif
}
s16 scene_y(s16 u, s16 v) { return (s16)(SC_OY + (u + v) * HH); }

u8 unit_at(s16 x, s16 z)
{
    u8 i;
    for (i = 0; i < NUNIT; i++)
        if (st.unit[i].x == x && st.unit[i].z == z && !DC_OBJ(st.unit[i].dc)) return i + 1;
    return 0;
}

// ---------------------------------------------------------------- the battle's units
// FFT's model (tools/battle.py, checked on the original at Ramza's first turn): raw stats of 24
// bits drawn from the unit type's base and variance, grown per level, times the job's
// multiplier; then the equipment; the abilities it knows: its job's JP (100..199 at level 1)
// spent in its skill set's order on each ability whose learn rate roll passes. Once per
// battle: the 32-bit divisions do not matter.
// Our own 32-bit xorshift for the battle's draws, seeded from the runtime's: the runtime's
// wyhash16 (a 16-bit state) ties two draws in a row (after a 4 % critical roll its next draw,
// the critical's bonus, was never 0: criticals seen 3.8 % for the original's 2.0 %)
static u32 rng32 = 1;
static void rand_seed(u16 s) { rng32 = 0x2545F491UL ^ ((u32)s << 16 | s); }
static u16 rand15(void)                                   // 0..32767, as the PS1's rand()
{
    rng32 ^= rng32 << 13; rng32 ^= rng32 >> 17; rng32 ^= rng32 << 5;
    return (u16)(rng32 >> 17);
}

void unit_stats(Unit *u)
{
    const Job *j = &jobs[u->job];
    u16 s[5];
    u8 i, k;
    for (i = 0; i < 5; i++) {
        u32 v = u->raw[i];
        u8 g = j->growth[i] ? j->growth[i] : 1;
        for (k = 2; k <= u->level; k++) v += v / (u16)(g + k - 1);
        if (v > 0xFFFFFFUL) v = 0xFFFFFFUL;
        v = v * j->mult[i] / 100 >> 14;
        s[i] = v ? (u16)v : 1;
    }
    u->max_hp = s[0] > 999 ? 999 : s[0];
    u->max_mp = s[1] > 999 ? 999 : s[1];
    u->move = j->move; u->jump = j->jump;
    for (i = 0; i < EQ_N; i++) {                   // helmet / armour HP and MP, item attributes
        const Item *it;
        if (u->eq[i] == 0xFF) continue;
        it = &items[u->eq[i]];
        u->max_hp += it->hp; u->max_mp += it->mp;
        s[2] += it->sp; s[3] += it->pa; s[4] += it->ma;
        u->move += it->move; u->jump += it->jump;
    }
    if (u->max_hp > 999) u->max_hp = 999;
    if (u->max_mp > 999) u->max_mp = 999;
    if (u->jump > 7) u->jump = 7;
    u->sp = (u8)(s[2] > 50 ? 50 : s[2]);
    u->pa = (u8)(s[3] > 99 ? 99 : s[3]);
    u->ma = (u8)(s[4] > 99 ? 99 : s[4]);
    u->hp = u->max_hp; u->mp = u->max_mp;
}

#ifdef FFT_OPENING
static void opening_units(void);
static void opening_start(u16 n);
static u8 map_update(void);
static void map_render(void);
static u8 opening_attack_range(u8 a, u8 o, u8 c);
static void opening_damage(void);
static u8 opening_hit(u8 a,u8 t);
static void opening_effect(void);
static u8 opening_pending(void);
static void opening_resolve(void);
#endif

void battle_units(void)
{
#ifdef FFT_OPENING
    opening_units();
#else
    u8 i, k;
    for (i = 0; i < NUNIT; i++) {
        const UnitDef *d = &unit_defs[i];
        Unit *u = &st.unit[i];
        u16 day = d->day;
#ifdef FFT_STORY_GAR
        day=RD16((const u8*)d+6,0);
#endif
        u->x = d->x; u->z = d->z; u->gfx = d->gfx; u->team = d->team; u->face = d->face;
        u->job = d->job; u->level = d->level; u->ct = 0; u->dc = 3; u->female = d->type == 1;
        u->brave = d->brave ? d->brave : (u8)(45 + ((30UL * rand15()) >> 15));
        u->faith = d->faith ? d->faith : (u8)(45 + ((30UL * rand15()) >> 15));
        if (!day) day = (u16)(1 + ((365UL * rand15()) >> 15));
        for (k = 0; k < 12 && day >= ZODIAC_DAY(k); k++) ;
        u->zodiac = (u8)((k + 9) % 12);
        for (k = 0; k < 5; k++)
            u->raw[k] = ((u32)gen_base[d->type][k] << 14) + (((u32)rand15() * gen_var[d->type][k]) >> 1);
        for (k = 0; k < EQ_N; k++)                 // one of the two candidates (FFT: of n)
            u->eq[k] = d->eq[k][d->eq[k][1] != 0xFF ? rand15() >> 14 : 0];
        unit_stats(u);
    }
    for (i = 0; i < NUNIT; i++) {                  // drawn after the stats (theirs kept per seed)
        const UnitDef *d = &unit_defs[i];
        Unit *u = &st.unit[i];
        u->set = d->set; u->status = 0; u->learned = d->learned;
        if (d->learned == 0xFF) {                  // its job's JP, then each ability in order:
            u16 jp = (u16)(100 + ((100UL * rand15()) >> 15));   // its learn rate, if JP are left
            const u8 *a = skill_sets[d->set].abil;
            u->learned = 0; u->tjp = jp;
            for (k = 0; k < SET_N && a[k] != 0xFF; k++)
                if (((100UL * rand15()) >> 15) < abilities[a[k]].rate && jp >= AB_JP(&abilities[a[k]])) {
                    jp -= AB_JP(&abilities[a[k]]); u->learned |= (u8)(1 << k);
                }
            u->jp = jp;
        }
    }
    for (i = 0; i < NUNIT; i++) {                  // then the JP of those whose abilities are
        const UnitDef *d = &unit_defs[i];          // fixed (Ramza), the EXP (ENTD: 0..99; a new
        Unit *u = &st.unit[i];                     // game's Ramza 0)
        if (d->learned != 0xFF) u->jp = u->tjp = (u16)(100 + ((100UL * rand15()) >> 15));
        u->exp = d->type == 2 ? 0 : (u8)((100UL * rand15()) >> 15);
        u->kos = 0; u->lv0 = u->level; u->gexp = u->gjp = 0;
    }
    for (i = 0; i < NUNIT; i++) {                  // the support ability (not a new game's Ramza):
        Unit *u = &st.unit[i];                     // Defend bought with the Squire job's JP after
        u16 jp = u->jp;                            // the reactions and supports listed before it,
        u->defend = 0;                             // then one of the two it knows
        if (unit_defs[i].type == 2) continue;
        if (jobs[u->job].row) {                    // another job: the Squire's JP and abilities
            const u8 *a = skill_sets[SET_BASIC].abil;
            jp = (u16)(100 + ((100UL * rand15()) >> 15));
            for (k = 0; k < SET_N && a[k] != 0xFF; k++)
                if (((100UL * rand15()) >> 15) < abilities[a[k]].rate && jp >= AB_JP(&abilities[a[k]]))
                    jp -= AB_JP(&abilities[a[k]]);
        }
        for (k = 0; k < NRSM_PRE; k++)
            if (((100UL * rand15()) >> 15) < rsm_pre[k][1] && jp >= rsm_pre[k][0]) jp -= rsm_pre[k][0];
        if (((100UL * rand15()) >> 15) < DEFEND_RATE && jp >= DEFEND_JP) u->defend = !(rand15() >> 14);
    }
    {
        u16 seed = (u16)(rt_seed ^ 0x5A17);
        for (i = 0; i < NUNIT; i++) {
            u8 j, clash;
            if (st.unit[i].gfx <= UG_DELITA) continue;
            do {
                seed = (u16)(seed * 109U + 89U);
                st.unit[i].name = (u8)seed;
                if (st.unit[i].name == 255) st.unit[i].name = 0;
                clash = 0;
                for (j = 1; j < i; j++) if (st.unit[j].gfx > UG_DELITA && st.unit[j].female == st.unit[i].female
                    && st.unit[j].name == st.unit[i].name) clash = 1;
            } while (clash);
        }
    }

#endif
}

// ---------------------------------------------------------------- turn order
// FFT's clock (RE_NOTES.md § Turn order, checked on the original's turns): each tick every unit's
// CT grows by its Speed (at most 254); a unit with CT >= 100 acts, the highest first, a tie to
// the lowest index; its CT loses 100 when picked and gains 20 for each of Move and Act left
// unused at the end, at most 60. A unit at 0 HP keeps its clock: each pick lowers its death
// counter (3 to 0), the next one turns it into a crystal or a chest (a player's unit 3 times in
// 4, an enemy 1 in 2) and its clock stops; the guest (Delita) never turns.
u8 ct_next(void)
{
    for (;;) {
        u8 i, best = 0xFF, max = 99;
        Unit *u;
#ifdef FFT_OPENING
        if (st.open_ct_phase && opening_pending() != 255) return 254;
#endif
        for (i = 0, u = st.unit; i < NUNIT; i++, u++)
            if (!DC_OBJ(u->dc) && u->ct > max) { max = u->ct; best = i; }
        if (best == 0xFF) {                        // a tick
#ifdef FFT_OPENING
            if (!st.open_ct_phase) {
                for (i = 0; i < NUNIT; i++)
                    if (st.pending[i].state && st.pending[i].ticks) st.pending[i].ticks--;
                st.open_ct_phase = 1;
                if (opening_pending() != 255) return 254;
            }
            st.open_ct_phase = 0;
#endif
            for (i = 0, u = st.unit; i < NUNIT; i++, u++)
                if (!DC_OBJ(u->dc)) { u16 c = u->ct + u->sp; u->ct = (u8)(c > 254 ? 254 : c); }
            continue;
        }
        u = &st.unit[best];
        u->ct = (u8)(max >= 200 ? max - 200 : max - 100);
        if (u->hp) return best;
        if (u->team == TEAM_GUEST) continue;
        if (u->dc) u->dc--;
        else {
            u->dc = rand15() & (u->team == TEAM_PLAYER ? 0x110 : 0x10) ? DC_CRYSTAL : DC_CHEST;
            st.morph_mask |= (u16)(1U << best); // retain all events even if one clock pass converts several
        }
    }
}

void ct_end(u8 who, u8 moved, u8 acted)
{
    u8 c = st.unit[who].ct;
    if (!moved) c += 20;
    if (!acted) c += 20;
    st.unit[who].ct = c > 60 ? 60 : c;
}

// FFT's AT list: the active unit, then each living unit's next turns, its clock run alone
// (no bonus at their end), sorted by tick then by the CT reached (the higher first), a tie in
// unit order. At each turn change only; reciprocal products replace the
// repeated ceil-divides (numerator <= 149, table exact through 815).
void turn_order(void)
{
    u16 tk[ORDER_N];
    u8 i, j, n = 1, t;
    Unit *u;
    st.turns[0] = st.act; tk[0] = 0;
    for (i = 0, u = st.unit; i < NUNIT; i++, u++) {
        u16 tick = 0, ct = u->ct;
        if (!u->hp) continue;
        for (t = 0; t < ORDER_N - 1; t++) {
            u16 k;
            if (ct < 100) {                        // the ticks to 100 at once
                u16 k2 = (u16)(((u32)(u16)(100 - ct + u->sp - 1) * ct_recip[u->sp]) >> 15);
                tick += k2; ct += k2 * u->sp;
            }
            if (tick >= 256) break;
            ct = ct >= 200 ? ct - 200 : ct - 100;
            k = (u16)(tick << 8 | (100 - ct));
            for (j = n; j > 1 && tk[j - 1] > k; j--)
                if (j < ORDER_N) { tk[j] = tk[j - 1]; st.turns[j] = st.turns[j - 1]; }
            if (j >= ORDER_N) break;               // later than every turn kept
            tk[j] = k; st.turns[j] = i;
            if (n < ORDER_N) n++;
        }
    }
    for (; n < ORDER_N; n++) st.turns[n] = 0xFF;
}

static void end_battle(u8 e);
static void bn_start(u8 kind);
static void ai_next(void);

// the next turn: the cursor and the camera go to its unit (its Defending ends); enemies and the
// guest think (the AI), a player's unit opens its menu; the battle ends if Ramza just turned to
// crystal
static void turn_start(void)
{
    u8 me;
    if (W) W->upd_k += TURN_K;
    st.act = ct_next();
#ifdef FFT_OPENING
    if (st.act == 254) { opening_resolve(); return; }
#endif
    if (st.act == U_RAMZA) st.rounds++;
    st.pick_t = 0;
    if (outcome()) { end_battle(outcome()); return; }
    st.unit[st.act].status &= (u8)~ST_DEFEND;
    st.cx = st.unit[st.act].x; st.cz = st.unit[st.act].z;
    me = st.unit[st.act].team == TEAM_PLAYER && !st.ai_all && !(st.auto_units & (1U << st.act));
    st.ai = me ? AI_OFF : AI_THINK; st.ai_t = 0;
    if (me && st.prot != 0xFF) { st.want = st.prot; st.prot = 0xFF; }   // the player's view back
    if (W) { W->ai_k = 0; W->frz = W->frz_n = 0; W->frz_h = 0; }
    st.mode = me ? M_MENU : M_BROWSE;
    st.menu = MN_MOVE; st.moved = st.acted = 0;
    turn_order();
}

void turn_end(u8 moved, u8 acted)
{
    ct_end(st.act, moved, acted);
    turn_start();
}

// ---------------------------------------------------------------- drawing targets
// Every drawing call takes scene coordinates; the target maps them to its own pixels (origin
// ox, oy) and clips to pixels [x0, x1) and rows [y0, y1) (Dst: with the work memory's types).
static Dst D;

static void dst_scene(void)
{
    D.l = scene_l; D.d = scene_d; D.stride = SC_BYTES; D.ox = D.oy = 0;
    D.x0 = 0; D.x1 = SC_W; D.y0 = 0; D.y1 = SC_H;
}

static void dst_screen(s16 camx, s16 camy)
{
    D.l = rt_light; D.d = rt_dark; D.stride = RT_PBYTES; D.ox = camx; D.oy = camy;
    D.x0 = 0; D.x1 = RT_W; D.y0 = 0; D.y1 = RT_H;
}

// A masked sprite of up to 24 columns (u32 rows, bit 31 = column 0): dest = dest & ~m | src & m,
// written as three 16-bit words per plane and row, each clipped
static void blit24(s16 x, s16 y, s16 rows, const PlaneWord *l, const PlaneWord *d, const PlaneWord *m)
{
    s16 w, sh, r = 0, k;
    u16 ck[3];
    u8 *pl, *pd;
    x -= D.ox; y -= D.oy;
    if (x >= D.x1 || x + 24 <= D.x0) return;
    if (y < D.y0) { r = D.y0 - y; y = D.y0; }
    if (y + rows - r > D.y1) rows = D.y1 - y + r;
    if (r >= rows) return;
    w = x >> 4; sh = x & 15;
    for (k = 0; k < 3; k++) {
        s16 lo = (w + k) << 4, a = lo < D.x0 ? D.x0 : lo, b = lo + 16 > D.x1 ? D.x1 : lo + 16;
        ck[k] = a < b ? (u16)((0xFFFFU >> (a - lo)) & (0xFFFFU << (lo + 16 - b))) : 0;
    }
    pl = D.l + y * D.stride + 2 * w; pd = D.d + y * D.stride + 2 * w;
    for (; r < rows; r++, pl += D.stride, pd += D.stride) {
        u32 mm = m[r] >> sh, ll = l[r] >> sh, dd = d[r] >> sh;
        u16 k0 = ck[0] & (u16)(mm >> 16), k1 = ck[1] & (u16)mm;
        if (k0) { RMW16(pl, k0, (u16)(ll >> 16) & k0); RMW16(pd, k0, (u16)(dd >> 16) & k0); }
        if (k1) { RMW16(pl + 2, k1, (u16)ll & k1); RMW16(pd + 2, k1, (u16)dd & k1); }
        if (sh > 8) {
            u16 k2 = ck[2] & (u16)(m[r] << (16 - sh));
            if (k2) {
                RMW16(pl + 4, k2, (u16)(l[r] << (16 - sh)) & k2);
                RMW16(pd + 4, k2, (u16)(d[r] << (16 - sh)) & k2);
            }
        }
    }
}

// Convex polygon (n <= 4 points, target coordinates) filled with an 8x8 pattern aligned to the
// target. Pixel (x, y) is inside when its centre is. Spans in 16-bit words.

#endif
#if MOD_PART(MOD_TURN)
#if !defined(FFT_EXPLORATION) || defined(FFT_FIXED) || defined(FFT_ULTRA_TRANSITION)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define lmask CONTOUR_REF_lmask
#else
static const u16 lmask[16] = { 0xFFFF, 0x7FFF, 0x3FFF, 0x1FFF, 0x0FFF, 0x07FF, 0x03FF, 0x01FF,
    0x00FF, 0x007F, 0x003F, 0x001F, 0x000F, 0x0007, 0x0003, 0x0001 };
#endif   // pixels >= x & 15

// rows ymin.. of the target from span_l/span_r, an 8 x 8 pattern of 16-bit rows aligned to
// the target's pixels (the rotation frames)
static void fill_spans(s16 ymin, s16 rows, const u16 *pl, const u16 *pd)
{
    u8 *ql = D.l + ymin * D.stride, *qd = D.d + ymin * D.stride;
    const s16 *sa = span_l, *sb = span_r;
    s16 stride = D.stride, py8 = (ymin + D.oy) & 7;
    for (; rows > 0; rows--, ql += stride, qd += stride, py8 = (py8 + 1) & 7) {
        s16 xa = *sa++, xb = *sb++, n;
        u16 rl, rd, k, kr;
        u8 *al, *ad;
        if (xa < D.x0) xa = D.x0;
        if (xb > D.x1) xb = D.x1;
        if (xa >= xb) continue;
        rl = pl[py8]; rd = pd[py8];
        al = ql + ((xa >> 3) & ~1); ad = qd + ((xa >> 3) & ~1);
        k = lmask[xa & 15];
        kr = (u16)(0xFFFFU << (15 - ((xb - 1) & 15)));
        n = ((xb - 1) >> 4) - (xa >> 4);
        if (!n) k &= kr;
        RMW16(al, k, rl & k); RMW16(ad, k, rd & k);
        if (!n) continue;
        for (al += 2, ad += 2; --n > 0; al += 2, ad += 2) { WR16(al, 0, rl); WR16(ad, 0, rd); }
        RMW16(al, kr, rl & kr); RMW16(ad, kr, rd & kr);
    }
}

static void fill_poly(const s16 *px, const s16 *py, u8 n, const u16 *pl, const u16 *pd)
{
    s16 ymin = 32767, ymax = -32768, y, i, rows;
    for (i = 0; i < n; i++) {
        s16 yy = py[i] - D.oy;
        if (yy < ymin) ymin = yy;
        if (yy > ymax) ymax = yy;
    }
    if (ymin < D.y0) ymin = D.y0;
    if (ymax > D.y1) ymax = D.y1;
    rows = ymax - ymin;
    if (rows <= 0) return;
    {
        s16 *a = span_l, *b = span_r;
        for (y = rows; y > 0; y--) { *a++ = 32767; *b++ = -32768; }
    }
    for (i = 0; i < n; i++) {
        s16 j = i + 1 < n ? i + 1 : 0;
        s16 x0 = px[i] - D.ox, y0 = py[i] - D.oy, x1 = px[j] - D.ox, y1 = py[j] - D.oy;
        s16 ya, yb, *sl, *sr, dx8;
        s32 x, dx;
        if (y0 == y1) continue;
        if (y0 > y1) { s16 t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        ya = y0 < ymin ? ymin : y0;
        yb = y1 > ymax ? ymax : y1;
        if (ya >= yb) continue;
        dx8 = divs32_16((s32)(x1 - x0) << 8, y1 - y0);            // 8.8 slope: |dx| < 128
        dx = (s32)dx8 << 8;
        x = ((s32)x0 << 16) + (muls16(dx8, (s16)(2 * (ya - y0) + 1)) << 7) + 0x8000;  // row centre
        sl = span_l + (ya - ymin); sr = span_r + (ya - ymin);
        for (y = yb - ya; y > 0; y--, x += dx, sl++, sr++) {
            s16 xi = (s16)(x >> 16);
            if (xi < *sl) *sl = xi;
            if (xi > *sr) *sr = xi;
        }
    }
    fill_spans(ymin, rows, pl, pd);
}
#endif
#endif
#if MOD_CORE

#define DENY_FRAMES 16                  // the cursor's blinks when [2nd] cannot pick its tile
#define AI_FAST 3                       // AI turn steps a frame while [2nd] is held
#define FAST_K 10
#define RINGS(m) ((m) == M_TARGET || (m) == M_AIM || ((m) == M_ORDER && (st.back == M_TARGET || st.back == M_AIM)))
                                                 // the modes that show st.reach
#define AIMING(m) ((m) == M_AIM || ((m) == M_ORDER && st.back == M_AIM))   // its rings inverted:
                                                 // black, a white edge (the user's pick)

// A tile's marker (a reach ring, or the cursor: cur) in view rot: the flat diamond (hi_*,
// cur_*, 24 x 12) at its standing height, or on a sloped tile the outline of its four corners
// at their heights (art.py slope_*), scene box left x, top y, its rows
typedef struct {
    const PlaneWord *l, *d, *m; s16 x, y, rows; u8 depth;
#ifdef FFT_REACH_CONTOUR
    u8 edges, shape;
#endif
} Marker;
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define hi_l CONTOUR_REF_hi_l
#define hi_m CONTOUR_REF_hi_m
#define cur_l CONTOUR_REF_cur_l
#define foe_gfx CONTOUR_REF_foe_gfx
#define foe_ko_gfx CONTOUR_REF_foe_ko_gfx
#define active_gfx CONTOUR_REF_active_gfx
#define hand_gfx CONTOUR_REF_hand_gfx
#define slope_key CONTOUR_REF_slope_key
#define slope_top CONTOUR_REF_slope_top
#define slope_rows CONTOUR_REF_slope_rows
#define slope_off CONTOUR_REF_slope_off
#define turn_pat CONTOUR_REF_turn_pat
#endif
static void marker(u8 rot, u8 i, u8 cur, Marker *k)
{
    s16 u, v, lo = 255, n;
    u8 h[4], c, sk = 0;
#ifdef FFT_REACH_CONTOUR
    k->edges=k->shape=0;
#endif
    view_of(rot, i % MAP_W, i / MAP_W, &u, &v);
    k->x = scene_x(rot, u, v) - HW; k->depth = (u8)(u + v);
    for (c = 0; c < 4; c++) {
#ifdef FFT_SOURCE_CAMERA
        h[c] = corner_h(rot, u, v, (4-c)&3);
#else
        h[c] = corner_h(rot, u, v, c);
#endif
        if (h[c] < lo) lo = h[c];
    }
    for (c = 0; c < 4; c++) sk |= (u8)((h[c] - lo > 3 ? 3 : h[c] - lo) << (2 * c));
    for (n = 0; sk && n < NSLOPE; n++)
        if (slope_key[n] == sk) {
            u8 o = slope_off[n];
#ifdef FFT_REACH_CONTOUR
            k->shape=(u8)(n+1);
#endif
            k->l = (cur ? sc_l : sl_l) + o; k->d = (cur ? sc_d : sl_d) + o; k->m = (cur ? sc_m : sl_m) + o;
            k->rows = slope_rows[n]; k->y = scene_y(u, v) - (lo + 2) * HU + slope_top[n];
            return;
        }
    k->l = cur ? cur_l : hi_l; k->d = cur ? cur_d : hi_d; k->m = cur ? cur_m : hi_m;
    k->rows = 12; k->y = scene_y(u, v) - map_tiles[i].stand * (HU / 2);
}

// A reach tile (its marker) drawn into the scene on a tile of view diagonal depth, but where
// the view has something in front of its pixels (a greater diagonal): there the view shows, but
// for a pattern of them: a hidden ring stays visible over the building, set apart from the seen
// ones. A 16-bit word at a time, its two depth bytes compared at once (diagonals < 128: t + 127 -
// depth has bit 7 set when t > depth, no carry between the bytes): ~6k a ring (a blit then the
// view put back over the whole box cost ~19k, and wiped a nearer tile's ring drawn before).
static const u8 hide_pat[2] = { 0xCC, 0x33 };   // 2-pixel dashes (picked over the whole ring
                                                // and a checkerboard)
static void __attribute__((noinline)) ring_into_scene(const Marker *mk)
{
    s16 r, k, w0 = mk->x >> 4, sh = mk->x & 15, yy = mk->y;
    u16 dd = (u16)(0x7F7F - (mk->depth | mk->depth << 8));
    u16 hp = (u16)~(hide_pat[yy & 1] * 0x0101), o = (u16)(yy * SC_BYTES + 2 * w0);
    u32 inv = AIMING(st.mode) ? 0xFFFFFFFFUL : 0;   // black and white: one plane for both
#ifdef FFT_REACH_CONTOUR
    const u8 *edge=mk->edges ? contour_gfx+contour_off[mk->shape] : RT_NULL;
#endif
    for (r = 0; r < mk->rows; r++, yy++, o += SC_BYTES, hp = (u16)~hp) {
        u32 m = mk->m[r], l = (mk->l[r] ^ inv) & m, a;
#ifdef FFT_REACH_CONTOUR
        if(mk->edges){
            u32 selected=0;
            if(mk->edges&1)selected|=RD32(edge,0);
            if(mk->edges&2)selected|=RD32(edge,2);
            if(mk->edges&4)selected|=RD32(edge,4);
            if(mk->edges&8)selected|=RD32(edge,6);
            m&=selected;l&=m;edge+=16;
        }
#endif
        u16 m3[3], l3[3];
        if ((u16)yy >= SC_H || !m) continue;
        a = m >> sh; m3[0] = (u16)(a >> 16); m3[1] = (u16)a; m3[2] = (u16)(m << (16 - sh));
        a = l >> sh; l3[0] = (u16)(a >> 16); l3[1] = (u16)a; l3[2] = (u16)(l << (16 - sh));
#ifdef FFT_VIEW_SPLIT
        const u8 *ol = scene_l, *od = scene_d;      // past the backup: the row as it is
        if (W->bk_of[yy] != 0xFF) { ol = W->bk[W->bk_of[yy]][0] - yy * SC_BYTES; od = W->bk[W->bk_of[yy]][1] - yy * SC_BYTES; }
#else
        const u8 *ol = vbuf, *od = vbuf + SC_PLANE;
#endif
        for (k = 0; k < 3; k++) {
            u16 mw = m3[k], g1, g2, c, oo = (u16)(o + 2 * k);
            const u8 *t = VDEPTH + oo;     // depths, masks: constant offsets
            u8 *q = scene_l + oo;
            if (!mw || (u16)(w0 + k) >= SC_BYTES / 2) continue;
            if (st.over) c = 0;                         // over the scenery: no depth test
            else {
                g1 = (u16)(RD16(t, 0) + dd) & 0x8080; g1 = (u16)((g1 << 1) - (g1 >> 7));
                g2 = (u16)(RD16(t + 2 * SC_PLANE, 0) + dd) & 0x8080; g2 = (u16)((g2 << 1) - (g2 >> 7));
                c = (u16)(((g1 & RD16(t + SC_PLANE, 0)) | (g2 & RD16(t + 3 * SC_PLANE, 0))) & mw & hp);
            }
            WR16(q, 0, (u16)((RD16(q, 0) & ~mw) | (l3[k] & mw & ~c) | (RD16(ol + oo, 0) & c)));
            WR16(q + SC_PLANE, 0, (u16)((RD16(q + SC_PLANE, 0) & ~mw) | (l3[k] & mw & ~c) | (RD16(od + oo, 0) & c)));
        }
    }
}

// The scene of orientation rot with the current reach rings: the view copied from its file,
// then a dotted ring on each reachable tile, behind what stands in front of it
static u8 scene_seq;                    // hl_seq it was made for
static u16 scene_ver;                   // bumped when the scene is made again whole (boxes
                                        // changed otherwise are patched into W->bg)
static u8 hl_seq;                       // bumped when the highlights may have changed

#ifdef FFT_TEXTURE_PREVIEW
static void preview_iso_apply(u8 rot);
#endif
#if defined(FFT_OPENING) || defined(FFT_STORY_GAR)
// Chapter views store each occlusion mask row XORed with the row above (extract.py
// rowxor_masks: ~3 KB less archive per map); undone here a long at a time.
static void unxor_masks(u8 *v)          // v: the depth/mask planes
{
    u8 k;
    for (k = 1; k <= 3; k += 2) {
        u32 *p = (u32 *)(v + k * SC_PLANE + SC_BYTES), *e = (u32 *)(v + (k + 1) * SC_PLANE);
        do *p ^= p[-SC_BYTES / 4]; while (++p < e);
    }
}
#endif

#ifdef FFT_VIEW_SPLIT
static void bk_reset(void)
{
    memset(W->bk_of, 0xFF, sizeof W->bk_of); W->bk_n = 0; W->bk_over = 0;
}

// Keep the view's colours of scene row y before a ring changes it; 0: the backup is full of
// rows still changed (rings drawn now or rows still to put back)
static u8 bk_save(s16 y)
{
    u8 s;
    if (W->bk_of[y] != 0xFF) return 1;
    if (W->bk_n < BK_N) s = W->bk_n++;
    else {                                      // a row put back since: its slot again
        for (s = 0; s < BK_N; s++) {
            s16 r = W->bk_row[s];
            if ((r < W->ring_y0 || r >= W->ring_y1) && (r < W->drop_y0 || r >= W->drop_y1)) break;
        }
        if (s == BK_N) return 0;
        W->bk_of[W->bk_row[s]] = 0xFF;
    }
    W->bk_of[y] = s; W->bk_row[s] = (u8)y;
    memcpy(W->bk[s][0], scene_l + y * SC_BYTES, SC_BYTES);
    memcpy(W->bk[s][1], scene_d + y * SC_BYTES, SC_BYTES);
    return 1;
}
#endif

static void unpack_view(u8 rot)
{
    if (vbuf_rot == rot) return;
#ifdef FFT_VIEW_SPLIT                           // b'V2', colour stream length (extract.pack_view)
    zx0_unpack(views[rot] + 4, scene_l);
    zx0_unpack(views[rot] + 4 + RD16(views[rot], 1), vbuf);
    unxor_masks(vbuf);
    bk_reset();
#else
    zx0_unpack(views[rot], vbuf);
#if defined(FFT_OPENING) || defined(FFT_STORY_GAR)
    unxor_masks(vbuf);
#endif
#endif
#ifdef FFT_TEXTURE_PREVIEW
    preview_iso_apply(rot);
#endif
    vbuf_rot = rot;
}

// is scene pixel (x, y) in front of everything nearer than view diagonal depth (not covered)?
#ifndef FFT_WATER_BANK
static u8 seen(s16 x, s16 y, u8 depth)
{
    const u8 *t1 = VDEPTH, *m1 = t1 + SC_PLANE, *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
    u16 o = (u16)(y * SC_BYTES + (x >> 3));
    u8 b = (u8)(0x80 >> (x & 7));
    return !((t1[o] > depth && m1[o] & b) || (t2[o] > depth && m2[o] & b));
}
#endif

// The cursor's mask (marker mk of tile c) dashed as the hidden rings where the view has something
// in front of it (a canal tile behind a higher street tile): the view's depth bytes compared a
// byte (8 pixels) at a time, made again only when the cursor or the view changes (optional work:
// without room in the frame's budget the cursor is drawn whole a frame or two); the player's
// cursor only: the AI's frames are the fullest
static const PlaneWord *cursor_mask(const Marker *mk, u16 c)
{
    u16 k = (u16)(c | st.rot << 8 | 0x8000), o;   // c: the tile, | 0x1000 a reach tile's marker
    s16 r, b0 = mk->x >> 3, sh = mk->x & 7, yy = mk->y, j;
    const u8 *t1 = VDEPTH;
    u8 dp = mk->depth;
    if (vbuf_rot != st.rot || st.ai) return mk->m;  // (an AI turn: whole, its frames are full)
    if (W->cur_k == k) return W->cur_m;
    if (W->left < CUR_K && (W->cur_wait++ < WAIT_MAX || !W->bg_drawn)) return mk->m;
    W->cur_wait = 0; W->left -= CUR_K;
    W->cur_k = k;
    o = (u16)(yy * SC_BYTES + b0);
    for (r = 0; r < mk->rows; r++, yy++, o += SC_BYTES) {
        u32 cv = 0;
        if ((u16)yy < SC_H)
            for (j = 0; j < 4; j++) {
                const u8 *t = t1 + o + j;
                u8 h = 0;
                if ((u16)(b0 + j) < SC_BYTES) {
                    if (t[0] > dp) h = t[SC_PLANE];
                    if (t[2 * SC_PLANE] > dp) h |= t[3 * SC_PLANE];
                }
                cv = cv << 8 | h;
            }
        cv &= yy & 1 ? 0xCCCCCCCCUL : 0x33333333UL;  // hidden but for the dashes (hide_pat)
        W->cur_m[r] = mk->m[r] & ~(cv << sh);
    }
    return W->cur_m;
}

// The water's glints of orientation rot: 2-pixel dashes on a lattice of the scene's pixels
// (every other row, one even column in 8, staggered by row) that slides 2 px right each phase
// (a current), on the water tiles' tops where the view shows them (not under a bank or a roof)
static void make_glints(u8 rot)
{
    u8 i;
#ifdef FFT_WATER_BANK                   // the chapter's descriptor (tools/chapter_desc.py)
    const u8 *p=cdesc+CD(WATER);
    p+=RD16(p,rot);
    for(i=0;i<4;i++) {
        u8 n=(u8)RD16(p,0);p+=2;W->glints[i]=n;
        W->puddle[i]=p;p+=(u16)n*2;
    }
#else
    memset(W->glints, 0, sizeof W->glints);
    for (i = 0; i < MAP_W * MAP_H; i++) {
        s16 u, v, x, y, r, px;
        if (!map_tiles[i].water) continue;
        view_of(rot, i % MAP_W, i / MAP_W, &u, &v);
        x = scene_x(rot, u, v) - HW; y = scene_y(u, v) - map_tiles[i].stand * (HU / 2);
        for (r = 1; r < 11; r++) {
            s16 py = y + r, k = r < 6 ? r : 11 - r;
            if ((py & 1) || (u16)py >= SC_H) continue;
            for (px = (x + 10 - 2 * k + 1) & ~1; px + 1 < x + 14 + 2 * k; px += 2) {
                u8 ph = (u8)(((px >> 1) + 3 * (py >> 1)) & 3), n;
                if (px < 0 || px + 1 >= SC_W) continue;
                n = W->glints[ph];
                if (n >= GLINTS || !seen(px, py, (u8)(u + v)) || !seen(px + 1, py, (u8)(u + v))) continue;
                W->glint[ph][n].x = px; W->glint[ph][n].y = py;
                W->glints[ph] = (u8)(n + 1);
            }
        }
    }
#endif
}

// this frame's glints on the screen, white (a phase every 8 frames)
static void draw_glints(void)
{
    u8 ph = (u8)((st.tick >> 3) & 3), n = W->glints[ph], i;
#ifdef FFT_WATER_BANK
    const u8 *p=W->puddle[ph];
#endif
    for (i = 0; i < n; i++) {
#ifdef FFT_WATER_BANK
        s16 x=(s16)(*p++<<1)-st.camx,y=(s16)*p++-st.camy,k;
#else
        s16 x = W->glint[ph][i].x - st.camx, y = W->glint[ph][i].y - st.camy, k;
#endif
        if ((u16)y >= RT_H || x < 0 || x + 1 >= RT_W) continue;
        for (k = x; k <= x + 1; k++) {
            u16 o = (u16)(y * RT_PBYTES + (k >> 3));
            u8 b = (u8)~(0x80 >> (k & 7));
            ((u8 *)rt_light)[o] &= b; ((u8 *)rt_dark)[o] &= b;
        }
    }
}

// the screen-aligned copy of the scene (W->bg) brought up to date on a box of the scene (pixels
// x .. x + w - 1, rows y .. y + h - 1) that changed: it stays valid while rings come and go
static void bg_patch(s16 x, s16 y, s16 w, s16 h)
{
    s16 j, j0, j1, r, sh = st.camx & 15, wb = (st.camx >> 4) << 1;
    if (!(W->bg_ok || W->bg_part) || W->bg_x != st.camx || W->bg_y != st.camy) return;
    x -= st.camx; y -= st.camy;                // (a copy partly made: the rest comes after)
    if (y < 0) { h += y; y = 0; }
    if (y + h > RT_H) h = RT_H - y;
    if (h <= 0 || x + w <= 0 || x >= RT_W) return;
    j0 = x < 0 ? 0 : x >> 4; j1 = (x + w - 1) >> 4;
    if (j1 >= RT_W / 16) j1 = RT_W / 16 - 1;
    for (r = y; r < y + h; r++) {
        const u8 *sl = scene_l + (st.camy + r) * SC_BYTES + wb, *sd = sl + SC_PLANE;
        u8 *bl = (u8 *)W->bg[0] + r * (RT_W / 8), *bd = (u8 *)W->bg[1] + r * (RT_W / 8);
        for (j = j0; j <= j1; j++) {
            WR16(bl, j, (u16)((RD32(sl, j) << sh) >> 16));
            WR16(bd, j, (u16)((RD32(sd, j) << sh) >> 16));
        }
    }
}

// a new view, or many rings to drop (a Move's): copied whole (130k); a few (an attack's): only
// their boxes put back from the view (~7k each; the frame an attack starts went over the
// budget with the whole copy). The new rings are then all drawn in one frame (~18k each), as
// FFT shows them at once: a Move's is a stall (zone 10 under ti-cycles), as a turn of the view

static u8 bg_usable(void)
{
    return W->bg_ok && W->bg_x == st.camx && W->bg_y == st.camy && W->bg_ver == scene_ver;
}
// scene rows [y, y + n) put back from the view (u32 copies: ~0.6k a row)
static void scene_rows(s16 y, s16 n)
{
#ifdef FFT_VIEW_SPLIT                           // from the backup (rows never changed are kept)
    for (; n > 0; n--, y++) {
        u8 s = W->bk_of[y];
        if (s == 0xFF) continue;
        memcpy(scene_l + y * SC_BYTES, W->bk[s][0], SC_BYTES);
        memcpy(scene_d + y * SC_BYTES, W->bk[s][1], SC_BYTES);
    }
#else
    PlaneWord *a = (PlaneWord *)(scene_l + y * SC_BYTES), *b = (PlaneWord *)(scene_d + y * SC_BYTES);
    const PlaneWord *p = (const PlaneWord *)(vbuf + y * SC_BYTES), *q = (const PlaneWord *)(vbuf + SC_PLANE + y * SC_BYTES);
    u16 k = (u16)(n * (SC_BYTES / 4));
    while (k--) { *a++ = *p++; *b++ = *q++; }
#endif
}

static u8 ring_style(void)
{
    return RINGS(st.mode) ? (u8)(1 | (AIMING(st.mode) ? 2 : 0) | (st.over ? 4 : 0)
#ifdef FFT_REACH_CONTOUR
        | (!AIMING(st.mode) && st.reach_contour && contour_gfx ? 8 : 0)
#endif
        ) : 0;
}

// Reach costs can change without changing any displayed tile. Compare exact
// membership bits only on invalidation, never in the ordinary frame path.
static u8 rings_unchanged(void)
{
    u16 i=0;u8 j=0;
    if(W->ring_style!=ring_style())return 0;
    if(!W->ring_style)return 1;
    while(i<MAP_W*MAP_H){
        u8 bits=0,bit=1;
        do {if(st.reach[i]!=0xFF)bits|=bit;i++;bit=(u8)(bit<<1);}
        while(bit && i<MAP_W*MAP_H);
        if(bits!=W->ring_bits[j++])return 0;
    }
    return 1;
}

#ifdef FFT_REACH_CONTOUR
// Four-neighbour boundary, including holes and disconnected landing regions.
// Screen edges are top-right, right-bottom, bottom-left, left-top. Rotate the
// world neighbour bits, then reflect them with the same source-camera rule.
static u8 reach_edges(u8 i,u8 rot)
{
    #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define reflected CONTOUR_REF_reflected
#else
static const u8 reflected[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
#endif
    u8 bits=0,x=TILE_X[i],z=TILE_Z[i];
    if(!z||st.reach[i-MAP_W]==0xFF)bits|=1;
    if(x==MAP_W-1||st.reach[i+1]==0xFF)bits|=2;
    if(z==MAP_H-1||st.reach[i+MAP_W]==0xFF)bits|=4;
    if(!x||st.reach[i-1]==0xFF)bits|=8;
    bits=(u8)(((bits<<rot)|(bits>>(4-rot)))&15);
#ifdef FFT_SOURCE_CAMERA
    return reflected[bits];
#else
    return bits;
#endif
}
#endif

static void ensure_scene(u8 rot)
{
    s16 patch_x0=SC_W,patch_x1=0;
    u8 i;
    Marker mk;
#if defined(FFT_MEASURE) && defined(RT_CYCLES)
    if (scene_rot != rot) frame_stall = 1;       // initial load or a rotated view
#endif
    if (scene_rot == rot && scene_seq == hl_seq) {
        if (W->drop_y0 < W->drop_y1) goto drop;
        if (W->ring_i >= MAP_W * MAP_H) return;
        goto rings;
    }
    if(scene_rot==rot && W->drop_y0>=W->drop_y1 && W->ring_i>=MAP_W*MAP_H && rings_unchanged()){
        scene_seq=hl_seq;return;
    }
    unpack_view(rot);
    if (scene_rot != rot) {
        make_glints(rot);
#ifndef FFT_VIEW_SPLIT                          // (split: unpack_view wrote them there)
        memcpy(scene_l, vbuf, 2 * SC_PLANE);
#endif
        scene_ver++;
        W->drop_y0 = W->drop_y1 = 0;
    } else if (W->ring_y0 < W->ring_y1) {          // the rows the rings are on, put back below
        if (W->drop_y0 >= W->drop_y1) { W->drop_y0 = W->ring_y0; W->drop_y1 = W->ring_y1; }
        else {
            if (W->ring_y0 < W->drop_y0) W->drop_y0 = W->ring_y0;
            if (W->ring_y1 > W->drop_y1) W->drop_y1 = W->ring_y1;
        }
    }
    scene_rot = rot; scene_seq = hl_seq;
    W->ring_i = 0;
    W->ring_y0 = SC_H; W->ring_y1 = 0;
drop:
    if (W->drop_y0 < W->drop_y1) {                 // the rings going: their rows put back from
        s16 n = W->left > 8 * ROW_K ? W->left / ROW_K : 8;   // the view (and into the screen's
        if (n > W->drop_y1 - W->drop_y0) n = W->drop_y1 - W->drop_y0;   // copy) as the budget
        if (W->left < n * ROW_K && (W->drop_wait < WAIT_MAX || !bg_usable())) { W->drop_wait++; return; }
        W->drop_wait = 0;                          // allows, then the new ones (a whole scene
#ifdef FFT_VIEW_SPLIT
        if (W->bk_over) {                          // rings went past the backup: the colours
            zx0_unpack(views[scene_rot] + 4, scene_l);   // again, whole (no rings are drawn now)
            bk_reset(); scene_ver++;
            W->drop_y0 = W->drop_y1; W->left -= RELOAD_K;
            goto rings;
        }
#endif
        scene_rows(W->drop_y0, n);                 // copy, 130k, or the rings' boxes one by one,
        bg_patch(0, W->drop_y0, SC_W, n);          // ~10k each, went over the budget)
        W->drop_y0 += n; W->left -= n * ROW_K;
        if (W->drop_y0 < W->drop_y1) return;
    }
rings:
    dst_scene();
    if (!RINGS(st.mode)) { W->ring_i = MAP_W * MAP_H; W->ring_style=0; return; }
#if defined(FFT_MEASURE) && defined(RT_CYCLES)
    frame_stall = 1;
#endif
    ZB(10);
    memset(W->ring_bits,0,sizeof W->ring_bits);
    for (; W->ring_i < MAP_W * MAP_H; W->ring_i++) {
        i = W->ring_i;
        if (st.reach[i] == 0xFF) continue;
        W->ring_bits[i>>3]|=(u8)(1U<<(i&7));
#ifdef FFT_REACH_CONTOUR
        if(st.reach_contour && contour_gfx && !AIMING(st.mode)){
            u8 edges=reach_edges(i,rot);
            if(!edges)continue;
            marker(rot,i,1,&mk);mk.edges=edges;
        }else marker(rot,i,0,&mk);
#else
        marker(rot, i, 0, &mk);
#endif
        W->left -= RING_K * mk.rows / 12;
        if (mk.y < W->ring_y0) W->ring_y0 = mk.y < 0 ? 0 : mk.y;     // (first: the backup never
        if (mk.y + mk.rows > W->ring_y1) W->ring_y1 = mk.y + mk.rows > SC_H ? SC_H : mk.y + mk.rows;
#ifdef FFT_VIEW_SPLIT                              // reuses a row of this ring)
        { s16 y; for (y = W->ring_y0 > mk.y ? W->ring_y0 : mk.y; y < mk.y + mk.rows && y < SC_H; y++) if (!bk_save(y)) W->bk_over = 1; }
#endif
        ring_into_scene(&mk);
        if(mk.x<patch_x0)patch_x0=mk.x;
        if(mk.x+24>patch_x1)patch_x1=mk.x+24;
    }
    // All rings appear together. Patch their union once, avoiding repeated
    // screen-cache shifts where adjacent/sloped markers overlap.
    bg_patch(patch_x0,W->ring_y0,patch_x1-patch_x0,W->ring_y1-W->ring_y0);
    W->ring_style=ring_style();
    ZE(10);
}

u16 scene_checksum(void)
{
    u16 a = 0, b = 0, i;
    for (i = 0; i < (u16)(SC_BYTES * SC_H); i++) {
        a = (a + scene_l[i]) % 255; b = (b + a) % 255;
        a = (a + scene_d[i]) % 255; b = (b + a) % 255;
    }
    return (u16)(b << 8 | a);
}

// ---------------------------------------------------------------- reach and paths
static const s8 dir_x[4] = { 1, -1, 0, 0 }, dir_z[4] = { 0, 0, 1, -1 };

// FFT's movement spread (battle_move_calculate_spread, measured on the original's move ranges;
// RE_NOTES.md § Moving): the Move points each tile costs from tile c: a step 1, from water to
// water 2, not into water from land with the last point; a jump over one tile 2 (from water 3;
// Jump 2 or more: to a tile no lower by at most Jump half units, over ground no higher than it,
// no enemy there); a step's height within Jump (out of water: from its surface); enemies block,
// allies can be crossed. limit: the most it spends (dist 0xFF beyond); from: the previous tile.
static u8 lower(u8 c)                    // a tile's height a unit leaves it from (half units)
{
    return (u8)(map_tiles[c].stand + (map_tiles[c].water << 1));
}

// the moves out of each tile a unit with that Jump can make whatever the units (to a walkable
// tile, within its heights; a jump: over lower ground), made once: each its tile and kind (1 a
// jump, 2 into water)
static void edges(u8 jump)
{
    static const s8 step[4] = { 1, -1, MAP_W, -MAP_W };
    u8 t, k, j;
    W->ej = jump;
    for (t = 0; t < MAP_W * MAP_H; t++) {
        u8 x = TILE_X[t], z = TILE_Z[t], h = W->hl[t], n = 0, *e = W->eg[t];
        for (k = 0; k < 4; k++) {
            u8 room = k == 0 ? MAP_W - 1 - x : k == 1 ? x : k == 2 ? MAP_H - 1 - z : z, nc = t;
            for (j = 0; j <= (jump >= 2) && j < room; j++) {
                s16 dh;
                nc = (u8)(nc + step[k]);
                if (!(W->hw[nc] & 1)) continue;
                dh = (s16)W->hs[nc] - h;
                if (dh > 2 * jump || dh < -2 * jump || (j && (dh < 0 || dh > jump))) continue;
                if (j && W->hl[nc - step[k]] > W->hs[nc]) continue;   // over higher ground
                e[n++] = nc; e[n++] = (u8)(j | (W->hw[nc] & 2));
            }
        }
        W->en[t] = n;
    }
}

// started by spread_start, carried on by spread_go a number of tiles at a time (the AI's spread
// over the map takes several frames); 1 once done
static void spread_start(u8 who, u8 c, u8 limit, u8 *dist, u8 *from, u8 out, const u8 *need, u8 left)
{
    const Unit *un = &st.unit[who];
    u8 *blk = W->blk, i, foe = un->team == TEAM_ENEMY;
    if (W->ej != un->jump) edges(un->jump);
    memset(blk, 0, MAP_W * MAP_H);
    for (i = 0; i < NUNIT; i++) {
        const Unit *u = &st.unit[i];
        if (!DC_OBJ(u->dc) && (u->team == TEAM_ENEMY) != foe) blk[u->z * MAP_W + u->x] = 1;
    }
    memset(dist, 0xFF, MAP_W * MAP_H);
    W->sq_n[0] = W->sq_n[1] = W->sq_n[2] = W->sq_n[3] = 0;
    dist[c] = 0;
    W->sq[0][W->sq_n[0]++] = c;
    W->sp_d = 0; W->sp_i = 0; W->sp_limit = limit; W->sp_out = out; W->sp_left = left;
    W->sp_dist = dist; W->sp_from = from; W->sp_need = need;
}

static u8 spread_go(u16 n)
{
    u8 *blk = W->blk, *sn = W->sq_n, *sq[4], *dist = W->sp_dist, *from = W->sp_from;
    const u8 *hw = W->hw, *en = W->en, *need = W->sp_need;
    u8 d = W->sp_d, i = W->sp_i, b, limit = W->sp_limit, out = W->sp_out, left = W->sp_left;
    for (b = 0; b < 4; b++) sq[b] = W->sq[b];
    for (; d < limit; d++, i = 0) {
        const u8 *q = sq[d & 3];
        b = d & 3;
        if (!sn[0] && !sn[1] && !sn[2] && !sn[3]) break;     // nothing left to go on from
        for (; i < sn[b]; i++) {
            u8 t = q[i], k, m, wet;
            const u8 *e;
            if (dist[t] != d) continue;                       // reached cheaper since
            if (need && need[t] & 1 && !--left) return 1;     // all it was asked for: final
            if (!n--) { W->sp_d = d; W->sp_i = i; W->sp_left = (u8)(left + (need && need[t] & 1)); return 0; }
            wet = (u8)(hw[t] & 2); e = W->eg[t]; m = en[t];
            for (k = 0; k < m; k += 2) {
                u8 nc = e[k], f = e[k + 1], nd;
                if (blk[nc] || (f & 1 && blk[((u16)t + nc) >> 1])) continue;
                nd = (u8)(d + 1 + (f & 1) + (wet && (out || f)));
                if (nd > limit || nd >= dist[nc] || (f & 2 && !wet && nd == limit)) continue;
                dist[nc] = nd;
                if (from) from[nc] = t;
                sq[nd & 3][sn[nd & 3]++] = nc;
            }
        }
        sn[b] = 0;
    }
    return 1;
}

static void bfs(u8 who, u8 *from)
{
    const Unit *un = &st.unit[who];
    spread_start(who, (u8)(un->z * MAP_W + un->x), un->move, st.reach, from, 0, RT_NULL, 0);
    spread_go(0xFFFF);
}

void compute_reach(u8 who)
{
    u8 i;
    hl_seq++;
    bfs(who, bfs_from);                           // the steps kept for make_path
    W->from_who = who + 1;
    W->upd_k += REACH_K;
    for (i = 0; i < NUNIT; i++)                   // can't stop on a unit (a crystal or a chest:
        if (i != who && st.unit[i].x < MAP_W && st.unit[i].z < MAP_H
            && unit_at(st.unit[i].x, st.unit[i].z))       // can, it is picked up)
            st.reach[st.unit[i].z * MAP_W + st.unit[i].x] = 0xFF;
    for (i = 0; i < MAP_W * MAP_H; i++)            // nor on a tree (crossed only)
        if (map_tiles[i].walk == 2) st.reach[i] = 0xFF;
}

// the path to (tx, tz) from the steps of who's reach (made again unless compute_reach made them:
// a spread costs ~90k)
u8 make_path(u8 who, u8 tx, u8 tz)
{
    u8 c = tz * MAP_W + tx, s = st.unit[who].z * MAP_W + st.unit[who].x, n = 0, i, kept = W->from_who == who + 1;
    W->from_who = 0;
    if (!kept) bfs(who, bfs_from);
    if (st.reach[c] == 0xFF) { st.path_n = 0; return 0; }
    for (i = c; i != s; i = bfs_from[i]) n++;     // its steps (a jump is one)
    if (n >= PATH_MAX) { st.path_n = 0; return 0; }
    for (i = n + 1; i-- > 0;) {
        st.path[i][0] = c % MAP_W; st.path[i][1] = c / MAP_W;
        c = bfs_from[c];
    }
    st.path_n = n + 1;                                             // path[0] = the start
    if (!kept) { compute_reach(who); W->from_who = 0; }
    else hl_seq++;
    return st.path_n;
}

// ---------------------------------------------------------------- attack
// FFT's Attack with the battle's weapons (RE_NOTES.md § Attack: read in the decompilation,
// checked on the original's previews and attacks). A sword or knife strikes the four tiles
// around the unit whose standing height is at most 3 below or 2.5 above its own; the target
// facing the attacker evades with its class evade (no shields, accessories or Weapon Guard
// at Gariland), from the side or the back it is always hit. XA: PA (a sword, bare hands) or
// (PA + Speed) / 2 (a knife), changed by the signs' compatibility, then on a critical hit
// (4 %) XA + 0..XA - 1 and the target pushed a tile away; damage XA * WP (bare hands: PA *
// Brave / 100), at most 999.
void attack_range(u8 who)
{
    const Unit *u = &st.unit[who];
    s16 h = map_tiles[u->z * MAP_W + u->x].stand;
    u8 d;
    hl_seq++;
    memset(st.reach, 0xFF, sizeof st.reach);
#ifdef FFT_OPENING
    if (u->eq[EQ_WEAPON] != 255 && opening_range[u->eq[EQ_WEAPON]] > 1) {
        u8 c, o = (u8)(u->z * MAP_W + u->x);
        for (c = 0; c < MAP_W * MAP_H; c++) if (opening_attack_range(who, o, c)) st.reach[c] = 1;
        return;
    }
#endif
    for (d = 0; d < 4; d++) {
        s16 x = u->x + dir_x[d], z = u->z + dir_z[d];
        const Tile *t = tile_at(x, z);
        if (t && t->walk && t->stand + 6 >= h && t->stand <= h + 5) st.reach[z * MAP_W + x] = 1;
    }
}

// from the target: the attack's dominant axis against its facing (a tie: either axis)
u8 attack_side(u8 a, u8 t)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 dx = (s16)ua->x - ut->x, dz = (s16)ua->z - ut->z;
    s16 ax = dx < 0 ? -dx : dx, az = dz < 0 ? -dz : dz, c = 0;
    if (!ax && !az) return SIDE_BACK;
    if (ax >= az) c += dir_x[ut->face] * (dx < 0 ? -1 : dx > 0);
    if (az >= ax) c += dir_z[ut->face] * (dz < 0 ? -1 : dz > 0);
    return c > 0 ? SIDE_FRONT : c < 0 ? SIDE_BACK : SIDE_SIDE;
}

// from the front the target evades with its class evade; Defending halves the base hit (100):
// the chance (base - evade) * 100 / base
u8 hit_chance(u8 a, u8 t)
{
#ifdef FFT_OPENING
    return opening_hit(a,t);
#else
    const Unit *ut = &st.unit[t];
    u8 base = ut->status & ST_DEFEND ? 50 : 100, ev = attack_side(a, t) == SIDE_FRONT ? jobs[ut->job].cev : 0;
    return (u8)((base - ev) * 100 / base);
#endif
}

// XA = x changed by the signs' compatibility (at least 1)
static s16 zodiac_xa(const Unit *ua, const Unit *ut, s16 xa)
{
    u8 c = zodiac_compat[ua->zodiac > ut->zodiac ? ua->zodiac - ut->zodiac : ut->zodiac - ua->zodiac];
    u16 p = (u16)(xa * 25);
    if (c == 3) c = ua->female == ut->female ? 4 : 5;       // opposite signs: worst, best
    if (c == 1) xa -= p / 100;
    else if (c == 2) xa += p / 100;
    else if (c == 4) xa -= p / 50;
    else if (c == 5) xa += p / 50;
    return xa < 1 ? 1 : xa;
}

u16 attack_damage(u8 a, u8 t, u8 crit, u16 r)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    const Item *w = ua->eq[EQ_WEAPON] != 0xFF ? &items[ua->eq[EQ_WEAPON]] : RT_NULL;
    s16 xa = zodiac_xa(ua, ut, w && w->knife ? (ua->pa + ua->sp) >> 1 : ua->pa), ya;
    u16 d;
#ifdef FFT_OPENING
    if (st.pending[t].state == 1) xa += xa >> 1; // source XA * 3 / 2 while Charging
#endif
    if (crit) xa += (s16)(((u32)r * (u16)xa) >> 15);
    ya = w ? w->wp : (s16)((u16)(ua->pa * ua->brave) / 100);
    if (ya < 1) ya = 1;
    d = (u16)(xa * ya);
    return d > 999 ? 999 : d;
}

// the evades rolled: a miss when rand() * base / 32768 < the evade (base 100, 50 Defending)
static u8 roll_hit(u8 a, u8 t)
{
#ifdef FFT_OPENING
    return (u16)(((u32)rand15()*100)>>15)<opening_hit(a,t);
#else
    const Unit *ut = &st.unit[t];
    u8 base = ut->status & ST_DEFEND ? 50 : 100, ev = attack_side(a, t) == SIDE_FRONT ? jobs[ut->job].cev : 0;
    return (u8)(((u32)rand15() * base >> 15) >= ev);
#endif
}

// the act starts: the user turns to its target (M_ACT)
static void act_start(u8 a, u8 t)
{
    Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 dx = (s16)ut->x - ua->x, dz = (s16)ut->z - ua->z;
    st.atk = a; st.tgt = t; st.act_t = 0; st.mode = M_ACT;
    st.crit = st.push = 0; st.dmg = st.self = 0; st.gain = st.lvup = st.jpg = 0;
    if (W) W->num_ok = 0;
    if (dx || dz)
        ua->face = (dx < 0 ? -dx : dx) > (dz < 0 ? -dz : dz) ? (dx > 0 ? 0 : 1) : (dz > 0 ? 2 : 3);
}

// knockback: one tile away along the dominant axis (a tie: z), onto a walkable, free, dry tile
// no higher (FFT compares the tiles' facing edges), not when the blow kills; a fall of more
// than Jump levels costs (levels - Jump) * max HP / 10
static void knockback(u8 a, u8 t)
{
    Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 dx = (s16)ut->x - ua->x, dz = (s16)ut->z - ua->z, kx = ut->x, kz = ut->z;
    const Tile *s = &map_tiles[ut->z * MAP_W + ut->x], *n;
    if (st.dmg >= ut->hp || (!dx && !dz)) return;
    if ((dx < 0 ? -dx : dx) > (dz < 0 ? -dz : dz)) kx += dx > 0 ? 1 : -1;
    else kz += dz > 0 ? 1 : -1;
    n = tile_at(kx, kz);
    if (n && n->walk == 1 && !n->water && n->stand <= s->stand && !unit_at(kx, kz)) {
        s16 fall = ((s->stand - n->stand) >> 1) - ut->jump;
        st.push = 1; st.kx = (u8)kx; st.kz = (u8)kz;
        if (fall > 0) st.dmg += (u16)((u32)fall * ut->max_hp / 10);
        if (st.dmg > 999) st.dmg = 999;
    }
}

// the hit rolled, then the critical (4 %: XA + 0..XA - 1 and a knockback) and the damage
void attack(u8 a, u8 t)
{
#ifdef FFT_OPENING
    st.pending[a].state = 0;
#endif
    act_start(a, t);
    st.abil = NO_ABIL;
    st.hit = roll_hit(a, t);
    if (!st.hit) return;
    st.crit = (u8)(((u32)rand15() * 100 >> 15) < 4);
    st.dmg = attack_damage(a, t, st.crit, st.crit ? rand15() : 0);
    if (st.crit) knockback(a, t);
}

// ---------------------------------------------------------------- abilities
// FFT's abilities at Gariland (RE_NOTES.md § Abilities: read in the decompilation, checked on
// the original's previews and outcomes): what a unit knows is drawn with it (battle_units)
u8 known(u8 who, u8 k)
{
    const Unit *u = &st.unit[who];
    const u8 *a = skill_sets[u->set].abil;
    u8 i;
    for (i = 0; i < SET_N && a[i] != 0xFF; i++)
        if (u->learned & (1 << i) && !k--) return a[i];
    return 0xFF;
}

u8 can_use(u8 who, u8 ab)
{
    return !(abilities[ab].flags & AF_ITEM) || st.unit[who].team != TEAM_PLAYER || st.bag[abilities[ab].bag];
}

// the tiles at most `range` steps away (FFT spreads the range over the map, heights aside),
// walkable, within the vertical tolerance, the user's own only with AF_SELF
void skill_range(u8 who, u8 ab)
{
    const Ability *b = &abilities[ab];
    const Unit *u = &st.unit[who];
    s16 h = map_tiles[u->z * MAP_W + u->x].stand, x, z;
    hl_seq++;
    memset(st.reach, 0xFF, sizeof st.reach);
    for (z = 0; z < MAP_H; z++)
        for (x = 0; x < MAP_W; x++) {
            s16 dx = x - u->x, dz = z - u->z, d = (dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz);
            const Tile *t = &map_tiles[z * MAP_W + x];
            if (d > b->range || (!d && !(b->flags & AF_SELF)) || !t->walk) continue;
            if (b->vert != 0xFF && (t->stand > h ? t->stand - h : h - t->stand) > b->vert) continue;
            st.reach[z * MAP_W + x] = 1;
        }
}

// Throw Stone is evaded as an attack; Dash, Wish and the items always land; a cure needs one of
// its statuses on the target, Phoenix Down a KO target
u8 skill_hit(u8 a, u8 t, u8 ab)
{
    const Ability *b = &abilities[ab];
    const Unit *ut = &st.unit[t];
    if (b->kind == AB_CURE) return ut->status & b->y ? 100 : 0;
    if (b->kind == AB_REVIVE) return ut->hp ? 0 : 100;
    return b->flags & AF_EVADE ? hit_chance(a, t) : 100;
}

// AB_DAMAGE: (r * Y / 32768 + 1) * XA, XA the user's PA and the signs (no critical); AB_HEAL:
// Y x 10; AB_REVIVE: r * Y / 32768 + 1; AB_WISH: twice the user's max HP / 5
u16 skill_amount(u8 a, u8 t, u8 ab, u16 r)
{
    const Ability *b = &abilities[ab];
    const Unit *ua = &st.unit[a];
    u16 k = (u16)(((u32)r * b->y >> 15) + 1);
#ifdef FFT_OPENING
    if (b->kind == AB_RAISE || b->kind == AB_BREAK) return b->y;
    if (b->flags & AF_WEAPON) {
        const Item *w = ua->eq[EQ_WEAPON] == 255 ? RT_NULL : &items[ua->eq[EQ_WEAPON]];
        s16 pa = ua->pa,xa;
        if (b->flags & AF_CHARGE) {
            if (w && w->knife) pa = (pa + ua->sp) >> 1;
            xa=zodiac_xa(ua,&st.unit[t],pa+b->y);
            if(st.pending[t].state==1)xa+=xa>>1;
            return (u16)(xa * (w ? w->wp : 1));
        }
        xa=zodiac_xa(ua,&st.unit[t],pa);
        if(st.pending[t].state==1)xa+=xa>>1;
        return (u16)(xa * ((w ? w->wp : 1) + b->y));
    }
#endif
    if (b->kind == AB_DAMAGE) {
        u16 d = (u16)(k * zodiac_xa(ua, &st.unit[t], ua->pa));
        return d > 999 ? 999 : d;
    }
    if (b->kind == AB_HEAL) return (u16)(b->y * 10);
    if (b->kind == AB_REVIVE) return k;
    if (b->kind == AB_WISH) return (u16)(ua->max_hp / 5 * 2);
    return 0;
}

// rolled and started (M_ACT); an item leaves the bag whatever the outcome; Dash and Throw Stone
// push the target when rand() * its Brave / 32768 < rand() * the user's Brave / 32768
void skill(u8 a, u8 t, u8 ab)
{
    const Ability *b = &abilities[ab];
#ifdef FFT_OPENING
    if (!st.open_resolving) {
        st.pending[a].state = 0;
        if (b->flags & AF_CHARGE) {
            st.pending[a].state = 1;
            st.pending[a].ticks = opening_charge_ticks[b->y - 1];
            st.pending[a].ability = ab;
            st.pending[a].tile = st.unit[t].z * MAP_W + st.unit[t].x;
            st.pending[a].origin = st.unit[a].z * MAP_W + st.unit[a].x;
            st.pending[a].weapon = st.unit[a].eq[EQ_WEAPON];
            // Retail Charge commits movement as well as the action.
            st.moved = st.acted = 1;
            st.mode = M_MENU; st.menu = MN_WAIT;
            if (st.ai) ai_next();
            return;
        }
    }
#endif
    act_start(a, t);
    st.abil = ab;
    if (b->flags & AF_ITEM && st.unit[a].team == TEAM_PLAYER) {
        st.bag[b->bag]--;
        if (W) W->win_k[0] = 0;                    // the item list shows the count
    }
    st.hit = skill_hit(a, t, ab) == 100;
    if (b->flags & AF_EVADE) st.hit = roll_hit(a, t);
    if (!st.hit) return;
    st.dmg = skill_amount(a, t, ab, rand15());
    if (b->kind == AB_CURE) st.dmg = st.unit[t].status & b->y;   // the statuses removed (shown)
    if (b->kind == AB_WISH) st.self = st.unit[a].max_hp / 5;
    if (b->kind == AB_DAMAGE
#ifdef FFT_OPENING
        && !(b->flags & AF_WEAPON)
#endif
    ) {
        u16 r = (u16)((u32)rand15() * st.unit[a].brave >> 15);
        if (((u32)rand15() * st.unit[t].brave >> 15) < r) knockback(a, t);
    }
}

// Defend (a unit whose support ability it is): Defending until its next turn, its evades doubled
void defend(u8 a)
{
    act_start(a, a);
    st.abil = ABIL_DEFEND; st.hit = 1;
}

u8 act_kind(void)
{
    return st.abil == NO_ABIL ? AB_DAMAGE : st.abil == ABIL_DEFEND ? AB_DEFEND : abilities[st.abil].kind;
}

// ---------------------------------------------------------------- KO and the end
// FFT's rewards (RE_NOTES.md § KO and the end: read in the decompilation, checked on the
// original): an action that did something earns its user 10 EXP (a kill: 20 / the times the
// target was KO) + the target's level - the user's, 1 to 99; at 100 EXP a level up (EXP back
// to 0, the stats grown, HP and MP kept); and 8 + 2 x its job level + its level / 4 JP, a
// quarter of them to each living unit of its side in the same job. A KO user earns nothing.
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define job_jp CONTOUR_REF_job_jp
#else
static const u16 job_jp[8] = { 100, 200, 350, 550, 800, 1150, 1550, 2100 };
#endif   // job levels

u8 job_level(const Unit *u)
{
    u8 k = 0;
    while (k < 8 && u->tjp >= job_jp[k]) k++;
    return k;
}

static void add_jp(Unit *u, u16 jp)
{
    u->jp = u->jp + jp > 9999 ? 9999 : u->jp + jp;
    u->tjp = u->tjp + jp > 9999 ? 9999 : u->tjp + jp;
    u->gjp += jp;
}

void rewards(u8 a, u8 t, u8 kill)
{
    Unit *ua = &st.unit[a], *ut = &st.unit[t], *u;
    s16 e = (kill ? 20 / ut->kos : 10) + ut->level - ua->level;
    u16 jp;
    u8 i, blue = ua->team != TEAM_ENEMY;
    if (!ua->hp) return;
    if (e < 1) e = 1;
    if (e > 99) e = 99;
    st.gain = (u8)e;
    jp = (u16)(8 + 2 * job_level(ua) + (ua->level >> 2));
    st.jpg = (u8)jp;
    ua->gexp += (u16)e;
    ua->exp += (u8)e;
    if (ua->exp >= EXP_LEVEL) {
        if (ua->level < 99) {                       // the stats grown from the raw ones
            u16 hp = ua->hp, mp = ua->mp;
            ua->level++; ua->exp = 0; st.lvup = 1;
            unit_stats(ua);
            ua->hp = hp < ua->max_hp ? hp : ua->max_hp;
            ua->mp = mp < ua->max_mp ? mp : ua->max_mp;
        } else ua->exp = 99;
    }
    add_jp(ua, jp);
    for (i = 0, u = st.unit; i < NUNIT; i++, u++)
        if (u != ua && u->hp && (u->team != TEAM_ENEMY) == blue && jobs[u->job].row == jobs[ua->job].row)
            add_jp(u, jp >> 2);
}

// victory: every enemy KO (or a crystal or a chest); defeat: Ramza a crystal or a chest, or
// every unit of the player's side KO, the guest included (FFT checks the defeat first)
u8 outcome(void)
{
    u8 i, foes = 0, blue = 0;
    const Unit *u;
    for (i = 0, u = st.unit; i < NUNIT; i++, u++)
        if (u->hp) { if (u->team == TEAM_ENEMY) foes++; else blue++; }
    if (DC_OBJ(st.unit[U_RAMZA].dc) || !blue) return END_DEFEAT;
    return foes ? END_NONE : END_VICTORY;
}

// a crystal gives back all HP and MP (FFT also offers its abilities: not kept); a chest one of
// the dead unit's equipment, drawn (shown: there is no inventory); then the object is gone
void pick_up(u8 who, u8 obj)
{
    Unit *u = &st.unit[who], *o = &st.unit[obj];
    if (o->dc == DC_CRYSTAL) {
        u->hp = u->max_hp; u->mp = u->max_mp;
        st.pick = 0xFF;
    } else {
        u8 k, n = 0, eq[EQ_N];
        for (k = 0; k < EQ_N; k++) if (o->eq[k] != 0xFF) eq[n++] = o->eq[k];
        st.pick = eq[(u32)rand15() * n >> 15];
    }
    o->dc = DC_GONE; o->x = 0xFF;
    st.mode = M_PICK; st.pick_t = 0;
}

static void end_battle(u8 e)
{
    st.end = e; st.mode = M_END; st.ai = AI_OFF;
    bn_start(e == END_VICTORY ? BN_VICTORY : BN_DEFEAT);   // then the results
}

// M_ACT, once per frame: the outcome lands at ACT_HIT (HP lost or healed, statuses removed, the
// target pushed; a KO or a revival: the turn order again), the menu comes back at ACT_END or,
// after the EXP and the JP shown, ACT_GAIN_END (the turn ends if the user fell: Wish)
static void attack_step(void)
{
    Unit *ut = &st.unit[st.tgt], *ua = &st.unit[st.atk];
    if (st.act_t == ACT_HIT && st.hit) {
        u8 kind = act_kind(), ko = !ut->hp;
#ifdef FFT_OPENING
        if (kind == AB_RAISE) { if (ua->pa < 99) ua->pa++; rewards(st.atk,st.tgt,0); goto shown; }
        if (kind == AB_BREAK) {
            u8 *v = abilities[st.abil].y == 2 ? &ut->sp : &ut->ma;
            *v = *v > abilities[st.abil].y ? *v - abilities[st.abil].y : 1;
            rewards(st.atk, st.tgt, 0); goto shown;
        }
#endif
        if (kind == AB_DEFEND) {
            ut->status |= ST_DEFEND;
            goto shown;
        }
        if (kind == AB_DAMAGE) {
            ut->hp = st.dmg >= ut->hp ? 0 : ut->hp - st.dmg;
            if (st.push) { ut->x = st.kx; ut->z = st.kz; }
            if (!ut->hp) ut->kos++;
        } else if (kind == AB_CURE) ut->status &= (u8)~abilities[st.abil].y;
        else {
            ut->hp = st.dmg >= ut->max_hp - ut->hp ? ut->max_hp : ut->hp + st.dmg;
            if (ko) ut->dc = 3;
        }
#ifdef FFT_OPENING
        opening_damage();
#endif
        rewards(st.atk, st.tgt, kind == AB_DAMAGE && !ut->hp);
        if (st.self) ua->hp = st.self >= ua->hp ? 0 : ua->hp - st.self;
        if ((u8)!ut->hp != ko || !ua->hp) turn_order();
    }
shown:
    if (++st.act_t >= (st.gain ? ACT_GAIN_END : ACT_END)) {
        if (outcome()) { end_battle(outcome()); return; }
#ifdef FFT_OPENING
        if (st.open_resolving) { st.open_resolving = 0; turn_start(); return; }
#endif
        if (!st.unit[st.act].hp) { turn_end(st.moved, 1); return; }
        st.mode = M_MENU; st.menu = st.moved ? MN_WAIT : MN_MOVE;
        st.cx = st.unit[st.act].x; st.cz = st.unit[st.act].z;
        if (st.ai) ai_next();
    }
}

#endif
// ---------------------------------------------------------------- the AI
// FFT's AI for the enemies and the guest (RE_NOTES.md § Enemy AI: read in the decompilation,
// checked on the original's turns). FFT plays each candidate in its battle engine, random rolls
// at their middle (no critical), and scores the field: a unit is worth HP * 128 / max HP, -32
// Critical, -192 KO, the actor's side +, the other -; here that change is computed on the units
// an action touches. A turn: a focus (the nearest enemy; a guest under 45/128 HP or a Critical
// unit retreating: the nearest unit that heals, or a crystal), each tile's danger (10 - steps,
// summed over the enemies acting before its next turn), the steps from the focus, the tiles to
// act from and to fall back to; each usable ability's best (origin, target) kept if it beats
// standing on the fallback tile, the best eight drawn in order on their hit chance (none: to the
// fallback tile, Defend if it has it); acting from its own tile, it acts first, then moves to
// the safest tile. The setup, then an ability per frame.
#define MAP_N (MAP_W * MAP_H)
#define AI_UNREACHED 125                // FFT's steps to a tile its spread does not reach
#define AI_BAD (-0x7FFF)
enum { NT_ENEMY, NT_HEALER, NT_IMMINENT, NT_ANY_ENEMY };

static u8 blue(const Unit *u) { return u->team != TEAM_ENEMY; }

static u8 tile_h(const Tile *t)         // FFT's tile height: its lowest corner
{
    u8 h = t->c[0];
    if (t->c[1] < h) h = t->c[1];
    if (t->c[2] < h) h = t->c[2];
    if (t->c[3] < h) h = t->c[3];
    return h;
}

#if MOD_PART(MOD_AI)                    // with modules: the AI module (module.h)
// a unit's worth with hp HP and those statuses (Poison -26, Darkness -64: FFT's weights)
static s16 worth(const Unit *u, u16 hp, u8 status)
{
    s16 s;
    if (!hp) return -192;
    s = divs32_16((long)hp << 7, (short)u->max_hp);
    if ((u16)(hp * 5) <= u->max_hp) s -= 32;
    if (status & ST_POISON) s -= 26;
    if (status & ST_DARK) s -= 64;
    return s;
}

// clock ticks before a unit acts (a KO one, a crystal or a chest: never)
static u16 ai_ticks(const Unit *u)
{
    if (!u->hp) return 0xFFFF;
    return u->ct >= 100 ? 0 : (u16)((u16)(100 - u->ct + u->sp - 1) / u->sp);
}

// an ability the unit can use now: the enemies' items of their level, the party's in the bag
static u8 ai_usable(u8 a, u8 ab)
{
    const Unit *u = &st.unit[a];
    if (!(abilities[ab].flags & AF_ITEM)) return 1;
    return u->team == TEAM_ENEMY ? abilities[ab].lv <= u->level : st.bag[abilities[ab].bag] != 0;
}

// its range for the AI (an item: at most the unit's Move, as FFT's AI takes it)
static u8 ai_range(u8 a, u8 ab)
{
#ifdef FFT_OPENING
    if (ab == NO_ABIL) return st.unit[a].eq[EQ_WEAPON] == 255 ? 1 : opening_range[st.unit[a].eq[EQ_WEAPON]];
#endif
    if (ab == NO_ABIL) return 1;
    if (abilities[ab].flags & AF_ITEM && st.unit[a].move < abilities[ab].range) return st.unit[a].move;
    return abilities[ab].range;
}

// whether ab used from tile o reaches tile c (attack_range and skill_range's rules)
static u8 ai_reaches(u8 a, u8 ab, u8 o, u8 c)
{
    const Tile *s = &map_tiles[o], *t = &map_tiles[c];
    s16 dx = (s16)TILE_X[c] - TILE_X[o], dz = (s16)TILE_Z[c] - TILE_Z[o], h = (s16)t->stand - s->stand;
    u8 d = (u8)((dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz));
    if (!t->walk) return 0;
#ifdef FFT_OPENING
    if (ab == NO_ABIL) return opening_attack_range(a, o, c);
#endif
    if (ab == NO_ABIL) return d == 1 && h >= -6 && h <= 5;
    if (d > ai_range(a, ab) || (!d && !(abilities[ab].flags & AF_SELF))) return 0;
    return abilities[ab].vert == 0xFF || (h < 0 ? -h : h) <= abilities[ab].vert;
}

// worth using ab on t (FFT's classify): damage on a living enemy, HP on an ally under half HP,
// a cure on an ally with one of its statuses, a revival on a KO ally
static u8 ai_useful(u8 a, u8 ab, u8 t)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    u8 kind = ab == NO_ABIL ? AB_DAMAGE : abilities[ab].kind;
    if (DC_OBJ(ut->dc)) return 0;
    if (kind == AB_DAMAGE) return ut->hp && blue(ut) != blue(ua) && t != a;
    if (blue(ut) != blue(ua)) return 0;
    if (kind == AB_REVIVE) return !ut->hp;
    if (!ut->hp) return 0;
    if (kind == AB_CURE) return (ut->status & abilities[ab].y) != 0;
    return (u16)(ut->hp << 1) < ut->max_hp;
}

// a unit that can give HP to its allies (FFT's CAN_HEAL_ALLIES: Potion, Wish)
static u8 ai_heals(u8 i)
{
    u8 k, ab;
    for (k = 0; (ab = known(i, k)) != 0xFF; k++)
        if ((abilities[ab].kind == AB_HEAL || abilities[ab].kind == AB_WISH) && ai_usable(i, ab)) return 1;
    return 0;
}

// the nearest unit (steps heights aside, a tie: the first) of a kind; the actor if none
static u8 ai_nearest(u8 a, u8 mode, u16 skip)
{
    const Unit *ua = &st.unit[a];
    u16 lim = ai_ticks(ua);
    u8 i, best = a, bd = 0xFF;
    for (i = 0; i < NUNIT; i++) {
        const Unit *u = &st.unit[i];
        s16 dx = (s16)u->x - ua->x, dz = (s16)u->z - ua->z;
        u8 d;
        if (u->dc == DC_GONE || skip & (1 << i)) continue;
        if (mode == NT_HEALER && u->dc == DC_CRYSTAL) ;     // a crystal heals too
        else if (!u->hp) continue;
        else if (mode == NT_HEALER) { if (blue(u) != blue(ua) || !ai_heals(i)) continue; }
        else if (blue(u) == blue(ua)) continue;
        d = (u8)((dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz));
        if (mode == NT_IMMINENT && (ai_ticks(u) > lim || (d < 2 &&
            (s16)tile_h(&map_tiles[u->z * MAP_W + u->x]) - tile_h(&map_tiles[ua->z * MAP_W + ua->x]) < -2))) continue;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

// the Move points from tile c over the map by unit a's moves (FFT's spread from its focus), as
// far as the tiles it can stop on
static void ai_flood(u8 a, u8 c)
{
    u8 i, n = 0;
    for (i = 0; i < MAP_N; i++) n += W->ai_m[i] & 1;        // until its own tiles' are final
    spread_start(a, c, AI_UNREACHED - 1, W->ai_dist, RT_NULL, 1, W->ai_m, n);
}

// what stepping on tile c gives unit a: a crystal's HP, +2 for a crystal or a chest unless it is
// Critical or retreating (FFT's tile priority, negated)
static s16 ai_gain(u8 a, u8 c)
{
    const Unit *u = &st.unit[a];
    s16 g = 0;
    u8 i;
    for (i = 0; i < NUNIT; i++) {
        const Unit *o = &st.unit[i];
        if (!DC_OBJ(o->dc) || o->dc == DC_GONE || o->z * MAP_W + o->x != c) continue;
        if (o->dc == DC_CRYSTAL) g += worth(u, u->max_hp, u->status) - worth(u, u->hp, u->status);
        if (!(u->hp && (u16)(u->hp * 5) <= u->max_hp) && !W->ai_retreat) g += 2;
    }
    return g;
}

// FFT's choice of a tile among those of set bit: the lowest gain-and-danger key, then the
// fewest steps from the focus (dist_first: the fewest steps, then the key); a tie: the later
static u8 ai_select(u8 bit, s16 maxd, u8 dist_first)
{
    s16 best = 0x7FFF, bd = 0x7FFF;
    u8 c, r = 0xFF;
    for (c = 0; c < MAP_N; c++) {
        s16 sc;
        s16 d;
        if (!(W->ai_m[c] & bit)) continue;
        d = W->ai_dist[c];
        if (maxd < d) continue;
        sc = W->ai_key[c];
        if (dist_first) {
            if (maxd == d && best < sc) continue;
            maxd = d;
        } else {
            if (best < sc) continue;
            if (sc == best && bd < d) continue;
        }
        best = sc; bd = d; r = c;
    }
    return r;
}

static u8 ai_select_or_nearest(u8 bit, s16 maxd)
{
    u8 c = ai_select(bit, maxd, 0);
    return c != 0xFF ? c : ai_select(bit, 0x7FFF, 1);
}

// FFT's direct trajectory (battle_effect_check_direct_trajectory_between_units: Throw Stone, an
// item thrown 2 tiles or more): a straight line from the user's body to the target's (two
// thirds of a unit's 36-unit height above its feet; a KO target: 12), a point every 2 units,
// blocked inside a tile's column (to its height) or another standing unit's 16 x 16 x 36 box
// (world units: a tile 28, a height level 12)
static u16 isqrt(u32 v)
{
    u16 r = 0, b = 0x100;
    while (b) { if ((u32)(r | b) * (r | b) <= v) r |= b; b >>= 1; }
    return r;
}

static u8 ai_clear(u8 a, u8 t)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 x0 = (s16)(ua->x * 28 + 14), z0 = (s16)(ua->z * 28 + 14), x1 = (s16)(ut->x * 28 + 14), z1 = (s16)(ut->z * 28 + 14);
    s16 lx = (x0 < x1 ? x0 : x1) - 8, hx = (x0 < x1 ? x1 : x0) + 8, lz = (z0 < z1 ? z0 : z1) - 8, hz = (z0 < z1 ? z1 : z0) + 8;
    s32 x = (s32)x0 << 16, z = (s32)z0 << 16, y, dx, dy, dz;
    s16 ox[NUNIT], oz[NUNIT], oy[NUNIT];                    // the boxes of the other standing units
    u16 l, n, i;                                            // near the line
    u8 k, m = 0;
    for (k = 0; k < NUNIT; k++) {
        const Unit *u = &st.unit[k];
        s16 ux = (s16)(u->x * 28 + 14), uz = (s16)(u->z * 28 + 14);
        if (k == a || k == t || !u->hp || ux <= lx || ux >= hx || uz <= lz || uz >= hz) continue;
        ox[m] = ux; oz[m] = uz; oy[m++] = (s16)(W->hs[u->z * MAP_W + u->x] * 6);
    }
    y = ((s32)W->hs[ua->z * MAP_W + ua->x] * 6 + 24) << 16;
    dx = ((s32)x1 << 16) - x;
    dz = ((s32)z1 << 16) - z;
    dy = (((s32)W->hs[ut->z * MAP_W + ut->x] * 6 + (ut->hp ? 24 : 12)) << 16) - y;
    l = isqrt((u32)((dx >> 16) * (dx >> 16) + (dy >> 16) * (dy >> 16) + (dz >> 16) * (dz >> 16)));
    if (!l) return 1;
    n = l >> 1;
    dx = dx / l * 2; dy = dy / l * 2; dz = dz / l * 2;     // a step: 2 units along the line
    for (i = 0; i < n; i++) {
        s16 px, pz, py;
        u8 c;
        x += dx; y += dy; z += dz;
        px = (s16)(x >> 16); pz = (s16)(z >> 16); py = (s16)(y >> 16);
        if (py <= 0 || (u16)px >= MAP_W * 28 || (u16)pz >= MAP_H * 28) continue;
        c = (u8)(TILE_COL[px] + TILE_ROW[pz]);                  // the tile under the point
#ifdef FFT_OPENING
        if ((u16)py <= W->top[c]) return 0; // py > 0 above; tower blockers require a word
#else
        if (py <= W->top[c]) return 0;
#endif
        for (k = 0; k < m; k++)
            if (py > oy[k] && py < oy[k] + 36 && px > ox[k] - 8 && px < ox[k] + 8 && pz > oz[k] - 8 && pz < oz[k] + 8)
                return 0;
    }
    return 1;
}

// FFT's score of actor a standing on o (already there) using ab on t: the worth changed, the
// pickup at o; its hit chance (AI_BAD: no use)
static s16 ai_score(u8 a, u8 ab, u8 t, u8 o, u8 *hit)
{
    Unit *ua = &st.unit[a], *ut = &st.unit[t];
    u8 kind = ab == NO_ABIL ? AB_DAMAGE : abilities[ab].kind, status = ut->status;
    u16 hp = ut->hp, m;
    s16 d;
    *hit = ab == NO_ABIL ? hit_chance(a, t) : skill_hit(a, t, ab);
    if (!*hit) return AI_BAD;
    if (ab != NO_ABIL && abilities[ab].range > 1 && (!(abilities[ab].flags & AF_ITEM)
        || (u8)((ua->x > ut->x ? ua->x - ut->x : ut->x - ua->x) + (ua->z > ut->z ? ua->z - ut->z : ut->z - ua->z)) >= 2)
        && (W->ai_traj = 1, !ai_clear(a, t))) return AI_BAD;
    if (kind == AB_DAMAGE) {
        m = ab == NO_ABIL ? attack_damage(a, t, 0, 0) : skill_amount(a, t, ab, 0x4000);
        hp = m >= hp ? 0 : hp - m;
    } else if (kind == AB_CURE) status &= (u8)~abilities[ab].y;
    else {
        m = skill_amount(a, t, ab, 0x4000);
        hp = m >= ut->max_hp - hp ? ut->max_hp : hp + m;
    }
    d = worth(ut, hp, status) - worth(ut, ut->hp, ut->status);
    if (blue(ut) != blue(ua)) d = -d;
#ifdef FFT_OPENING
    if (kind == AB_RAISE) return 0; // no unlimited self-buffing in guest AI
    if (kind == AB_BREAK) return AI_BAD; // conservative guest/enemy action policy
    if(ab!=NO_ABIL&&(abilities[ab].flags&AF_CHARGE)
        && ut->ct+(u16)ut->sp*opening_charge_ticks[abilities[ab].y-1]>=100) d >>= 1;
#endif
    if (kind == AB_WISH) {
        m = ua->max_hp / 5;
        d += worth(ua, m >= ua->hp ? 0 : ua->hp - m, ua->status) - worth(ua, ua->hp, ua->status);
    }
    return d + W->ai_gain[o];
}

// a candidate into the ranking (FFT: above the fallback tile's score; then by score, hit
// chance, the origin's danger; a tie: the earlier)
static u8 ai_better(s16 pri, u8 hit, u8 o, const AiAct *e)
{
    if (pri != e->pri) return pri > e->pri;
    if (hit != e->hit) return hit > e->hit;
    return W->ai_prox[o] < W->ai_prox[e->o];
}

static void ai_insert(const AiAct *c)
{
    AiAct *r = W->ai_rank;
    u8 i, j;
    if (c->pri <= W->ai_base) return;
    for (i = 0; i < W->ai_n && !ai_better(c->pri, c->hit, c->o, &r[i]); i++) ;
    if (i >= 8) return;
    for (j = W->ai_n < 8 ? W->ai_n : 7; j > i; j--) r[j] = r[j - 1];
    r[i] = *c;
    if (W->ai_n < 8) W->ai_n++;
}

// one candidate: a (standing on o) using ab on unit i of tile t, kept if better than *best;
// whether it could be tried (a hit chance, a clear trajectory)
static u8 ai_try(u8 a, u8 ab, u8 o, u8 t, u8 i, AiAct *best)
{
    u8 hit;
    s16 p = ai_score(a, ab, i, o, &hit);
    if (p == AI_BAD) return 0;
    if (best->pri == AI_BAD || ai_better(p, hit, o, best)) {
        best->pri = p; best->hit = hit; best->ab = ab; best->o = o; best->t = t;
    }
    return 1;
}

// ability ab's best candidate into the ranking (only on the actor itself: self). Attack from
// every tile; the others try each target once, from the safest tile that reaches it (an item
// the actor could use on itself: from every tile); an ability that can reach the user also from
// its own tile and the tile it would move to. Started by ai_ability, carried on by
// ai_ability_go a number of candidates at a time (1 once done, its best ranked).
static void ai_ability(u8 a, u8 ab, u8 self)
{
    Unit *ua = &st.unit[a];
    u8 x0 = ua->x, z0 = ua->z, me = (u8)(z0 * MAP_W + x0), i, n;
    AiAct *best = &W->ab_best;
    W->ab = ab; W->ab_k = 0; W->ab_done = 0; W->ab_n = 0;
    W->ab_all = ab == NO_ABIL || (abilities[ab].flags & AF_ITEM && ai_useful(a, ab, a));
    W->ai_use = 0;
    for (i = 0; i < NUNIT; i++)
        if ((!self || i == a) && ai_useful(a, ab, i)) W->ai_use |= (u16)(1 << i);
    best->pri = AI_BAD; best->hit = 0; best->f = 0;
    if (!W->ai_use) { W->ab_k = W->ai_no; return; }
    if (ab != NO_ABIL && abilities[ab].flags & AF_SELF && W->ai_use & (1 << a)) {
        if (ai_try(a, ab, W->ai_c1, me, a, best)) best->t = me;  // from its tile, then to the tile
        best->f = best->pri != AI_BAD;                           // it would move to (the same:
        ua->x = TILE_X[W->ai_c1]; ua->z = TILE_Z[W->ai_c1];        // first wins)
        if (ai_try(a, ab, W->ai_c1, W->ai_c1, a, best)) best->f = best->t == me;
        ua->x = x0; ua->z = z0;
        if (best->pri != AI_BAD) ai_insert(best);
        best->pri = AI_BAD; best->f = 0;
    }
    for (i = n = 0; i < NUNIT; i++)             // the targets but itself, in map order
        if (i != a && W->ai_use & (1 << i)) {
            u8 j, c2 = (u8)(st.unit[i].z * MAP_W + st.unit[i].x);
            for (j = n++; j > 0 && W->ai_tt[j - 1] > c2; j--) { W->ai_tt[j] = W->ai_tt[j - 1]; W->ai_tu[j] = W->ai_tu[j - 1]; }
            W->ai_tt[j] = c2; W->ai_tu[j] = i;
        }
    W->ab_n = n;
}

static u8 ai_ability_go(u8 a, s16 *work)
{
    Unit *ua = &st.unit[a];
    u8 x0 = ua->x, z0 = ua->z, ab = W->ab, n = W->ab_n, i, c, o;
    AiAct *best = &W->ab_best;
    for (; W->ab_k < W->ai_no && *work > 0; W->ab_k++) {
        u8 j, self = (u8)(W->ai_use >> a & 1);      // on itself: its tile is the origin
        o = ab == NO_ABIL ? W->ai_grid[W->ab_k] : W->ai_ord[W->ab_k];
        ua->x = TILE_X[o]; ua->z = TILE_Z[o];
        *work -= 1;
        for (j = 0; j <= n; j++) {
            if (self && (j == n || o < W->ai_tt[j])) {
                self = 0;
                if (ai_reaches(a, ab, o, o)) { *work -= 1; ai_try(a, ab, o, o, a, best); }
            }
            if (j == n) break;
            c = W->ai_tt[j]; i = W->ai_tu[j];
            if (W->ab_done & (1 << i) || !ai_reaches(a, ab, o, c)) continue;
            W->ai_traj = 0;
            if (ai_try(a, ab, o, c, i, best) && !W->ab_all) W->ab_done |= (u16)(1 << i);
            *work -= W->ai_traj ? 8 : 1;
        }
    }
    ua->x = x0; ua->z = z0;
    if (W->ab_k < W->ai_no) return 0;
    if (best->pri != AI_BAD) ai_insert(best);
    return 1;
}

// the setup of unit a's decision (acted: it has acted, no ability left), in two frames: its
// abilities, the tiles it can stop on and their danger, the focus; then the steps from the focus
// and the tiles to act from and to fall back to
static void ai_setup(u8 a, u8 acted)
{
    Unit *u = &st.unit[a];
    u8 me = (u8)(u->z * MAP_W + u->x), i, k, c, ab, n = 0, er = 0xFF, ar = 0xFF, ranged = 0, ally = 0, weak = 0;
    u8 ex[NUNIT], ez[NUNIT], ne = 0;
    u16 lim = ai_ticks(u);
    W->ai_n = 0;
    if (!acted) W->ai_list[n++] = NO_ABIL;
    for (k = 0; !acted && (ab = known(a, k)) != 0xFF; k++)
        if (ai_usable(a, ab)) W->ai_list[n++] = ab;
    W->ai_nab = n;
    for (k = 0; k < n; k++) {                   // the reach: the shortest range against an enemy,
        u8 rg;                                  // for an ally (an item counts half)
        ab = W->ai_list[k];
        rg = ai_range(a, ab);
        if (rg > 1) ranged = 1;
        if (ab == NO_ABIL || abilities[ab].kind == AB_DAMAGE) { if (rg < er) er = rg; }
        else {
            if (abilities[ab].flags & AF_ITEM) rg = (u8)((rg + 1) >> 1);
            if (rg < ar) ar = rg;
        }
    }
    W->ai_er = er; W->ai_ar = ar; W->ai_ranged = ranged;
    // the tiles it can stop on (only its own once it has moved), their danger
    memset(W->ai_m, 0, MAP_N);
    if (!st.moved) {
        bfs(a, RT_NULL);
        for (c = 0; c < MAP_N; c++) W->ai_m[c] = st.reach[c] != 0xFF && map_tiles[c].walk == 1;
        for (i = 0; i < NUNIT; i++)
            if (!DC_OBJ(st.unit[i].dc)) W->ai_m[st.unit[i].z * MAP_W + st.unit[i].x] = 0;
    }
    W->ai_m[me] = 1;
    for (i = 0; i < NUNIT; i++) {               // the enemies acting before its next turn
        const Unit *e = &st.unit[i];
        if (e->hp && blue(e) != blue(u) && ai_ticks(e) <= lim) { ex[ne] = e->x; ez[ne++] = e->z; }
    }
    for (c = 0; c < MAP_N; c++) {
        u8 p = 0;
        if (W->ai_m[c])
            for (i = 0; i < ne; i++) {
                s16 dx = (s16)ex[i] - TILE_X[c], dz = (s16)ez[i] - TILE_Z[c], dd;
                dd = (dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz);
                if (dd < 10) p += (u8)(10 - dd);
            }
        W->ai_prox[c] = p;
    }
    // a Critical unit retreats while its side has two units, one not Critical (the enemies lose
    // that for good)
    for (i = 0; i < NUNIT; i++)
        if (st.unit[i].hp && blue(&st.unit[i]) == blue(u)) {
            ally++;
            if ((u16)(st.unit[i].hp * 5) <= st.unit[i].max_hp) weak++;
        }
    if ((ally < 2 || ally == weak) && u->team == TEAM_ENEMY) st.ai_nosup = 1;
    W->ai_retreat = (u16)(u->hp * 5) <= u->max_hp && ally >= 2 && ally != weak && !(u->team == TEAM_ENEMY && st.ai_nosup);
    memset(W->ai_gain, 0, sizeof W->ai_gain);   // a crystal or a chest it can step on
    for (i = 0; i < NUNIT; i++)
        if (DC_OBJ(st.unit[i].dc) && st.unit[i].dc != DC_GONE) {
            c = (u8)(st.unit[i].z * MAP_W + st.unit[i].x);
            if (W->ai_m[c]) W->ai_gain[c] = ai_gain(a, c);
        }
    for (c = 0; c < MAP_N; c++)                 // FFT's tile priority, then the danger
        W->ai_key[c] = (s16)((s16)-W->ai_gain[c] * 128 + W->ai_prox[c]);   // (under 128)
    // the focus, the steps from it (spread over the next frames)
    if (W->ai_retreat || (blue(u) && divs32_16((long)u->hp << 7, (short)u->max_hp) < 45)) W->ai_focus = ai_nearest(a, NT_HEALER, 0);
    else W->ai_focus = ai_nearest(a, NT_ENEMY, 0);
    if (W->ai_focus == a) { memset(W->ai_dist, 0, MAP_N); W->sp_d = W->sp_limit = 0; }
    else ai_flood(a, (u8)(st.unit[W->ai_focus].z * MAP_W + st.unit[W->ai_focus].x));
}

static void ai_setup2(u8 a, u8 acted)
{
    Unit *u = &st.unit[a];
    u8 me = (u8)(u->z * MAP_W + u->x), i, k, c, f = W->ai_focus, er = W->ai_er, ar = W->ai_ar;
    s16 d, r;
    for (i = 0; i < MAP_N; i++) if (W->ai_dist[i] == 0xFF) W->ai_dist[i] = AI_UNREACHED;
    if (W->ai_retreat) {                        // the safest tile, then the nearest to the focus
        W->ai_c0 = W->ai_c1 = ai_select(1, 0xFF, 0);
        W->ai_m[W->ai_c0] |= 2; W->ai_m[me] |= 2;
    } else {
        if ((u16)(u->hp << 1) < u->max_hp)      // under half HP: the crystals draw it
            for (i = 0; i < NUNIT; i++)
                if (st.unit[i].dc == DC_CRYSTAL) W->ai_dist[st.unit[i].z * MAP_W + st.unit[i].x] = 1;
        d = W->ai_dist[me];
        r = f != a && blue(&st.unit[f]) != blue(u) ? (er == 0xFF ? 0xFF : er + u->move) : (ar == 0xFF ? 0 : ar) + u->move;
        k = 0;                                  // set 1: the tiles for the search
        for (c = 0; c < MAP_N; c++)
            if (W->ai_m[c] && W->ai_dist[c] <= (r >= d ? u->move + d : d - 1)) { W->ai_m[c] |= 2; k = 1; }
        if (!k) for (c = 0; c < MAP_N; c++) W->ai_m[c] |= (u8)(W->ai_m[c] << 1);
        if (r >= d) {                           // within reach: the safest tile, and to act from
            W->ai_c1 = ai_select(2, 0x7FFF, 0); // (not yet acted) the safest one step nearer (3
            if (!acted) {                       // steps with a ranged ability on an enemy)
                s16 lim2 = d - 1;
                if (f != a && blue(&st.unit[f]) != blue(u) && W->ai_ranged && lim2 < 3) lim2 = 3;
                W->ai_c0 = ai_select_or_nearest(1, lim2);
            } else W->ai_c0 = W->ai_c1;
        } else {                                // out of reach: nearer, the safest from which
            if (u->team == TEAM_ENEMY && !(rand15() % 7)) r >>= 1;   // to strike next turn
            W->ai_c0 = W->ai_c1 = ai_select_or_nearest(2, r);
        }
        W->ai_m[me] |= 2;
    }
    W->ai_base = W->ai_gain[W->ai_c0];
    // the origins: in map order (Attack), the safest first (the others; a tie: the later)
    W->ai_no = 0;
    for (c = 0; c < MAP_N; c++) {
        s16 kc;
        u8 j;
        if (!(W->ai_m[c] & 2)) continue;
        W->ai_grid[W->ai_no] = c;
        kc = W->ai_key[c];
        for (j = W->ai_no; j > 0 && W->ai_key[W->ai_ord[j - 1]] >= kc; j--)
            W->ai_ord[j] = W->ai_ord[j - 1];
        W->ai_ord[j] = c;
        W->ai_no++;
    }
}

// the ranking drawn: each action taken if rand() % 100 < its hit chance, else the first; none:
// to the fallback tile, Defending if it can
static void ai_choose(u8 a)
{
    Unit *u = &st.unit[a];
    u8 i, me = (u8)(u->z * MAP_W + u->x);
    const AiAct *r = W->ai_rank;
    for (i = 0; i < W->ai_n && rand15() % 100 >= r[i].hit; i++) ;
    if (i >= W->ai_n) i = 0;
    if (W->ai_n) {
        st.ai_ab = r[i].ab; st.ai_o = r[i].f ? me : r[i].o; st.ai_tgt = r[i].t;
    } else {
        st.ai_ab = u->defend && !st.acted ? ABIL_DEFEND : AI_NONE;
        st.ai_o = W->ai_c0; st.ai_tgt = W->ai_c0;
    }
    st.ai_first = st.ai_ab != AI_NONE && st.ai_ab != ABIL_DEFEND && st.ai_o == me;
}

// the thinking, a step a frame: the setup's first half; the spread from the focus, AI_SPREAD
// tiles a frame; the setup's second half; the abilities' candidates, AI_WORK a frame (a tile to
// act from 1, a candidate 1, with a trajectory 8, the next ability 5; ~4k cycles a unit); the
// choice. 1 once the plan is made
#define AI_SPREAD 90
#define AI_WORK 50
static u8 ai_think(void)
{
    u8 a = st.act, done;
    s16 w = AI_WORK;
    if (W->ai_k == 0) {                             // its abilities, tiles, danger, focus
        ai_setup(a, st.acted);
        W->ai_k = 1;
    } else if (W->ai_k == 1) {                      // the steps from the focus
        done = spread_go(AI_SPREAD);
        if (done) W->ai_k = 2;
    } else if (W->ai_k == 2) {                      // the tiles to act from and to fall back to
        ai_setup2(a, st.acted);
        W->ai_k = 3; W->ai_a = 0; W->ai_self = W->ai_retreat;
        if (W->ai_nab) ai_ability(a, W->ai_list[0], W->ai_self);
    } else {                                        // each usable ability's best candidate
        while (W->ai_a < W->ai_nab) {
            if (w <= 0) return 0;
            done = ai_ability_go(a, &w);
            if (!done) return 0;
            if (++W->ai_a < W->ai_nab) { ai_ability(a, W->ai_list[W->ai_a], W->ai_self); w -= 5; }
        }
        if (W->ai_n || !W->ai_self || !W->ai_nab) { ai_choose(a); return 1; }
        W->ai_self = 0; W->ai_a = 0;                // retreating: on itself first, then on any
        ai_ability(a, W->ai_list[0], 0);            // unit
    }
    return 0;
}

#if MOD_CORE                            // tests' hooks (no modules)
void ai_plan(u8 a, u8 acted)
{
    u8 act = st.act, ac = st.acted;
    st.act = a; st.acted = acted; W->ai_k = 0;
    while (!ai_think()) ;
    st.act = act; st.acted = ac;
}

const AiAct *ai_ranked(u8 *n) { *n = W->ai_n; return W->ai_rank; }

u8 ai_fallback(void) { return W->ai_c0; }
#endif

// FFT's facing at Wait: towards the nearest enemy that acts before its next turn (else the
// nearest enemy) by quadrant, unless the tile that way is off the map, blocked or 3 higher; else
// the next such enemy; at last its own facing, or the last open side
u8 ai_facing(u8 a)
{
    static const s8 fx[4] = { 0, -1, 0, 1 }, fz[4] = { -1, 0, 1, 0 };   // FFT's S W N E
    static const u8 ours[4] = { 3, 1, 2, 0 };                          // FFT's facing <-> ours
    const Unit *u = &st.unit[a];
    u8 ok[4], d, last = 0, h = tile_h(&map_tiles[u->z * MAP_W + u->x]), t, any = 0;
    u16 skip = 0;
    for (d = 0; d < 4; d++) {
        const Tile *n = tile_at(u->x + fx[d], u->z + fz[d]);
        ok[d] = n && n->walk && (s16)tile_h(n) - h < 3;
        if (ok[d]) last = d;
    }
    if (!(ok[0] | ok[1] | ok[2] | ok[3])) ok[0] = ok[1] = ok[2] = ok[3] = 1;
    for (;;) {
        s16 dx, dz, p, q;
        t = ai_nearest(a, NT_IMMINENT, skip);
        if (t == a) { t = ai_nearest(a, NT_ANY_ENEMY, 0); any = 1; }
        if (t == a) return u->face;
        dx = (s16)st.unit[t].x - u->x; dz = (s16)st.unit[t].z - u->z;
        p = dz - dx; q = dz + dx;
        d = p <= 0 && q < 0 ? 0 : p > 0 && q <= 0 ? 1 : p >= 0 && q > 0 ? 2 : 3;
        if (ok[d]) return ours[d];
        if (!any) { skip |= (u16)(1 << t); continue; }
        return ok[ours[u->face]] ? u->face : ours[last];
    }
}
#endif

#if !MOD_CORE && MOD_PART(MOD_AI)
u16 mod_entry(u8 op, u8 a) { return op ? ai_facing(a) : ai_think(); }
#endif
#if MOD_CORE
u8 ai_frozen(void) { return W->frz; }
#ifdef FFT_MODULES
// The modules' bank (tools/module.py): "FMB1", a u16 offset per module, each "FMD1", its entry,
// size and context offsets (u16), its image. One module at a time in the zone, copied when
// another is called; the zone is in the program (module.h)
#ifdef FFT_OPENING                      // the largest image of the bank (tools/module.py)
#define MOD_ZONE 7748
#else
#define MOD_ZONE 7604
#endif
static u16 mod_zone[MOD_ZONE / 2];
static const u8 *mod_bank;
static u8 mod_id;                       // the module in the zone + 1 (0: none)
long __attribute__((__stkparm__)) __divsi3(long a, long b);
long __attribute__((__stkparm__)) __mulsi3(long a, long b);
static void draw_unit_covered(s16 sx, s16 sy, u8 rot, u8 i, const Cover *c);
static void cover(u16 *cm, s16 fx, s16 fy, u8 depth);
#ifdef FFT_ACTOR_BANK
static const u8 *actor_frame(u16 logical, u8 slot);
#endif
static void ensure_scene(u8 rot);
static void copy_view(void);
static Feet unit_feet(u8 rot, u8 i);
static u8 unit_direction(u8 rot, u8 i);
#ifdef FFT_OPENING
static const u8 *opening_story, *pgfx;
static void opening_battle(u8 direct);
static void opening_after(void);
#endif
static void ui_text(s16 x, s16 y, const char *s, u8 color, s16 limit);
static u8 put_s(char *s, u8 n, const char *t);
static u8 put_n(char *s, u8 n, u16 v);
static void text_into(u16 *l, u16 *d, s16 x, s16 y, const char *s, u8 color);
static void portrait(u8 gfx, s16 x, s16 y);
static void draw_hand(s16 x, s16 y);
static void copy_box(const u16 *l, const u16 *d, u8 x, u8 y, u8 words, u8 rows);
#ifndef FFT_OPENING
static u8 status_ability(void);
static u8 status_ability_count(void);
static void gauge(s16 x, s16 y, u16 v, u16 max, u8 color, u8 width);
#endif

static u16 mod_call(u8 id, u8 op, u8 a)
{
    const u8 *m = mod_bank + RD16(mod_bank, 2 + id);
    if (mod_id != id + 1) {
        ModCtx *c = (ModCtx *)((u8 *)mod_zone + RD16(m, 4));
        memcpy(mod_zone, m + 10, RD16(m, 3));
        c->st = &st; c->w = W; c->abilities = abilities; c->map_tiles = map_tiles;
        c->contour_gfx = contour_gfx; c->cdesc = cdesc;
        c->rand15 = rand15; c->known = known; c->spread_start = spread_start; c->spread_go = spread_go;
        c->bfs = bfs; c->hit_chance = hit_chance; c->attack_damage = attack_damage;
        c->skill_hit = skill_hit; c->skill_amount = skill_amount; c->tile_at = tile_at;
        c->D = &D; c->rt_light = &rt_light; c->rt_dark = &rt_dark; c->view_of = view_of;
        c->scene_x = scene_x; c->scene_y = scene_y; c->draw_unit_covered = draw_unit_covered;
        c->scene_l = &scene_l; c->items = items; c->ui_text = ui_text; c->put_s = put_s; c->put_n = put_n;
        c->unit_name = unit_name; c->text_into = text_into; c->portrait = portrait;
        c->draw_hand = draw_hand; c->copy_box = copy_box; c->draw_rect = draw_rect;
        c->draw_text = draw_text; c->job_level = job_level;
#ifndef FFT_OPENING
        c->status_ability = status_ability; c->status_ability_count = status_ability_count;
#endif
#ifdef FFT_OPENING
        c->opening_story = &opening_story; c->pgfx = &pgfx; c->opening_battle = opening_battle;
        c->opening_after = opening_after;
#endif
#ifdef FFT_ACTOR_BANK
        c->actor_frame = actor_frame;
#endif
        c->cover = cover; c->ensure_scene = ensure_scene; c->copy_view = copy_view;
        c->unit_feet = unit_feet; c->unit_direction = unit_direction; c->draw_sprite = draw_sprite;
        c->rt_keys = &rt_keys; c->rt_prev = &rt_prev; c->scene_rot = &scene_rot; c->vbuf_rot = &vbuf_rot;
        c->cgfx = cgfx;
#ifndef FFT_OPENING
        c->gauge = gauge;
#endif
#ifdef FFT_OPENING
        c->opening_attack_range = opening_attack_range;
#endif
        c->divsi3 = __divsi3; c->mulsi3 = __mulsi3;
        mod_id = id + 1;
    }
    return ((u16 (*)(u8, u8))((u8 *)mod_zone + RD16(m, 2)))(op, a);
}

// the bank found, each module of this build and within the zone
static u8 mod_ready(void)
{
    u8 id;
    mod_id = 0;
    if (!mod_bank || RD16(mod_bank, 0) != 0x464D || RD16(mod_bank, 1) != 0x4231) return 0;
    for (id = 0; id < MOD_COUNT; id++) {
        const u8 *m = mod_bank + RD16(mod_bank, 2 + id);
        if (RD16(m, 3) > MOD_ZONE || RD16(m + 10 + RD16(m, 4), 0) != MOD_SIG) return 0;
    }
    return 1;
}

static u8 ai_think(void) { return (u8)mod_call(MOD_AI, 0, 0); }
static void draw_turn(void) { mod_call(MOD_TURN, 0, 0); }
static void draw_end(void) { mod_call(MOD_SCREENS, 0, 0); }
static void draw_status(void) { mod_call(MOD_SCREENS, 1, 0); }
#ifdef FFT_OPENING
static u8 map_update(void) { return (u8)mod_call(MOD_BEATS, 0, 0); }
static void map_render(void) { mod_call(MOD_BEATS, 1, 0); }
static void map_start(u8 after) { mod_call(MOD_BEATS, 2, after); }
#endif
u8 ai_facing(u8 a) { return (u8)mod_call(MOD_AI, 1, a); }
#endif

// ---------------------------------------------------------------- units on screen
// frames of walking step i (to path[i + 1]): FFT's climbs, drops and jumps over a tile are slower
static u8 step_frames(u8 i)
{
    const u8 *a = st.path[i], *b = st.path[i + 1];
    return map_tiles[a[1] * MAP_W + a[0]].stand != map_tiles[b[1] * MAP_W + b[0]].stand
        || (u8)(a[0] - b[0] + 1) > 2 || (u8)(a[1] - b[1] + 1) > 2 ? CLIMB_FRAMES : WALK_FRAMES;
}

// a step's frame t: how far along (64ths) and the hop (pixels, up to 8 at mid-step) of a climb,
// the first WALK_FRAMES for a walk, the next CLIMB_FRAMES for a climb (no divide per frame)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define walk_q CONTOUR_REF_walk_q
#else
static const u8 walk_q[WALK_FRAMES + CLIMB_FRAMES] = { 0, 9, 18, 27, 37, 46, 55,
    0, 6, 13, 19, 26, 32, 38, 45, 51, 58 };
#endif
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define walk_h CONTOUR_REF_walk_h
#else
static const u8 walk_h[CLIMB_FRAMES] = { 0, 3, 5, 7, 8, 8, 8, 7, 5, 3 };
#endif

// FFT keeps a grounded human at the canal floor and selects the submerged SHP.
// At depth 1 a hop above that floor uses the ordinary pose. Both endpoints of
// a flat canal step are wet; a drop into water becomes submerged on landing.
#ifdef FFT_EXPLORATION
__attribute__((noinline))                       // GCC4TI register allocation workaround
#endif
u8 unit_in_water(u8 i)
{
#ifdef FFT_OPENING
    (void)i;return 0; // Source water is depth four and blocked for the human party.
#else
    const Unit *u = &st.unit[i];
    if (DC_OBJ(u->dc) || u->x >= MAP_W || u->z >= MAP_H) return 0;
    if (st.mode == M_WALK && st.sel == i && st.path_i + 1 < st.path_n) {
        const u8 *a = st.path[st.path_i], *b = st.path[st.path_i + 1];
        if (step_frames(st.path_i) == CLIMB_FRAMES && st.walk_t) return 0;
        // On a level step, change terrain at the tile edge.
        if (st.walk_t >= (WALK_FRAMES + 1) / 2 && step_frames(st.path_i) == WALK_FRAMES) a = b;
        return map_tiles[a[1] * MAP_W + a[0]].water;
    }
    return map_tiles[u->z * MAP_W + u->x].water;
#endif
}

// a unit's feet: scene position of its tile centre at its standing height, interpolated
// while walking (with a hop on a climb, a drop or a jump); depth: the diagonal it is drawn
// after (both tiles' while walking)
static Feet unit_feet(u8 rot, u8 i)
{
    Feet f;
    s16 u, v;
    const Unit *un = &st.unit[i];
    view_of(rot, un->x, un->z, &u, &v);
    f.x = scene_x(rot, u, v);
    f.y = scene_y(u, v) + HH - map_tiles[un->z * MAP_W + un->x].stand * (HU / 2);
    f.depth = (u8)(u + v);
    if (st.mode == M_WALK && st.sel == i && st.path_i + 1 < st.path_n) {
        const u8 *a = st.path[st.path_i], *b = st.path[st.path_i + 1];
        s16 u2, v2, x2, y2, t = st.walk_t, hop = 0, q;
        s16 sa = map_tiles[a[1] * MAP_W + a[0]].stand, sb = map_tiles[b[1] * MAP_W + b[0]].stand;
        view_of(rot, a[0], a[1], &u, &v);
        view_of(rot, b[0], b[1], &u2, &v2);
        f.x = scene_x(rot, u, v); f.y = scene_y(u, v) + HH - sa * (HU / 2);
        x2 = scene_x(rot, u2, v2); y2 = scene_y(u2, v2) + HH - sb * (HU / 2);
        if (step_frames(st.path_i) == CLIMB_FRAMES) { hop = walk_h[t]; t += WALK_FRAMES; }
        q = walk_q[t];
        f.x += ((x2 - f.x) * q) >> 6;
        f.y += (((y2 - f.y) * q) >> 6) - hop;
        f.depth = (u8)(u + v > u2 + v2 ? u + v : u2 + v2);
    }
    return f;
}

// Cover mask of a unit: the pixels in front of it (the view's depth: diagonals greater than
// its own) in its sprite's 16 x COVER_H box, one bit per pixel. Kept per unit (W->cover) and
// made again only when its feet, its depth or the view change: a unit standing still costs nothing.
static void cover(u16 *cm, s16 fx, s16 fy, u8 depth)
{
    const u8 *t1 = VDEPTH, *m1 = t1 + SC_PLANE, *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
    s16 bx = fx - 8, by = fy - UNIT_FOOT, b0 = bx >> 3, sh = 8 - (bx & 7), r, k;
    for (r = 0; r < COVER_H; r++) {
        s16 y = by + r;
        u32 m = 0;
        if ((u16)y < SC_H) {
            u16 o = (u16)(y * SC_BYTES + b0);
            for (k = 0; k < 3; k++, o++) {
                m <<= 8;
                if ((u16)(b0 + k) >= SC_BYTES) continue;
                if (t1[o] > depth) m |= m1[o];
                if (t2[o] > depth) m |= m2[o];
            }
        }
        cm[r] = (u16)(m >> sh);
    }
}

// FFT's battle idle (TYPE1.SEQ 6 / 7: frames 11 10 9 10 11 12 13 12 for 6 8 10 8 ticks at
// 60 Hz) and walk (8 / 9: the same frames for 2 4 6 4 ticks), in our frames (~30 per second)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define idle_seq CONTOUR_REF_idle_seq
#else
static const u8 idle_seq[32] = { 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3 };
#endif
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define walk_seq CONTOUR_REF_walk_seq
#else
static const u8 walk_seq[16] = { 2, 1, 1, 0, 0, 0, 1, 1, 2, 3, 3, 4, 4, 4, 3, 3 };
#endif

// Armed body and blade phase, 0 outside a weapon Attack. Skills and bare hands use fftu.
static u8 armed_pose(u8 i)
{
    if (st.mode != M_ACT || st.abil != NO_ABIL || i != st.atk || st.unit[i].eq[EQ_WEAPON] == 0xFF
        || st.act_t < ACT_WIND || st.act_t >= BLADE_END) return 0;
    return st.act_t < ACT_BLOW ? 1 : st.act_t == ACT_BLOW ? 2 : st.act_t == ACT_BLOW + 1 ? 3 : 4;
}

static u8 hit_flash(u8 i)
{
    u8 t = (u8)(st.act_t - ACT_HIT);
    if (st.mode != M_ACT || i != st.tgt || !st.hit || st.abil == ABIL_DEFEND || act_kind() != AB_DAMAGE
        || t >= FLASH_FRAMES) return 0;
    return t < 2 || t >= 10 ? 1 : 2;
}

// a unit's flash: a blow's, or the enemies pulsing as a hit while the objective shows (FFT
// lights up the units it names)
static u8 unit_flash(u8 i)
{
    u8 t = (u8)(st.banner_t & 15);
    if (st.banner == BN_START && st.unit[i].team == TEAM_ENEMY && st.unit[i].hp)
        return t < 2 || (t >= 10 && t < 12) ? 1 : t < 10 ? 2 : 0;
    return st.mode == M_ACT && i == st.tgt ? hit_flash(i) : 0;
}

// View facing: 0 front, 1 back, 2 mirrored front, 3 mirrored back.
static u8 unit_direction(u8 rot, u8 i)
{
    #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define facing_sprite CONTOUR_REF_facing_sprite
#else
static const u8 facing_sprite[4][4] = { { 2, 1, 0, 3 }, { 0, 3, 1, 2 }, { 1, 2, 3, 0 }, { 3, 0, 2, 1 } };
#endif
    return facing_sprite[rot][st.unit[i].face]
#ifdef FFT_SOURCE_CAMERA
        ^2
#endif
        ;
}

// a unit's pose (units.h: P_*), 0 for the idle or the walk: KO lying, the attacker's swing,
// the target hit, kneeling with little HP left (FFT's Critical: HP <= max / 5)
static u8 unit_pose(u8 i)
{
    const Unit *un = &st.unit[i];
    if (st.mode == M_ACT && st.abil != ABIL_DEFEND) {
        if (i == st.atk && st.act_t >= ACT_WIND && st.act_t < ACT_HIT)   // Throw Stone (the evaded
                                                                         // skill) and items:
            return st.act_t < ACT_BLOW ? P_WIND    // FFT's throw after the same wind-up
                : st.abil != NO_ABIL && abilities[st.abil].flags & (AF_EVADE | AF_ITEM) ? P_THROW : P_BLOW;
        if (i == st.tgt && st.act_t >= ACT_HIT && st.act_t < ACT_HIT + HIT_FRAMES && st.hit
            && act_kind() == AB_DAMAGE) return P_HIT;
    }
    if (!un->hp) return P_KO;
#ifdef FFT_OPENING
    if (st.pending[i].state == 1) return P_WIND;
#endif
    return (u16)(un->hp * 5) <= un->max_hp ? P_KNEEL : 0;
}

// The clock changes loot logically at once. Present pending events in unit order;
// the dissolve is purely visual, with the same sprite cover and no random draws.
static u8 morph_first(void)
{
    u8 i = 0;
    u16 m = st.morph_mask;
    while (m && !(m & 1)) { m >>= 1; i++; }
    return i;
}

#ifdef FFT_ACTOR_BANK
static const u8 *unit_frame(u8 rot, u8 i);
#include "actors.inc"

// Walking/action poses have priority; rotate the remaining decode opportunities
// among visible actors so the frontmost unit cannot starve the other animations.
static void actor_animate(const u8 *visible)
{
    u8 i,k;
    if(st.mode==M_WALK && visible[st.sel])unit_frame(st.rot,st.sel);
    if(st.mode==M_ACT) {
        if(visible[st.atk])unit_frame(st.rot,st.atk);
        if(visible[st.tgt])unit_frame(st.rot,st.tgt);
    }
    for(i=0;i<NUNIT && W->actor_budget;i++) {
        k=W->actor_next;
        if(++W->actor_next==NUNIT)W->actor_next=0;
        if(visible[k])unit_frame(st.rot,k);
    }
    W->actor_budget=0;
}
#endif

static const u8 *object_frame(u8 i)
{
    static const u8 chest_bob[8] = {
        OBJ_CHEST, OBJ_CHEST, OBJ_CHEST_UP1, OBJ_CHEST_UP1,
        OBJ_CHEST_UP2, OBJ_CHEST_UP2, OBJ_CHEST_UP1, OBJ_CHEST_UP1
    };
    u8 phase = (u8)(((st.tick >> 2) + i) & (CRYSTAL_FRAMES - 1));
    // Keep the conversion's target steady; then float at most two sprite pixels.
    u8 obj = st.unit[i].dc == DC_CHEST
        ? (st.morph_mask & (u16)(1U << i) ? OBJ_CHEST : chest_bob[(st.tick >> 3) & 7])
        : phase ? (u8)(phase + 1) : OBJ_CRYSTAL;
#ifdef FFT_ACTOR_BANK
    return actor_frame((u16)(ACTOR_OBJECTS + obj), NUNIT);
#else
    return ugfx + (u32)UNIT_GFX_N * 2 * UNIT_FRAMES * UNIT_FB + (u16)obj * UNIT_FB;
#endif
}

static const u8 *morph_frame(const u8 *old, const u8 *obj)
{
    #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define dissolve CONTOUR_REF_dissolve
#else
static const u16 dissolve[5][4] = {
        { 0, 0, 0, 0 }, { 0xAAAA, 0, 0xAAAA, 0 },
        { 0xAAAA, 0x5555, 0xAAAA, 0x5555 },
        { 0xFFFF, 0x5555, 0xFFFF, 0x5555 }, { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }
    };
#endif
    u8 r, plane, t = st.morph_t, phase = t < 8 ? 0 : t < 24 ? (u8)((t - 8) >> 2) : 4;
    u8 *out = (u8 *)W->morph_g;
    for (plane = 0; plane < 4; plane++, old += 2 * UNIT_SH, obj += 2 * UNIT_SH, out += 2 * UNIT_SH)
        for (r = 0; r < UNIT_SH; r++) {
            u16 a = RD16(old, r), b = RD16(obj, r);
            WR16(out, r, a ^ ((a ^ b) & dissolve[phase][r & 3]));
        }
    out = (u8 *)W->morph_g;
    if (t >= 4 && t < 24)
        for (r = 0; r < UNIT_FOOT - 1; r++) {
            u16 in = RD16(out, 3 * UNIT_SH + r);
            WR16(out, r, RD16(out, r) & ~in);
            WR16(out, UNIT_SH + r, RD16(out, UNIT_SH + r) & ~in);
        }
    return out;
}

// a unit's frame (units.h) seen from orientation rot: its facing turned into the view, front
// frames looking down-left (+v), back ones up-left (-u), mirrored for +u and -v as in FFT (the
// mirrored frames are stored after the others)
static const u8 *unit_frame(u8 rot, u8 i)
{
    const Unit *un = &st.unit[i];
    u8 t = (u8)(st.tick + 11 * i), k, dir = unit_direction(rot, i), armed;
    if (DC_OBJ(un->dc)) {
        if (st.morph_mask & (u16)(1U << i)) {
#ifdef FFT_ACTOR_BANK
            const u8 *ko = actor_frame((u16)(un->gfx * 44 + P_KO + (dir & 1) + (dir & 2 ? 22 : 0)), i);
#else
            const u8 *ko = ugfx + (u32)(u16)(un->gfx * (2 * UNIT_FRAMES)
                + P_KO + (dir & 1) + (dir & 2 ? UNIT_FRAMES : 0)) * UNIT_FB;
#endif
            return morph_first() == i ? morph_frame(ko, object_frame(i)) : ko;
        }
        return object_frame(i);
    }
    armed = st.mode == M_ACT && i == st.atk ? armed_pose(i) : 0;
#ifdef FFT_ACTOR_BANK
    if (armed) return actor_frame((u16)(ACTOR_ARMED + un->gfx * 16 + (dir >> 1) * 8 + (dir & 1) * 4 + armed - 1), i);
#else
    if (armed) return wgfx + (u32)(u16)(un->gfx * 16 + (dir >> 1) * 8 + (dir & 1) * 4 + armed - 1) * UNIT_FB;
#endif
    k = unit_pose(i);
    if (k) k += dir & 1;                              // a pose: front, back
    else {
        k = st.mode == M_WALK && i == st.sel ? walk_seq[t & 15] : idle_seq[t & 31];
        if (dir & 1) k += 5;
    }
    if (dir & 2) k += UNIT_FRAMES;
#ifdef FFT_ACTOR_BANK
    return actor_frame((u16)(un->gfx * (2 * UNIT_FRAMES) + k), i);
#else
    return ugfx + (u32)(u16)(un->gfx * (2 * UNIT_FRAMES) + k) * UNIT_FB;
#endif
}

// 3 x 5 digits and '/' for the panels' numbers, turned a quarter: per glyph column (left to
// right), its 5 pixels (bit 4: the top row)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define digits CONTOUR_REF_digits
#else
static const u8 digits[11][3] = { { 0x1F, 0x11, 0x1F }, { 0x09, 0x1F, 0x01 }, { 0x17, 0x15, 0x1D },
    { 0x15, 0x15, 0x1F }, { 0x1C, 0x04, 0x1F }, { 0x1D, 0x15, 0x17 }, { 0x1F, 0x15, 0x17 },
    { 0x10, 0x10, 0x1F }, { 0x1F, 0x15, 0x1F }, { 0x1D, 0x15, 0x1F }, { 0x03, 0x04, 0x18 } };
#endif

#define DC_Y 18                         // a death counter's top above the feet

// A covered body's interior is white, preserving its white/black contour: letting a busy
// roof texture show through made the hollow silhouette look like ordinary sprite detail.
// Covered shadows stay transparent. c RT_NULL draws the ordinary unoccluded sprite.
// On the TI the rows it leaves alone are drawn straight from the data file,
// the others copied first.
// Depth-1 shader: a short, constant-height pass. The ordinary dry shader
// stays unchanged; standing units reuse their cached terrain classification.
#ifndef FFT_OPENING
static void draw_unit_water(s16 sx, s16 sy, u8 rot, u8 i, const Cover *c)
{
    RtSprite s;
    u16 mk[WATER_ROWS], ml[WATER_ROWS], md[WATER_ROWS], l, d = 0, m;
    const u8 *g = unit_frame(rot, i);
    u8 r, flash = unit_flash(i);
    for (r = 0; r < WATER_ROWS; r++) {
        u16 in = RD16(g, 3 * UNIT_SH + r), k = c ? c->cm[r] & in : 0;
        mk[r] = RD16(g, 2 * UNIT_SH + r); // covered body pixels are opaque white, not terrain
        ml[r] = (flash ? (RD16(g, r) & ~in) | (flash == 1 ? in : 0) : RD16(g, r)) & ~k;
        md[r] = RD16(g, UNIT_SH + r) & ~(k | (flash ? in : 0));
    }
    sy -= UNIT_FOOT;
    s.w = 16; s.h = WATER_ROWS; s.light = ml; s.dark = md; s.mask = mk;
    draw_sprite(sx - 8, sy, &s);
    // The light edge follows the same contour cover rule as the body.
    l = (u16)~RD16(g, 2 * UNIT_SH + WATER_ROWS - 1);
    if (c) l &= (u16)~(c->cm[WATER_ROWS - 1] & RD16(g, 3 * UNIT_SH + WATER_ROWS - 1));
    m = (u16)~l;
    s.h = 1; s.light = &l; s.dark = &d; s.mask = &m;
    draw_sprite(sx - 8, sy + WATER_ROWS - 1, &s);
    sy += UNIT_FOOT;
    if (DC_OBJ(st.unit[i].dc)) return;
    if (st.unit[i].team == TEAM_ENEMY) {        // the team at a glance: a diamond above enemies,
        if (st.unit[i].hp) {                    // never covered (it shows a hidden one too); a
            s.h = FOE_H;                        // small one above a KO enemy
            s.light = foe_gfx[0]; s.dark = foe_gfx[1]; s.mask = foe_gfx[2];
        } else { s.h = FOE_KO_H; s.light = foe_ko_gfx[0]; s.dark = foe_ko_gfx[1]; s.mask = foe_ko_gfx[2]; }
        draw_sprite(sx - 8, sy - UNIT_FOOT - s.h + 1, &s);
    }
    if (!st.unit[i].hp && st.unit[i].team != TEAM_GUEST) {   // a KO unit's death counter above it
        s.w = 8; s.h = 7; s.light = s.dark = W->dcg[st.unit[i].dc][0]; s.mask = W->dcg[st.unit[i].dc][1];
        draw_sprite(sx - 3, sy - DC_Y, &s);
    }
}
#endif

static void draw_unit_covered(s16 sx, s16 sy, u8 rot, u8 i, const Cover *c)
{
#ifndef FFT_OPENING
    if ((c ? c->wet : unit_in_water(i)) && !DC_OBJ(st.unit[i].dc)) {
        draw_unit_water(sx, sy, rot, i, c); return;
    }
#endif
    RtSprite s;
    u16 mk[COVER_H], ml[COVER_H], md[COVER_H], h[COVER_H];
    u8 r, h0 = UNIT_SH, h1 = 0, flash = unit_flash(i);
    const u8 *g = unit_frame(rot, i);
    s.w = 16;
    if (c) {
        const u16 *cm = c->cm;
        for (r = c->r0; r <= c->r1; r++)
            if ((h[r] = cm[r] & RD16(g, 3 * UNIT_SH + r)) != 0) { if (r < h0) h0 = r; h1 = r; }
    }
    sy -= UNIT_FOOT;
    if (flash) {
        for (r = 0; r < UNIT_SH; r++) {
            u16 k = c ? c->cm[r] & RD16(g, 3 * UNIT_SH + r) : 0;
            u16 in = r < UNIT_FOOT - 1 ? RD16(g, 3 * UNIT_SH + r) : 0;   // leave the shadow dark
            mk[r] = RD16(g, 2 * UNIT_SH + r) | (r >= UNIT_FOOT ? k : 0);
            ml[r] = ((RD16(g, r) & ~in) | (flash == 1 ? in : 0)) & ~k;
            md[r] = RD16(g, UNIT_SH + r) & ~(in | k);
        }
        s.h = UNIT_SH; s.light = ml; s.dark = md; s.mask = mk;
        draw_sprite(sx - 8, sy, &s);
        goto markers;
    }
#ifdef __m68k__                                // native big-endian rows: drawn in place
    if (h0) {
        s.h = h0;
        s.light = g; s.dark = g + 2 * UNIT_SH; s.mask = g + 4 * UNIT_SH;
        draw_sprite(sx - 8, sy, &s);
    }
    if (h0 < UNIT_SH) {
        u8 e = h1 + 1;
        for (r = h0; r < e; r++) {
            u16 k = h[r];
            mk[r] = RD16(g, 2 * UNIT_SH + r) | (r >= UNIT_FOOT ? k : 0);
            ml[r] = RD16(g, r) & ~k; md[r] = RD16(g, UNIT_SH + r) & ~k;
        }
        s.h = e - h0;
        s.light = ml + h0; s.dark = md + h0; s.mask = mk + h0;
        draw_sprite(sx - 8, sy + h0, &s);
        if (e < UNIT_SH) {
            s.h = UNIT_SH - e;
            s.light = g + 2 * e; s.dark = g + 2 * (UNIT_SH + e); s.mask = g + 2 * (2 * UNIT_SH + e);
            draw_sprite(sx - 8, sy + e, &s);
        }
    }
#else                                          // the PC copies every row (byte order)
    for (r = 0; r < UNIT_SH; r++) {
        u16 k = r >= h0 && r <= h1 ? h[r] : 0;
        mk[r] = RD16(g, 2 * UNIT_SH + r) | (r >= UNIT_FOOT ? k : 0);
        ml[r] = RD16(g, r) & ~k; md[r] = RD16(g, UNIT_SH + r) & ~k;
    }
    s.h = UNIT_SH;
    s.light = ml; s.dark = md; s.mask = mk;
    draw_sprite(sx - 8, sy, &s);
#endif
markers:
    sy += UNIT_FOOT;
    if (DC_OBJ(st.unit[i].dc)) return;
    if (st.unit[i].team == TEAM_ENEMY) {        // the team at a glance: a diamond above enemies,
        if (st.unit[i].hp) {                    // never covered (it shows a hidden one too); a
            s.h = FOE_H;                        // small one above a KO enemy
            s.light = foe_gfx[0]; s.dark = foe_gfx[1]; s.mask = foe_gfx[2];
        } else { s.h = FOE_KO_H; s.light = foe_ko_gfx[0]; s.dark = foe_ko_gfx[1]; s.mask = foe_ko_gfx[2]; }
        draw_sprite(sx - 8, sy - UNIT_FOOT - s.h + 1, &s);
    }
    if (!st.unit[i].hp && st.unit[i].team != TEAM_GUEST) {   // a KO unit's death counter above it
        s.w = 8; s.h = 7; s.light = s.dark = W->dcg[st.unit[i].dc][0]; s.mask = W->dcg[st.unit[i].dc][1];
        draw_sprite(sx - 3, sy - DC_Y, &s);
    }
}

#ifndef FFT_COMPACT_UI
static u8 weapon_pose(u8 i, u8 phase)
{
    u8 dir = unit_direction(st.rot, i);
    return (u8)((
#ifdef FFT_OPENING
        st.unit[i].gfx == UG_CHEMIST_M
#else
        st.unit[i].gfx >= UG_CHEMIST_M
#endif
        ? 24 : 0)
        + (items[st.unit[i].eq[EQ_WEAPON]].knife ? 0 : 12)
        + (dir >> 1) * 6 + (dir & 1) * 3 + phase - 2);
}

// Three fixed swing-phase slots; duplicate logical poses in a slot share the same key.
static const u8 *weapon_frame(u8 pose, u8 slot)
{
    u8 id = wgfx[WEAPON_INDEX + pose], cache_key = id + 1;
    if (W->blade_k[slot] != cache_key) {
        ZB(19);
        zx0_unpack(wgfx + WEAPON_OFF + RD16(wgfx + WEAPON_ZOFF, id), (u8 *)W->blade[slot]);
        ZE(19);
        W->blade_k[slot] = cache_key;
    }
    return (const u8 *)W->blade[slot];
}

// The original blade behind the armed body, at the same sorted depth, through scenery.
// Its 32-column cover is cached independently of the body's 16-column cover.
static void draw_weapon(s16 sx, s16 sy, u8 i, u8 depth)
{
    u8 phase = armed_pose(i), k, r, rows;
    const u8 *g;
    u32 l[WEAPON_SH], d[WEAPON_SH], m[WEAPON_SH];
    RtSprite sp;
    s16 bx, by, b0, sh;
    if (phase < 2) return;
#ifdef FFT_OPENING
    if (st.unit[i].eq[EQ_WEAPON] != 255 && opening_range[st.unit[i].eq[EQ_WEAPON]] > 1) return;
#endif
    rows = unit_in_water(i) ? WEAPON_FOOT - HU : WEAPON_SH;
    k = weapon_pose(i, phase);
    g = weapon_frame(k, phase - 2);
    bx = sx + weapon_x[k]; by = sy - WEAPON_FOOT;
    b0 = bx >> 3; sh = bx & 7;
    if (W->wx != bx || W->wy != by || W->wrot != st.rot || W->wdepth != depth) {
        const u8 *t1 = VDEPTH, *m1 = t1 + SC_PLANE, *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
        for (r = 0; r < WEAPON_SH; r++) {
            s16 y = by + r;
            u32 cm = 0;
            u8 tail = 0, j;
            if ((u16)y < SC_H) {
                u16 o = (u16)(y * SC_BYTES + b0);
                for (j = 0; j < 5; j++, o++) {
                    u8 b = 0;
                    if ((u16)(b0 + j) < SC_BYTES) {
                        if (t1[o] > depth) b |= m1[o];
                        if (t2[o] > depth) b |= m2[o];
                    }
                    if (j < 4) cm = cm << 8 | b; else tail = b;
                }
            }
            W->wcover[r] = (cm << sh) | ((u32)tail >> (8 - sh));
        }
        W->wx = bx; W->wy = by; W->wrot = st.rot; W->wdepth = depth;
    }
    for (r = 0; r < rows; r++) {
        u32 c = W->wcover[r];
        l[r] = RD32(g, 2 * r) & ~c; d[r] = RD32(g, 2 * (WEAPON_SH + r)) & ~c;
        m[r] = RD32(g, 2 * (2 * WEAPON_SH + r)) | c;
    }
    sp.w = 32; sp.h = rows; sp.light = l; sp.dark = d; sp.mask = m;
    draw_sprite(bx - st.camx, by - st.camy, &sp);
}
#endif

// the death counter's digits 0-3 as sprites (black, a white outline), made once
static void make_counters(void)
{
    u8 n, r, g;
    for (n = 0; n < 4; n++) {
        u8 b[7];
        for (r = 0; r < 7; r++)
            for (b[r] = 0, g = 0; g < 3; g++)
                if (r && r < 6 && digits[n][g] & (0x20 >> r)) b[r] |= (u8)(0x40 >> g);
        for (r = 0; r < 7; r++) {
            u8 m = (u8)(b[r] | (r ? b[r - 1] : 0) | (r < 6 ? b[r + 1] : 0));
            W->dcg[n][0][r] = b[r]; W->dcg[n][1][r] = (u8)~(m | m << 1 | m >> 1);
        }
    }
}

// Shared world and original sprite frames; no second tactical engine.
#ifdef FFT_EXPLORATION
#include "perspective.inc"
#endif

#endif
#if MOD_PART(MOD_TURN)
// ---------------------------------------------------------------- the rotation animation
#if !defined(FFT_EXPLORATION) || defined(FFT_FIXED)
// 32 angles per turn: S = round(6 sqrt2 sin a * 256), view r is at 45 + 90 r degrees (index
// 4 + 8 r), where the projection equals the cardinal one exactly (6 * 256)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define sin32 CONTOUR_REF_sin32
#else
static const s16 sin32[32] = { 0, 424, 831, 1207, 1536, 1806, 2007, 2130, 2172, 2130, 2007,
    1806, 1536, 1207, 831, 424, 0, -424, -831, -1207, -1536, -1806, -2007, -2130, -2172, -2130,
    -2007, -1806, -1536, -1207, -831, -424 };
#endif

#if !defined(FFT_TURN_LOWPOLY_NATIVE) || defined(FFT_TURN_TEXTURE)
static void __attribute__((noinline)) turn_expand(void)
{
    const u8 *hl=half[0],*hd=half[1]; u8 *dl=rt_light,*dd=rt_dark; s16 i,j;
    for(i=0;i<TH;i++,dl+=2*RT_PBYTES,dd+=2*RT_PBYTES)
        for(j=0;j<TW/8;j++) {
            u16 l=dbl[*hl++],d=dbl[*hd++];
            WR16(dl,j,l); WR16(dl+RT_PBYTES,j,l);
            WR16(dd,j,d); WR16(dd+RT_PBYTES,j,d);
        }
}
#endif

// Drawn at half resolution (80 x 50, every size halved, a level = 3 px) then doubled onto the
// screen: the rows are what costs (ti-cycles: ~9k per polygon at full size)
#ifdef FFT_TURN_LOWPOLY_NATIVE
#define TURN_W RT_W
#define TURN_H RT_H
#define TURN_PIVOT_SHIFT 0
#define TURN_X_SHIFT 8
#define TURN_Y_SHIFT 9
#define TURN_HEIGHT HU
#define TURN_SORT_BIAS 1024
#define TURN_SORT_SHIFT 3
#else
#define TURN_W TW
#define TURN_H TH
#define TURN_PIVOT_SHIFT 1
#define TURN_X_SHIFT 9
#define TURN_Y_SHIFT 10
#define TURN_HEIGHT (HU / 2)
#define TURN_SORT_BIAS 512
#define TURN_SORT_SHIFT 2
#endif

#ifndef FFT_TURN_NO_EDGES
// Short tile-top segments. Step the byte address/mask; no per-pixel multiply/divide.
// Bounds checks also cover partially off-screen tiles without another clip buffer.
static void turn_edge(s16 x, s16 y, s16 xb, s16 yb, u8 white)
{
    s16 dx, dy, sy, err, off, rowstep;
    u8 mask;
    if (x > xb) { s16 t=x; x=xb; xb=t; t=y; y=yb; yb=t; }
    if (xb < 0 || x >= D.x1 || (y < 0 && yb < 0) || (y >= D.y1 && yb >= D.y1)) return;
    dx=xb-x; dy=yb-y; sy=dy<0 ? -1:1; if (dy<0) dy=-dy;
    err=dx-dy; rowstep=sy*D.stride; off=y*D.stride+(x>>3);
    mask=(u8)(0x80U>>(x&7));
    if (x>=0 && xb<D.x1 && y>=0 && y<D.y1 && yb>=0 && yb<D.y1) {
        // Most lips are wholly inside the LCD: keep plane pointers and skip pixel clipping.
        u8 *l=D.l+off, *d=D.d+off;
        s16 count=(dx>dy ? dx:dy)+1;
        for (;;) {
            s16 twice;
            if (white) { *l&=(u8)~mask; *d&=(u8)~mask; }
            else { *l|=mask; *d|=mask; }
            if (!--count) return;
            twice=err+err;
            if (twice > -dy) {
                err-=dy; mask>>=1; if (!mask) { mask=0x80; l++; d++; }
            }
            if (twice < dx) { err+=dx; l+=rowstep; d+=rowstep; }
        }
    }
    for (;;) {
        s16 twice;
        if ((u16)x < (u16)D.x1 && (u16)y < (u16)D.y1) {
            if (white) { D.l[off]&=(u8)~mask; D.d[off]&=(u8)~mask; }
            else { D.l[off]|=mask; D.d[off]|=mask; }
        }
        if (x==xb && y==yb) break;
        twice=err+err;
        if (twice > -dy) {
            err-=dy; x++; mask>>=1; if (!mask) { mask=0x80; off++; }
        }
        if (twice < dx) { err+=dx; y+=sy; off+=rowstep; }
    }
}
#endif

static void draw_turn(void)
{
    s16 a = (s16)(4 + 8 * st.rot + st.turn * (s16)(st.turn_t + 1) * (8 / (TURN_FRAMES + 1))) & 31;
    s16 S = sin32[a], C = sin32[(a + 8) & 31];
    s16 pvx, pvy, cu, cv, i, j, x, z;
    u8 n = 0;
    // the pivot (cursor tile centre) stays where it is on screen
    view_of(st.rot, st.cx, st.cz, &cu, &cv);
    pvx = scene_x(st.rot, cu, cv) - st.camx;
    pvy = scene_y(cu, cv) + HH - st.camy;
#ifdef FFT_TURN_LOWPOLY_NATIVE
    draw_clear(); D.l=rt_light; D.d=rt_dark; D.stride=RT_PBYTES;
#else
    memset(half, 0, sizeof half);
    D.l = half[0]; D.d = half[1]; D.stride = TW / 8;
#endif
    D.ox = D.oy = 0;
    D.x0 = 0; D.x1 = TURN_W; D.y0 = 0; D.y1 = TURN_H;
    for (z = 0; z <= MAP_H; z++)                                   // grid corners at height 0
        for (x = 0; x <= MAP_W; x++) {
            s16 x2 = 2 * (x - st.cx) - 1, z2 = 2 * (z - st.cz) - 1;
            gx[x][z] = (pvx >> TURN_PIVOT_SHIFT)
#ifdef FFT_SOURCE_CAMERA
                -
#else
                +
#endif
                (s16)((muls16(x2, C) - muls16(z2, S)) >> TURN_X_SHIFT);
            gy[x][z] = (pvy >> TURN_PIVOT_SHIFT) + (s16)((muls16(x2, S) + muls16(z2, C)) >> TURN_Y_SHIFT);
        }
    {                                           // counting sort by the centre's depth
        u8 *cnt = W->count;
        memset(cnt, 0, sizeof W->count);
        for (z = 0, i = 0; z < MAP_H; z++)
            for (x = 0; x < MAP_W; x++, i++) {
                s16 k = (gy[x][z] + gy[x + 1][z + 1] + TURN_SORT_BIAS) >> TURN_SORT_SHIFT;
                key[i] = k = k < 0 ? 0 : k > 255 ? 255 : k;
                cnt[k]++;
            }
        for (i = 0, j = 0; i < 256; i++) { u8 c = cnt[i]; cnt[i] = (u8)j; j += c; }
        for (i = 0; i < MAP_W * MAP_H; i++) order[cnt[key[i]]++] = (u8)i;
        n = MAP_W * MAP_H;
    }
    for (i = 0; i < n; i++) {
        u8 o = order[i], f;
#ifndef FFT_TURN_NO_EDGES
        u8 ledges=0;
#endif
        const Tile *t = &map_tiles[o];
        s16 tx = o % MAP_W, tz = o / MAP_W, cx[4], cy[4], px[4], py[4];
        cx[0] = gx[tx][tz]; cx[1] = gx[tx + 1][tz]; cx[2] = gx[tx + 1][tz + 1]; cx[3] = gx[tx][tz + 1];
        cy[0] = gy[tx][tz]; cy[1] = gy[tx + 1][tz]; cy[2] = gy[tx + 1][tz + 1]; cy[3] = gy[tx][tz + 1];
        {                                       // off screen: box of the column down to h 0
            s16 xa = cx[0], xb = cx[0], ya = cy[0], yb = cy[0];
            u8 c;
            for (c = 1; c < 4; c++) {
                if (cx[c] < xa) xa = cx[c];
                if (cx[c] > xb) xb = cx[c];
                if (cy[c] < ya) ya = cy[c];
                if (cy[c] > yb) yb = cy[c];
            }
            if (xb < 0 || xa >= TURN_W || yb < 0 || ya - MAXH * TURN_HEIGHT >= TURN_H) continue;
        }
        cy[0] -= t->c[0] * TURN_HEIGHT; cy[1] -= t->c[1] * TURN_HEIGHT;
        cy[2] -= t->c[2] * TURN_HEIGHT; cy[3] -= t->c[3] * TURN_HEIGHT;
        // faces towards the viewer: +x (corners 1, 2) if S > 0, -x (3, 0) if S < 0,
        // +z (2, 3) if C > 0, -z (0, 1) if C < 0; lit when facing left
        for (f = 0; f < 4; f++) {
            static const u8 fa[4] = { 1, 3, 2, 0 }, fb[4] = { 2, 0, 3, 1 };
            static const s8 fdx[4] = { 1, -1, 0, 0 }, fdz[4] = { 0, 0, 1, -1 };
            static const u8 mir[4][4] = { { 1, 0, 3, 2 }, { 1, 0, 3, 2 }, { 3, 2, 1, 0 }, { 3, 2, 1, 0 } };
            s16 facing = f == 0 ? S : f == 1 ? -S : f == 2 ? C : -C, lit;
            const Tile *nt;
            u8 ha, hb, na = 0, nb = 0, k;
#ifdef FFT_TURN_NO_EDGES
            if (facing <= 0) continue;
#endif
            nt = tile_at(tx + fdx[f], tz + fdz[f]);
            ha = t->c[fa[f]]; hb = t->c[fb[f]];
            if (nt) { na = nt->c[mir[f][fa[f]]]; nb = nt->c[mir[f][fb[f]]]; }
            if (na >= ha && nb >= hb) continue;
#ifndef FFT_TURN_NO_EDGES
            ledges|=(u8)(1<<f);
            if (facing <= 0) continue;
#endif
            px[0] = cx[fa[f]]; py[0] = cy[fa[f]];
            px[1] = cx[fb[f]]; py[1] = cy[fb[f]];
            px[2] = cx[fb[f]]; py[2] = cy[fb[f]] + (hb > nb ? (hb - nb) * TURN_HEIGHT : 0);
            px[3] = cx[fa[f]]; py[3] = cy[fa[f]] + (ha > na ? (ha - na) * TURN_HEIGHT : 0);
            lit = f == 0 ? C : f == 1 ? -C : f == 2 ? -S : S;      // normal's screen x < 0
            k = t->look[1 + f] + (lit >= 0);                       // the views' grey, one
            if (k >= SHADES) k = SHADES - 1;                       // darker facing right
            fill_poly(px, py, 4, turn_pat[k][0], turn_pat[k][1]);
        }
        fill_poly(cx, cy, 4, turn_pat[t->look[0]][0], turn_pat[t->look[0]][1]);
#ifndef FFT_TURN_NO_EDGES
        // Top lips only; painter order lets nearer terrain cover earlier strokes.
        if (ledges&1) turn_edge(cx[1],cy[1],cx[2],cy[2],t->look[0]>=2);
        if (ledges&2) turn_edge(cx[3],cy[3],cx[0],cy[0],t->look[0]>=2);
        if (ledges&4) turn_edge(cx[2],cy[2],cx[3],cy[3],t->look[0]>=2);
        if (ledges&8) turn_edge(cx[0],cy[0],cx[1],cy[1],t->look[0]>=2);
#endif
    }
#ifndef FFT_TURN_LOWPOLY_NATIVE
    turn_expand();
#endif

    // units on top, at their projected feet (no occlusion while turning)
    for (i = 0; i < NUNIT; i++) {
        const Unit *un = &st.unit[i];
        s16 x2 = 2 * (un->x - st.cx), z2 = 2 * (un->z - st.cz), sx, sy;
        if (un->x >= MAP_W) continue;
        sx = pvx + (s16)((muls16(x2, C) - muls16(z2, S)) >> 8);
        sy = pvy + (s16)((muls16(x2, S) + muls16(z2, C)) >> 9) - map_tiles[un->z * MAP_W + un->x].stand * (HU / 2);
        draw_unit_covered(sx, sy, st.rot, (u8)i, RT_NULL);
    }
}
#ifdef FFT_TURN_TEXTURE
#include "turn_texture.inc"
#endif
#ifdef FFT_FIXED
static void render_turn(void)
{
#ifdef FFT_TURN_TEXTURE
    if(tt_ready) {tt_turn(); return;}
#endif
    draw_turn();
}
#endif
#endif
#if !MOD_CORE
u16 mod_entry(u8 op, u8 a) { (void)op; (void)a; draw_turn(); return 0; }
#endif
#endif
#if MOD_CORE

// ---------------------------------------------------------------- camera
#define CAM_MX 24                       // the margins (pixels) a unit's feet keep from the edges
#define CAM_MT 39                       // body, smaller round pointer and six pixels of headroom
#define CAM_MB 12                       // bottom margin while following the unit
static void cam_clamp(s16 *x, s16 *y)
{
    if (*x < 0) *x = 0;
    if (*x > SC_W - RT_W) *x = SC_W - RT_W;
    if (*y < 0) *y = 0;
    if (*y > SC_H - RT_H) *y = SC_H - RT_H;
}

// is scene point (x, y) (a unit's feet) on the screen with a margin (its sprite, the HUD)?
static u8 cam_sees(s16 x, s16 y)
{
    x -= st.camx; y -= st.camy;
    return x >= CAM_MX && x < RT_W - CAM_MX && y >= CAM_MT && y < RT_H - CAM_MB;
}

// Where the camera goes: the cursor centred; while walking or choosing a facing, it stays
// until the unit nears the edge, then centres it; while a unit aims or acts, it stays when the unit and its
// target are both on screen, else centres the point between them
static void cam_target(s16 *tx, s16 *ty)
{
    s16 u, v, x, y;
    if (st.morph_mask) {
        Feet f = unit_feet(st.rot, morph_first());
        *tx = f.x - RT_W / 2; *ty = f.y - RT_H / 2 - 6;
        cam_clamp(tx, ty); return;
    }
    view_of(st.rot, st.cx, st.cz, &u, &v);
    x = scene_x(st.rot, u, v);
    y = scene_y(u, v) + HH - map_tiles[st.cz * MAP_W + st.cx].stand * (HU / 2);
    if (st.mode == M_WALK || st.mode == M_FACE) {
        Feet f = unit_feet(st.rot, st.mode == M_FACE ? st.act : st.sel);
        if (cam_sees(f.x, f.y)) { *tx = st.camx; *ty = st.camy; return; }
        x = f.x; y = f.y;
    } else if (st.banner == BN_START) {           // the objective: the enemies it lights up
        s16 sx = 0, sy = 0, n = 0, i;
        for (i = 0; i < NUNIT; i++)
            if (st.unit[i].team == TEAM_ENEMY && st.unit[i].hp) { Feet f = unit_feet(st.rot, (u8)i); sx += f.x; sy += f.y; n++; }
        if (n) { *tx = sx / n - RT_W / 2; *ty = sy / n - 68; cam_clamp(tx, ty); return; }
    } else if (st.mode == M_AIM || st.mode == M_ACT) {
        Feet a = unit_feet(st.rot, st.mode == M_ACT ? st.atk : st.act);
        if (st.mode == M_ACT) { Feet t = unit_feet(st.rot, st.tgt); x = t.x; y = t.y; }
        if (cam_sees(a.x, a.y) && cam_sees(x, y)) { *tx = st.camx; *ty = st.camy; return; }
        x = (x + a.x) >> 1; y = (y + a.y) >> 1;
    }
    *tx = x - RT_W / 2;
    *ty = y - RT_H / 2 - 6;
    cam_clamp(tx, ty);
}

static s16 ease(s16 c, s16 t)
{
    s16 d = (t - c) >> 2;
    if (!d) d = t > c ? 1 : t < c ? -1 : 0;
    return c + d;
}

// ---------------------------------------------------------------- game
static u8 *sbuf;

// a checksum of the picture just drawn, its 160 x 100 pixels (an AI unit thinks once the
// picture stands still): two sums (Fletcher's), no multiply or long rotation (~25k)
static u32 planes_sum(void)
{
    const u8 *l = rt_light, *d = rt_dark;
    u32 a = 0, b = 0;
    u16 y;
    for (y = 0; y < RT_H; y++, l += RT_PBYTES, d += RT_PBYTES) {
        const PlaneWord *p = (const PlaneWord *)l, *q = (const PlaneWord *)d;
        a += p[0]; b += a; a += p[1]; b += a; a += p[2]; b += a; a += p[3]; b += a; a += p[4]; b += a;
        a += q[0]; b += a; a += q[1]; b += a; a += q[2]; b += a; a += q[3]; b += a; a += q[4]; b += a;
    }
    return a ^ b;
}
static void hud_labels(void);

static void
#ifdef __m68k__
__attribute__((__stkparm__))
#endif
release(void)
{
#ifdef FFT_CONFIG
    pidle_free();
#endif
#ifdef FFT_ACTOR_BANK
    if (acache) free(acache);
    acache = RT_NULL;
#endif
    if (sbuf) free(sbuf);
    if (vbuf) free(vbuf);
    sbuf = vbuf = RT_NULL;
    scene_l = scene_d = RT_NULL;
}

void game_init(void)
{
    u16 i;
#ifdef FFT_REACH_CONTOUR
    u16 view_size=0;
    contour_gfx=RT_NULL;
#endif
    rt_state = &st;
    rt_state_size = sizeof(st);
#ifdef __m68k__
    sbuf = vbuf = RT_NULL;
#ifdef FFT_ACTOR_BANK
    acache = RT_NULL;                          // statics survive between runs
#endif
#ifdef FFT_CONFIG
    pidle=RT_NULL;
#endif
#endif
#if defined(__m68k__) && defined(FFT_CONFIG) && !defined(RT_CYCLES)
    // Compact movable AMS variables before our large malloc blocks lock their layout.
    // Take all archive/data pointers only afterwards. The cycle runner has no AMS heap.
    HeapCompress();
#endif
    for (i = 0; i < 4; i++) {                  // read in place (archived on the TI): fetched
#ifdef FFT_OPENING
        char name[6] = "ffov0";
#elif defined(FFT_STORY_GAR)
        char name[6] = "ffgv0";
#else
        char name[6] = "fftv0";                // again each run, the archive may have moved
#endif
        name[4] = (char)('0' + i);
        views[i] = rt_file(name,
#ifdef FFT_REACH_CONTOUR
            i==0 ? &view_size :
#endif
            RT_NULL);
    }
#ifdef FFT_REACH_CONTOUR
#ifdef FFT_CONTOUR_OFFSET
    contour_gfx=views[0] ? views[0]+FFT_CONTOUR_OFFSET:RT_NULL; // historical isolated control
#else
    // ttbin2oth's OTH/dat variable includes a six-byte trailer on TI; the
    // PC file is raw. Validate the footer and its offset before any mask read.
#ifdef __m68k__
    if(view_size>=6)view_size-=6;
#endif
    if(views[0] && !(view_size&1) && view_size>=CONTOUR_BYTES+10){
        const u8 *f=views[0]+view_size-10;
        if(RD32(f,0)==0x46433032UL){
            u16 off=RD16(f,3);
            if(!(off&1) && off+CONTOUR_BYTES==view_size-10 &&
               RD16(f,4)==CONTOUR_BYTES)contour_gfx=views[0]+off;
        }
    }
#endif
    if(!contour_gfx){W=RT_NULL;return;}
#endif
#ifdef FFT_OPENING
    ugfx = rt_file("ffoa", RT_NULL);
    igfx = rt_file("ffoi", RT_NULL);
#elif defined(FFT_STORY_GAR) && !defined(FFT_RAW_ACTORS)
    ugfx = rt_file("ffga", RT_NULL);
    igfx = rt_file("ffgi", RT_NULL);
#elif defined(FFT_STORY_GAR)                    // control: the raw banks read in place
    ugfx = rt_file("fftu", RT_NULL);
    wgfx = rt_file("fftw", RT_NULL);
    igfx = rt_file("ffgi", RT_NULL);
#else
    ugfx = rt_file("fftu", RT_NULL);
    wgfx = rt_file("fftw", RT_NULL);
    igfx = rt_file("ffti", RT_NULL);
#endif
#ifdef FFT_ACTOR_BANK
    wgfx = rt_file("ffcw", RT_NULL);           // the blades both chapters share
    abank[0] = rt_file("ffca", RT_NULL); abank[1] = rt_file("ffcb", RT_NULL);
    if (!abank[0] || !abank[1]) igfx = RT_NULL;
    else abank_n0 = RD16(abank[0], 0);
#endif
#ifdef FFT_CHAPTER
    cgfx = rt_file("ffcu", RT_NULL);
#ifdef FFT_OPENING
    cdesc = rt_file("ffod", RT_NULL);
#else
    cdesc = rt_file("ffgd", RT_NULL);
#endif
#ifdef FFT_MODULES
#ifdef FFT_OPENING
    mod_bank = rt_file("ffom", RT_NULL);
#else
    mod_bank = rt_file("ffgm", RT_NULL);
#endif
    if (!mod_ready()) igfx = RT_NULL;
#endif
    if (!cgfx || !cdesc || RD16(cdesc, 0) != 0x4643 || RD16(cdesc, 1) != 0x4431) igfx = RT_NULL;
    else {
        jobs = (const Job *)(igfx + CD(JOBS)); items = (const Item *)(igfx + CD(ITEMS));
        abilities = (const Ability *)(igfx + CD(ABILITIES));
        skill_sets = (const SkillSet *)(igfx + CD(SKILL_SETS));
        unit_defs = (const UnitDef *)(igfx + CD(UNIT_DEFS));
    }
#endif
    if (!igfx) { W = RT_NULL; return; }       // a message, ESC quits
    map_tiles = (const Tile *)(igfx + UI_TILES);
#ifdef __m68k__
    sl_l = (const PlaneWord *)UI_SHARED(SLOPES);
#else
    for (i = 0; i < 4 * SLOPE_ROWS; i++) slope_pc[i] = RD32(UI_SHARED(SLOPES), 2 * i);
    sl_l = slope_pc;
#endif
    sl_m = sl_l + SLOPE_ROWS; sc_l = sl_m + SLOPE_ROWS; sc_m = sc_l + SLOPE_ROWS;
    if (!sbuf) {                               // two blocks: AMS allocates at most ~64 KB
        vbuf = malloc(VIEW_BYTES);
        sbuf = vbuf ? malloc(2 * SC_PLANE + sizeof(Work)) : RT_NULL;
#ifdef FFT_ACTOR_BANK
        acache = sbuf ? malloc(AC_N * (UNIT_FB + 4)) : RT_NULL;
        if (!acache && sbuf) { free(sbuf); sbuf = RT_NULL; }
        ac_key = (u16 *)acache[AC_N]; ac_age = ac_key + AC_N;
#endif
        if (!sbuf) {                           // not enough memory: a message, ESC quits
            if (vbuf) free(vbuf);
            vbuf = RT_NULL; W = RT_NULL;
            return;
        }
        W = (Work *)(sbuf + 2 * SC_PLANE);
        memset(W, 0, sizeof(Work));
        for (i = 0; i < MAP_W * MAP_H; i++) {
#ifndef FFT_TABLES_ARCHIVED
            TILE_X[i] = (u8)(i % MAP_W); TILE_Z[i] = (u8)(i / MAP_W);
#endif
            W->hs[i] = map_tiles[i].stand; W->hl[i] = lower((u8)i);
            W->hw[i] = (u8)((map_tiles[i].walk != 0) | map_tiles[i].water << 1);
            W->top[i] = (u16)(tile_h(&map_tiles[i]) * 12);
        }
#ifndef FFT_TABLES_ARCHIVED
        for (i = 0; i < MAP_W * 28; i++) TILE_COL[i] = (u8)(i / 28);
        for (i = 0; i < MAP_H * 28; i++) TILE_ROW[i] = (u8)(i / 28 * MAP_W);
#endif
        edges(3);                              // Gariland's units all have Jump 3
        for (i = 0; i < NUNIT; i++) W->cover[i].rot = 0xFF;
        W->wrot = 0xFF;
        hud_labels();
        make_counters();
        scene_l = sbuf; scene_d = sbuf + SC_PLANE;
#ifdef __m68k__
        atexit(release);
#else
        (void)release;
#endif
    }
#ifdef FFT_EXPLORATION
    perspective_init();
#ifdef FFT_CONFIG
    pidle_init(); // allocation failure preserves the cold renderer
    pidle_active=0;
#endif
#ifdef FFT_TURN_TEXTURE
    tt_init();
#endif
#ifdef FFT_ULTRA_TRANSITION
    ultra_assets();
#endif
#ifdef FFT_CINEMA
    cinema_assets();
#ifdef FFT_TEXTURE_PREVIEW
    preview_assets();
    preview_iso_assets();
#endif
#endif

#endif
#ifndef FFT_COMPACT_UI
    memset(W->blade_k, 0, sizeof W->blade_k); // rt_file may reload the bank on the PC
#endif
    scene_rot = vbuf_rot = 0xFF;
#ifndef FFT_TABLES_ARCHIVED
    for (i = 0; i < 256; i++) {
        u16 d = 0;
        u8 b;
        for (b = 0; b < 8; b++) if (i & (0x80 >> b)) d |= 0xC000 >> (2 * b);
        dbl[i] = d;
    }
#endif
}

// a target for the weapon or the ability: a living unit (Phoenix Down: a KO one; unit_at never
// gives a crystal or a chest)
static u8 aim_ok(u8 t)
{
    const Unit *u = &st.unit[t];
    if (st.abil != NO_ABIL && abilities[st.abil].kind == AB_REVIVE) return !u->hp;
    return u->hp != 0;
}

static void aim(u8 ab)
{
    if (W) W->upd_k += AIM_K;
    st.mode = M_AIM; st.abil = ab;
    if (ab == NO_ABIL) attack_range(st.act); else skill_range(st.act, ab);
    if (W) W->win_k[0] = 0;                        // the preview: for this ability
}

void game_scenario(u16 n)
{
#ifdef FFT_OPENING
    opening_start(n);
#ifdef FFT_CONTOUR_DEFAULT
    st.reach_contour=1; // isolated benchmark control, ordinary battles default to full tiles
#endif
#else
#if !defined(FFT_EXPLORATION) && !defined(FFT_STORY_GAR)
    u8 i;
#endif
#ifdef FFT_EXPLORATION
    if (!n) n = 61;                            // exploration launcher: Ramza immediately
#endif
    memset(&st, 0, sizeof st);
#ifdef FFT_CONFIG
    st.pzoom = 1; st.psolid = 1;      // unchecked options: low-poly, wide, filled
#endif
#ifdef FFT_CONTOUR_DEFAULT
    st.reach_contour=1;
#endif
    st.want = st.prot = st.ai_view = 0xFF;
    if (!W) return;
#ifdef FFT_ACTOR_BANK
    actor_reset();                                // every actor decoded again, at once
#endif
    rt_seed = n ? n : rt_ticks();                 // a new draw each battle; the tests: fixed
#ifdef FFT_EXPLORATION
    if (n >= 61 && n <= 68) rt_seed = 61;         // identical actors/equipment for all cameras
#endif
    rand_seed(rt_seed);
    battle_units();                               // Gariland's eleven units at their start tiles
    memcpy(st.bag, bag_init, NBAG);
    st.ai_all = n == 16;                          // the AI plays both sides to the end
    turn_start();
    while (n && n != 16 && st.act != U_RAMZA) turn_end(0, 0);   // tests: Ramza's first turn
#if !defined(FFT_EXPLORATION) && !defined(FFT_STORY_GAR)
    if (n >= 1 && n <= 4) {                       // an enemy behind a building, each view
        st.unit[U_HIDE].x = 1; st.unit[U_HIDE].z = 11;    // (fft_test --find)
        st.rot = (u8)(n - 1);
        st.cx = st.unit[U_HIDE].x; st.cz = st.unit[U_HIDE].z;
    } else if (n == 5) {                          // Ramza selected, his range shown
        st.mode = M_TARGET; st.sel = U_RAMZA;
        compute_reach(U_RAMZA);
    } else if (n == 6) {                          // in the middle of a rotation
        st.turn = 1; st.turn_t = TURN_FRAMES / 2;
    } else if (n == 7) {                          // Ramza walking
        st.sel = U_RAMZA;
        if (make_path(U_RAMZA, 4, 8)) { st.mode = M_WALK; st.path_i = 1; st.walk_t = 2; }    } else if (n == 8 || n == 9) {                // an enemy in front of Ramza: aimed at, hit
        st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].face = 2;
        st.mode = M_AIM; st.cx = 4; st.cz = 10;
        attack_range(U_RAMZA);
        if (n == 9) {                             // (a hit: drawn again after a miss)
            st.acted = 1;
            do attack(U_RAMZA, 1); while (!st.hit);
            while (st.act_t < ACT_HIT + 3) attack_step();
        }
    } else if (n >= 11 && n <= 13) {              // abilities: the female Chemist's items (she
        Unit *c = &st.unit[U_CHEMIST];            // always knows Potion: rate 100, 30 JP), the
        st.unit[U_RAMZA].hp = 20;                 // Potion aimed at Ramza hurt; Ramza's Wish
        st.act = U_CHEMIST; st.cx = c->x; st.cz = c->z;   // landing on the recruit south of him
        st.mode = M_SKILLS; st.skill = 0;
        if (n == 12) { aim(known(U_CHEMIST, 0)); st.cx = 4; st.cz = 11; }
        if (n == 13) {
            st.act = U_RAMZA; st.acted = 1; st.unit[10].hp = 10;
            skill(U_RAMZA, 10, known(U_RAMZA, 0));
            while (st.act_t < ACT_HIT + 3) attack_step();
        }
    } else if (n == 14) {                         // the end at hand: four enemies KO, the last
        for (i = 2; i <= 5; i++) st.unit[i].hp = 0;   // with 1 HP in front of Ramza, its back to
        st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].face = 3; st.unit[1].hp = 1;   // him
    } else if (n == 15) {                         // the recruits KO, Delita in front of Ramza
        for (i = 7; i < NUNIT; i++) st.unit[i].hp = 0;    // (5 HP: his Wish knocks him out)
        st.unit[0].x = 4; st.unit[0].z = 10; st.unit[U_RAMZA].hp = 5;
    } else if (n >= 17 && n <= 25) {              // armed blows in four views, and bare hands
        u8 a = U_RAMZA, knife = n >= 21;
        st.rot = (u8)((n - 17) & 3);
        st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].face = 2;
        for (i = 0; i < NITEM; i++) if (items[i].id == (knife ? 1 : 0x13)) break;
        st.unit[a].eq[EQ_WEAPON] = i; unit_stats(&st.unit[a]);
        if (n == 25) {                          // Gariland's enemy Chemist is unarmed
            a = 3; st.unit[a].x = 4; st.unit[a].z = 11;
            st.unit[a].eq[EQ_WEAPON] = 0xFF;
            st.unit[U_RAMZA].x = 200;
        }
        st.act = a; st.ai = AI_OFF; st.cx = 4; st.cz = 10;
        do attack(a, 1); while (!st.hit || st.push);
    }
    if (n >= 26 && n <= 40) {              // milestone 26: each interface state
        st.ai = AI_OFF; st.mode = M_MENU; st.act = st.sel = U_RAMZA;
        st.cx = st.unit[U_RAMZA].x; st.cz = st.unit[U_RAMZA].z;
        if (n == 27) st.mode = M_ACTS;
        if (n == 28) { st.act = U_CHEMIST; st.mode = M_SKILLS; }
        if (n == 29 || n == 30) {
            st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].face = 2;
            if (n == 30) { st.act = U_CHEMIST; st.unit[U_RAMZA].hp = 20; aim(known(U_CHEMIST, 0)); st.cx = 4; st.cz = 11; }
            else { aim(NO_ABIL); st.cx = 4; st.cz = 10; }
        }
        if (n >= 31 && n <= 33) { st.mode = M_STATUS; st.back = M_MENU; st.page = n == 32; if (n == 33) st.sel = 1; }
        if (n == 34) { st.mode = M_BROWSE; st.cx = 1; st.cz = 5; }
        if (n == 35) { st.mode = M_BROWSE; st.cx = st.unit[7].x; st.cz = st.unit[7].z; }
        if (n == 36 && make_path(U_RAMZA, 4, 8)) { st.mode = M_WALK; st.path_i = 0; st.walk_t = 0; }
        if (n == 37) { st.unit[1].x = 4; st.unit[1].z = 10; do attack(U_RAMZA, 1); while (!st.hit); }
        if (n == 38) { st.mode = M_ORDER; st.back = M_MENU; }
        if (n == 39) { st.mode = M_QUIT; st.quit = 1; }
        if (n == 40) { st.act = U_CHEMIST; st.mode = M_STATUS; st.sel = U_CHEMIST; st.back = M_MENU; st.page = 1; }
    }
    if (n >= 41 && n <= 52) {              // milestone 27: canal display and traversal
        static const u8 actors[6] = { U_RAMZA, 0, 1, 5, 3, U_CHEMIST };
        static const u8 pos[6][2] = { {5, 10}, {5, 9}, {5, 8}, {5, 11}, {6, 11}, {7, 11} };
        for (i = 0; i < NUNIT; i++) st.unit[i].x = 200;
        st.ai = AI_OFF; st.mode = M_BROWSE; st.act = st.sel = U_RAMZA;
        st.unit[U_RAMZA].x = 5; st.unit[U_RAMZA].z = 10;
        st.cx = 5; st.cz = 10;
        if (n <= 44) {
            st.rot = (u8)(n - 41);
            for (i = 0; i < 6; i++) { st.unit[actors[i]].x = pos[i][0]; st.unit[actors[i]].z = pos[i][1]; }
        } else if (n <= 47) {
            u8 x = 5, z = 11;
            if (n == 45) { st.unit[U_RAMZA].x = 4; st.unit[U_RAMZA].z = 11; }
            if (n == 47) { st.unit[U_RAMZA].z = 11; x = 4; }
            if (make_path(U_RAMZA, x, z)) { st.mode = M_WALK; st.path_i = 0; st.walk_t = 0; }
            st.cx = st.unit[U_RAMZA].x; st.cz = st.unit[U_RAMZA].z;
        } else if (n == 48 || n == 49 || n == 52) {
            st.unit[1].x = 5; st.unit[1].z = 9;
            if (n == 49) st.unit[1].hp = 1;
            if (n == 52) {
                for (i = 0; i < NABIL; i++) if (abilities[i].flags & AF_EVADE && abilities[i].range > 1) break;
                do skill(U_RAMZA, 1, i); while (!st.hit);
            } else do attack(U_RAMZA, 1); while (!st.hit || st.push);
        } else if (n == 50) { st.unit[1].x = 5; st.unit[1].z = 9; st.unit[1].hp = 0; st.unit[1].dc = DC_CRYSTAL; }
        else if (n == 51) st.unit[U_RAMZA].hp = 1;
    }
    if (n >= 53 && n <= 58) {                     // equipment and ability inspection
        st.ai = AI_OFF; st.mode = M_STATUS; st.back = M_MENU; st.page = st.status_info = 1;
        st.sel = U_RAMZA; st.status_row = n == 53 ? EQ_WEAPON : n == 55 ? 4 : 0;
        if (n >= 56) {
            st.status_col = 1;
            if (n == 56) { for (i = 0; i < SET_N && known(st.sel, i) != NO_ABIL; i++) if (abilities[known(st.sel, i)].kind == AB_WISH) break; st.status_row = i; }
            if (n == 57) st.sel = st.act = U_CHEMIST;
            if (n == 58) { st.unit[st.sel].defend = 1; for (i = 0; i < SET_N && known(st.sel, i) != NO_ABIL; i++) ; st.status_row = i; }
        }
    }
    if (n == 59 || n == 60) end_battle(n == 59 ? END_VICTORY : END_DEFEAT);
#endif
    if (n >= 61 && n <= 68) {                 // direct Ramza; the three shoulder views
        st.ai = AI_OFF; st.mode = M_MENU; st.act = st.sel = U_RAMZA;
        st.cx = st.unit[U_RAMZA].x; st.cz = st.unit[U_RAMZA].z;
        st.want = st.prot = 0xFF; st.turn = 0;
#ifdef FFT_EXPLORATION
        if (n >= 62 && n <= 64) perspective_enter((u8)(n - 61));
#ifdef FFT_CINEMA
#ifdef FFT_CAMERA_MOTION
        st.cenabled = 1;
#endif
        if (n >= 65) {
            // Native combat fixtures: original enemies, Ramza already in attack range.
            st.unit[U_RAMZA].x = n == 66 ? 4 : 5;
            st.unit[U_RAMZA].z = n == 66 ? 6 : 5;
            if (n == 66) st.unit[U_RAMZA].learned |= 2; // Throw Stone for the ranged review
            st.cx = st.unit[U_RAMZA].x; st.cz = st.unit[U_RAMZA].z;
            st.menu = MN_ACT;
        }
#endif
#endif
    }
#ifndef FFT_STORY_GAR
    if (n >= 69 && n <= 72) {                 // loot presentation: crystal, chest, queue, covered
        st.ai = AI_OFF; st.mode = M_MENU; st.act = st.sel = U_RAMZA;
        st.want = st.prot = 0xFF; st.turn = st.banner = 0;
        st.unit[1].x = n == 72 ? 1 : 5; st.unit[1].z = n == 72 ? 11 : 9;
        st.unit[1].hp = 0; st.unit[1].dc = n == 70 ? DC_CHEST : DC_CRYSTAL;
        st.morph_mask = 1U << 1; st.morph_t = 0;
        if (n == 71) {
            st.unit[2].x = 6; st.unit[2].z = 9; st.unit[2].hp = 0; st.unit[2].dc = DC_CHEST;
            st.morph_mask |= 1U << 2;
        }
        st.cx = st.unit[U_RAMZA].x; st.cz = st.unit[U_RAMZA].z;
    }
#endif
    W->status_i = 0; W->pan_k[0][0] = W->pan_k[1][0] = 0; W->pan_ok[0] = W->pan_ok[1] = 0; W->fc_key = 0; W->win_k[0] = 0;
    if (!n) bn_start(BN_START);                  // a battle: its objective first
    cam_target(&st.camx, &st.camy);
    hl_seq++;
#endif
}

static void move_cursor(s16 du, s16 dv)
{
    s16 u, v, x, z;
#ifdef FFT_SOURCE_CAMERA
    s16 swap=du;du=dv;dv=swap;
#endif
    view_of(st.rot, st.cx, st.cz, &u, &v);
    u += du; v += dv;
    if ((u16)u >= view_w(st.rot) || (u16)v >= view_h(st.rot)) return;
    world_of(st.rot, u, v, &x, &z);
    st.cx = (u8)x; st.cz = (u8)z;
}

// a step (du, dv) in the view as a facing (world +x, -x, +z, -z), and back
static u8 face_of(s16 du, s16 dv)
{
    s16 x0, z0, x, z;
#ifdef FFT_SOURCE_CAMERA
    s16 swap=du;du=dv;dv=swap;
#endif
    world_of(st.rot, 1, 1, &x0, &z0);
    world_of(st.rot, 1 + du, 1 + dv, &x, &z);
    return x > x0 ? 0 : x < x0 ? 1 : z > z0 ? 2 : 3;
}

static void to_menu(void)
{
    st.mode = M_MENU;
    st.cx = st.unit[st.act].x; st.cz = st.unit[st.act].z;
}

// Manual / this unit / all allies. Never reset Move/Act or advance the CT clock.
// Cancellation lets a committed walk, blow or loot message complete normally.
static void auto_control(u8 choice)
{
    u8 i;
    if (!choice) {
        st.auto_units=0; st.ai_all=0;
        if (st.unit[st.act].team!=TEAM_PLAYER || !st.ai) return;
    } else if (choice==2) {
        for (i=0;i<NUNIT;i++) if (st.unit[i].team==TEAM_PLAYER) st.auto_units|=(u16)(1U<<i);
    } else st.auto_units|=(u16)(1U<<st.act);
    st.ai=choice ? AI_THINK:AI_OFF; st.ai_t=0;
    W->ai_k=W->frz=W->frz_n=W->hold=0; W->frz_h=0;
    if (choice) st.mode=M_BROWSE;
    else if (st.mode!=M_WALK && st.mode!=M_ACT && st.mode!=M_PICK) to_menu();
    hl_seq++;
}

// the turn menu: Move and Act once each (a move undone with [shift] before acting), Wait
// (then the facing), Status
static void menu_keys(void)
{
    Unit *u = &st.unit[st.act];
    if (input_pressed(K_UP)) st.menu = st.menu ? st.menu - 1 : MN_N - 1;
    if (input_pressed(K_DOWN)) st.menu = st.menu < MN_N - 1 ? st.menu + 1 : 0;
    if (input_pressed(K_A | K_ENTER)) {
        if (st.menu == MN_MOVE && !st.moved) {
            st.mode = M_TARGET; st.sel = st.act;
            compute_reach(st.act);
        } else if (st.menu == MN_ACT && !st.acted) {
            st.mode = M_ACTS; st.sub = 0;
        } else if (st.menu == MN_WAIT) {
            st.mode = M_FACE; st.wface = u->face;
        } else if (st.menu == MN_STATUS) {
            st.mode = M_STATUS; W->status_i = 0; st.page = 0; st.sel = st.act; st.back = M_MENU;
            st.status_col = st.status_row = st.status_info = 0;
        } else if (st.menu == MN_AUTO) {
            st.mode=M_AUTO; st.sub=1;
        }
    } else if (input_pressed(K_ESC | K_B)) {
        if (st.moved && !st.acted && !st.pick_t) {   // the move undone (not after a pick-up)
            u->x = st.mx; u->z = st.mz; u->face = st.mface;
            st.moved = 0; st.menu = MN_MOVE;
            to_menu();
        } else st.mode = M_BROWSE;                 // looking around
    }
}

// Act's list (Attack, the unit's skill set) and the abilities it knows (an item without any
// left greyed); [shift] goes back a level
static void acts_keys(void)
{
    u8 n = 0;
    if (st.mode == M_ACTS) {
        u8 m = (u8)(2 + st.unit[st.act].defend);  // Attack, the skill set, Defend
        if (input_pressed(K_UP)) st.sub = st.sub ? st.sub - 1 : m - 1;
        if (input_pressed(K_DOWN)) st.sub = st.sub < m - 1 ? st.sub + 1 : 0;
        if (input_pressed(K_A | K_ENTER)) {
            if (!st.sub) aim(NO_ABIL);
            else if (st.sub == 2) { st.acted = 1; defend(st.act); }
            else if (known(st.act, 0) != 0xFF) { st.mode = M_SKILLS; st.skill = 0; }
        } else if (input_pressed(K_ESC | K_B)) st.mode = M_MENU;
        return;
    }
    while (known(st.act, n) != 0xFF) n++;
    if (input_pressed(K_UP)) st.skill = st.skill ? st.skill - 1 : n - 1;
    if (input_pressed(K_DOWN)) st.skill = st.skill < n - 1 ? st.skill + 1 : 0;
    if (input_pressed(K_A | K_ENTER)) {
        u8 ab = known(st.act, st.skill);
        if (can_use(st.act, ab)) aim(ab);
    } else if (input_pressed(K_ESC | K_B)) st.mode = M_ACTS;
}

// How much of unit i view rot hides (the AI's camera): the tiles in front of it (greater
// diagonals) whose top face rises over its sprite, from the map's corner heights (the other
// views' depth is not unpacked), at four of the sprite's columns: the rows hidden, 0..4 * VIS_H
// (the tests: as the view's depth says half hidden or not on 558 of 560 tiles and views)
#define VIS_H 22
#define HIDE(j, r) do { s16 r_ = (r); if (r_ > hid[j]) hid[j] = r_; } while (0)
u8 view_hides(u8 rot, u8 i)
{
    static const s8 wdx[4] = { 1, 0, -1, 0 }, wdz[4] = { 0, -1, 0, 1 };   // world step of view +u
    const Unit *un = &st.unit[i];
    s16 u, v, k, d, base, x, z, hid[4] = { 0, 0, 0, 0 };
    s8 sx = (s8)(wdx[rot] + wdx[(rot + 3) & 3]), sz = (s8)(wdz[rot] + wdz[(rot + 3) & 3]);   // +u +v
    u8 j, n = 0, c0 = (u8)(-rot & 3), c1 = (u8)((1 - rot) & 3), c3 = (u8)((3 - rot) & 3);
    view_of(rot, un->x, un->z, &u, &v);
    base = HH - map_tiles[un->z * MAP_W + un->x].stand * (HU / 2);
    for (k = -1; k <= 1; k++) {                     // the three columns of tiles over the sprite
        d = k ? 1 : 2;                              // (d: diagonals in front), from the nearest:
        world_of(rot, u + ((d + k) >> 1), v + ((d - k) >> 1), &x, &z);   // a step is +u +v
        for (; d * HH <= base + MAXH * HU; d += 2, x += sx, z += sz) {
            const u8 *c;
            s16 h0, top, l, r;
            if ((u16)x >= MAP_W || (u16)z >= MAP_H) continue;
            c = map_tiles[z * MAP_W + x].c;
            h0 = c[c0];                             // its top face's upper edges: from the left
            top = base - d * HH + h0 * HU;          // corner up to the top one (rows above the
            l = (s16)c[c3] - h0 - 1;                // feet), down to the right one; the sprite's
            r = (s16)c[c1] - h0 - 1;                // columns -5 -2 2 5 against the tile's centre
            if (!k) {
                HIDE(0, top + (l * 5 >> 1)); HIDE(1, top + l); HIDE(2, top + r); HIDE(3, top + (r * 5 >> 1));
            } else if (k > 0) {                     // the tile to the right (centre +HW)
                HIDE(2, top + l * 5); HIDE(3, top + (l * 7 >> 1));
            } else {
                HIDE(0, top + (r * 7 >> 1)); HIDE(1, top + r * 5);
            }
        }
    }
    for (j = 0; j < 4; j++) n += (u8)(hid[j] > VIS_H ? VIS_H : hid[j]);
    return n;
}

#ifndef __m68k__
// the same rows read from the current view's depth (the tests; 0xFF: the view not unpacked)
u8 view_hides_now(u8 i)
{
    static const s8 col[4] = { -5, -2, 2, 5 };
    const u8 *t1 = VDEPTH, *m1 = t1 + SC_PLANE, *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
    Feet f;
    u8 j, n = 0;
    s16 y;
    if (vbuf_rot != st.rot) return 0xFF;
    f = unit_feet(st.rot, i);
    for (j = 0; j < 4; j++) {
        s16 x = f.x + col[j];
        u8 bit = (u8)(0x80 >> (x & 7));
        for (y = f.y - VIS_H; y < f.y; y++) {
            u16 o = (u16)(y * SC_BYTES + (x >> 3));
            if ((u16)y >= SC_H) continue;
            if ((t1[o] > f.depth && m1[o] & bit) || (t2[o] > f.depth && m2[o] & bit)) n++;
        }
    }
    return n;
}
#endif

// An AI unit's plan made (the frame not drawn): the view it will act in. When, from the tile it
// acts from, it or its target is more than half hidden: the view hiding the least of the worse
// of them (FFT turns the camera after the move; the player's view back on the player's turn)
static void ai_camera(void)
{
    Unit *a = &st.unit[st.act];
    u8 o = unit_at(st.ai_tgt % MAP_W, st.ai_tgt / MAP_W), x = a->x, z = a->z, r, h, g, bh = 0xFF;
    st.ai_view = 0xFF;
    if (st.acted || st.ai_ab == AI_NONE || st.ai_ab == ABIL_DEFEND) return;
    a->x = st.ai_o % MAP_W; a->z = st.ai_o / MAP_W;
    for (r = 0; r < 4; r++) {
        u8 q = (st.rot + r) & 3;
        h = view_hides(q, st.act);
        if (o && o - 1 != st.act && (g = view_hides(q, o - 1)) > h) h = g;
        if (!r && h <= 2 * VIS_H) break;
        if (h < bh) { bh = h; st.ai_view = q; }
    }
    a->x = x; a->z = z;
    if (st.ai_view == st.rot) st.ai_view = 0xFF;
}

// an AI turn on screen, as a player's without the menus: what comes next in its plan (st.ai_*):
// act (from its tile, or once moved: the view turned if needed, the cursor gliding to the
// target, the preview, an ability's name), move (the cursor gliding to the tile, its range
// shown, then the walk), after acting from its tile think again where to go, then the facing
static void ai_next(void)
{
    Unit *u = &st.unit[st.act];
    u8 me = (u8)(u->z * MAP_W + u->x);
    st.ai_t = 0;
    st.mode = M_BROWSE;
    W->frz = W->frz_n = 0; W->frz_h = 0;
    st.cx = u->x; st.cz = u->z;
    if (!st.acted && st.ai_ab != AI_NONE && (st.ai_o == me || st.moved)) {
        if (st.ai_ab == ABIL_DEFEND) { st.acted = 1; defend(st.act); return; }
        st.ai = AI_AIM;
        aim(st.ai_ab);
        if (st.ai_view != 0xFF && st.ai_view != st.rot) {   // the view turned first
            if (st.prot == 0xFF) st.prot = st.rot;
            st.want = st.ai_view;
        }
        st.ai_view = 0xFF;
    } else if (!st.moved && st.ai_o != me) {
        st.ai = AI_MOVE;
        st.mode = M_TARGET; st.sel = st.act;
        compute_reach(st.act);
    } else if (st.acted && !st.moved && st.ai_first) {
        st.ai = AI_THINK; st.ai_first = 0;
        st.ai_t = AI_HOLD;                      // thinking again after acting: no hold
        W->ai_k = 0;
    } else {
        st.ai = AI_FACE;
        st.mode = M_FACE;
        u->face = ai_facing(st.act); W->upd_k += FACE_K;
    }
}

// AI_MOVE, AI_AIM, once per frame: the cursor glides to the tile (a tile every AI_GLIDE
// frames), then stays: the move range AI_SHOW frames, the preview AI_PREVIEW, an ability's name
// AI_NAME more; done?
static u8 ai_shown(void)
{
    u8 c = st.ai == AI_MOVE ? st.ai_o : st.ai_tgt, x = c % MAP_W, z = c / MAP_W;
    if (st.cx != x || st.cz != z) {
        if (++st.ai_t >= AI_GLIDE) {
            st.ai_t = 0;
            if (st.cx != x) st.cx += st.cx < x ? 1 : -1;
            else st.cz += st.cz < z ? 1 : -1;
        }
        return 0;
    }
    if (!W->hud_ok) return 0;                   // the time shown counts once the HUD is drawn
    return ++st.ai_t >= (st.ai == AI_MOVE ? AI_SHOW : st.ai_ab == NO_ABIL ? AI_PREVIEW : AI_PREVIEW + AI_NAME);
}

static void ai_step(void)
{
    if (st.ai == AI_THINK) {                    // once the picture stands still (game_render)
        if (W->frz || ++W->frz_n > 40) {        // (40 frames at most: the picture no longer
            u8 done;                            // drawn anyway)
            ZB(9); done = ai_think(); ZE(9);
            if (done) st.ai = AI_GO;            // (this frame keeps the picture)
        }
    } else if (st.ai == AI_GO) {                // a new turn's plan made: its unit shown, the
        if (st.ai_t < AI_HOLD) { st.ai_t++; W->frz = 0; }   // world moving, AI_HOLD frames;
        else { ai_camera(); ai_next(); W->hold = 1; }   // then the plan shown from the next
    }                                           // frame (this one keeps the picture)
    else if (st.ai == AI_FACE ? ++st.ai_t >= AI_SHOW / 2 : ai_shown()) {
        if (st.ai == AI_MOVE) {
            Unit *u = &st.unit[st.act];
            if (make_path(st.act, st.ai_o % MAP_W, st.ai_o / MAP_W)) {
                st.mx = u->x; st.mz = u->z; st.mface = u->face;
                st.mode = M_WALK; st.path_i = 0; st.walk_t = 0;
            } else { st.moved = 1; ai_next(); }
        } else if (st.ai == AI_AIM) {
            u8 o = unit_at(st.ai_tgt % MAP_W, st.ai_tgt / MAP_W);
            st.acted = 1;
            if (!o) ai_next();
            else if (st.ai_ab == NO_ABIL) attack(st.act, o - 1);
            else skill(st.act, o - 1, st.ai_ab);
        } else turn_end(st.moved, st.acted);
    }
}

// can tile c be picked: a reachable tile to walk to, or (aim) a unit in reach to act on?
static u8 pickable(u8 c, u8 aim)
{
    u8 o;
    if (!aim) return st.reach[c] != 0xFF && st.reach[c] > 0;
    o = unit_at(c % MAP_W, c / MAP_W);
    return st.reach[c] == 1 && o && aim_ok(o - 1);
}

// [2nd] on a tile that cannot be picked: the tile drawn on the same pixels (in every view a
// canal tile at height 0 and the street tile 2 units higher one tile diagonally in front of
// it share 21-27 spots) when that one can be (+ 1; 0: none)
static u8 twin(u8 c, u8 aim)
{
    s16 u, v, x, y;
    u8 i;
    view_of(st.rot, c % MAP_W, c / MAP_W, &u, &v);
    x = scene_x(st.rot, u, v); y = scene_y(u, v) - map_tiles[c].stand * (HU / 2);
    for (i = 0; i < MAP_W * MAP_H; i++) {
        if (i == c) continue;
        view_of(st.rot, i % MAP_W, i / MAP_W, &u, &v);
        if (scene_x(st.rot, u, v) == x && scene_y(u, v) - map_tiles[i].stand * (HU / 2) == y && pickable(i, aim))
            return (u8)(i + 1);
    }
    return 0;
}

#ifndef FFT_OPENING
static u8 status_ability_count(void)
{
    u8 n = 0;
    while (known(st.sel, n) != NO_ABIL) n++;
    n += st.unit[st.sel].defend;
    return n ? n : 1;
}

static u8 status_ability(void)
{
    u8 ab = known(st.sel, st.status_row);
    return ab == NO_ABIL && st.unit[st.sel].defend ? ABIL_DEFEND : ab;
}

#endif
static void status_keys(void)
{
#ifdef FFT_OPENING
    if(input_pressed(K_B|K_ESC)){st.mode=st.back;W->pan_ok[0]=W->pan_ok[1]=0;W->pan_k[0][0]=W->pan_k[1][0]=0;}
    if(input_pressed(K_LEFT|K_RIGHT|K_A|K_ENTER)){st.page^=1;W->status_i=0;}
#else
    if (st.status_info) {
        if (input_pressed(K_A | K_ENTER | K_ESC | K_B)) st.status_info = 0;
    } else if (!st.page) {
        if (input_pressed(K_LEFT | K_RIGHT)) { st.page = 1; st.status_col = st.status_row = 0; }
        if (input_pressed(K_A | K_ENTER | K_ESC | K_B)) st.mode = st.back;
    } else {
        u8 count;
        if (input_pressed(K_LEFT)) {
            if (st.status_col) st.status_col = 0;
            else { st.page = 0; return; }
        }
        if (input_pressed(K_RIGHT)) {
            if (!st.status_col) st.status_col = 1;
            else { st.page = 0; return; }
        }
        count = st.status_col ? status_ability_count() : 5;
        if (st.status_row >= count) st.status_row = count - 1;
        if (input_pressed(K_UP)) st.status_row = st.status_row ? st.status_row - 1 : count - 1;
        if (input_pressed(K_DOWN)) st.status_row = st.status_row + 1 < count ? st.status_row + 1 : 0;
        if (input_pressed(K_A | K_ENTER)) st.status_info = 1;
        if (input_pressed(K_ESC | K_B)) st.mode = st.back;
    }
#endif
#ifdef FFT_STORY_GAR
    if(st.mode!=M_STATUS){W->pan_ok[0]=W->pan_ok[1]=0;W->pan_k[0][0]=W->pan_k[1][0]=0;}
#endif
}

// a frame's update (fast: an extra step of a held [2nd], the keys already read)
static u8 update(u8 fast)
{
    if (W) W->upd_k = 0;
    s16 tx, ty;
    u8 mode0 = st.mode;
    if (!W || !ugfx || !wgfx || !igfx || !views[0] || !views[1] || !views[2] || !views[3]) return !input_pressed(K_ESC);
    if (!fast && st.ai && st.mode!=M_END && (st.auto_units || st.ai_all) && input_pressed(K_ESC | K_B)) {
        auto_control(0); return 1;                 // consume cancellation, including during animation
    }
    if (st.ai != AI_THINK || st.banner || st.morph_mask) st.tick++;   // (a unit thinking: the picture stands still)
    if (st.deny) st.deny--;
    if (st.morph_mask) {
        W->frz = W->hold = 0;
        // Keep the KO visible during the pan, then play the whole effect on screen.
        cam_target(&tx, &ty);
        st.camx = ease(st.camx, tx); st.camy = ease(st.camy, ty);
        if (st.camx == tx && st.camy == ty && ++st.morph_t >= MORPH_FRAMES) {
            st.morph_mask &= (u16)~(1U << morph_first()); st.morph_t = 0;
        }
        return 1;
    }
    if (st.banner) {                               // a banner: the units move, the battle waits
        if (input_pressed(K_A | K_ENTER))          // skipped: the shade leaves, or it ends
            st.banner_t = st.banner_t < W->bn_end - DK_N ? (u8)(W->bn_end - DK_N - 1) : W->bn_end;
        if (++st.banner_t >= W->bn_end) {
            st.banner = 0; W->status_i = 0;
            if (st.mode == M_END) { scene_rot = 0xFF; W->end_n = 0; }   // the scene buffer: the results
        }
        cam_target(&tx, &ty);
        st.camx = ease(st.camx, tx); st.camy = ease(st.camy, ty);
        return 1;
    }
    if (!fast && input_pressed(K_MODE)) { st.over ^= 1; hl_seq++; }  // the reach over the scenery, or not
    if (st.turn) {                                 // rotating: nothing else moves the view
        if (++st.turn_t >= TURN_FRAMES) {          // done: the cursor stays where it was
            s16 u, v, px, py;
            view_of(st.rot, st.cx, st.cz, &u, &v);
            px = scene_x(st.rot, u, v) - st.camx; py = scene_y(u, v) - st.camy;
            st.rot = (st.rot + st.turn) & 3;
            st.turn = 0; st.turn_t = 0;
            view_of(st.rot, st.cx, st.cz, &u, &v);
            st.camx = scene_x(st.rot, u, v) - px; st.camy = scene_y(u, v) - py;
            cam_clamp(&st.camx, &st.camy);
        }
    } else if (!fast && input_pressed(K_F5 | K_F1)) {   // the player's view from now on
        st.turn = input_pressed(K_F5) ? 1 : -1; st.turn_t = 0;
        st.want = st.prot = 0xFF;
    } else if (st.want != 0xFF) {                  // turning to the view an AI unit wants
        if (st.want == st.rot) st.want = 0xFF;
        else { st.turn = ((st.want - st.rot) & 3) == 3 ? -1 : 1; st.turn_t = 0; }
    }

    if (st.mode == M_WALK) {
        const u8 *a = st.path[st.path_i], *b = st.path[st.path_i + 1];
        st.unit[st.sel].face = b[0] > a[0] ? 0 : b[0] < a[0] ? 1 : b[1] > a[1] ? 2 : 3;
        if (++st.walk_t >= step_frames(st.path_i)) {
            st.walk_t = 0;
            st.path_i++;
            st.unit[st.sel].x = st.path[st.path_i][0];
            st.unit[st.sel].z = st.path[st.path_i][1];
            st.cx = st.unit[st.sel].x; st.cz = st.unit[st.sel].z;
            if (st.path_i + 1 >= st.path_n) {      // back to the menu, on Act (Wait once acted),
                u8 i;                              // or a crystal or a chest there picked up
                st.mode = M_MENU; st.moved = 1; st.menu = st.acted ? MN_WAIT : MN_ACT;
                for (i = 0; i < NUNIT; i++)
                    if (st.unit[i].x == st.cx && st.unit[i].z == st.cz && DC_OBJ(st.unit[i].dc)) pick_up(st.sel, i);
                if (st.ai && st.mode == M_MENU) ai_next();
            }
        }
    } else if (st.mode == M_ACT) {
        attack_step();
    } else if (st.mode == M_PICK) {                // the message, then the menu (no undo)
        if (++st.pick_t >= PICK_FRAMES) { st.mode = M_MENU; if (st.ai) ai_next(); }
    } else if (st.mode == M_END) {                 // the results: the battle again, or quit
#ifdef FFT_STORY_GAR
        if(input_pressed(K_A|K_ENTER|K_ESC))return 0;
#else
        if (input_pressed(K_A | K_ENTER)) game_scenario(0);
        else if (input_pressed(K_ESC)) return 0;
#endif
    } else if (st.ai) {                            // an AI unit's turn
        if (!st.turn) ai_step();
    } else if (!st.turn) {
        if (input_pressed(K_APPS) && (st.mode == M_BROWSE || st.mode == M_MENU || st.mode == M_TARGET
            || st.mode == M_AIM || st.mode == M_ACTS || st.mode == M_SKILLS)) {
            st.back = st.mode; st.mode = M_ORDER;  // the turn order, closed by any key
        } else if (st.mode == M_ORDER) {
            if (input_pressed(K_A | K_B | K_ENTER | K_ESC | K_APPS | K_UP | K_DOWN | K_LEFT | K_RIGHT)) st.mode = st.back;
        } else if (st.mode == M_QUIT) {            // "Quit?": No unless Yes is picked
            if (input_pressed(K_UP | K_DOWN)) st.quit ^= 1;
            if (input_pressed(K_A | K_ENTER)) { if (!st.quit) return 0; st.mode = M_BROWSE; }
            else if (input_pressed(K_ESC | K_B)) st.mode = M_BROWSE;
        } else if (st.mode == M_AUTO) {
            if (input_pressed(K_UP)) st.sub=st.sub ? st.sub-1:2;
            if (input_pressed(K_DOWN)) st.sub=st.sub<2 ? st.sub+1:0;
            if (input_pressed(K_A | K_ENTER)) {
                auto_control(st.sub);
                if (!st.sub) to_menu();
            } else if (input_pressed(K_ESC | K_B)) to_menu();
        } else if (st.mode == M_MENU) {
            menu_keys();
        } else if (st.mode == M_ACTS || st.mode == M_SKILLS) {
            acts_keys();
        } else if (st.mode == M_FACE) {            // the arrows turn the unit, as the cursor
            Unit *u = &st.unit[st.act];
            if (input_pressed(K_UP)) u->face = face_of(0, -1);
            if (input_pressed(K_DOWN)) u->face = face_of(0, 1);
            if (input_pressed(K_LEFT)) u->face = face_of(-1, 0);
            if (input_pressed(K_RIGHT)) u->face = face_of(1, 0);
            if (input_pressed(K_A | K_ENTER)) turn_end(st.moved, st.acted);
            else if (input_pressed(K_ESC | K_B)) { u->face = st.wface; st.mode = M_MENU; }
        } else if (st.mode == M_STATUS) {
            status_keys();
        } else {                                   // the cursor: looking around, or a tile
            u32 arrows = rt_keys & (K_UP | K_DOWN | K_LEFT | K_RIGHT);
            if (input_pressed(K_UP | K_DOWN | K_LEFT | K_RIGHT)) st.rep = 0;
            if (arrows && (st.rep == 0 || (st.rep >= 8 && !((st.rep - 8) % 3)))) {
                if (arrows & K_UP) move_cursor(0, -1);
                if (arrows & K_DOWN) move_cursor(0, 1);
                if (arrows & K_LEFT) move_cursor(-1, 0);
                if (arrows & K_RIGHT) move_cursor(1, 0);
            }
            st.rep = arrows ? (u8)(st.rep < 255 ? st.rep + 1 : st.rep) : 0;
            if (input_pressed(K_A | K_ENTER)) {
                u8 o, c = (u8)(st.cz * MAP_W + st.cx);
                if (
                    (st.mode == M_TARGET || st.mode == M_AIM) && !pickable(c, st.mode == M_AIM)) {
                    u8 t = twin(c, st.mode == M_AIM);
                    if (t) { c = t - 1; st.cx = c % MAP_W; st.cz = c / MAP_W; }
                }
                o = unit_at(st.cx, st.cz);
                if (st.mode == M_TARGET) {
                    Unit *u = &st.unit[st.sel];
                    if (st.reach[c] != 0xFF && st.reach[c] > 0 && make_path(st.sel, st.cx, st.cz)) {
                        st.mx = u->x; st.mz = u->z; st.mface = u->face;
                        st.mode = M_WALK; st.path_i = 0; st.walk_t = 0;
                    } else st.deny = DENY_FRAMES;
                } else if (st.mode == M_AIM) {     // a unit in the weapon's or the ability's reach
                    if (st.reach[c] == 1 && o && aim_ok(o - 1)) {
                        st.acted = 1;
                        if (st.abil == NO_ABIL) attack(st.act, o - 1); else skill(st.act, o - 1, st.abil);
                    } else st.deny = DENY_FRAMES;
                } else if (o) {                    // a unit's numbers
                    st.mode = M_STATUS; W->status_i = 0; st.page = 0; st.sel = o - 1; st.back = M_BROWSE;
                    st.status_col = st.status_row = st.status_info = 0;
                } else to_menu();
            } else if (input_pressed(K_ESC | K_B)) {
                if (st.mode == M_BROWSE && input_pressed(K_ESC)) { st.mode = M_QUIT; st.quit = 1; }
                else if (st.mode == M_AIM) {            // back to the list it came from
                    st.mode = st.abil == NO_ABIL ? M_ACTS : M_SKILLS;
                    st.cx = st.unit[st.act].x; st.cz = st.unit[st.act].z;
                } else to_menu();
            }
        }
    }
    if (!st.turn) {
        cam_target(&tx, &ty);
        st.camx = ease(st.camx, tx);
        st.camy = ease(st.camy, ty);
    }
    if (RINGS(st.mode) != RINGS(mode0) || AIMING(st.mode) != AIMING(mode0)) hl_seq++;   // the reach rings
    return 1;
}

// [2nd] / [ENTER] held through an AI turn: AI_FAST steps a frame (its glides, holds, walks and
// blows), not while it thinks (its frames are full), nor after a step with heavy work (W->upd_k),
// nor when the last frame had less than FAST_K left (a step costs ~6k)
#if defined(FFT_CONFIG) || defined(FFT_REACH_CONTOUR)
#include "config.inc"
#endif

u8 game_update(void)
{
    u8 r, k = st.ai == AI_THINK ? AI_FAST : 1;   // (thinking this frame: its last step too)
    FRAME_BEGIN();
#ifdef FFT_ACTOR_BANK
#ifndef ACTOR_BUDGET
#define ACTOR_BUDGET 1                     // new poses decoded per frame (255: all, the raw-bank look)
#endif
    if(W){W->actor_budget=ACTOR_BUDGET;W->actor_misses=0;W->ac_now++;}
#endif
#ifdef FFT_OPENING
    if (st.open_stage || st.open_fade || (st.end == END_VICTORY && !st.banner)) return map_update();
#endif
#if defined(FFT_CONFIG) || defined(FFT_REACH_CONTOUR)
    if (config_update()) return 1;
#endif
#ifdef FFT_CINEMA
    if (st.cphase) { cinema_update(); return 1; }
#ifdef FFT_TEXTURE_PREVIEW
    if (!st.perspective && !st.turn && (st.mode == M_MENU || st.mode == M_BROWSE)
        && input_pressed(K_DIGIT(8))) preview_iso_set((preview_iso_style+1)%3);
#endif
#ifdef FFT_CAMERA_MOTION
    if (!st.perspective && st.mode != M_ACT && input_pressed(K_DIGIT(7))) st.cenabled ^= 1;
#endif
#endif
#ifdef FFT_EXPLORATION
#if defined(FFT_FIXED) && defined(FFT_CAMERA_MOTION)
    if (st.ptrans) {
        st.tick++;
        if (st.ptrans == 1 || st.ptrans == 3) {
            if (++st.ptrans_t > 16) { st.ptrans = st.ptrans_t = st.pfrom = 0; plow = 0; }
        } else if (st.ptrans_t) st.ptrans_t--;
        else {
            st.ptrans = st.ptrans_t = st.pfrom = st.perspective = 0; plow = 0;
            scene_rot = vbuf_rot = 0xFF; W->bg_ok = 0; W->status_i = 0;
        }
        pvalid = 0;
        return 1;
    }
#endif
    if (st.perspective) { perspective_keys(); return 1; }
    if (W && !st.ai && !st.banner && !st.morph_mask && !st.turn && st.want == 0xFF
        && (st.mode == M_MENU || st.mode == M_BROWSE) && input_pressed(K_C)) {
#ifdef FFT_FIXED
        if (input_held(K_MODE)) st.pzoom = 1;
#endif
        perspective_enter(1);
#if defined(FFT_FIXED) && defined(FFT_CAMERA_MOTION)
        st.ptrans = 1; st.ptrans_t = 0;
#endif
        return 1;
    }
#endif
    r = update(0);
#ifdef FFT_CINEMA
    if (r && cinema_eligible()) { cinema_begin(); return r; }
#endif
    for (; r && k < AI_FAST && !st.morph_mask && !st.banner && st.ai && st.ai != AI_THINK && rt_keys & (K_A | K_ENTER) && !W->upd_k
           && W->left >= FAST_K * k; k++)
    {
        r = update(1);
#ifdef FFT_CINEMA
        if (r && cinema_eligible()) { cinema_begin(); break; }
#endif
    }
    return r;
}

// the camera's window of the scene: 100 rows of 160 px from any pixel position, 32 bits at a
// time (a 32-bit word and the next 16 bits shifted)
static void __attribute__((noinline)) copy_view(void)   // its own registers (inlined: spilled)
{
    s16 y, sh = st.camx & 15, rs = 16 - sh;
    const u8 *sl = scene_l + st.camy * SC_BYTES + ((st.camx >> 4) << 1);
    const u8 *sd = scene_d + st.camy * SC_BYTES + ((st.camx >> 4) << 1);
    u8 *dl = rt_light, *dd = rt_dark;
#ifdef __m68k__                                 // a row: 5 longs, each a long and the next word
    for (y = 0; y < RT_H; y++, sl += SC_BYTES, sd += SC_BYTES, dl += RT_PBYTES, dd += RT_PBYTES) {
        const u16 *a = (const u16 *)sl, *b = (const u16 *)sd;
        PlaneWord *p = (PlaneWord *)dl, *q = (PlaneWord *)dd;
        if (!sh) {
            const PlaneWord *c = (const PlaneWord *)a, *e = (const PlaneWord *)b;
            p[0] = c[0]; p[1] = c[1]; p[2] = c[2]; p[3] = c[3]; p[4] = c[4];
            q[0] = e[0]; q[1] = e[1]; q[2] = e[2]; q[3] = e[3]; q[4] = e[4];
            continue;
        }
#define CV(k) p[k] = *(const PlaneWord *)(a + 2 * k) << sh | a[2 * k + 2] >> rs; \
              q[k] = *(const PlaneWord *)(b + 2 * k) << sh | b[2 * k + 2] >> rs
        CV(0); CV(1); CV(2); CV(3); CV(4);
#undef CV
    }
#else
    s16 i;
    for (y = 0; y < RT_H; y++, sl += SC_BYTES, sd += SC_BYTES, dl += RT_PBYTES, dd += RT_PBYTES) {
        if (!sh) {
            for (i = 0; i < RT_W / 16; i += 2) { WR32(dl, i, RD32(sl, i)); WR32(dd, i, RD32(sd, i)); }
        } else {
            for (i = 0; i < RT_W / 16; i += 2) {
                WR32(dl, i, RD32(sl, i) << sh | RD16(sl, i + 2) >> rs);
                WR32(dd, i, RD32(sd, i) << sh | RD16(sd, i + 2) >> rs);
            }
        }
    }
#endif
}

// the scene on the screen: the camera and the scene standing still, the last copy kept aligned
// to the screen (~25k), else the view copied (~105k), kept once the camera stands still
#ifndef __m68k__
u32 bg_used, bg_bad;                           // the tests: frames from the copy, those wrong
#endif
static void screen_scene(void)
{
    u8 *l = rt_light, *d = rt_dark;
    PlaneWord *p = W->bg[0], *q = W->bg[1], *a, *b;
    s16 y;
    // A cache completed below saves work from the next frame only. Optional HUD / cursor
    // work must not force itself into this frame as though it had used the cheap copy.
    W->bg_drawn = bg_usable();
    if (W->bg_drawn) {
        for (y = 0; y < RT_H; y++, l += RT_PBYTES, d += RT_PBYTES) {
            a = (PlaneWord *)l; b = (PlaneWord *)d;
            a[0] = *p++; a[1] = *p++; a[2] = *p++; a[3] = *p++; a[4] = *p++;
            b[0] = *q++; b[1] = *q++; b[2] = *q++; b[3] = *q++; b[4] = *q++;
        }
#ifndef __m68k__
        {                                      // the tests: the copy is the view's
            static u8 t[2][RT_PSIZE];
            memcpy(t[0], rt_light, RT_PSIZE); memcpy(t[1], rt_dark, RT_PSIZE);
            copy_view();
            bg_used++;
            if (memcmp(t[0], rt_light, RT_PSIZE) || memcmp(t[1], rt_dark, RT_PSIZE)) bg_bad++;
        }
#endif
        return;
    }
    copy_view();                               // kept once the camera stands still: a quarter
    if (W->bg_x != st.camx || W->bg_y != st.camy || W->bg_ver != scene_ver) {   // of the rows a frame,
        W->bg_wait = W->bg_part = W->bg_ok = 0;   // with room or anyway after a frame or two (it
        W->bg_x = st.camx; W->bg_y = st.camy; W->bg_ver = scene_ver;   // frees ~80k a frame)
        return;
    }
    if (!W->bg_part && W->left < SAVE_K && ++W->bg_wait <= SAVE_WAIT) return;
    W->left -= SAVE_K; W->bg_wait = 0;
    W->bg_ok = 0;                              // (the copy being remade)
    y = W->bg_part * (RT_H / 4);
    l += y * RT_PBYTES; d += y * RT_PBYTES; p += y * (RT_W / 32); q += y * (RT_W / 32);
    for (y = 0; y < RT_H / 4; y++, l += RT_PBYTES, d += RT_PBYTES) {
        a = (PlaneWord *)l; b = (PlaneWord *)d;
        *p++ = a[0]; *p++ = a[1]; *p++ = a[2]; *p++ = a[3]; *p++ = a[4];
        *q++ = b[0]; *q++ = b[1]; *q++ = b[2]; *q++ = b[3]; *q++ = b[4];
    }
    W->bg_part = (u8)((W->bg_part + 1) & 3); W->bg_ok = !W->bg_part;
}

const char *unit_name(u8 i)
{
#ifdef FFT_OPENING
    return (const char *)(igfx+RD16(igfx+OPENING_NAMES,i));
#else
    const Unit *u = &st.unit[i];
    if (u->gfx == UG_RAMZA) return "Ramza";
    if (u->gfx == UG_DELITA) return "Delita";
    {
        const u8 *names = igfx + (u->female ? UI_FEMALE : UI_MALE);
        return (const char *)names + RD16(names, u->name);
    }
#endif
}

static void hud_labels(void)
{
    #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define tri CONTOUR_REF_tri
#else
static const u8 tri[8] = { 0, 0x7E, 0x3E, 0x1E, 0x0E, 0x06, 0x02, 0 };
#endif
    u8 g, r, b;
    for (g = 0; g < 4; g++)
        for (r = 0; r < 8; r++) {
            u8 f = tri[g == 1 || g == 2 ? 7 - r : r], m = 0;
            if (g >= 2) { for (b = 0; b < 8; b++) if (f & (1 << b)) m |= (u8)(0x80 >> b); f = m; }
            W->arrow[g][0][r] = f;
        }
    for (g = 0; g < 4; g++)
        for (r = 0; r < 8; r++) {
            u8 o = (u8)((r ? W->arrow[g][0][r - 1] : 0) | W->arrow[g][0][r] | (r < 7 ? W->arrow[g][0][r + 1] : 0));
            W->arrow[g][1][r] = (u8)(o | o << 1 | o >> 1);
        }
}

// Canonical AMS small glyphs; shift five byte rows while building cached UI parts.
#include "text.inc"

static u8 put_s(char *s, u8 n, const char *t) { while (*t) s[n++] = *t++; return n; }
static u8 put_n(char *s, u8 n, u16 v)
{
    char b[5];
    u8 k = 0;
    do { b[k++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (k) s[n++] = b[--k];
    return n;
}


// AMS text drawn into a HUD buffer (rows of RT_PBYTES), the screen planes put back
static void text_into(u16 *l, u16 *d, s16 x, s16 y, const char *s, u8 color)
{
    void *sl = rt_light, *sd = rt_dark;
    rt_light = l; rt_dark = d;
    ui_text(x, y, s, color, RT_W);
    rt_light = sl; rt_dark = sd;
}

// Original portrait with palette-zero transparency and an offline white silhouette outline.
static void portrait(u8 gfx, s16 x, s16 y)
{
#ifdef FFT_OPENING
    if(gfx>1)gfx--;
#endif
    const u8 *p = igfx + UI_PORTRAITS + (u16)gfx * 348;
#ifdef __m68k__
    RtSprite sp = { 32, 29, p, p + 116, p + 232 };
#else
    u32 rows[3][29]; u8 k, r;
    RtSprite sp = { 32, 29, rows[0], rows[1], rows[2] };
    for (k = 0; k < 3; k++) for (r = 0; r < 29; r++) rows[k][r] = RD32(p + k * 116, r * 2);
#endif
    draw_sprite(x, y, &sp);
}

#ifndef FFT_OPENING
static void gauge(s16 x, s16 y, u16 v, u16 max, u8 color, u8 width)
{
    u8 n = max ? (u8)((width * v + max - 1) / max) : 0;
    if (n > width) n = width;
    draw_rect(x, y, width, 2, C_WHITE);
    if (n) draw_rect(x, y, n, 2, color);
}

// Outlined small text built into the transparent panel, never redrawn glyph by glyph on screen.
#endif
static void outlined_text(u8 *l, u8 *d, u8 *mask, s16 x, s16 y, const char *s, s16 limit)
{
    void *sl = rt_light, *sd = rt_dark;
    u8 r;
    l += (y - 1) * RT_PBYTES; d += (y - 1) * RT_PBYTES; mask += (y - 1) * RT_PBYTES;
    for (r = 0; r < 7; r++) memset(HUD_L + r * RT_PBYTES, 0, 10);
    rt_light = HUD_L; rt_dark = HUD_D;
    ui_text(x, 1, s, C_BLACK, limit);
    rt_light = sl; rt_dark = sd;
    for (r = 0; r < 7; r++, l += RT_PBYTES, d += RT_PBYTES, mask += RT_PBYTES) {
        const u8 *p = HUD_L + r * RT_PBYTES;
        u32 a = RD32(p, 0), b = RD32(p, 2), oa, ob;
        u16 c = RD16(p, 4), oc;
        if (r) { a |= RD32(p - RT_PBYTES, 0); b |= RD32(p - RT_PBYTES, 2); c |= RD16(p - RT_PBYTES, 4); }
        if (r < 6) { a |= RD32(p + RT_PBYTES, 0); b |= RD32(p + RT_PBYTES, 2); c |= RD16(p + RT_PBYTES, 4); }
        oa = a | a << 1 | a >> 1 | b >> 31;
        ob = b | b << 1 | b >> 1 | a << 31 | c >> 15;
        oc = (u16)(c | c << 1 | c >> 1 | b << 15);
        WR32(l, 0, (RD32(l, 0) & ~oa) | RD32(p, 0));
        WR32(d, 0, (RD32(d, 0) & ~oa) | RD32(p, 0));
        WR32(l, 2, (RD32(l, 2) & ~ob) | RD32(p, 2));
        WR32(d, 2, (RD32(d, 2) & ~ob) | RD32(p, 2));
        WR16(l, 4, (u16)((RD16(l, 4) & ~oc) | RD16(p, 4)));
        WR16(d, 4, (u16)((RD16(d, 4) & ~oc) | RD16(p, 4)));
        WR32(mask, 0, RD32(mask, 0) | oa);
        WR32(mask, 2, RD32(mask, 2) | ob);
        WR16(mask, 4, RD16(mask, 4) | oc);
    }
}

#define PAN_MAKE(side) W->pan[side][W->pan_f[side] ^ 1]   // the panel being made
static void panel_text(u8 side, s16 x, s16 y, const char *s, s16 limit)
{
    outlined_text((u8 *)PAN_MAKE(side)[0], (u8 *)PAN_MAKE(side)[1], (u8 *)PAN_MAKE(side)[2], x, y, s, limit);
}

static u8 ui_width(const char *s)
{
    u8 width = 0;
    while (*s) width += UI_SHARED(ADV)[(u8)*s++ - 32];
    return width;
}

static void panel_gauge(u8 side, s16 y, u16 v, u16 max, u8 color)
{
    void *sl = rt_light, *sd = rt_dark;
    s16 x = side ? TARGET_STATS_X : 22;
    u16 n = max ? (u16)((PANEL_GAUGE_W * v + max - 1) / max) : 0;
    if (n > PANEL_GAUGE_W) n = PANEL_GAUGE_W;
    // Rounded white silhouette: the four outer corner pixels stay transparent.
    // Its left edge shares the text anchor; the two-row fill starts one pixel inside.
    draw_rect(x + 1, y - 1, PANEL_GAUGE_W, 4, C_WHITE);
    draw_rect(x, y, PANEL_GAUGE_W + 2, 2, C_WHITE);
    draw_rect(x + 1, y, PANEL_GAUGE_W, 2, C_LGRAY);
    if (n) draw_rect(x + 1, y, n, 2, color);
    rt_light = rt_dark = PAN_MAKE(side)[2];
    draw_rect(x + 1, y - 1, PANEL_GAUGE_W, 4, C_BLACK);
    draw_rect(x, y, PANEL_GAUGE_W + 2, 2, C_BLACK);
    rt_light = sl; rt_dark = sd;
}

#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define st_names CONTOUR_REF_st_names
#else
static const char *const st_names[4] = { "Defend", "Poison", "Dark", "Silence" };
#endif   // ST_*

// One compact prediction line centered above the arrow; both character names stay visible.
// The preview uses the combat formulas directly; no separate target window is needed.
u8 forecast_text(u8 a, u8 t, u8 ab, char *s)
{
    const Ability *b = ab == NO_ABIL ? RT_NULL : &abilities[ab];
    u8 hit = b ? skill_hit(a, t, ab) : hit_chance(a, t), n = 0, j;
    if (b && b->kind == AB_CURE) {
        if (hit) {
            for (j = 0; !(st.unit[t].status & b->y & (1 << j)); j++) ;
            s[n++] = '-'; n = put_s(s, n, st_names[j]);
        } else n = put_s(s, n, "Miss");
    } else {                                   // a sure KO (the least damage left): "KO";
        const Unit *ut = &st.unit[t];          // a heal up to the HP the target can take
        u8 harm = !b || b->kind == AB_DAMAGE;
        u16 v = b ? skill_amount(a, t, ab, 0) : attack_damage(a, t, 0, 0);
        if (harm && ut->hp && v >= ut->hp) n = put_s(s, n, "KO");
        else {
            if (!harm && (!b || b->kind != AB_REVIVE) && v > ut->max_hp - ut->hp) v = ut->max_hp - ut->hp;
            s[n++] = harm ? '-' : '+';
            n = put_n(s, n, v);
            n = put_s(s, n, "HP");
        }
    }
    s[n++] = ' '; n = put_n(s, n, hit); s[n++] = '%'; s[n] = 0;
    return n;
}

static void make_forecast(u8 a, u8 t, u8 ab)
{
    char s[28];
    forecast_text(a, t, ab, s);
    {                                          // a white box framed black, black text,
        void *sl = rt_light, *sd = rt_dark;    // centered; its corners left out
        u8 w = (u8)(ui_width(s) + 5), x = (u8)((RT_W - w) >> 1);
        memset(W->forecast, 0, sizeof W->forecast);
        rt_light = rt_dark = W->forecast[2];
        draw_rect(x, 0, w, FC_ROWS, C_BLACK);
        draw_rect(x, 0, 1, 1, C_WHITE); draw_rect(x + w - 1, 0, 1, 1, C_WHITE);
        draw_rect(x, FC_ROWS - 1, 1, 1, C_WHITE); draw_rect(x + w - 1, FC_ROWS - 1, 1, 1, C_WHITE);
        rt_light = W->forecast[0]; rt_dark = W->forecast[1];
        draw_rect(x + 1, 0, w - 2, FC_ROWS, C_BLACK);
        draw_rect(x, 1, w, FC_ROWS - 2, C_BLACK);
        draw_rect(x + 1, 1, w - 2, FC_ROWS - 2, C_WHITE);
        rt_light = sl; rt_dark = sd;
        text_into(W->forecast[0], W->forecast[1], x + 2, 2, s, C_BLACK);
    }
}

static void make_panel(u8 side, u8 i)
{
    void *sl = rt_light, *sd = rt_dark;
    const Unit *u = &st.unit[i];
    u8 k = W->pan_n[side]++, n;
    s16 x = side ? TARGET_STATS_X : 22, limit = side ? 60 : 74;
    char s[24];
    rt_light = PAN_MAKE(side)[0]; rt_dark = PAN_MAKE(side)[1];
    if (!k) {
        memset(PAN_MAKE(side), 0, sizeof PAN_MAKE(side));
        u8 gfx=u->gfx;
#ifdef FFT_OPENING
        if(gfx>1)gfx--; // The trimmed portrait table has no Delita entry.
#endif
        const u8 *p = igfx + UI_PORTRAITS + (u16)gfx * 348 + 232;
        u8 r;
        portrait(u->gfx, side ? 61 : 0, 0);
        for (r = 0; r < 29; r++) {
            u8 *m = (u8 *)PAN_MAKE(side)[2] + r * RT_PBYTES;
            u32 bits = ~RD32(p, r * 2);
            if (side) {
                WR16(m, 3, (u16)(bits >> 29));
                WR16(m, 4, (u16)(bits >> 13));
            } else WR32(m, 0, bits);
        }
    } else if (k == 1) {
        const char *name = unit_name(i);
        panel_text(side, side ? limit - ui_width(name) : x, 1, name, limit);
    }
    else if (k == 4) {
        n = put_s(s, 0, "Lv"); n = put_n(s, n, u->level); s[n] = 0;
        panel_text(side, side ? 62 : 1, 23, s, side ? 80 : 20);
        W->pan_f[side] ^= 1; W->pan_ok[side] = 1;  // made: shown from now on
        memcpy(W->pan_fk[side], W->pan_k[side], sizeof W->pan_fk[side]);
    } else {
        u16 v = k == 2 ? u->hp : u->mp, m = k == 2 ? u->max_hp : u->max_mp;
        u8 width;
        n = put_n(s, 0, v); s[n++] = '/'; n = put_n(s, n, m); s[n] = 0;
        width = ui_width(s);
        panel_text(side, x, k == 2 ? 8 : 19, k == 2 ? "HP" : "MP", limit);
        panel_text(side, x + PANEL_GAUGE_W + 1 - width, k == 2 ? 8 : 19, s, x + PANEL_GAUGE_W + 1);
        panel_gauge(side, k == 2 ? 15 : 26, v, m, k == 2 ? C_BLACK : C_DGRAY);
    }
    rt_light = sl; rt_dark = sd;
}

static void draw_forecast(void)
{
    const u8 *l = (const u8 *)W->forecast[0], *d = (const u8 *)W->forecast[1], *m = (const u8 *)W->forecast[2];
    u8 *p = (u8 *)rt_light + (PANEL_Y - FC_ROWS - 1) * RT_PBYTES;
    u8 *q = (u8 *)rt_dark + (PANEL_Y - FC_ROWS - 1) * RT_PBYTES, r, k;
    for (r = 0; r < FC_ROWS; r++, l += RT_PBYTES, d += RT_PBYTES, m += RT_PBYTES, p += RT_PBYTES, q += RT_PBYTES)
        for (k = 2; k < 8; k += 2) {           // the box stays within bytes 4..15 (96 px)
            u32 a = ~RD32(m, k);
            WR32(p, k, (RD32(p, k) & a) | RD32(l, k)); WR32(q, k, (RD32(q, k) & a) | RD32(d, k));
        }
}

// The cached panel is aligned to ten bytes on either screen side, so its mask needs no shifts.
static void draw_panel(u8 side)
{
    const u8 *l = (const u8 *)W->pan[side][W->pan_f[side]][0], *d = (const u8 *)W->pan[side][W->pan_f[side]][1];
    const u8 *m = (const u8 *)W->pan[side][W->pan_f[side]][2];
    u8 *p = (u8 *)rt_light + PANEL_Y * RT_PBYTES + (side ? 10 : 0);
    u8 *q = (u8 *)rt_dark + PANEL_Y * RT_PBYTES + (side ? 10 : 0), r;
    for (r = 0; r < PANEL_ROWS; r++, l += RT_PBYTES, d += RT_PBYTES, m += RT_PBYTES, p += RT_PBYTES, q += RT_PBYTES) {
        u32 a = ~RD32(m, 0), b = ~RD32(m, 2);
        u16 c = (u16)~RD16(m, 4);
        WR32(p, 0, (RD32(p, 0) & a) | RD32(l, 0)); WR32(q, 0, (RD32(q, 0) & a) | RD32(d, 0));
        WR32(p, 2, (RD32(p, 2) & b) | RD32(l, 2)); WR32(q, 2, (RD32(q, 2) & b) | RD32(d, 2));
        WR16(p, 4, (u16)((RD16(p, 4) & c) | RD16(l, 4))); WR16(q, 4, (u16)((RD16(q, 4) & c) | RD16(d, 4)));
    }
}

// Bottom-right command menu and submenus, ability name, turn order or Quit.
// Cached one part per frame: raised title / framed box, then each line. Hand sits outside.
static u8 win_lines(u8 m)
{
    u8 n = 0;
    if (m == M_MENU) return MN_N;
    if (m == M_AUTO) return 3;
    if (m == M_QUIT) return 3;
    if (m == M_ORDER) { while (n < ORDER_N && st.turns[n] != 0xFF) n++; return n; }
    if (m == M_ACTS) return (u8)(2 + st.unit[W->win_k[2]].defend);
    if (m == M_ACT || m == M_PICK) return 1;
    while (known(W->win_k[2], n) != 0xFF) n++;    // M_SKILLS
    return n;
}

static void make_window(void)
{

#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define entries CONTOUR_REF_entries
#else
static const char *const entries[MN_N] = { "Move", "Act", "Wait", "Status", "Auto-battle" };
#endif


#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define automatic CONTOUR_REF_automatic
#else
static const char *const automatic[3] = { "Manual", "This unit", "All allies" };
#endif

    u16 *l = W->win[0], *d = W->win[1];
    const Unit *u = &st.unit[W->win_k[2]];
    u8 r, i, n, k = W->win_n++, m = W->win_k[0], c = C_BLACK;
    char s[28];
    if (!k) {
        u8 h = win_lines(m) * 6 + (m == M_MENU || m == M_AUTO || m == M_ACTS || m == M_SKILLS || m == M_QUIT ? 15 : 11), w = 4;
        const char *title = m == M_MENU ? "Menu" : m == M_ACTS || m == M_ACT ? "Act"
            : m == M_AUTO ? "Auto-battle" : m == M_SKILLS ? skill_sets[u->set].name : m == M_ORDER ? "Turn order" : m == M_PICK ? "Found" : "Quit?";
        W->win_w = w; W->win_h = h; W->win_m = (u8)(win_lines(m) + 1);
        for (r = 0; r < WIN_ROWS; r++)
            for (i = 0; i < w; i++) {
                u8 body = r >= 3 && r < h;
                u16 e = body ? r == 3 || r == h - 1 ? 0xFFFF : i == 0 ? 0x8000 : i == 3 ? 1 : 0 : 0;
                u16 light = body ? 0xFFFF : 0;
                if (body && r > 3 && r < h - 1) {
                    if (!i) light &= (u16)~0x4000;
                }
                WR16((u8 *)l, r * (RT_PBYTES / 2) + i, light);
                WR16((u8 *)d, r * (RT_PBYTES / 2) + i, e);
            }
        {
            void *sl = rt_light, *sd = rt_dark;
            rt_light = l; rt_dark = d;
            draw_rect(0, 0, ui_width(title) + 6, 8, C_BLACK);
            rt_light = sl; rt_dark = sd;
        }
        text_into(l, d, 3, 1, title, C_WHITE);
        return;
    }
    k--;
    n = 0;
    if (m == M_QUIT) {
        n = put_s(s, 0, k == 0 ? "Quit?" : k == 1 ? "Yes" : "No");
    } else if (m == M_ORDER) {                     // the next turns, the active unit first;
        const Unit *t = &st.unit[st.turns[k]];     // enemies in dark grey
        s[0] = (char)('1' + k); s[1] = ' ';
        n = put_s(s, 2, unit_name(st.turns[k]));
        if (t->team == TEAM_ENEMY) c = C_DGRAY;
    } else if (m == M_AUTO) {
        n = put_s(s, 0, automatic[k]);
    } else if (m == M_MENU) {
        n = put_s(s, 0, entries[k]);
        if (k <= MN_ACT && W->win_k[1] & (1 << k)) c = C_DGRAY;
    } else if (m == M_ACTS) {
        n = put_s(s, 0, k == 2 ? st_names[0] : k ? skill_sets[u->set].name : "Attack");
        if (k == 1 && W->win_k[1]) c = C_DGRAY;
    } else if (m == M_SKILLS) {                    // an item: how many are left
        u8 ab = known(W->win_k[2], k);
        n = put_s(s, 0, abilities[ab].name);
        if (abilities[ab].flags & AF_ITEM && u->team == TEAM_PLAYER) {
            s[n++] = ' '; n = put_n(s, n, st.bag[abilities[ab].bag]);
        }
        if (!can_use(W->win_k[2], ab)) c = C_DGRAY;
    } else if (m == M_ACT) {
        n = put_s(s, 0, st.abil == NO_ABIL ? "Attack" : st.abil == ABIL_DEFEND ? st_names[0] : abilities[st.abil].name);
    } else if (m == M_PICK) {                      // a crystal or a chest picked up
        if (st.pick == 0xFF) n = put_s(s, 0, "HP MP full");
        else { n = put_s(s, 0, "Got "); n = put_s(s, n, items[st.pick].name); }
    }
    s[n] = 0;
    text_into(l, d, m == M_MENU || m == M_AUTO || m == M_ACTS || m == M_SKILLS || m == M_QUIT ? 6 : 3, 10 + 6 * k, s, c);
}

static void draw_hand(s16 x, s16 y)
{
    #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define sway CONTOUR_REF_sway
#else
static const u8 sway[8] = { 0, 1, 2, 3, 3, 2, 1, 0 };
#endif
    RtSprite sp = { 32, HAND_H, hand_gfx[0], hand_gfx[1], hand_gfx[2] };
    draw_sprite(x + sway[(st.tick >> 2) & 7], y, &sp);
}

static void copy_box(const u16 *l, const u16 *d, u8 x, u8 y, u8 words, u8 rows)
{
    u8 r;
    u8 *al = (u8 *)rt_light + y * RT_PBYTES + x, *ad = (u8 *)rt_dark + y * RT_PBYTES + x;
    for (r = 0; r < rows; r++, al += RT_PBYTES, ad += RT_PBYTES, l += RT_PBYTES / 2, d += RT_PBYTES / 2) {
        PlaneWord *a = (PlaneWord *)al, *b = (PlaneWord *)ad;
        const PlaneWord *p = (const PlaneWord *)l, *q = (const PlaneWord *)d;
        a[0] = p[0]; b[0] = q[0];
        if (words == 2) continue;
        a[1] = p[1]; b[1] = q[1];
        if (words == 5) { ((u16 *)a)[4] = l[4]; ((u16 *)b)[4] = d[4]; }
        else if (words == 10) { a[2] = p[2]; b[2] = q[2]; a[3] = p[3]; b[3] = q[3]; a[4] = p[4]; b[4] = q[4]; }
    }
}

// "Enemy" and "Guest", outlined, once: drawn above a portrait of that team
static void make_tags(void)
{

#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define team_names CONTOUR_REF_team_names
#else
static const char *const team_names[2] = { "Enemy", "Guest" };
#endif

    u8 t;
    memset(W->tag, 0, sizeof W->tag);
    for (t = 0; t < 2; t++) {
        outlined_text((u8 *)W->tag[t][0], (u8 *)W->tag[t][1], (u8 *)W->tag[t][2], 1, 1, team_names[t], 31);
        W->tag_w[t] = ui_width(team_names[t]);
    }
    W->tag_ok = 1;
}

// the tag over the outer edge of a side's portrait: left-aligned on the left, right on the right
static void draw_tag(u8 side, u8 t)
{
    u8 r, sh = side ? (u8)(30 - W->tag_w[t]) : 0;
    const u8 *l = (const u8 *)W->tag[t][0], *d = (const u8 *)W->tag[t][1], *m = (const u8 *)W->tag[t][2];
    u8 *p = (u8 *)rt_light + (PANEL_Y - 7) * RT_PBYTES + (side ? 16 : 0);
    u8 *q = (u8 *)rt_dark + (PANEL_Y - 7) * RT_PBYTES + (side ? 16 : 0);
    for (r = 0; r < 7; r++, l += RT_PBYTES, d += RT_PBYTES, m += RT_PBYTES, p += RT_PBYTES, q += RT_PBYTES) {
        u32 k = ~(RD32(m, 0) >> sh);
        WR32(p, 0, (RD32(p, 0) & k) | RD32(l, 0) >> sh);
        WR32(q, 0, (RD32(q, 0) & k) | RD32(d, 0) >> sh);
    }
}

// what a side's panel shows: the unit, its HP, MP, level and maxima
static void panel_keys(u8 who, u16 *keys)
{
    const Unit *u = &st.unit[who - 1];
    keys[0] = who; keys[1] = u->hp; keys[2] = u->mp; keys[3] = u->level;
    keys[4] = u->max_hp; keys[5] = u->max_mp;
}

// a part of a panel to make (the one shown stays until the new one is whole); any?
static u8 panel_part(const u8 *pk)
{
    u8 i, j;
    for (i = 0; i < 2; i++) if (pk[i]) {
        u16 keys[6];
        u8 diff = 0, shown = W->pan_ok[i];
        panel_keys(pk[i], keys);
        for (j = 0; j < 6; j++) { diff |= keys[j] != W->pan_k[i][j]; shown &= keys[j] == W->pan_fk[i][j]; }
        if (diff) {
            for (j = 0; j < 6; j++) W->pan_k[i][j] = keys[j];
            W->pan_n[i] = shown ? 5 : 0;
            if (!i && !shown) W->fc_key = 0;   // the attacker changed: the prediction again
        }
        if (W->pan_n[i] < 5) { ZB(12); make_panel(i, pk[i] - 1); ZE(12); return 1; }
    }
    return 0;
}

// a side's panel shown and up to date?
static u8 panel_shown(u8 side, u8 who)
{
    u16 keys[6];
    panel_keys(who, keys);
    return W->pan_ok[side] && !memcmp(keys, W->pan_fk[side], sizeof keys);
}

// The HUD's parts are made one a frame at least (more while the budget has room): a menu, then
// the panels; the active unit (or the attacker) on the left, the unit under the cursor (or the
// target) on the right, during moves and blows too
static void draw_hud(void)
{
    u8 o, i, k[3], pk[2], forecast = 0, hidden = st.turn, parts;
    u8 pre = st.mode == M_WALK && !st.ai, ahead = 0;   // the menu after the walk made during it
    u16 forecast_key;
    o = unit_at(st.cx, st.cz);
    pk[0] = hidden ? 0 : st.mode == M_ACT ? st.atk + 1 : st.act + 1;
    pk[1] = hidden ? 0 : st.mode == M_ACT ? (st.tgt != st.atk ? st.tgt + 1 : 0)
        : o && o - 1 != st.act && (st.mode == M_AIM || st.mode == M_BROWSE || st.mode == M_TARGET) ? o : 0;
    k[0] = st.mode == M_MENU || st.mode == M_AUTO || st.mode == M_ACTS || st.mode == M_SKILLS
        || st.mode == M_PICK || st.mode == M_ORDER || st.mode == M_QUIT ? st.mode : 0;
    if (pre) k[0] = M_MENU;
    k[1] = k[0] == M_MENU ? (u8)(pre | st.moved | st.acted << 1) : k[0] == M_ACTS ? known(st.act, 0) == 0xFF
        : k[0] == M_PICK ? st.pick : k[0] == M_ORDER ? st.turns[1] : 0;
    k[2] = st.act;
    if (st.mode == M_AIM && o && st.reach[st.cz * MAP_W + st.cx] == 1 && aim_ok(o - 1)) {
        k[0] = M_AIM; k[1] = st.act; k[2] = o - 1;
        if (st.ai == AI_AIM && st.abil != NO_ABIL && st.ai_t >= AI_PREVIEW && st.cz * MAP_W + st.cx == st.ai_tgt) {
            k[0] = M_ACT; k[1] = st.abil; k[2] = st.act;
        }
    }
    if (k[0] == M_AIM) { forecast = k[2] + 1; k[0] = 0; }
    forecast_key = forecast ? (u16)((u16)(u8)(st.abil + 1) << 8 | forecast) : 0;
    // one part even without room (the budget reckons high), but not in a frame already full,
    // unless it waited WAIT_MAX frames
#ifdef LEFT_TRACE
    BENCH_VALUE((u32)(u16)W->left);
#endif
    if (W->left >= 0 || (++W->hud_wait > WAIT_MAX && W->bg_drawn)) W->hud_wait = 0;
    for (parts = W->hud_wait ? 1 : 0; parts == 0 || W->left >= HUD_K; parts++) {
        u8 win = k[0] && (k[0] != W->win_k[0] || k[1] != W->win_k[1] || k[2] != W->win_k[2] || W->win_n < W->win_m);
        if (win && (!pre || ahead)) {              // a menu first; one made ahead after the panels
            if (k[0] != W->win_k[0] || k[1] != W->win_k[1] || k[2] != W->win_k[2]) {
                for (i = 0; i < 3; i++) W->win_k[i] = k[i];
                W->win_n = 0;
            }
            ZB(11); make_window(); ZE(11);
        } else if (!W->tag_ok) make_tags();
        else if (panel_part(pk)) ;
        else if (forecast && W->fc_key != forecast_key) {   // its own part
            ZB(13); make_forecast(pk[0] - 1, forecast - 1, (u8)((forecast_key >> 8) - 1)); ZE(13);
            W->fc_key = forecast_key;
        }
        else if (win) { ahead = 1; continue; }
        else break;
        W->left -= HUD_K;
    }
    for (i = 0; i < 2; i++) if (pk[i] && W->pan_ok[i] && W->pan_fk[i][0] == pk[i]) {
        u8 team = st.unit[pk[i] - 1].team;
        draw_panel(i);
        if (team != TEAM_PLAYER) draw_tag(i, team == TEAM_GUEST);
    }
    if (forecast && W->fc_key == forecast_key && W->pan_ok[0] && W->pan_fk[0][0] == pk[0])
        draw_forecast();
    if (forecast && W->fc_key == forecast_key && W->pan_ok[0] && W->pan_ok[1]
        && W->pan_fk[0][0] == pk[0] && W->pan_fk[1][0] == pk[1]) {
        #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define forecast_arrow CONTOUR_REF_forecast_arrow
#else
static const u8 forecast_arrow[11] = { 0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x60, 0 };
#endif
        #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define forecast_mask CONTOUR_REF_forecast_mask
#else
static const u8 forecast_mask[11] = { 0x0F, 0x07, 0x03, 0x81, 0xC0, 0xE0, 0xC0, 0x81, 0x03, 0x07, 0x0F };
#endif
        RtSprite sp = { 8, 11, forecast_arrow, forecast_arrow, forecast_mask };
        draw_sprite(76, PANEL_Y + 9, &sp);
    }
    if (!hidden && !pre && k[0] && W->win_k[0] == k[0] && W->win_k[1] == k[1] && W->win_k[2] == k[2] && W->win_n == W->win_m) {
        u8 y = k[0] == M_ACT ? PANEL_Y - W->win_h : RT_H - W->win_h;
        u8 r;
        u16 *l = (u16 *)rt_light + y * (RT_PBYTES / 2) + 6;
        u16 *d = (u16 *)rt_dark + y * (RT_PBYTES / 2) + 6;
        // Above the frame, copy only the title tab; the scenery fills the rest.
        for (r = 0; r < 3; r++, l += RT_PBYTES / 2, d += RT_PBYTES / 2)
            for (i = 0; i < W->win_w; i++) {
                u16 keep = (u16)~W->win[0][i];
                l[i] = (l[i] & keep) | W->win[0][r * (RT_PBYTES / 2) + i];
                d[i] = (d[i] & keep) | W->win[1][r * (RT_PBYTES / 2) + i];
            }
        copy_box(W->win[0] + 3 * (RT_PBYTES / 2), W->win[1] + 3 * (RT_PBYTES / 2), 12, y + 3, W->win_w, W->win_h - 3);
        if (k[0] == M_MENU || k[0] == M_AUTO || k[0] == M_ACTS || k[0] == M_SKILLS || k[0] == M_QUIT) {
            u8 e = k[0] == M_MENU ? st.menu : k[0] == M_ACTS || k[0] == M_AUTO ? st.sub : k[0] == M_SKILLS ? st.skill : st.quit + 1;
            draw_hand(80, y + 7 + 6 * e);
        }
    }
    W->hud_ok = (!pk[0] || panel_shown(0, pk[0])) && (!pk[1] || panel_shown(1, pk[1]))
        && (!forecast || W->fc_key == forecast_key)
        && (hidden || pre || !k[0] || (W->win_k[0] == k[0] && W->win_k[1] == k[1] && W->win_k[2] == k[2] && W->win_n == W->win_m));
}

#ifndef __m68k__
u8 hud_ready(void) { return W && W->hud_ok; }
#endif

#endif
#if MOD_CORE || MOD_PART(MOD_SCREENS)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define help_lines CONTOUR_REF_help_lines
#else
static const char *const help_lines[2] = { "ENTER: stats/equip   SHIFT: back", "[ENTER] again   [ESC] quit" };
#endif

#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define signs CONTOUR_REF_signs
#else
static const char *const signs[12] = { "Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
    "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces" };
#endif

#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define equip_labels CONTOUR_REF_equip_labels
#else
static const char *const equip_labels[5] = { "Head", "Body", "Accessory", "Weapon", "Shield" };
#endif

#endif
#if MOD_PART(MOD_SCREENS)                  // with modules: the screens module (module.h)
#if !MOD_CORE && defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define st_names CONTOUR_REF_st_names
#endif
// Details describe the equipped item or learned ability; browsing never changes the loadout.
#ifdef FFT_OPENING
static void draw_status(void)
{
    const Unit *u=&st.unit[st.sel];
    u8 k=W->status_i;char line[40];
    if(k<9){
        void *l=rt_light,*d=rt_dark;
        rt_light=W->status[0];rt_dark=W->status[1];
        if(!k){draw_rect(0,0,160,100,C_WHITE);ui_text(4,4,unit_name(st.sel),C_BLACK,156);}
        else if(k==1){u8 n=put_s(line,0,"HP ");n=put_n(line,n,u->hp);line[n++]='/';n=put_n(line,n,u->max_hp);line[n]=0;ui_text(4,18,line,C_BLACK,156);}
        else if(k==2){u8 n=put_s(line,0,"MP ");n=put_n(line,n,u->mp);line[n]=0;ui_text(4,28,line,C_BLACK,156);}
        else if(k<8){
            u8 slot=k-3;
            if(st.page)ui_text(4,40+slot*8,u->eq[slot]==255?"-":items[u->eq[slot]].name,C_BLACK,156);
            else {
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define stat_labels CONTOUR_REF_stat_labels
#else
static const char *const stat_labels[5]={"Level ","PA ","Speed ","Move ","Jump "};
#endif
u8 v=slot==0?u->level:slot==1?u->pa:slot==2?u->sp:slot==3?u->move:u->jump;u8 n=put_s(line,0,stat_labels[slot]);n=put_n(line,n,v);line[n]=0;ui_text(4,40+slot*8,line,C_BLACK,156);}
        }else ui_text(4,90,help_lines[0],C_BLACK,158);
        rt_light=l;rt_dark=d;W->status_i++;
    }
    copy_box(W->status[0],W->status[1],0,0,10,100);
}
#else
static void status_detail(u8 k)
{
    char s[64];
    u8 n = 0, id = st.status_col ? status_ability() : st.unit[st.sel].eq[st.status_row];
    if (!st.status_col) {
        const Item *it;
        if (k == 3) { ui_text(6, 38, equip_labels[st.status_row], C_DGRAY, 159); return; }
        if (k == 4) { ui_text(6, 47, id == 0xFF ? "No item" : items[id].name, C_BLACK, 159); return; }
        if (id == 0xFF) return;
        it = &items[id];
        if (k == 5) { n = put_s(s, n, "HP +"); n = put_n(s, n, it->hp); n = put_s(s, n, "   MP +"); n = put_n(s, n, it->mp); }
        else if (k == 6) { n = put_s(s, n, "PA +"); n = put_n(s, n, it->pa); n = put_s(s, n, "   MA +"); n = put_n(s, n, it->ma); n = put_s(s, n, "   Speed +"); n = put_n(s, n, it->sp); }
        else if (k == 7) { n = put_s(s, n, "Move +"); n = put_n(s, n, it->move); n = put_s(s, n, "   Jump +"); n = put_n(s, n, it->jump); }
        else if (it->wp) { n = put_s(s, n, "Weapon power "); n = put_n(s, n, it->wp); }
    } else {
        const Ability *ab;
        if (k == 3) { ui_text(6, 38, id == NO_ABIL ? "No ability" : id == ABIL_DEFEND ? "Defend" : abilities[id].name, C_BLACK, 159); return; }
        if (id == NO_ABIL) return;
        if (id == ABIL_DEFEND) { if (k == 4) ui_text(6, 47, "Raises evasion until next turn", C_BLACK, 159); return; }
        ab = &abilities[id];
        if (k == 4) {
            n = put_s(s, n, "Range "); n = put_n(s, n, ab->range); n = put_s(s, n, "   Height ");
            if (ab->vert == 0xFF) n = put_s(s, n, "Any");
            else { n = put_n(s, n, ab->vert >> 1); if (ab->vert & 1) n = put_s(s, n, ".5"); s[n++] = 'h'; }
        }
        else if (k == 5) { n = put_s(s, n, "JP to learn "); n = put_n(s, n, AB_JP(ab)); }
        else if (k == 6) {
            #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define effects CONTOUR_REF_effects
#else
static const char *const effects[] = { "Physical damage", "Removes status ailments", "Give HP to an ally", "Restores HP", "Revives a KO ally", "Raises evasion" };
#endif
            n = put_s(s, n, effects[ab->kind]);
        } else if (k == 7 && ab->flags & AF_ITEM) { n = put_s(s, n, "In stock "); n = st.unit[st.sel].team == TEAM_PLAYER ? put_n(s, n, st.bag[ab->bag]) : put_s(s, n, "Unlimited"); }
        else if (k == 8) n = put_s(s, n, ab->flags & AF_EVADE ? "Can be evaded" : "Cannot be evaded");
    }
    s[n] = 0; ui_text(6, 38 + 8 * (k - 3), s, C_BLACK, 159);
}

// Full-screen Status with original FFT fields and selectable equipment / abilities.
// Building a line per frame keeps even entry and paging within the ordinary frame budget.
static void draw_status(void)
{
    void *sl = rt_light, *sd = rt_dark;
    const Unit *u = &st.unit[st.sel];
    u8 k, n = 0, i;
    char s[64];
    if (W->status_i != st.sel + 1 || W->status_page != st.page || W->status_info != st.status_info ||
        (st.status_info && (W->status_col != st.status_col || W->status_row != st.status_row))) {
        W->status_i = st.sel + 1; W->status_page = st.page; W->status_n = 0;
        W->status_info = st.status_info; W->status_col = st.status_col; W->status_row = st.status_row;
    }
    rt_light = W->status[0]; rt_dark = W->status[1];
    k = W->status_n;
    if (k < 9) {
        W->status_n++;
        if (!k) {
            memset(W->status, 0, sizeof W->status);
            draw_rect(0, 0, RT_W, 32, C_LGRAY); draw_rect(0, 32, RT_W, 1, C_DGRAY);
            portrait(u->gfx, 2, 1);
            ui_text(24, 2, unit_name(st.sel), C_BLACK, 92);
            ui_text(24, 9, u->gfx >= UG_CHEMIST_M ? "Chemist" : "Squire", C_BLACK, 92);
            n = put_s(s, 0, "Lv "); n = put_n(s, n, u->level); n = put_s(s, n, " Exp "); n = put_n(s, n, u->exp); s[n] = 0;
            ui_text(24, 17, s, C_BLACK, 92);
            ui_text(24, 25, st.page ? "EQUIP / SKILLS" : "STATUS", C_DGRAY, 92);
            ui_text(4, 95, st.status_info ? "[ESC] back" : st.page ? "[2nd] details" : "< > page", C_DGRAY, 81);
            if (!st.status_info) ui_text(94, 95, "[ESC] return", C_DGRAY, 159);
        } else if (k <= 2) {
            u16 v = k == 1 ? u->hp : u->mp, m = k == 1 ? u->max_hp : u->max_mp;
            n = put_s(s, 0, k == 1 ? "HP " : "MP "); n = put_n(s, n, v); s[n++] = '/'; n = put_n(s, n, m); s[n] = 0;
            ui_text(96, k == 1 ? 3 : 17, s, C_BLACK, 159);
            gauge(96, k == 1 ? 10 : 24, v, m, k == 1 ? C_BLACK : C_DGRAY, 60);
        } else if (st.status_info) status_detail(k);
        else if (!st.page) {
            if (k == 3) { n = put_s(s, 0, "Brave "); n = put_n(s, n, u->brave); n = put_s(s, n, "   Faith "); n = put_n(s, n, u->faith); }
            else if (k == 4) { n = put_s(s, 0, "Zodiac "); n = put_s(s, n, signs[u->zodiac]); }
            else if (k == 5) { n = put_s(s, 0, "Move "); n = put_n(s, n, u->move); n = put_s(s, n, "   Jump "); n = put_n(s, n, u->jump); n = put_s(s, n, "   Speed "); n = put_n(s, n, u->sp); }
            else if (k == 6) { n = put_s(s, 0, "PA "); n = put_n(s, n, u->pa); n = put_s(s, n, "   MA "); n = put_n(s, n, u->ma); n = put_s(s, n, "   CT "); n = put_n(s, n, u->ct); }
            else if (k == 7) { n = put_s(s, 0, "JP "); n = put_n(s, n, u->jp); n = put_s(s, n, "   Job Lv "); n = put_n(s, n, job_level(u)); }
            else { n = put_s(s, 0, "Status "); if (!u->hp) n = put_s(s, n, "KO"); else if (!u->status) n = put_s(s, n, "Normal"); else for (i = 0; i < 4; i++) if (u->status & (1 << i)) { n = put_s(s, n, st_names[i]); s[n++] = ' '; } }
            s[n] = 0; ui_text(6, 38 + 8 * (k - 3), s, C_BLACK, 159);
        } else {
            if (k == 3) {
                ui_text(23, 38, "EQUIPMENT", C_DGRAY, 81);
                ui_text(103, 38, "ABILITIES", C_DGRAY, 159);
            } else {
                u8 slot = k - 4, ab = known(st.sel, slot);
                ui_text(23, 44 + 8 * slot, u->eq[slot] == 0xFF ? "-" : items[u->eq[slot]].name, C_BLACK, 81);
                ui_text(103, 44 + 8 * slot, ab == NO_ABIL ? u->defend && slot == status_ability_count() - 1 ? "Defend" : "-" : abilities[ab].name, C_BLACK, 159);
                if (!slot && u->defend && status_ability_count() == 6) ui_text(103, 84, "Defend", C_BLACK, 159);
            }
        }
    }
    rt_light = sl; rt_dark = sd;
    copy_box(W->status[0], W->status[1], 0, 0, 10, RT_H);
    if (st.page && !st.status_info && W->status_n == 9)
        draw_hand(st.status_col ? 81 : 1, 41 + 8 * st.status_row);
}

#endif
#endif
#if MOD_CORE

static void draw_active(const Feet *f)
{
    #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define bob CONTOUR_REF_bob
#else
static const u8 bob[8] = { 0, 1, 2, 3, 3, 2, 1, 0 };
#endif   // up and down as the glove sways
    RtSprite sp = { 16, ACTIVE_H, active_gfx[0], active_gfx[1], active_gfx[2] };
    draw_sprite(f->x - st.camx - 8, f->y - st.camy - UNIT_FOOT - ACTIVE_H + 1 - bob[(st.tick >> 2) & 7], &sp);
}

// A number rising NUM_RISE pixels above a unit (k: 0 the target's, 1 the EXP, 2 the JP): AMS
// text written once into the strip's spare rows, then a 32-pixel sprite with a white outline
static void make_number(u8 k, const char *s)
{
    u32 g[7], o = 0;
    u8 r;
    for (r = 8; r < HUD_ROWS; r++) memset(HUD_L + r * RT_PBYTES, 0, 4);
    text_into(W->hud[0], W->hud[1], 1, 8, s, C_BLACK);
    for (r = 0; r < 7; r++) {
        const u8 *b = HUD_L + (7 + r) * RT_PBYTES;
        g[r] = r && r < 6 ? (u32)b[0] << 24 | (u32)b[1] << 16 | (u32)b[2] << 8 | b[3] : 0;
        o |= g[r];
    }
    for (W->num_w[k] = 32; W->num_w[k] && !(o & (1UL << (32 - W->num_w[k]))); W->num_w[k]--) ;
    for (r = 0; r < 7; r++) {
        u32 m = g[r] | (r ? g[r - 1] : 0) | (r < 6 ? g[r + 1] : 0);
        W->num[k][0][r] = g[r];
        W->num[k][1][r] = ~(m | m << 1 | m >> 1);
    }
}

// the target's from ACT_HIT (HP lost or healed, the status removed, Miss), then the EXP its
// user earned from ACT_HIT + GAIN_T ("LvUp" for a level up) and its JP from ACT_HIT + JP_T
// above the user
static void draw_number(void)
{
    u8 t = (u8)(st.act_t - ACT_HIT), k = 0;
    Feet f;
    RtSprite sp;
    char s[8];
    if (st.gain && t >= JP_T) { k = 2; t -= JP_T; }
    else if (st.gain && t >= GAIN_T) { k = 1; t -= GAIN_T; }
    if (t >= (k ? GAIN_N : NUM_T)) return;
    f = unit_feet(st.rot, k ? st.atk : st.tgt);
    if (!(W->num_ok & (1 << k))) {                 // one a frame (AMS text)
        u8 kind = act_kind(), n = 0;
        if (k == 2) { n = put_s(s, 0, "JP+"); n = put_n(s, n, st.jpg); }
        else if (k) {
            if (st.lvup) n = put_s(s, 0, "LvUp");
            else { n = put_s(s, 0, "EXP+"); n = put_n(s, n, st.gain); }
        } else if (!st.hit) n = put_s(s, 0, "Miss");
#ifdef FFT_OPENING
        else if (kind == AB_RAISE) n=put_s(s,0,"PA+1");
        else if (kind == AB_BREAK) n=put_s(s,0,abilities[st.abil].y==2?"SP-2":"MA-3");
#endif
        else if (kind == AB_DEFEND) n = put_s(s, 0, st_names[0]);
        else if (kind == AB_CURE) {                // the first status removed
            while (!(st.dmg & (1 << n))) n++;
            n = put_s(s, 0, st_names[n]);
        } else {
            if (kind != AB_DAMAGE) s[n++] = '+';
            n = put_n(s, n, st.dmg);
        }
        s[n] = 0;
        make_number(k, s);
        W->num_ok |= (u8)(1 << k);
    }
    if (k) t <<= 1;                         // reward rise follows the doubled display pace
    sp.w = 32; sp.h = 7; sp.light = sp.dark = W->num[k][0]; sp.mask = W->num[k][1];
    draw_sprite(f.x - st.camx - ((W->num_w[k] + 2) >> 1), f.y - st.camy - UNIT_FOOT - 6 - (t < NUM_RISE ? t : NUM_RISE), &sp);
}

// the facing arrows around the active unit at Wait, the one it faces black
static void draw_arrows(void)
{
    static const s8 ax[4] = { 13, 13, -13, -13 }, ay[4] = { -6, 7, 7, -6 };
    static const s8 fdx[4] = { 1, -1, 0, 0 }, fdz[4] = { 0, 0, 1, -1 };
    const Unit *un = &st.unit[st.act];
    Feet f = unit_feet(st.rot, st.act);
    s16 u0, v0, du, dv;
    u8 g, r, sel, lm[8], mk[8];
    RtSprite sp;
    view_of(st.rot, 1, 1, &u0, &v0);
    view_of(st.rot, 1 + fdx[un->face], 1 + fdz[un->face], &du, &dv);
    du -= u0; dv -= v0;
    sel = dv < 0 ? 0 : du > 0 ? 1 : dv > 0 ? 2 : 3;
    sp.w = 8; sp.h = 8; sp.light = sp.dark = lm; sp.mask = mk;
    for (g = 0; g < 4; g++) {
        for (r = 0; r < 8; r++) {
            u8 fi = W->arrow[g][0][r], o = W->arrow[g][1][r];
            lm[r] = g == sel ? fi : (u8)(o & ~fi);
            mk[r] = (u8)~o;
        }
        draw_sprite(f.x - st.camx + ax[g] - 4, f.y - st.camy + ay[g] - 4, &sp);
    }
}

// Lato Black's exact former vector fill is sampled offline into four small text strips
// in ffti. Reveal those packed rows at runtime: no glyph rasterization can overrun the
// banner timer, and the same four greys/white outline remain. No full-screen images.
#define BN_Y (st.banner == BN_START ? 2 : 34)
#define BN_SPEED 12                     // twice the previous six-pixel reveal speed
#define BN_HOLD 40                      // the complete message stays visible before the shade leaves
#define BN_K 20                         // packed masked strips (measured with ti-cycles)
static const u8 bn_width[4] = BN_WIDTHS;

static u8 bn_line(u8 kind, u8 line) { return kind == BN_START ? line : (u8)(kind); }

static void __attribute__((noinline)) bn_start(u8 kind)
{
    u8 l, t = DK_N;
    st.banner = kind; st.banner_t = 0;
    if (!W) return;
    for (l = 0; l < (kind == BN_START ? 2 : 1); l++) {
        W->bn_t0[l] = t;
        t += (u8)((bn_width[bn_line(kind, l)] + BN_SPEED - 1) / BN_SPEED);
    }
    W->bn_end = (u8)(t + BN_HOLD + DK_N);
}

// the scenery shaded a grey darker (white light, light dark, dark black) behind a diagonal edge
// x + y = c coming from the top left: before it (in), or past it (out: it leaves the same way)
static void bn_shade1(u8 *l, u8 *d, u32 k)    // a long of both planes (k: its pixels)
{
    u32 L = RD32(l, 0), D = RD32(d, 0);
    WR32(l, 0, (L & ~k) | ((~L | D) & k));
    WR32(d, 0, D | (L & k));
}
static void bn_shade(void)
{
    u8 *l = rt_light, *d = rt_dark, out = st.banner_t >= W->bn_end - DK_N;
    s16 c = (s16)(out ? st.banner_t - (W->bn_end - DK_N) : st.banner_t + 1) * DK_SPEED, y;
    for (y = 0; y < RT_H; y++, l += RT_PBYTES, d += RT_PBYTES) {
        s16 e = c - y, a, b;
        u8 *pl, *pd, *pe;
        if (e < 0) e = 0;
        if (e > RT_W) e = RT_W;
        a = out ? e : 0; b = out ? RT_W : e;
        if (a >= b) continue;
        pl = l + (a >> 5) * 4; pd = d + (a >> 5) * 4; pe = l + (b >> 5) * 4;
        if (a & 31) {                          // the edge's long, then the whole ones
            u32 k = 0xFFFFFFFFUL >> (a & 31);
            if (pl == pe) k &= ~(0xFFFFFFFFUL >> (b & 31));
            bn_shade1(pl, pd, k);
            if (pl == pe) continue;
            pl += 4; pd += 4;
        }
        for (; pl < pe; pl += 4, pd += 4) {
#ifdef __m68k__
            u32 L = *(PlaneWord *)pl, D = *(PlaneWord *)pd;
            *(PlaneWord *)pl = ~L | D; *(PlaneWord *)pd = D | L;
#else
            bn_shade1(pl, pd, 0xFFFFFFFFUL);
#endif
        }
        if (b & 31) bn_shade1(pl, pd, ~(0xFFFFFFFFUL >> (b & 31)));
    }
}

// Reveal word-aligned strips directly from the archived bank. Outlined text is all
// present before its first frame; the complete word always gets BN_HOLD frames.
static void bn_draw(void)
{
    const u8 *bank = UI_SHARED(BANNERS);
    u8 line;
    for (line = 0; line < (st.banner == BN_START ? 2 : 1); line++) {
        u8 k = bn_line(st.banner, line), r;
        const u8 *g = bank + RD16(bank, k), *l = g + 4;
        u16 size = (u16)(g[2] * g[3]);
        const u8 *d = l + size, *m = d + size;
        s16 c = (s16)(st.banner_t - W->bn_t0[line]) * BN_SPEED + ((RT_W - bn_width[k]) >> 1);
        u8 full;
        u16 tail = 0;
        if (st.banner_t <= W->bn_t0[line] || st.banner_t >= W->bn_end - DK_N) continue;
        if (c > RT_W) c = RT_W;
        c -= g[0];
        if (c <= 0) continue;
        full = (u8)(c >> 4);
        if (full >= g[3] / 2) full = g[3] / 2;
        else if (c & 15) tail = (u16)(0xFFFFU << (16 - (c & 15)));
        for (r = 0; r < g[2]; r++, l += g[3], d += g[3], m += g[3]) {
            u8 *sl = (u8 *)rt_light + (BN_Y + g[1] + r) * RT_PBYTES + (g[0] >> 3);
            u8 *sd = (u8 *)rt_dark + (BN_Y + g[1] + r) * RT_PBYTES + (g[0] >> 3);
            s16 w;
            for (w = 0; w < full; w++) {
                u16 km = RD16(m, w);
                if (!km) continue;
                RMW16(sl + 2 * w, km, RD16(l, w) & km);
                RMW16(sd + 2 * w, km, RD16(d, w) & km);
            }
            if (tail) {
                u16 km = RD16(m, w) & tail;
                RMW16(sl + 2 * w, km, RD16(l, w) & km);
                RMW16(sd + 2 * w, km, RD16(d, w) & km);
            }
        }
    }
}

#endif
#if MOD_PART(MOD_SCREENS)
// the results (M_END): the outcome, then each of the player's units with its level (before >
// after), the EXP and JP it earned, and the keys; drawn into the scene buffer one line a frame
// (AMS text), copied to the screen
static void draw_end(void)
{
    u8 *l = scene_l, *d = scene_l + RT_PSIZE, k = W->end_n, i, j = 0, n;
    char s[16];
    if (k < 9) {
        W->end_n++;
        if (!k) { memset(l, 0, RT_PSIZE); memset(d, 0, RT_PSIZE); }
        else if (k == 1) {
            void *sl = rt_light, *sd = rt_dark;
            rt_light = l; rt_dark = d;
            draw_text(8, 4, st.end == END_VICTORY ? "Victory!" : "Game Over", F_MEDIUM, C_BLACK);
            rt_light = sl; rt_dark = sd;
        } else if (k < 7) {                        // the (k - 2)-th unit of the player's side
            const Unit *u = st.unit;
            for (i = 0; i < NUNIT; i++, u++) if (u->team == TEAM_PLAYER && j++ == k - 2) break;
            if (i < NUNIT) {
                s16 y = 10 + 10 * k;
                text_into((u16 *)l, (u16 *)d, 4, y, unit_name(i), C_BLACK);
                n = put_s(s, 0, "Lv "); n = put_n(s, n, u->lv0);
                if (u->level != u->lv0) { s[n++] = '>'; n = put_n(s, n, u->level); }
                s[n] = 0; text_into((u16 *)l, (u16 *)d, 52, y, s, C_BLACK);
                n = put_s(s, 0, "EXP "); n = put_n(s, n, u->gexp); s[n] = 0;
                text_into((u16 *)l, (u16 *)d, 82, y, s, C_BLACK);
                n = put_s(s, 0, "JP "); n = put_n(s, n, u->gjp); s[n] = 0;
                text_into((u16 *)l, (u16 *)d, 120, y, s, C_BLACK);
            }
        } else if (k == 7) {                       // the battle: Ramza's turns, the player's
            u8 ko = 0, left = 0;                   // side's KOs, crystals and chests left
            const Unit *u = st.unit;
            for (i = 0; i < NUNIT; i++, u++) {
                if (u->team == TEAM_PLAYER) ko += u->kos;
                if (u->dc == DC_CRYSTAL || u->dc == DC_CHEST) left++;
            }
            n = put_s(s, 0, "Rounds "); n = put_n(s, n, st.rounds); s[n] = 0;
            text_into((u16 *)l, (u16 *)d, 4, 78, s, C_BLACK);
            n = put_s(s, 0, "Fallen "); n = put_n(s, n, ko); s[n] = 0;
            text_into((u16 *)l, (u16 *)d, 52, 78, s, C_BLACK);
            n = put_s(s, 0, "Loot left "); n = put_n(s, n, left); s[n] = 0;
            text_into((u16 *)l, (u16 *)d, 92, 78, s, C_BLACK);
        } else text_into((u16 *)l, (u16 *)d, 8, 90, help_lines[1], C_BLACK);
    }
    memcpy(rt_light, l, RT_PSIZE); memcpy(rt_dark, d, RT_PSIZE);
}
#if !MOD_CORE
u16 mod_entry(u8 op, u8 a) { (void)a; if (op) draw_status(); else draw_end(); return 0; }
#endif
#endif
#if MOD_CORE

void game_render(void)
{
#ifndef FFT_COMPACT_UI
    u8 ord[NUNIT], vis[NUNIT], i, j, cached, tg;
    s16 n;
    Feet f[NUNIT];
    s16 u, v;
#ifdef FFT_CONFIG
    pidle_interrupt();
#endif
#if defined(FFT_CONFIG) || defined(FFT_REACH_CONTOUR)
    if (st.config) {
        config_draw();
#ifdef FFT_CONFIG
        pidle_tick(PIDLE_BUDGET);
#endif
        FRAME_END(); return;
    }
#endif
    if (W &&
#ifdef FFT_CINEMA
        !st.cphase &&
#endif
        (!st.morph_mask && (W->hold || (!st.banner && (st.ai == AI_THINK || st.ai == AI_GO) && W->frz)))) {   // the same picture in
        W->hold = 0;                                                           // both buffers: the
        FRAME_END(); return;                                                                // AI thinks
    }
    if (!W || !ugfx || !wgfx || !igfx || !views[0] || !views[1] || !views[2] || !views[3]) {
        draw_rect(0, 0, RT_W, RT_H, C_WHITE);
        draw_text(4, 40, "Missing data or memory", F_SMALL, C_BLACK);   // (banks, or the heap)
        FRAME_END(); return;
    }
#ifdef FFT_EXPLORATION
#ifdef FFT_CINEMA
    if (st.cphase && st.perspective) { cinema_render(); FRAME_END(); return; }
#endif
    if (st.perspective) { perspective_render(); FRAME_END(); return; }
#endif
#ifdef FFT_OPENING
    if (st.open_stage || st.open_fade) { map_render(); FRAME_END(); return; }
#endif
    if (st.mode == M_END && !st.banner && !st.morph_mask) { draw_end(); FRAME_END(); return; }
    if (st.mode == M_STATUS) { draw_status(); FRAME_END(); return; }
    if (st.turn) {                                 // the view it turns to, unpacked meanwhile
#if defined(FFT_MEASURE) && defined(RT_CYCLES)
        frame_stall = 1;
#endif
#ifdef FFT_CONFIG
        ZB(7); render_turn(); ZE(7);
#elif defined(FFT_TEXTURE_PREVIEW)
        ZB(7); if (pbank && preview_ready) preview_turn(); else copy_view(); ZE(7);
#elif defined(FFT_FIXED) && !defined(FFT_TURN_HOLD)
        ZB(7); render_turn(); ZE(7);     // native rotation, independent of perspective cuts
#elif defined(FFT_EXPLORATION)
        copy_view();                            // exploration fits one TI variable: retain the
                                                // old view while the next one unpacks
#else
        ZB(7); draw_turn(); ZE(7);
#endif
        ZB(8); unpack_view((st.rot + st.turn) & 3); ZE(8);
        FRAME_END(); return;
    }
    for (i = 0, n = 0; i < NUNIT; i++) {           // units back to front, those on screen and
        s16 bx, by;                                // the frame's cost without the optional work
        u16 fk = (u16)(st.unit[i].x | st.unit[i].z << 8 | st.rot << 12 | 0x8000);
        if (st.unit[i].x >= MAP_W || st.unit[i].z >= MAP_H) {
            f[i].x = f[i].y = 0; f[i].depth = 0;   // removed unit / test injection: no map access
        } else if (st.mode == M_WALK && i == st.sel) f[i] = unit_feet(st.rot, i), W->feet_k[i] = 0;
        else if (W->feet_k[i] == fk) f[i] = W->feet[i];   // kept while it stands
        else { f[i] = W->feet[i] = unit_feet(st.rot, i); W->feet_k[i] = fk; }
        if (st.mode == M_ACT && i == st.tgt && hit_flash(i)) {   // visual recoil only
            static const s8 dx[4] = { -1, -1, 1, 1 }, dy[4] = { 1, -1, 1, -1 };
            u8 dir = unit_direction(st.rot, st.atk), t = (u8)(st.act_t - ACT_HIT);
            u8 step = t < 6 ? 2 : t < 10 ? 1 : 0;
            f[i].x += dx[dir] * step; f[i].y += dy[dir] * (step != 0);
        }
        for (j = i; j > 0 && f[ord[j - 1]].depth > f[i].depth; j--) ord[j] = ord[j - 1];
        ord[j] = i;
        bx = f[i].x - st.camx - 8; by = f[i].y - st.camy - UNIT_FOOT;
        vis[i] = st.unit[i].x < MAP_W && bx > -16 && bx < RT_W && by > -UNIT_SH && by - FOE_H < RT_H;
        if (vis[i]) {
            const Cover *c = &W->cover[i];
            n += UNIT_K;
            if (unit_flash(i)) n += 7;
            if (st.mode == M_ACT) {
                if (i == st.atk && armed_pose(i) >= 2) n += 45;
            }
            if (c->x != f[i].x || c->y != f[i].y || c->depth != f[i].depth || c->rot != st.rot) n += COVER_K;
        }
    }
    if (AIMING(st.mode) && st.tick & 8 && (tg = unit_at(st.cx, st.cz)) && vis[tg - 1]) n += UNIT_K;
    else tg = 0;
    if (st.ai == AI_THINK && !st.morph_mask) n += SUM_K;
    if (st.morph_mask) n += 20;
    if (st.banner) n += BN_K + DK_K - HUD_K;   // the banner and its shade instead of the HUD
    n += W->upd_k;
    cached = bg_usable();
    W->left = (s16)(FRAME_K - BASE_K - n - (cached ? BG_K : VIEW_K));
    ZB(6);
    ensure_scene(st.rot);
    ZE(6);
    if (cached && !bg_usable()) W->left -= VIEW_K - BG_K;
    ZB(3); screen_scene();
    if (W->bg_drawn) draw_glints();            // (not while the camera pans: the view copied, ~105k)
    if (st.banner && !st.morph_mask) bn_shade();                 // (the scenery only: the units stay bright)
    ZE(3);
    ZB(4);
#ifdef FFT_ACTOR_BANK
    actor_animate(vis);
#endif
    for (i = 0; i < NUNIT; i++) {
        u8 k = ord[i];
        Cover *c = &W->cover[k];
        if (!vis[k]) continue;                     // off screen
        if (c->x != f[k].x || c->y != f[k].y || c->depth != f[k].depth || c->rot != st.rot) {
            c->x = f[k].x; c->y = f[k].y; c->depth = f[k].depth; c->rot = st.rot;
            cover(c->cm, f[k].x, f[k].y, f[k].depth);
            c->wet = unit_in_water(k);
            for (c->r0 = 2; c->r0 <= UNIT_SH - 3 && !c->cm[c->r0]; c->r0++) ;
            for (c->r1 = UNIT_SH - 3; c->r1 >= c->r0 && !c->cm[c->r1]; c->r1--) ;
        }
        if (st.mode == M_ACT && k == st.atk) draw_weapon(f[k].x, f[k].y, k, f[k].depth);
        draw_unit_covered(f[k].x - st.camx, f[k].y - st.camy, st.rot, k, c);
    }
    if (tg) {                                  // the aimed unit over the others, every other 8
        tg--;                                  // frames: it blinks when one stands in front
        draw_unit_covered(f[tg].x - st.camx, f[tg].y - st.camy, st.rot, tg, RT_NULL);
    }
#ifdef FFT_ACTOR_BANK
    W->left-=(s16)W->actor_misses*32;
#endif
    ZE(4); ZB(5);
#ifdef FFT_OPENING
    opening_effect();
#endif
    // the cursor, above everything (blinking), dashed behind the scenery
    dst_screen(st.camx, st.camy);
    view_of(st.rot, st.cx, st.cz, &u, &v);
    if (st.mode == M_ACT && st.act_t >= ACT_HIT) draw_number();
    if (st.mode == M_FACE) draw_arrows();
    // During AI thinking the active-unit pointer identifies the actor. The
    // tile cursor returns when the plan is shown; a new turn already spends
    // its frame on the clock, camera and unit covers.
    else if (!st.banner && !st.morph_mask && st.ai != AI_THINK && st.mode != M_ACT && (st.tick & 8 || RINGS(st.mode)) && !(st.deny & 4)
#ifdef FFT_SOURCE_CAMERA
        && map_tiles[st.cz*MAP_W+st.cx].walk
#endif
    ) {
        Marker mk;
        u32 fl[24];                            // alive through blit24, including return from inspection
        u8 c = (u8)(st.cz * MAP_W + st.cx), ring = RINGS(st.mode);
        marker(st.rot, c, !ring, &mk);
        if (ring && !AIMING(st.mode)) {         // over the rings: the tile as their inverse
                                                // (the user's pick over a checkerboard): black,
            for (n = 0; n < mk.rows; n++)       // a white edge; when aiming the plain ring
                fl[n] = mk.m[n] & ~mk.l[n];     // (white, a black edge) over the black ones
            mk.l = mk.d = fl;
        }
        blit24(mk.x, mk.y, mk.rows, mk.l, mk.d, cursor_mask(&mk, (u16)(c | ring << 12)));
    }
    if (vis[st.act] && st.mode != M_WALK && st.mode != M_ACT && !st.banner && !st.morph_mask) draw_active(&f[st.act]);
    if (st.banner && !st.morph_mask) bn_draw(); else if (!st.morph_mask) draw_hud();
    ZE(5);
    // Prepare at most one swing phase after ordinary UI work during each wind-up
    // frame. Fast-forward may expose any of the three phases first.
    if (st.mode == M_ACT && st.act_t < ACT_BLOW && armed_pose(st.atk) == 1 && W->left >= 0) {
        u8 slot;
        for (slot = 0; slot < 3; slot++) {
            u8 pose = weapon_pose(st.atk, slot + 2);
            if (W->blade_k[slot] != wgfx[WEAPON_INDEX + pose] + 1) {
                weapon_frame(pose, slot); W->left -= 25; break;
            }
        }
    }
    if (st.ai == AI_THINK && !st.banner && !st.morph_mask) {      // two pictures in a row the same: stop drawing
        s16 tx, ty;
        cam_target(&tx, &ty);
        if (tx != st.camx || ty != st.camy) W->frz_h = 0;   // still panning: not checked
        else {
            u32 h = planes_sum();
            if (h == W->frz_h) W->frz = 1;
            W->frz_h = h;
        }
    }
#ifdef FFT_CONFIG
    if(W->left>=60) pidle_tick(48);
#endif
    FRAME_END();
#endif
}

#ifdef FFT_OPENING
#include "opening/opening.inc"
#endif
#endif
#if !MOD_CORE && MOD_PART(MOD_BEATS) && defined(FFT_OPENING)
#include "opening/opening.inc"                  // its beats only (MOD_PART there)
#endif
