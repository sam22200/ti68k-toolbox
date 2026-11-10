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
#include "tactical_grid.h"
#include "action_effect.h"
#include "surface_geometry.h"
#include "knockback_geometry.h"
#include "step_geometry.h"
#include "unit_card.h"
#include "choice_menu.h"
#include "weapon_panel.h"
#include "movement_rules.h"
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
#ifdef FFT_CHAPTER
#include "identity_pool.h"
#endif

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
#ifndef FFT_CHECK_NO_ZONES               // a size-bound check keeps only its own zones
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif
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
#ifdef FFT_CHAPTER
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

#ifdef FFT_CHAPTER
#define AB_JP(b) RD16((const u8 *)(b)+8,0)
#else
#define AB_JP(b) ((b)->jp)
#endif

#if MOD_CORE                            // a module reaches them through its context (module.h)
#ifdef FFT_CHAPTER
u8 map_w, map_h, encounter_units;
u16 map_n; // validated chapter sizes and actor count
s16 sc_h, sc_top;
#endif
#ifdef FFT_CHAPTER
State *battle_state;                    // State lives in RAM, not the near-limit packed image
#else
State st;
#endif
u8 *scene_l, *scene_d;
u8 scene_rot;
#endif
// the four views (fftv0..3, ZX0, read in place), unpacked one at a time into vbuf: light and
// dark planes, then twice (the frontmost pixels of each byte, then the next ones) their view
// diagonal and their mask (SC_PLANE each)
// DEPTH_ROW(y, t, x0, w): the depth row y: for x0 <= b < x0 + w, t[b] its depth, then the same
// byte of the mask, second depth and second mask DEPTH_STRIDE bytes apart; masks are 0 elsewhere.
#ifdef FFT_VIEW_SPLIT                   // the colours live in the scene buffer only
// Chapters: ragged depth (extract.ragged_depth): a DepthRow per scene row, then each plane as
// its rows' spans (the bytes holding mask bits, about half a row): ~18 KB, not 32 KB
typedef struct { u16 off; u8 x0, w; } DepthRow;   // off: the row's span start - x0
#define VIEW_BYTES FFT_CORE_DEPTH_MAX
#define DEPTH_ROW(y, t_, x0_, w_) do { const DepthRow *r_ = (const DepthRow *)vbuf + (y); \
    t_ = vbuf + r_->off; x0_ = r_->x0; w_ = r_->w; } while (0)
#define DEPTH_STRIDE vplane
#else
#define VDEPTH (vbuf + 2 * SC_PLANE)
#define VIEW_BYTES (6UL * SC_PLANE)          // 52,320 (Gariland 48,480): above a 16-bit int
#define DEPTH_ROW(y, t, x0, w) do { t = VDEPTH + (u16)((y) * SC_BYTES); x0 = 0; w = SC_BYTES; } while (0)
#define DEPTH_STRIDE SC_PLANE
#endif
#if MOD_CORE
static const u8 *views[4];
static const u8 *ugfx;                  // the units' frames (fftu, units.h), read in place
static const u8 *igfx;                  // portraits, names, marker and text (ffti)
#endif
#ifdef FFT_ACTOR_BANK
#define ACTOR_SLOTS (UNIT_CAPACITY + 2)  // bounded storage: units, an object, a blade
#define ACTOR_UNITS NUNIT
#define ACTOR_KEY(logical) RD16(ugfx, logical)   // ffoa/ffga: logical frame -> bank frame
#include "actors.h"
#endif
#ifdef FFT_CHAPTER
#if MOD_CORE
static const u8 *cdesc;                 // the chapter's descriptor (ffod, ffgd: chapter.h)
static const u8 *flow_bank,*flow_ui;      // optional source-backed sequencing/results; reused ffmg icons
static const u8 *opening_story,*pgfx;
static void map_start(u8 after);
const Job *jobs; const Item *items; const Ability *abilities;   // shared archived tables
#ifdef FFT_CHAPTER
const u8 *common, *charge_ticks;
const char *job_names; const u8 *job_sets;                       // ffcj, every chapter's
#endif
const UnitDef *unit_defs;
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
#ifdef FFT_VIEW_SPLIT
static u16 vplane;                      // bytes of one plane of the unpacked ragged depth
#endif
#endif

#if defined(FFT_CHAPTER) && !defined(FFT_COMPACT_UI)
#define FFT_PACKED_PANELS
#endif
#if defined(FFT_CHAPTER) && defined(FFT_VIEW_SPLIT)
#define FFT_SHARED_ROTATION_WORK
#endif
#define BK_N 128                        // FFT_VIEW_SPLIT: rows kept (a reach touches up to 126)
// Work memory, allocated with the scene (globals and statics are stored in the program file
// on the TI: -mno-bss, and the TI-89 refuses programs above 24 KB)
#define TW (RT_W / 2)                   // the rotation frames: half resolution
#define TH (RT_H / 2)
#define COVER_H UNIT_SH                // the shadow lies inside the sprite's box
#define GLINTS 250                      // glints per phase at most (chapters: FFT_WATER_BANK)
#define HUD_ROWS 14                     // scratch rows for outlined action numbers (8-13)
#define PANEL_ROWS 29                   // portrait sets the height; level overlays its lower corner
#define PANEL_Y (RT_H - PANEL_ROWS)
#define FC_ROWS 9                       // the prediction: a white box, black frame, small font
#define PANEL_GAUGE_W 40                // MP + gap + 999/999 in the existing small font
#define TARGET_STATS_X (61 - PANEL_GAUGE_W - 2) // rounded gauge ends against the target portrait
#define WIN_ROWS 47                     // bottom-right menu / preview: tab plus six lines
#define SKILL_ROWS ((WIN_ROWS - 15) / 6) // framed action lists fit five rows, then scroll
#define SKILL_TOP (st.skill < SKILL_ROWS ? 0 : st.skill - SKILL_ROWS + 1)
#ifdef FFT_CHAPTER
#include "mime_state.h"
#endif
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
#ifdef FFT_CHAPTER                      // the beats on the map (mapbeats.inc)
    char dialog[6][48];
    u8 dialog_n, dialog_w, dialog_h, dialog_next;
    u16 dialog_total, dialog_key, dialog_drawn;
    u8 dialog_white, dialog_part, dialog_face;
#endif
    Cover cover[UNIT_CAPACITY];         // per unit, for its feet, its depth and the view
    u32 wcover[WEAPON_SH];              // the blade's cover, kept while its anchor stands
    s16 wx, wy;
    u8 wrot, wdepth;
#ifndef FFT_COMPACT_UI
    u8 blade_k[3];                      // one unique frame + 1 per swing phase; zero invalid
    u32 blade[3][3][WEAPON_SH];         // three decoded blades, big-endian bytes on both hosts
#endif
    Feet feet[UNIT_CAPACITY];           // its feet (unit_feet) for its tile and the
    u16 feet_k[UNIT_CAPACITY];           // view (cell | rot << 8 | 0x8000; 0: none)
#ifndef FFT_SHARED_ROTATION_WORK
#ifdef FFT_TURN_LOWPOLY_NATIVE
    s16 span_l[RT_H + 8], span_r[RT_H + 8];
#else
    s16 span_l[TH + 8], span_r[TH + 8];
#endif
    u8 half[2][TW / 8 * TH];
#endif
    u8 from[MAP_NMAX];
#ifndef FFT_TABLES_ARCHIVED
    u16 dbl[256];                       // byte -> its bits doubled
#endif
    u8 order[MAP_NMAX];
#ifndef FFT_SHARED_ROTATION_WORK
    u16 count[256];
#endif
#ifdef FFT_WATER_BANK
    const u8 *puddle[4];                // archived even-x/2,y pairs; no decoded glint arrays
#else
    struct { s16 x, y; } glint[4][GLINTS];  // the water's glints of each phase (scene pixels)
#endif
    u8 glints[4];
#ifndef FFT_SHARED_ROTATION_WORK
    s16 key[MAP_NMAX], gx[MAP_WMAX + 1][MAP_HMAX + 1], gy[MAP_WMAX + 1][MAP_HMAX + 1];
#endif
#ifdef FFT_COMPACT_UI
    // No backing tactical panels in the bounded camera renderer. Its full-UI render
    // entry point is excluded below; legacy panel helpers are never called.
    u16 (*hud)[HUD_ROWS * RT_PBYTES / 2];
    u16 (*win)[WIN_ROWS * RT_PBYTES / 2];
#else
#ifdef FFT_PACKED_PANELS
    u16 hud[1][HUD_ROWS * RT_PBYTES / 2]; // black text scratch needs one plane
#else
    u16 hud[2][HUD_ROWS * RT_PBYTES / 2];   // outlined panel text (0-6) / action numbers (7-13)
#endif
    u16 win[2][WIN_ROWS * RT_PBYTES / 2];   // bottom-right menu / preview cache
#endif
    u16 win_k[3]; u8 win_w, win_h, win_n, win_m;   // mode, moved | acted << 1 (M_ACTS: no ability
                                        // known; M_ACT: the ability), unit;
                                        // its width in words, its rows; its part to draw next
                                        // its parts (the box, a line each; win_n == win_m: drawn)
#ifdef FFT_COMPACT_UI
    u16 (*forecast)[FC_ROWS * RT_PBYTES / 2];
#else
    u16 forecast[3][FC_ROWS * RT_PBYTES / 2]; // the prediction's box, 160 wide, above the arrow
#endif
    u16 pan_k[2][6], pan_fk[2][6];      // unit, HP, MP, level, maxima: being made, shown
    u16 fc_key;                         // target (four bits), then ability+1 (zero: Attack)
    u8 pan_n[2], pan_f[2], pan_ok[2];   // parts made (cutout portrait, name, HP, MP, level), the
                                        // buffer shown, one shown
    u8 tag_w[2], tag_ok;                // the tags' widths, made
    u8 hud_ok;                          // every panel and menu due this frame drawn (tests)
    u8 arrow[4][2][8];
#ifdef FFT_CHAPTER
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
    u16 ring_i;                          // next reach tile; MAP_N: all drawn
    u8 ring_bits[(MAP_NMAX + 7) / 8], ring_style; // last completed visual reach, not its costs
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
    CoreNavSearch nav;
    u8 blk[MAP_NMAX];
#ifdef FFT_CHAPTER
    u8 grid_row[MAP_HMAX];
    const u8 *surface;                 // archived source columns, independent of compact nodes
    const CoreGrid *grid;               // NULL: current dense lower-floor descriptor
    const u8 *node_x,*node_z;           // map-owned coordinate planes; source indices stay separate
#endif
#if !defined(FFT_TABLES_ARCHIVED) || defined(FFT_CHAPTER)
    u8 tx[MAP_NMAX], tz[MAP_NMAX];
#endif
    u8 hs[MAP_NMAX], hl[MAP_NMAX], hw[MAP_NMAX];
    #ifdef FFT_CHAPTER
    u8 *eg, (*via)[4], edge_shift;      // two bytes/link plus four cached first neighbors
#else
    u8 eg[MAP_NMAX][32], evia[MAP_NMAX][16];
#endif
    u8 en[MAP_NMAX], ej, surface_flags;
#ifdef FFT_CHAPTER
    u16 top[MAP_NMAX];             // map-56 towers exceed 255 source height units
#else
    u8 top[MAP_N];              // a tile's column for a projectile (FFT: its height x 12);
#endif
#if !defined(FFT_TABLES_ARCHIVED) || defined(FFT_CHAPTER)
    u8 col[MAP_WMAX * 28], row[MAP_HMAX * 28];    // world x, z -> the tile's column, z * MAP_W
#endif
    // the AI (fft.c § the AI): per tile the sets (1: can stop there, 2: to act from), the danger,
    // the steps from the focus, what stepping there gives; the tiles to act from
    // in map order and safest first; the ranking; the usable abilities, the unit's thinking step
#ifdef FFT_PACKED_PANELS
    // Status/equipment rendering and AI planning never own this storage together.
    // Keep the equipment draft/list and all movement/deferred state outside it.
    union {
        u16 status[2][RT_H * RT_PBYTES / 2];
        struct {
            u8 ai_m[MAP_NMAX],ai_prox[MAP_NMAX];
            u16 ai_turn_ticks[UNIT_CAPACITY];
            u16 ai_weapon_value[9][UNIT_CAPACITY];
            s16 ai_weapon_bonus[9][UNIT_CAPACITY];
            u8 ai_weapon_kind[UNIT_CAPACITY],ai_weapon_at;
            union {u16 ai_melee_mask[MAP_NMAX];u8 edge_ceiling_other[2][MAP_NMAX];};
            u16 ai_melee_at,ai_weapon_left;
            union {u8 ai_dist[MAP_NMAX];AreaAi area_ai;AreaUi area_ui;};
            AbId ab;u16 ab_k;u8 ab_n,ab_all,ai_traj;u16 ab_done;AiAct ab_best;
            union {s16 ai_gain[MAP_NMAX];AreaImpact area_impact;};
            union {s16 ai_key[MAP_NMAX];u8 edge_ceiling[2][MAP_NMAX];};
            u8 ai_ord[MAP_NMAX];AiAct ai_rank[8];
            AbId ai_list[1+SET_N];
            u8 ai_n,ai_nab,ai_k,ai_a,ai_self,ai_focus,ai_c0,ai_c1,ai_retreat;
            u8 ai_er,ai_ar,ai_ranged,ai_weapon;u16 ai_use;s16 ai_base;
            s16 ai_panel_bonus[9][UNIT_CAPACITY];
            u16 ai_panel_left;u8 ai_panel_at,ai_reflect_hands;
            u8 ai_reflect[MAP_NMAX][7];s16 ai_proc_gain[2][UNIT_CAPACITY];
            u8 ab_j,ab_self,ab_walk;
        };
    };
    u16 nature_units,nature_pending,rebuild;
    u8 move_kind,move_who,move_status,move_weather,move_code,move_job;
    u8 ai_tt[UNIT_CAPACITY],ai_tu[UNIT_CAPACITY];
    u8 ai_grid[MAP_NMAX];u16 ai_no; // equipment picker survives full-screen painting
#else
    u8 ai_m[MAP_NMAX], ai_prox[MAP_NMAX];
#ifdef FFT_CHAPTER
    u16 nature_units;                   // source monster actor mask, rebuilt at cold boundaries
    u16 ai_turn_ticks[UNIT_CAPACITY];        // next-turn clock estimates, fixed during one plan
    u16 ai_weapon_value[9][UNIT_CAPACITY]; // actor/target scalar cache; origin-independent
    s16 ai_weapon_bonus[9][UNIT_CAPACITY]; // drain and conditional utility, prepared with the scalars
    u8 ai_weapon_kind[UNIT_CAPACITY],ai_weapon_at;
    union {u16 ai_melee_mask[MAP_NMAX];u8 edge_ceiling_other[2][MAP_NMAX];};
    u16 ai_melee_at; // weapon obstruction at each origin; projectile cursor also carries a slot
    u16 ai_weapon_left;                 // ordinary plus learned Charge rows to prepare
    u16 nature_pending;                 // scalar post-impact request, drained after the owner returns
    u16 rebuild;                        // deferred derived stats; drained after overlay returns
    u8 move_kind,move_who,move_status,move_weather,move_code,move_job; // prepared graph capability/cache key
#endif
#ifdef FFT_CHAPTER
    // Focus search is finished before candidates; reclaim only its idle storage.
    union {u8 ai_dist[MAP_NMAX];AreaAi area_ai;AreaUi area_ui;};
#else
    u8 ai_dist[MAP_NMAX];
#endif
    u8 ai_tt[UNIT_CAPACITY], ai_tu[UNIT_CAPACITY]; // targets: tiles, units
    AbId ab; u16 ab_k; u8 ab_n, ab_all, ai_traj; // the ability being tried: its next origin, its targets,
    u16 ab_done;                        // no target used up, a trajectory just checked; the
    AiAct ab_best;                      // targets used up, its best so far
#ifdef FFT_CHAPTER
    union {s16 ai_gain[MAP_NMAX];AreaImpact area_impact;};
#else
    s16 ai_gain[MAP_NMAX];
#endif
    union {s16 ai_key[MAP_NMAX];u8 edge_ceiling[2][MAP_NMAX];}; // graph scratch is idle during planning
    u8 ai_grid[MAP_NMAX], ai_ord[MAP_NMAX]; u16 ai_no;
    AiAct ai_rank[8];
    AbId ai_list[1 + SET_N]; u8 ai_n, ai_nab, ai_k, ai_a, ai_self, ai_focus, ai_c0, ai_c1, ai_retreat;
    u8 ai_er, ai_ar, ai_ranged;         // its shortest reach on an enemy, an ally; a ranged ability
    u8 ai_weapon;                       // its weapon's reach (WEAPON_REACH: 1, a bow farther)
    u16 ai_use;                         // the units the ability is worth using on (bits)
    s16 ai_base;                        // the fallback tile's score: to beat
#endif
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
#ifdef FFT_CHAPTER
    CastBatch cast;                     // one owned packet through root impact/response drain
#endif
    // last: the big HUD caches (the fields above keep 16-bit offsets)
#ifdef FFT_COMPACT_UI
    u16 (*pan)[2][3][PANEL_ROWS * RT_PBYTES / 2];
    u16 (*tag)[3][7 * RT_PBYTES / 2];
#else
#ifdef FFT_PACKED_PANELS
    u16 pan[2][2][PANEL_ROWS * RT_PBYTES / 2]; // ten-byte L/D/mask slots,30-byte pitch
#elif defined(FFT_CHAPTER)
    union {
        u16 status[2][RT_H * RT_PBYTES / 2];
        u16 pan[2][2][3][PANEL_ROWS * RT_PBYTES / 2];
    };
#else
    u16 pan[2][2][3][PANEL_ROWS * RT_PBYTES / 2]; // per side, two 80-wide planes and opacity masks:
                                        // the one shown, the one being made (scene shows through)
#endif
#ifdef FFT_PACKED_PANELS
    u16 tag[2][7 * RT_PBYTES / 2];
#else
    u16 tag[2][3][7 * RT_PBYTES / 2];   // "Enemy" / "Guest" above a portrait, made once
#endif
#endif
#ifdef FFT_VIEW_SPLIT
    // The view's colours of the scene rows the reach rings were drawn on, kept before a ring
    // changes them (the colours are no longer kept whole: VIEW_BYTES)
    u8 bk_of[SC_HMAX];                     // a row's slot (0xFF: the row is still the view's)
    u8 bk_row[BK_N], bk_n, bk_over;     // each slot's row; slots used; rings past the backup
#ifdef FFT_SHARED_ROTATION_WORK
    // Rotation replaces the view before reach backups can be used again.
    union {
    u8 bk[BK_N][2][SC_BYTES] __attribute__((aligned(2)));
        struct {
#ifdef FFT_TURN_LOWPOLY_NATIVE
    s16 span_l[RT_H + 8], span_r[RT_H + 8];
#else
    s16 span_l[TH + 8], span_r[TH + 8];
#endif
    u8 half[2][TW / 8 * TH];
    u16 count[256];
    s16 key[MAP_NMAX], gx[MAP_WMAX + 1][MAP_HMAX + 1], gy[MAP_WMAX + 1][MAP_HMAX + 1];
        };
    };
#else
    u8 bk[BK_N][2][SC_BYTES] __attribute__((aligned(2)));
#endif
#endif
#ifdef FFT_CHAPTER
    MimeState mime;                      // cold imitation queue; no separate allocation
    WeaponPanel weapon_panel;           // frozen occupants of one selected weapon panel
    u8 preview_target,preview_count;     // pure recipient/card output; no unit-state cache
#ifndef FFT_PACKED_PANELS
    s16 ai_panel_bonus[9][UNIT_CAPACITY];
    u16 ai_panel_left;u8 ai_panel_at,ai_reflect_hands;
    u8 ai_reflect[MAP_NMAX][7];           // two receiver+1 nibbles per origin row
    s16 ai_proc_gain[2][UNIT_CAPACITY];   // expected spell utility after a reflected weapon hit
#endif
    u16 edge_at;u8 edge_phase,edge_jump; // resumable graph construction before AI search
#ifndef FFT_PACKED_PANELS
    u8 ab_j,ab_self,ab_walk;           // resumable scalar origin/target cursor
#endif
    MoveConfig move_config;            // survives an overlay replacement between graph slices
#else
    u8 ab_j,ab_self,ab_walk;
#endif
} Work;
#if defined(__m68k__) && defined(FFT_CHAPTER) && MOD_CORE
// malloc's hidden handle also belongs to the maximum AMS heap block.
typedef char SceneWorkFitsAmsBlock[(2UL*SC_BYTES*SC_HMAX+sizeof(Work)+2UL<=65520UL)?1:-1];
#endif
#ifdef FFT_CHAPTER
#define NAV_ENVELOPE (MAP_NMAX * 20U)              // nav_reserve(MAP_NMAX, 2): 16 + 4 bytes a node
// What a battle allocates beyond its program (game_init, bootstrap_files.inc): State and Save,
// the depth envelope, the scene and Work, the actor cache, the map envelope, AMS's headers
#define BATTLE_RAM_BYTES (BATTLE_STORAGE_BYTES + FFT_CORE_DEPTH_MAX + 2UL * SC_BYTES * SC_HMAX \
    + sizeof(Work) + AC_N * (UNIT_FB + 4UL) + NAV_ENVELOPE + 5 * 4)
#endif
#if MOD_CORE
static Work *W;
static u8 hl_seq;
#ifdef FFT_MODULES
static u8 mod_id;                       // invalidate whenever a tactical binding changes
static u16 mod_call(u8 id,u8 op,u8 a);
#endif
#endif
#if defined(FFT_CHAPTER) && MOD_CORE
#define NAV_BYTES(n) ((u16)(((u16)(n)<<6)+((u16)(n)<<2))) // two-floor rows plus neighbors
#define NAV_SINGLE_BYTES(n) ((u16)(((u16)(n)<<5)+((u16)(n)<<2)))
static u8 *navbuf;
static u16 nav_capacity;
static u8 nav_reserve(u16 count,u8 upper)
{
    // Bit1 selects the live builder's proven8/16-link maximum. Diagnostic
    // graph injection retains its wider16/32-link reserve with flags0/1.
    u8 shift=(upper&1)?6:5;if(upper&2)shift--;u16 bytes=(u16)((count<<shift)+(count<<2));
    if(bytes>nav_capacity) {
        u8 *next=malloc(bytes);
        if(!next)return 0;
        if(navbuf)free(navbuf);
        navbuf=next;nav_capacity=bytes;
    }
    W->eg=navbuf;W->via=(u8 (*)[4])(navbuf+(count<<shift));W->edge_shift=shift;
    return 1;
}
#ifdef FFT_MODULES
static void grid_tables(void)
{
    // Cold map/rebinding setup. Refresh context even if PARTY was last resident.
    mod_id=0;mod_call(MOD_PARTY,8,0);
}
#else
#include "grid_tables.inc"
#endif

#endif
#if defined(FFT_MODULES) || defined(FFT_MODULE)
#include "module.h"
#ifdef FFT_MODULES
static ModCtx *mod_context(u8 id);
#endif
#endif
#ifdef FFT_CHAPTER
#include "resource_file.h"
#include "campaign/flow.h"
#include "funds.inc"
#endif
#if defined(FFT_CHAPTER) && MOD_CORE
#ifdef FFT_MODULES
static void equipment_control(u8 op){mod_call(MOD_EQUIPMENT,op,st.act);}
static void movement_settle(u8 who){mod_call(MOD_EQUIPMENT,4,who);}
#else
static void equipment_control(u8 op);
static void movement_settle(u8 who);
#endif
#endif
static u8 bg_usable(void);

// A frame's budget, in thousands of cycles (ti-cycles): what a frame costs without its optional
// work is reckoned before drawing (the units on screen, their cover masks to make, the copy of
// the scene); the optional work (new rings, a part of the HUD, the screen's copy kept) waits for
// a frame with room for it: it shows a frame or two later, the frame stays within the budget
#define FRAME_K 332
#define BASE_K 65                       // glints, HUD copies, cursor, the units' sort
#ifdef FFT_CHAPTER
#define UNIT_K CD(UNIT_K)               // the chapter's (Orbonne's dry shader 16, Gariland 20)
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
#define BN_SPEED 12                     // white banner reveal, pixels per frame
#define BN_HOLD 40                      // hold the complete message before shading out
#define SAVE_K 13                       // a quarter of the screen's copy kept (halves, 26k,
                                        // went over the budget once forced)
#define SAVE_WAIT 2                     // frames the copy waits at most once the camera stops,
#define WAIT_MAX 8                      // the rings going or a HUD part (then done anyway)
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
#ifdef FFT_CHAPTER
#define TILE_COL (W->col)
#define TILE_ROW (W->row)
#else
#define TILE_COL CONTOUR_REF_tile_col
#define TILE_ROW CONTOUR_REF_tile_row
#endif
#else
#define dbl (W->dbl)
#define TILE_COL (W->col)
#define TILE_ROW (W->row)
#endif
#ifdef FFT_CHAPTER
#define TILE_X (W->node_x)
#define TILE_Z (W->node_z)
#else
#define TILE_X (W->tx)
#define TILE_Z (W->tz)
#endif
#define order (W->order)
#define key (W->key)
// Stable counting sort. A single bucket may contain all256 nodes.
static void tile_order(void)
{
    u16 i,total=0,*cnt=W->count;
    memset(cnt,0,sizeof W->count);
    for(i=0;i<MAP_N;i++)cnt[key[i]]++;
    for(i=0;i<256;i++){u16 n=cnt[i];cnt[i]=total;total+=n;}
    for(i=0;i<MAP_N;i++)order[cnt[key[i]]++]=(u8)i;
}


#define gx (W->gx)
#define gy (W->gy)
#define HUD_L ((u8 *)W->hud[0])
#ifdef FFT_PACKED_PANELS
#define HUD_D HUD_L
#else
#define HUD_D ((u8 *)W->hud[1])
#endif

static u8 node_layer(u8 cell)
{
#ifdef FFT_CHAPTER
    return W->grid&&W->grid->position?(u8)(W->grid->position[cell].z_layer>>5):0;
#else
    (void)cell;return 0;
#endif
}

#ifdef FFT_CHAPTER
#define DEF_LEARN(d) RD16((const u8 *)(d) + 24, 0)
#define DEF_DRAW(d) RD16((const u8 *)(d) + 26, 0)
#define DRAW_LEARNED 0xFFFF
#define HAS_ABILITY(a) ((a) != 0)
#else
#define DEF_LEARN(d) ((d)->learned)
#define DEF_DRAW(d) 0x3F
#define DRAW_BASIC 0x3F
#define DRAW_LEARNED 0xFF
#define HAS_ABILITY(a) ((a) != NO_ABIL)
#endif
#ifdef FFT_CHAPTER
#include "allegiance.h"
#include "throw_rules.h"
#endif
#include "status_unit.h"
#ifdef FFT_CHAPTER
#if MOD_CORE
static u8 magic_target(u8 t);
#endif
#define MIME_ACTIVE(i) (W->mime.active&&W->mime.who==(i))
#include "math_rules.h"
#endif
#ifdef FFT_CHAPTER
#include "hands.inc"
#ifdef FFT_CHAPTER
#include "weapon_math.h"
#endif
static u16 weapon_total(u8 a,u8 t,AbId ab);
static u16 weapon_forecast(u8 a,u8 t,AbId ab,u8 hand,u8 awake);
static u8 weapon_kind(u8 a,u8 t,AbId ab);
#if !MOD_CORE
#include "weapon_roll.inc"
#else
static void attack_roll(u8 a,u8 t);
static void weapon_prepare(u8 a,u8 t);
static u8 ai_clear(u8 a,u8 t);
static u8 weapon_commit(void);
static u8 weapon_proc_commit(void);
static u16 weapon_total(u8 a,u8 t,AbId ab);
static u16 weapon_forecast(u8 a,u8 t,AbId ab,u8 hand,u8 awake);
static u8 weapon_kind(u8 a,u8 t,AbId ab);
static u16 weapon_amount(u8 a,u8 t,u8 weapon,u8 crit,u16 r,u8 awake,u8 charge,u16 base_roll);
#endif

#include "equipment_rules.h"
#endif
#if MOD_CORE
static u16 rand15(void);
static u8 roll_hit(u8 a,u8 t);
#ifdef FFT_CHAPTER
static u8 magic_target(u8 t);
static u8 magic_hit(u8 a,u8 t,AbId ab,u8 execute);
#endif
static void knockback(u8 a,u8 t);
static s16 zodiac_xa(const Unit *a, const Unit *t, s16 xa);
#ifndef FFT_MODULES
#include "stats.inc"
#else
#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
void unit_stats(Unit *u)
{
    ModCtx *c;mod_call(MOD_PARTY,255,0);c=mod_context(MOD_PARTY);
    c->stats_unit=u;mod_call(MOD_PARTY,9,0);
}
#endif
#endif
#elif MOD_PART(MOD_PARTY) || MOD_PART(MOD_EQUIPMENT)
#undef unit_stats
#include "stats.inc"
#elif MOD_PART(MOD_ACTION) || MOD_PART(MOD_RESPONSE) || MOD_PART(MOD_WEAPON)
// Rebuild after returning to resident code; never evict an executing image.
#undef unit_stats
void unit_stats(Unit *u){u8 i;for(i=0;i<NUNIT;i++)if(u==&st.unit[i])W->rebuild|=(u16)1<<i;}
#endif
#if MOD_PART(MOD_PARTY)
#include "unit_setup.inc"
#include "banner_setup.inc"
#if !MOD_CORE
#include "grid_tables.inc"
#endif
#if !MOD_CORE
#include "setup_glyphs.inc"
#endif
#endif
#ifdef FFT_CHAPTER
static u16 magic_status_value(u8 t,AbId ab,u8 choice);
#include "magic_quantity.inc"
#include "cast_rules.h"
#include "jump_rules.h"
#endif
#if MOD_PART(MOD_AI) || MOD_PART(MOD_AI_SETUP) || MOD_PART(MOD_AREA_AI) || MOD_PART(MOD_AI_PREP)
#if MOD_CORE
static u8 ranged_reach(u8 a,u8 o,u8 c);
#endif
#include "ai_evaluation.inc"
#ifdef FFT_CHAPTER
#include "status_ai.h"
#include "restoration_ai.h"
#endif
#endif
#include "status_rules.h"
#ifdef FFT_CHAPTER
#include "dash_amount.inc"
#include "weapon_skill_amount.h"
#endif
#ifdef FFT_CHAPTER
#include "reflect_rules.h"
#include "item_restore.h"
#if MOD_CORE
#include "status_spell.h"
#endif
#endif
#if !MOD_CORE && MOD_PART(MOD_AI)
#include "weapon_ai.inc"
#endif
#if !MOD_CORE && (MOD_PART(MOD_FORMULA)||MOD_PART(MOD_NUMBERS)||MOD_PART(MOD_WEAPON)||MOD_PART(MOD_PLANNER)||MOD_PART(MOD_COMMAND)||MOD_PART(MOD_COMBAT))
#include "weapon_effects.inc"
#endif
#if defined(FFT_CHAPTER) && ((MOD_CORE && !defined(FFT_MODULES)) || (!MOD_CORE && MOD_PART(MOD_ORDER)))
#include "incapacity.inc"
#endif
#if !MOD_CORE && MOD_PART(MOD_ORDER)
#include "turn_order.inc"
#endif
#ifdef FFT_CHAPTER
#include "area_impact_rules.inc"
#endif
#if defined(FFT_CHAPTER) && (MOD_CORE || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_COMBAT) || MOD_PART(MOD_TARGETS) || MOD_PART(MOD_AREA_UI) || MOD_PART(MOD_PERFORMANCE) || MOD_PART(MOD_RESPONSE))
static u8 hamedo_begin(u8 a,u8 t,AbId ab);
static u8 action_available(u8 a,u8 t)
{
    return a<NUNIT&&t<NUNIT&&st.action.phase==CTX_IDLE&&st.unit[a].hp&&unit_can_act(&st.unit[a])&&!unit_airborne(&st.unit[t]);
}
#include "cast_target.h"
#include "mime_record.inc"
#endif
#if defined(FFT_CHAPTER) && (MOD_PART(MOD_FORMULA) || MOD_PART(MOD_ACTION) || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_COMBAT) || MOD_PART(MOD_RESPONSE) || MOD_PART(MOD_EQUIPMENT) || MOD_PART(MOD_NATURE) || MOD_PART(MOD_SAVE) || MOD_PART(MOD_PERFORMANCE) || MOD_PART(MOD_ORDER) || MOD_PART(MOD_WEAPON) || MOD_PART(MOD_STATUS))
#include "status_policy.inc"
#include "status_runtime.inc"
#if !MOD_CORE
#if MOD_PART(MOD_ACTION) || MOD_PART(MOD_FORMULA) || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_COMBAT) || MOD_PART(MOD_RESPONSE) || MOD_PART(MOD_EQUIPMENT)
#include "action_start.inc"
#endif
#endif
#if !MOD_CORE && MOD_PART(MOD_STATUS)
#include "reaction_policy_runtime.inc"
#endif
#if !MOD_CORE && MOD_PART(MOD_WEAPON)
#include "reaction_policy_runtime.inc"
#endif
#if MOD_PART(MOD_STATUS)
static u16 status_apply_request(void)
{
    u8 t=st.formula.target;u16 result;Unit *u;
    if(t>=NUNIT||st.formula.roll>1||!unit_status_request_supported(&st.status_delta,(u8)st.formula.roll))return STATUS_REJECTED;
    u=&st.unit[t];
    if(!u->hp||u->x>=MAP_W||u->z>=MAP_H||UNIT_CELL(u)>=MAP_N)return STATUS_REJECTED;
    result=core_status_apply(&u->statuses,&st.status_delta,u->status_flags,(u8)st.formula.roll,cj_status_rules(common),u->status_ct);
    if(st.pending[t].state==DEFER_PERFORMING&&!(core_status_current(&u->statuses,0)&1))st.pending[t].state=DEFER_NONE;
    if((st.pending[t].state==DEFER_CHARGING||st.pending[t].state>=DEFER_CAST_TILE)&&!(core_status_current(&u->statuses,0)&8))
        st.pending[t].state=DEFER_NONE;
    unit_status_refresh(u);
    status_clock_update(t);
    if(result){
        status_changed();
        if(t==st.act&&((st.mode==M_TARGET&&!unit_can_move(u))||
            ((st.mode==M_AIM||st.mode==M_CAST_RANGE||st.mode==M_AREA||st.mode==M_ACTS||st.mode==M_SKILLS)&&!unit_can_act(u)))){
            st.mode=M_MENU;cursor_place(UNIT_CELL(u));
        }
        if(t==st.act&&st.ai&&st.mode!=M_ACT&&st.mode!=M_WALK&&st.mode!=M_PICK&&st.mode!=M_END){
            st.ai=AI_THINK;st.mode=M_BROWSE;if(W)W->ai_k=0;
        }
    }
    return result;
}
#endif
#endif
#if !MOD_CORE && MOD_PART(MOD_ORDER)
#include "pending_next.inc"
#include "ct_next.inc"
#include "facing_arrows.inc"
#include "banners.inc"            // turn/objective banners: moved from NUMBERS (never with numbers)
// Turn-end status runtime (moved from FORMULA: the turn order follows it anyway).
u16 mod_entry(u8 op,u8 a)
{
    if(op==17)return ct_next();
    if(op==18)return pending_next();
    if(op==1)return battle_outcome();
    if(op==2){draw_arrows();return 0;}
    if(op==6){bn_shade();return 0;}
    if(op==7){bn_draw();return 0;}
    if(op==16){
        u8 end;if(a==U_RAMZA)st.rounds++;st.pick_t=0;end=battle_outcome();if(end)return end;
        status_turn_start(a);if(W){W->ai_k=0;W->frz=W->frz_n=0;W->frz_h=0;}
        st.menu=MN_MOVE;st.moved=st.acted=0;return 0;
    }
    if(op==14)return status_dead_turn(a);
    if(op==15){Unit *u=&st.unit[a];u->ct=core_status_end_ct(&u->statuses,u->ct,st.formula.roll&1,st.formula.roll&2);if(st.formula.roll&2)status_turn_action(a);return status_upkeep(a);}
    if(op==4){status_tick();return 0;}
    if(op==5)return status_upkeep(a);
    turn_order();return 0;
}
#endif
#if MOD_PART(MOD_AI) || MOD_PART(MOD_FORMULA) || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_COMBAT) || MOD_PART(MOD_NUMBERS)
#if defined(FFT_CHAPTER) && !MOD_CORE && MOD_PART(MOD_FORMULA)
#include "weapon_freeze.inc"
#endif
#include "formula.inc"
#endif
#if defined(FFT_CHAPTER) && (MOD_PART(MOD_TARGETS) || MOD_PART(MOD_AREA_AI) || MOD_PART(MOD_AI_PREP))
static void act_start(u8 a,u8 t);
static void action_pose_start(u8 a,u8 t);
#include "area_geometry.h"
#include "area_targets.inc"
#if !MOD_CORE

#if MOD_PART(MOD_TARGETS)
#include "action_start.inc"
#endif
#endif
#include "magic_forecast.inc"
#if MOD_PART(MOD_TARGETS)
#include "magic_prepare.inc"
#include "batch_prepare.inc"
#include "reaction_flood.inc"
#include "batch_forecast.inc"
#include "area_preview.inc"
#if !MOD_CORE

u16 mod_entry(u8 op,u8 a)
{
    if(op==11||op==12){reaction_area_launch(op==12);return 0;}
    if(op==1)return batch_prepare(M.cast_batch_out,0);
    if(op==2)return batch_owned_prepare();
    if(op==3)return batch_forecast(M.cast_batch_out);
    if(op==4)return area_preview();
    return (u16)cast_targets(M.cast_actor,M.cast_ref,M.cast_ability,a);
}
#endif
#endif
#if MOD_PART(MOD_AI_PREP)
#include "area_ai_prepare.inc"
#include "weapon_panel_ai.inc"
#if !MOD_CORE
u16 mod_entry(u8 op,u8 a){(void)a;return op==2?weapon_panel_ai():area_ai_prepare();}
#endif
#endif
#if MOD_PART(MOD_AREA_AI)
#include "area_ai.inc"
#if !MOD_CORE
u16 mod_entry(u8 op,u8 a){(void)op;(void)a;return area_ai_step();}
#endif
#endif
#endif
#if defined(FFT_CHAPTER) && MOD_PART(MOD_AREA_UI)
#if MOD_CORE
static void ui_text(s16 x,s16 y,const char *s,u8 color,s16 limit);
static u8 ui_width(const char *s);
static u8 put_s(char *s,u8 n,const char *t);
static u8 put_n(char *s,u8 n,u16 v);
static void text_into(u16 *l,u16 *d,s16 x,s16 y,const char *s,u8 color);
static u8 area_paint(u8 who);
static void choice_frame(const ChoiceLayout *p,const char *title);
static void choice_row(const ChoiceLayout *p,u8 row,const char *label,u8 color);
static void choice_cursor_animated(const ChoiceLayout *p,u8 row,u16 frame);
static void choice_list(const ChoiceLayout *p,const char *const *labels,u8 count,u8 selected,u8 color,u8 other,u16 frame);
#endif
#if !MOD_CORE
#define st_names CONTOUR_REF_st_names
u8 act_kind(void)
{
    if(st.abil==NO_ABIL||(st.abil<512&&(abilities[st.abil].flags&AF_CHARGE)))return st.effect_kind&EFFECT_KIND_MASK;
    if(st.abil>=512&&st.abil<NABIL)return st.effect_kind&EFFECT_KIND_MASK;
    if(st.effect_kind==AB_CATCH||st.effect_kind==AB_MP_DAMAGE)return st.effect_kind;
    if(st.effect_kind==AB_GOLEM)return AB_GOLEM;   // a Golem cast or guard
    if(st.abil<512&&((abilities[st.abil].flags&AF_CAST)||restore_ability(st.abil)||reaction_typed(st.abil)))return st.effect_kind&EFFECT_KIND_MASK;
    return st.abil==NO_ABIL?AB_DAMAGE:st.abil==ABIL_DEFEND?AB_DEFEND:abilities[st.abil].kind;
}
#include "numbers.inc"
#endif
#include "area_ui.inc"
#endif
#if !MOD_CORE && MOD_PART(MOD_PERFORMANCE)

#include "reward_value.h"
#include "reaction_policy_runtime.inc"
#include "allegiance.inc"
#include "status_commit.inc"
#include "performance_commit.inc"
#include "magic_commit.inc"
#include "batch_commit.inc"
#include "performance_lifecycle.inc"
u16 mod_entry(u8 op,u8 a)
{
    if(op==3)return performance_order(M.cast_actor,M.cast_ref,M.cast_ability,a);
    if(op==5)return can_use(M.cast_actor,M.cast_ability); // isolated capability query
    if(op==4)return performance_pending(a);
    if(op==7)return batch_commit(M.cast_batch_out);
    return batch_commit(&W->cast)&2;
}
#endif
#if !MOD_CORE && MOD_PART(MOD_NUMBERS)
#define st_names CONTOUR_REF_st_names
#include "forecast_text.inc"
#include "forecast_panel.inc"
u16 mod_entry(u8 op,u8 a)
{
    (void)a;
    if(op==5)return forecast_text(st.formula.actor,st.formula.target,st.formula.ability,M.forecast_out);
    if(op==6){make_forecast(st.formula.actor,st.formula.target,st.formula.ability);return 0;}
    return 0;
}
#endif
#if !MOD_CORE && MOD_PART(MOD_COMMAND)

#include "command.inc"
#include "incantation.inc"
u16 mod_entry(u8 op,u8 a)
{
    (void)a;
    if(!op)return command_skill(M.cast_actor,M.cast_ref,M.cast_ability);
    if(op==3)return command_cast(M.cast_actor,M.cast_ref,M.cast_ability,a);
    if(op==4)return command_pending(a);
    command_defend(st.formula.actor);return 0;
}
#endif
#if !MOD_CORE && MOD_PART(MOD_RESPONSE)
#include "rewards.inc"
#include "magic_prepare.inc"
#include "reaction_runtime.inc"
#include "hamedo.inc"
#include "attack_start.inc"
#include "jump_order.inc"
#include "charge_math.inc"
#include "strike_runtime.inc"
u16 mod_entry(u8 op,u8 a)
{
    if(op==7){strike_launch(a);return 0;}
    if(op==5)return jump_order(M.cast_actor,M.cast_ref,M.cast_ability);
    if(op==4)return attack_sequence(M.cast_actor,M.cast_ref,a);

    if(op==1)reaction_launch(a);
    if(op==2)reaction_return();
    if(op==3)reaction_rewards();
    return 0;
}
#endif
#if !MOD_CORE && MOD_PART(MOD_WEAPON)
#include "rewards.inc"
#include "weapon_commit.inc"
u16 mod_entry(u8 op,u8 a)
{
    if(op==6)return weapon_ai_prepare();
    if(op==4)return attack_prediction(M.cast_actor,M.cast_ref);
    if(op==5)return weapon_forecast(M.cast_actor,M.cast_ref,M.cast_ability,a&1,(a>>1)&1);
    if(op==1)return weapon_commit();
    return 0;
}
#endif
#if !MOD_CORE && MOD_PART(MOD_PLANNER)
#define FFT_DIRECT_NAME direct_target
#define FFT_DIRECT_PANEL
#include "direct_trajectory.inc"
#undef FFT_DIRECT_PANEL
#undef FFT_DIRECT_NAME
#include "arc_trajectory.inc"
#include "melee_target.inc"
#include "status_policy.inc"
#include "reaction_restore.inc"
#include "weapon_prepare.inc"
#include "strike_target.inc"
u16 mod_entry(u8 op,u8 a)
{
    (void)a;
    if(op==1){weapon_prepare(st.atk,st.tgt);return 0;}
    if(op==3||op==4){weapon_preview(st.formula.actor,st.formula.target,st.formula.ability);
        st.formula.target=W->preview_target;if(op==4)W->nature_pending|=16;return 0;}
    if(op==5)return strike_target();
    if(op==2){u8 who=st.formula.actor,t=st.formula.target,w=hand_weapon(&st.unit[who],0);
        if(w>=128)w=0;return cj_weapon(common,w)[1]&192?melee_target(who,t)!=255:
            (cj_weapon(common,w)[1]&16?arc_target(who,t)!=255:direct_target(who,t)!=255);}
    return melee_cache();
}
#endif
#if !MOD_CORE && MOD_PART(MOD_STATUS)
#include "reward_value.h"
#include "status_commit.inc"
#include "weapon_proc.inc"
#define FFT_STATUS_BATCH
#define batch_commit batch_status_commit
#define action_magic_commit status_magic_commit
#include "batch_commit.inc"
#undef action_magic_commit
#undef batch_commit
#undef FFT_STATUS_BATCH
u16 mod_entry(u8 op,u8 a)
{
    if(op==2)return weapon_proc_commit();
    if(op==3)return status_apply_request();
    if(op==12)return a?batch_status_commit(M.cast_batch_out):(batch_status_commit(&W->cast)&2);
    return status_scalar_commit();
}
#endif
#if !MOD_CORE && MOD_PART(MOD_ACTION)
#include "rewards.inc"
#include "action_module.inc"
#endif

#if MOD_CORE                            // a module's build (module.h): only its own part
#ifdef FFT_CHAPTER
#ifndef FFT_MODULES
static u8 action_reaction_blocked(u8 target);
#endif
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u16 battle_status_apply(u8 target,const CoreStatusDelta *delta,u8 removal_only)
{
    if(!delta||target>=NUNIT||!unit_status_request_supported(delta,removal_only))return STATUS_REJECTED;
    if(!st.unit[target].hp||st.unit[target].x>=MAP_W||st.unit[target].z>=MAP_H||UNIT_CELL(&st.unit[target])>=MAP_N)return STATUS_REJECTED;
    st.status_delta=*delta;st.formula.target=target;st.formula.roll=removal_only;
#ifdef FFT_MODULES
    return mod_call(MOD_STATUS,3,0);
#else
    return status_apply_request();
#endif
}
#endif
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u8 battle_reaction_blocked(u8 target)
{
    if(target>=NUNIT||!st.unit[target].hp||st.unit[target].x>=MAP_W||
       st.unit[target].z>=MAP_H||st.action.phase==CTX_IDLE)return 1;
    st.formula.target=target;
#ifdef FFT_MODULES
    return (u8)mod_call(MOD_ACTION,5,0);
#else
    return action_reaction_blocked(target);
#endif
}
#endif
#endif

// ---------------------------------------------------------------- the map in four orientations
u8 view_w(u8 rot) { return rot & 1 ? MAP_H : MAP_W; }
u8 view_h(u8 rot) { return rot & 1 ? MAP_W : MAP_H; }

void view_of(u8 rot, s16 x, s16 z, s16 *u, s16 *v)
{
    core_grid_view(MAP_W,MAP_H,rot,x,z,u,v);
}

void world_of(u8 rot, s16 u, s16 v, s16 *x, s16 *z)
{
    core_grid_world(MAP_W,MAP_H,rot,u,v,x,z);
}

const Tile *tile_at(s16 x, s16 z)
{
#ifdef FFT_CHAPTER
    u16 id;
    if(!W)return RT_NULL;
    id=node_at(x,z,0);return id<MAP_N?&map_tiles[id]:RT_NULL;
#else
    if ((u16)x >= MAP_W || (u16)z >= MAP_H) return RT_NULL;
    return &map_tiles[z * MAP_W + x];
#endif
}

u16 node_at(s16 x,s16 z,u8 layer)
{
    u8 cell;
#ifdef FFT_CHAPTER
    if(W->grid)return core_grid_find(W->grid,x,z,layer,&cell)?cell:256U;
    if(layer||!core_grid_rect_id(MAP_W,MAP_H,W->grid_row,x,z,&cell))return 256U;
    return cell;
#else
    if(layer||(u16)x>=MAP_W||(u16)z>=MAP_H)return 256U;
    return (u16)(z*MAP_W+x);
#endif
}
u8 unit_place(Unit *u,u8 cell)
{
    if((u16)cell>=MAP_N)return 0;
#ifdef FFT_CHAPTER
    u->cell=cell;
#endif
    u->x=TILE_X[cell];u->z=TILE_Z[cell];
#ifdef FFT_CHAPTER
    if(st.mode==M_AREA)W->area_ui.dirty=1;
#endif
    return 1;
}
u8 cursor_place(u8 cell)
{
    if((u16)cell>=MAP_N)return 0;
#ifdef FFT_CHAPTER
    st.cursor_cell=cell;
#endif
    st.cx=TILE_X[cell];st.cz=TILE_Z[cell];return 1;
}
u8 unit_at_cell(u8 cell)
{
    u8 i;
    for(i=0;i<NUNIT;i++)if(st.unit[i].x<MAP_W&&st.unit[i].z<MAP_H&&
        UNIT_CELL(&st.unit[i])==cell&&!DC_OBJ(st.unit[i].dc))return i+1;
    return 0;
}

// view corner c of a tile is world corner (c - rot) & 3 (both numbered (0,0) (1,0) (1,1) (0,1))
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
u8 corner_h(u8 rot, s16 u, s16 v, u8 c)
{
    s16 x, z;
    if ((u16)u >= view_w(rot) || (u16)v >= view_h(rot)) return 0;
    world_of(rot, u, v, &x, &z);
    {const Tile *t=tile_at(x,z);return t?t->c[(c-rot)&3]:0;}
}
#endif
#endif

s16 scene_x(u8 rot, s16 u, s16 v) {
#ifdef FFT_SOURCE_CAMERA
    return SC_W - (s16)((u - v + view_h(rot)) * HW);
#else
    return (s16)((u - v + view_h(rot)) * HW);
#endif
}
s16 scene_y(s16 u, s16 v) { return (s16)(SC_OY + (u + v) * HH); }

#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
u8 unit_at(s16 x, s16 z)
{
#ifdef FFT_CHAPTER
    u16 cell=node_at(x,z,0);
    return cell<MAP_N?unit_at_cell((u8)cell):0;
#else
    u8 i;
    for (i = 0; i < NUNIT; i++)
        if (st.unit[i].x == x && st.unit[i].z == z && !DC_OBJ(st.unit[i].dc)) return i + 1;
    return 0;
#endif
}
#endif

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



#ifdef FFT_CHAPTER                      // rules.inc
static void fixed_units(void);
static u8 ranged_reach(u8 a, u8 o, u8 c);
static void weapon_effect(void);
static u8 pending_next(void);
static u8 charge_resolve(void);
#endif
#ifdef FFT_CHAPTER
static void opening_start(u16 n);
static u8 map_update(void);
static void map_render(void);
static void save_campaign(void);
static void prepare_rewards(void);
static void party_load(void);
static void funds_load(void);
#endif



// ---------------------------------------------------------------- turn order
// FFT's clock (RE_NOTES.md § Turn order, checked on the original's turns): each tick every unit's
// CT grows by its Speed (at most 254); a unit with CT >= 100 acts, the highest first, a tie to
// the lowest index; its CT loses 100 when picked and gains 20 for each of Move and Act left
// unused at the end, at most 60. A unit at 0 HP keeps its clock: each pick lowers its death
// counter (3 to 0), the next one turns it into a crystal or a chest (a player's unit 3 times in
// 4, an enemy 1 in 2) and its clock stops; the guest (Delita) never turns.
#ifdef FFT_MODULES
u8 ct_next(void){return (u8)mod_call(MOD_ORDER,17,0);}
#else
#include "ct_next.inc"
#endif

// ORDER15 (the turn order follows): CT credit, then the source
// upkeep (Poison/Regen, Brave +1 below 10); a no-op upkeep changes nothing.
u8 ct_end(u8 who, u8 moved, u8 acted)
{
#ifdef FFT_CHAPTER
#ifdef FFT_MODULES
    st.formula.roll=(moved?1:0)|(acted?2:0);return (u8)mod_call(MOD_ORDER,15,who);
#else
    Unit *u=&st.unit[who];u->ct=core_status_end_ct(&u->statuses,u->ct,moved,acted);
    if(acted)status_turn_action(who);
    return status_upkeep(who);
#endif
#else
    u8 c = st.unit[who].ct;
    if (!moved) c += 20;
    if (!acted) c += 20;
    st.unit[who].ct = c > 60 ? 60 : c;
    return 0;
#endif
}

#ifdef FFT_MODULES
// Cold turn-list construction belongs to ORDER; impact callers have returned
// from that image before reaching this wrapper. No executing image is evicted.
void turn_order(void){mod_call(MOD_ORDER,0,0);}
#else
#include "turn_order.inc"
#endif

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
#ifdef FFT_CHAPTER
    // A terminal all-KO formation must not enter another death-clock search.
    if(outcome()){end_battle(outcome());return;}
    // Consume empty/invalid due actions iteratively, without nesting turn_start.
    // One successful launch yields until its complete root/response settlement.
    do {
        u8 next=ct_next();
        if(next==CT_WAIT){st.mode=M_CLOCK;st.ai=AI_OFF;return;}
        if(next!=CT_DUE){st.act=next;break;}
        if(charge_resolve())return;
    }while(1);
#else
    st.act = ct_next();
#endif
#ifdef FFT_MODULES
    {u8 end=(u8)mod_call(MOD_ORDER,16,st.act);if(end){end_battle(end);return;}}
#else
    if(st.act==U_RAMZA)st.rounds++;
    st.pick_t=0;
    if(outcome()){end_battle(outcome());return;}
#ifdef FFT_CHAPTER
    status_turn_start(st.act);
#else
    st.unit[st.act].status&=(u8)~ST_DEFEND;
#endif
#endif
    cursor_place(UNIT_CELL(&st.unit[st.act]));
    me = st.unit[st.act].team == TEAM_PLAYER && !st.ai_all && !(st.auto_units & (1U << st.act))
#ifdef FFT_CHAPTER
        && !(st.unit[st.act].status_flags & STATUS_NO_CONTROL)
#endif
        ;
    st.ai = me ? AI_OFF : AI_THINK; st.ai_t = 0;
    if (me && st.prot != 0xFF) { st.want = st.prot; st.prot = 0xFF; }   // the player's view back
#ifndef FFT_MODULES
    if(W){W->ai_k=0;W->frz=W->frz_n=0;W->frz_h=0;}
    st.menu=MN_MOVE;st.moved=st.acted=0;
#endif
    st.mode=me?M_MENU:M_BROWSE;
    turn_order();
}

void turn_end(u8 moved, u8 acted)
{
    if(ct_end(st.act, moved, acted)&&outcome()){end_battle(outcome());return;}
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
#define RINGS(m) ((m) == M_CAST_RANGE || (m) == M_AREA || (m) == M_TARGET || (m) == M_AIM || ((m) == M_ORDER && (st.back == M_TARGET || st.back == M_AIM || st.back==M_CAST_RANGE)))
                                                 // the modes that show st.reach
#define AIMING(m) ((m) == M_CAST_RANGE || (m) == M_AREA || (m) == M_AIM || ((m) == M_ORDER && (st.back == M_AIM || st.back==M_CAST_RANGE)))   // its rings inverted:
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
    view_of(rot,TILE_X[i],TILE_Z[i],&u,&v);
    k->x = scene_x(rot, u, v) - HW; k->depth = (u8)(u + v);
    for (c = 0; c < 4; c++) {
#ifdef FFT_SOURCE_CAMERA
        h[c]=map_tiles[i].c[((4-c)-rot)&3];
#else
        h[c]=map_tiles[i].c[(c-rot)&3];
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
        const u8 *dt; u16 x0, dw;
        DEPTH_ROW(yy, dt, x0, dw);
        for (k = 0; k < 3; k++) {
            u16 mw = m3[k], g1, g2, c, oo = (u16)(o + 2 * k), b = (u16)(2 * (w0 + k));
            const u8 *t = dt + b;          // depths, masks: constant offsets
            u8 *q = scene_l + oo;
            if (!mw || (u16)(w0 + k) >= SC_BYTES / 2) continue;
            if (st.over || (u16)(b - x0) >= dw) c = 0;  // over the scenery, or nothing in front
            else {
                g1 = (u16)(RD16(t, 0) + dd) & 0x8080; g1 = (u16)((g1 << 1) - (g1 >> 7));
                g2 = (u16)(RD16(t + 2 * DEPTH_STRIDE, 0) + dd) & 0x8080; g2 = (u16)((g2 << 1) - (g2 >> 7));
                c = (u16)(((g1 & RD16(t + DEPTH_STRIDE, 0)) | (g2 & RD16(t + 3 * DEPTH_STRIDE, 0))) & mw & hp);
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
#ifdef FFT_VIEW_SPLIT
// The ragged depth just unpacked (extract.ragged_depth): each row's offset, then its mask bytes
// XORed back with the row above where both spans cover them, a word at a time (even spans)
static void depth_rows(void)
{
    DepthRow *r = (DepthRow *)vbuf;
    u16 y, off = (u16)(SC_H * sizeof(DepthRow));
    for (y = 0; y < SC_H; y++) { r[y].off = (u16)(off - r[y].x0); off += r[y].w; }
    vplane = (u16)(off - SC_H * sizeof(DepthRow));
    for (y = 1; y < SC_H; y++, r++) {           // r[1]: row y, r[0]: the row above
        u8 lo = r[1].x0 > r[0].x0 ? r[1].x0 : r[0].x0;
        u8 hi = (u8)(r[1].x0 + r[1].w), h2 = (u8)(r[0].x0 + r[0].w);
        u16 *p = (u16 *)(vbuf + r[1].off + vplane + lo);        // first mask, the second
        const u16 *q = (const u16 *)(vbuf + r[0].off + vplane + lo);   // vplane words on
        if (h2 < hi) hi = h2;
        for (; lo < hi; lo += 2, p++, q++) { p[0] ^= q[0]; p[vplane] ^= q[vplane]; }
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

#ifdef FFT_VIEW_SPLIT
// V3 adds a compressed1-bit width mask between colours and depth. The row
// backups are obsolete after a complete colour reload, so reuse them as scratch.
typedef char EdgeMaskFitsBackups[sizeof(W->bk) >= SC_BYTES*SC_HMAX ? 1 : -1];
static const u8 *unpack_colours(u8 rot)
{
    const u8 *p=views[rot]+8,*mask;          // R2/R3 (extract.view_bank)
    u16 extra=RD16(views[rot],2);           // the width mask's stream (R3), 0 (R2)
    mask=p+RD16(views[rot],1);
    zx0_unpack(p,scene_l);
#ifdef FFT_EDGE_WIDTH_REVIEW
    if(extra && (st.scenery&1)) {
        PlaneWord *m=(PlaneWord *)W->bk,*l=(PlaneWord *)scene_l,*d=(PlaneWord *)scene_d;
        u16 n=(u16)SC_PLANE>>2;
        zx0_unpack(mask,(u8 *)W->bk);
        while(n--) {
            PlaneWord bits=*m++,ink=~*d&bits;
            *l=(*l&~bits)|ink;l++;
            *d=(*d&~bits)|ink;d++;
        }
    }
#endif
    bk_reset();
    return mask+extra;
}
#endif

static void unpack_view(u8 rot)
{
    if (vbuf_rot == rot) return;
#ifdef FFT_VIEW_SPLIT
    zx0_unpack(unpack_colours(rot),vbuf);
    depth_rows();
#else
    zx0_unpack(views[rot], vbuf);
#endif
#ifdef FFT_TEXTURE_PREVIEW
    preview_iso_apply(rot);
#endif
    vbuf_rot = rot;
}

#if !defined(__m68k__) && defined(FFT_VIEW_SPLIT)
// the tests: byte b of row y of depth plane k (0 depth, 1 mask, 2 depth, 3 mask), 0 outside
// the row's span, as the four full planes of the former layout
u8 depth_byte(u8 k, s16 y, u16 b)
{
    const u8 *t; u16 x0, w;
    DEPTH_ROW(y, t, x0, w);
    return (u16)(b - x0) < w ? t[b + k * DEPTH_STRIDE] : 0;
}
void depth_planes(u8 *out)
{
    u16 y, b; u8 k;
    for (k = 0; k < 4; k++) for (y = 0; y < SC_H; y++) for (b = 0; b < SC_BYTES; b++) *out++ = depth_byte(k, (s16)y, b);
}
#endif

// is scene pixel (x, y) in front of everything nearer than view diagonal depth (not covered)?
#ifndef FFT_WATER_BANK
static u8 seen(s16 x, s16 y, u8 depth)
{
    const u8 *t;
    u16 x0, w, o = (u16)(x >> 3);
    u8 b = (u8)(0x80 >> (x & 7));
    DEPTH_ROW(y, t, x0, w);
    if ((u16)(o - x0) >= w) return 1;
    t += o;
    return !((t[0] > depth && t[DEPTH_STRIDE] & b) || (t[2 * DEPTH_STRIDE] > depth && t[3 * DEPTH_STRIDE] & b));
}
#endif

// n bytes b0.. of scene row y (the last into *tail when given): the pixels with
// something in front of view depth `depth` (a greater diagonal). The cursor's, the units' and
// the blade's cover, each made again only when what it covers changes.
static u32 cover_bytes(s16 y, s16 b0, u8 depth, u8 n, u8 *tail)
{
    const u8 *t = vbuf;
    u16 x0 = 0, w = 0, s = DEPTH_STRIDE;
    u32 m = 0;
    u8 h;
    if ((u16)y < SC_H) DEPTH_ROW(y, t, x0, w);
    if (!w) n = 0;                              // nothing in front on this row
    for (t += b0; n; n--, t++, b0++) {
        h = 0;
        if ((u16)(b0 - x0) < w) {
            if (t[0] > depth) h = t[s];
            if (t[2 * s] > depth) h |= t[3 * s];
        }
        if (n == 1 && tail) *tail = h; else m = m << 8 | h;
    }
    return m;
}

// The cursor's mask (marker mk of tile c) dashed as the hidden rings where the view has something
// in front of it (a canal tile behind a higher street tile): the view's depth bytes compared a
// byte (8 pixels) at a time, made again only when the cursor or view changes.
// Visibility is mandatory on the first frame, including the AI's selected tile.
static const PlaneWord *cursor_mask(const Marker *mk, u16 c)
{
    u16 k = (u16)(c | st.rot << 8 | 0x8000);   // c: the tile, | 0x1000 a reach tile's marker
    s16 r, b0 = mk->x >> 3, sh = mk->x & 7, yy = mk->y;
    if (vbuf_rot != st.rot) return mk->m;
    if (W->cur_k == k) return W->cur_m;
    W->cur_wait = 0; W->left -= CUR_K;
    W->cur_k = k;
    for (r = 0; r < mk->rows; r++, yy++) {
        u32 cv = cover_bytes(yy, b0, mk->depth, 4, RT_NULL);
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
    u16 i;
#ifdef FFT_WATER_BANK                   // the chapter's descriptor (tools/chapter_desc.py)
    const u8 *p=cdesc+CD(WATER);
    p+=RD16(p,rot);
    for(i=0;i<4;i++) {
        u8 n=(u8)RD16(p,0);p+=2;W->glints[i]=n;
        W->puddle[i]=p;p+=(u16)n*2;
    }
#else
    memset(W->glints, 0, sizeof W->glints);
    for (i = 0; i < MAP_N; i++) {
        s16 u, v, x, y, r, px;
        if (!map_tiles[i].water) continue;
        view_of(rot, TILE_X[i], TILE_Z[i], &u, &v);
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
    while(i<MAP_N){
        u8 bits=0,bit=1;
        do {if(st.reach[i]!=0xFF)bits|=bit;i++;bit=(u8)(bit<<1);}
        while(bit && i<MAP_N);
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
#ifdef FFT_CHAPTER
    static const s8 dx[4]={0,1,0,-1},dz[4]={-1,0,1,0};
    u8 side,layer=node_layer(i);
    for(side=0;side<4;side++) {
        u16 next=node_at(x+dx[side],z+dz[side],layer);
        if(next>=MAP_N||st.reach[next]==255)bits|=(u8)(1U<<side);
    }
#else
    if(!z||st.reach[i-MAP_W]==0xFF)bits|=1;
    if(x==MAP_W-1||st.reach[i+1]==0xFF)bits|=2;
    if(z==MAP_H-1||st.reach[i+MAP_W]==0xFF)bits|=4;
    if(!x||st.reach[i-1]==0xFF)bits|=8;
#endif
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
    u16 i;
    Marker mk;
#if defined(FFT_MEASURE) && defined(RT_CYCLES)
    if (scene_rot != rot) frame_stall = 1;       // initial load or a rotated view
#endif
    if (scene_rot == rot && scene_seq == hl_seq) {
        if (W->drop_y0 < W->drop_y1) goto drop;
        if (W->ring_i >= MAP_N) return;
        goto rings;
    }
    if(scene_rot==rot && W->drop_y0>=W->drop_y1 && W->ring_i>=MAP_N && rings_unchanged()){
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
        // Input feedback is mandatory: old ranges disappear in this render,
        // never row-by-row over later frames (GAME_DESIGN.md).
        s16 n = W->drop_y1 - W->drop_y0;
        W->drop_wait = 0;
#ifdef FFT_VIEW_SPLIT
        if (W->bk_over) {                          // rings went past the backup: the colours
            unpack_colours(scene_rot);            // same selected width, no rings
            scene_ver++;
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
    if (!RINGS(st.mode)) { W->ring_i = MAP_N; W->ring_style=0; return; }
#if defined(FFT_MEASURE) && defined(RT_CYCLES)
    frame_stall = 1;
#endif
    ZB(10);
    memset(W->ring_bits,0,sizeof W->ring_bits);
    for (; W->ring_i < MAP_N; W->ring_i++) {
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

#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u16 scene_checksum(void)
{
    u16 a = 0, b = 0, i;
    for (i = 0; i < (u16)(SC_BYTES * SC_H); i++) {
        a = (a + scene_l[i]) % 255; b = (b + a) % 255;
        a = (a + scene_d[i]) % 255; b = (b + a) % 255;
    }
    return (u16)(b << 8 | a);
}
#endif

// ---------------------------------------------------------------- reach and paths
static const s8 dir_x[4] = { 1, -1, 0, 0 }, dir_z[4] = { 0, 0, 1, -1 };
#ifdef FFT_CHAPTER
#define EDGE_ROW(id) (W->eg+((u16)(id)<<W->edge_shift))
// Direction is stored only for leaps; absent first nodes remain explicit.
// This retains the accepted one-gap blocker policy while removing via rows.
static u16 edge_via(u8 id,u8 kind)
{return kind&4?MAP_N:W->via[id][kind>>6];}
#define CORE_NAV_COMPACT
#else
#define EDGE_ROW(id) W->eg[id]
#endif
#define CORE_NAV_MAP W
#ifdef FFT_CHAPTER
#define FFT_NAV_SPANS
#endif
#include "tactical_search.inc"
#undef CORE_NAV_MAP
#undef CORE_NAV_COMPACT

// FFT's movement spread (battle_move_calculate_spread, measured on the original's move ranges;
// RE_NOTES.md § Moving): the Move points each tile costs from tile c: a step 1, from water to
// water 2, not into water from land with the last point; a jump over one tile 2 (from water 3;
// Jump 2 or more: to a tile no lower by at most Jump half units, over ground no higher than it,
// no enemy there); a step's height within Jump (out of water: from its surface); enemies block,
// allies can be crossed. limit: the most it spends (dist 0xFF beyond); from: the previous tile.
#ifdef FFT_CHAPTER
#include "movement_runtime.inc"
#endif
static u8 lower(u8 c)                    // a tile's height a unit leaves it from (half units)
{
    return (u8)(map_tiles[c].stand + (map_tiles[c].water << 1));
}
static u8 tile_h(const Tile *t);

// the moves out of each tile a unit with that Jump can make whatever the units (to a walkable
// tile, within its heights; a jump: over lower ground), made once: each its tile and kind (1 a
// jump, 2 into water)
#ifdef FFT_MODULES
static void edges(u8 jump)
{
    ModCtx *c;mod_call(MOD_MOVEMENT,255,0);c=mod_context(MOD_MOVEMENT);
    c->cast_ability=jump;mod_call(MOD_MOVEMENT,0,255);
}
#else
#include "movement_edges.inc"
#endif

#ifdef FFT_CHAPTER
// Tactical binding only: the chapter frontend supplies matching scenery/depth.
// Validate before changing live pointers. Counts include256; cell IDs stay bytes.
// Topology allocation failure retains the previous complete binding.
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u8 battle_bind_grid(const CoreGrid *grid,const Tile *tiles,const u8 *x,const u8 *z)
{
    u16 i;u8 grid_upper=0;
    if(!W||!tiles||!x||!z)return 0;
    if(grid) {
        if(!core_grid_valid(grid)||grid->count>MAP_NMAX||
           grid->width>MAP_WMAX||grid->height>MAP_HMAX)return 0;
        for(i=0;i<grid->count;i++) {
            CoreTilePosition p;
            if(!core_grid_position(grid,(u8)i,&p))return 0;
            if(x[i]!=p.x||z[i]!=(p.z_layer&31))return 0;
            if(p.z_layer&32)grid_upper=SURFACE_GRID_UPPER;
        }
    }
    if(!nav_reserve(grid?grid->count:MAP_N,grid_upper!=0))return 0;
    if(grid){map_w=grid->width;map_h=grid->height;map_n=grid->count;}
    grid_tables();
    W->surface_flags=grid_upper;
    W->surface=RT_NULL; // a tactical rebind must not retain another map's collision columns
    W->grid=grid;W->node_x=x;W->node_z=z;map_tiles=tiles;
#ifdef FFT_MODULES
    mod_id=0;
#endif
    for(i=0;i<MAP_N;i++) {
        W->hs[i]=tiles[i].stand;W->hl[i]=lower((u8)i);
        W->hw[i]=(u8)((tiles[i].walk!=0)|(tiles[i].water<<1));
        W->top[i]=(u16)(tile_h(&tiles[i])*12);
    }
    W->ej=255;W->from_who=0;W->cur_k=0;
    memset(W->feet_k,0,sizeof W->feet_k);
    W->bg_ok=W->bg_part=0;
    return 1;
}
#endif
#endif

#ifdef FFT_CHAPTER
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u8 battle_bind_surface(const u8 *bank,u16 bytes)
{
    if(!W||!surface_valid(bank,bytes,MAP_W,MAP_H))return 0;
    W->surface=bank+SURFACE_HEADER;
    W->surface_flags=(W->surface_flags&SURFACE_GRID_UPPER)|surface_profile(W->surface,(u16)MAP_W*MAP_H)|(bank[3]=='2'?SURFACE_TERRAIN:0);
    if(W->surface_flags&SURFACE_GRID_UPPER)W->surface_flags|=SURFACE_ROOM;
    W->ej=255;movement_profile();
    return 1;
}
#endif
#endif

// started by spread_start, carried on by spread_go a number of tiles at a time (the AI's spread
// over the map takes several frames); 1 once done
static void spread_start(u8 who, u8 c, u8 limit, u8 *dist, u8 *from, u8 out, const u8 *need, u16 left)
{
    const Unit *un = &st.unit[who];
    u8 *blk = W->blk, i, foe = un->team == TEAM_ENEMY, jump=un->jump;
#ifdef FFT_CHAPTER
    if(W->surface)jump=step_jump_cap(jump);
    if(core_status_current(&un->statuses,2)&64)out|=CORE_NAV_FLOAT;
#endif
#ifdef FFT_CHAPTER
    if (!W->move_kind&&W->ej!=jump) edges(jump);
#else
    if(W->ej!=jump)edges(jump);
#endif
    memset(blk, 0, MAP_N);
    for (i = 0; i < NUNIT; i++) {
        const Unit *u = &st.unit[i];
        if (!DC_OBJ(u->dc) && (u->team == TEAM_ENEMY) != foe) blk[UNIT_CELL(u)] = 1;
    }
    // Resident callers use validated map IDs, generated edges and bounded
    // Move/AI limits; keep repeated parameter validation outside this hot path.
#ifdef FFT_CHAPTER
    if(W->move_kind){
        if(W->move_kind>=2){movement_direct(who,c,limit,dist,from,out);return;}
        out|=CORE_NAV_CUSTOM|CORE_NAV_WIDE;
    }
#endif
    core_nav_begin(&W->nav,MAP_N,c,limit,dist,from,blk,out,need,left);
}

static u8 spread_go(u16 n)
{
    // This build binds rows to Work at compile time; no dynamic graph view is needed.
    return core_nav_step(&W->nav,RT_NULL,n);
}

static void bfs(u8 who, u8 *from)
{
#if defined(FFT_CHAPTER) && !defined(FFT_MODULES)
    movement_ready(who);
#endif
    const Unit *un = &st.unit[who];
    spread_start(who, (u8)(UNIT_CELL(un)), un->move, st.reach, from, 0, RT_NULL, 0);
    spread_go(0xFFFF);
}

#ifdef FFT_MODULES
void compute_reach(u8 who){ModCtx *ctx;movement_ready(who);mod_call(MOD_MOVEMENT,255,0);ctx=mod_context(MOD_MOVEMENT);ctx->cast_actor=who;mod_call(MOD_MOVEMENT,8,0);}
#else
#include "reach_menu.inc"
#endif

// the path to (tx, tz) from the steps of who's reach (made again unless compute_reach made them:
// a spread costs ~90k)
#ifndef FFT_PRODUCTION_CORE
u8 make_path(u8 who, u8 tx, u8 tz)
{
    u16 cell=node_at(tx,tz,0);
    if(cell>=MAP_N){st.path_n=0;return 0;}
    return make_path_cell(who,(u8)cell);
}
#endif
#ifdef FFT_MODULES
u8 make_path_cell(u8 who,u8 c)
{
    ModCtx *ctx;movement_ready(who);mod_call(MOD_MOVEMENT,255,0);ctx=mod_context(MOD_MOVEMENT);
    ctx->cast_actor=who;ctx->cast_ref=c;return (u8)mod_call(MOD_MOVEMENT,7,0);
}
#else
#include "path_menu.inc"
#endif

// ---------------------------------------------------------------- attack
// FFT's Attack with the battle's weapons (RE_NOTES.md § Attack: read in the decompilation,
// checked on the original's previews and attacks). A sword or knife strikes the four tiles
// around the unit whose standing height is at most 3 below or 2.5 above its own; the target
// facing the attacker evades with its class evade (no shields, accessories or Weapon Guard
// at Gariland), from the side or the back it is always hit. XA: PA (a sword, bare hands) or
// (PA + Speed) / 2 (a knife), changed by the signs' compatibility, then on a critical hit
// (4 %) XA + 0..XA - 1 and the target pushed a tile away; damage XA * WP (bare hands: PA *
// Brave / 100), at most 999.
#ifdef FFT_MODULES
#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
void attack_range(u8 who){st.formula.actor=who;mod_call(MOD_TURN,2,0);}
#endif
#else
#include "attack_range.inc"
#endif

// from the target: the attack's dominant axis against its facing (a tie: either axis)
u8 attack_side(u8 a, u8 t)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 dx = (s16)ua->x - ut->x, dz = (s16)ua->z - ut->z;
    s16 ax=dx<0?-dx:dx,az=dz<0?-dz:dz,d;
    if(!ax&&!az)return SIDE_BACK;
    if(ut->face<2){if(ax<az)return SIDE_SIDE;d=dx;}
    else{if(az<ax)return SIDE_SIDE;d=dz;}
    if(ut->face&1)d=-d;
    return d>0?SIDE_FRONT:SIDE_BACK;
}

// from the front the target evades with its class evade, from the front or a side with its
// shield's (SHIELD_EV); Defending halves the base hit (100): the chance (base - evade) * 100
// / base, times (base - shield) / base
u8 hit_chance(u8 a,u8 t)
{
#ifdef FFT_CHAPTER
    return magic_hit(a,t,NO_ABIL,0);
#else

#ifdef FFT_CHAPTER
    if(!unit_can_act(&st.unit[a]))return 0;
    if(unit_petrified(&st.unit[t])||unit_airborne(&st.unit[t]))return 0;
    if((st.unit[t].status_flags&STATUS_NO_EVADE)||unit_transparent(&st.unit[a]))return 100;
#endif
    const Unit *ut = &st.unit[t];
    u8 side = attack_side(a, t), base = unit_physical_base(&st.unit[a],ut), ev = side == SIDE_FRONT ? jobs[ut->job].cev : 0;
    u16 hit;
    if (ev > base) ev = base;
    hit = (u16)((base - ev) * 100 / base);
#ifdef FFT_CHAPTER
    if (side != SIDE_BACK && (ev = SHIELD_EV(ut)) != 0) {
        if (ev > base) ev = base;
        hit = (u16)(hit * (base - ev) / base);
    }
#endif
    return (u8)hit;
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

static u16 weapon_amount(u8 a,u8 t,u8 weapon,u8 crit,u16 r,u8 awake,u8 charge,u16 base_roll)
{
    const Unit *ua=&st.unit[a],*ut=&st.unit[t];
    const Item *w=weapon!=255?&items[weapon]:RT_NULL;
    u8 jump=charge==255;
    s16 xa,ya;
#ifdef FFT_CHAPTER
    xa=jump?ua->pa:weapon_plain_gun(weapon)?w->wp:
        weapon_base_xa(weapon==255?0:cj_equipment_item(common,weapon)[0],ua,base_roll&32767);
#else
    xa=w&&w->knife&&!jump?(ua->pa+ua->sp)>>1:ua->pa;
#endif
    u16 d;
    if(jump){if((u8)(weapon-99)<8)xa+=xa>>1;}else{if(awake&2)xa=zodiac_xa(ua,ut,xa);xa+=charge;if(base_roll&32768)xa=(s16)(muls16(xa,5)>>2);}
#ifdef FFT_CHAPTER
    // Source support stage, before Berserk/Frog and target status/zodiac.
    if(!jump){
        if(core_two_hands(common,ua->job,st.support[a],ua->eq,weapon))xa<<=1;
        if(st.support[a]==SUPPORT_ATTACK_UP)xa=(s16)(muls16(xa<<2,21846)>>16);
        if(weapon==255&&st.support[a]==SUPPORT_MARTIAL_ARTS)xa+=xa>>1;
    }
    if(core_status_current(&ua->statuses,2)&8)xa+=xa>>1;
    if(UNIT_FROG(ua))xa=1;
    if(st.support[t]==SUPPORT_DEFENSE_UP)xa=(s16)(muls16(xa<<1,21846)>>16);
    xa=unit_protect_xa(ut,xa);
    if((core_status_current(&ut->statuses,0)&8)||
       ((core_status_current(&ut->statuses,4)&0x10)&&(!(awake&1)||(ut->statuses.innate[4]&0x10))))xa+=xa>>1;
    if(core_status_current(&ut->statuses,2)&6)xa+=xa>>1;   // Chicken or Frog
#endif
    if(!jump||(ua->zodiac!=12&&ut->zodiac!=12))xa=zodiac_xa(ua,ut,xa);
    if (crit) xa += (s16)(muls16((s16)r,xa)>>15);
    ya = w ? w->wp : (s16)((u16)(ua->pa * ua->brave) / 100);
    if (ya < 1) ya = 1;
    d = (u16)(xa * ya);
    return d;
}

static u16 __attribute__((noinline)) attack_weapon_damage(u8 a,u8 t,u8 weapon,u8 crit,u16 r,u8 awake,u8 charge)
{
    u16 n=weapon_amount(a,t,weapon,crit,r,awake,charge,16384);return charge&&charge!=255?n:n>999?999:n;
}

#ifdef FFT_CHAPTER
#include "magic_math.inc"
#include "reaction_live.h"
#include "magic_evasion.inc"
#ifdef FFT_MODULES
#include "reaction_live.h"
static u16 target_query(u8 who,u16 criterion){
    if(criterion&32768U)return criterion<32768U+NABIL?magic_applicable(who,criterion&32767U):0;
    return criterion==255?magic_target(who):criterion==254?reaction_live(who):criterion==253?reaction_critical_ready(who):criterion==252?jump_limits(who):(u16)TARGETS_INVALID;
}
#endif
#endif
#if !defined(__m68k__) || !defined(FFT_PRODUCTION_CORE) || defined(FFT_WEAPON_TEST_API)
u16 attack_damage(u8 a,u8 t,u8 crit,u16 r)
{
    return attack_weapon_damage(a,t,
#ifdef FFT_CHAPTER
        current_weapon(a),
#else
        st.unit[a].eq[EQ_WEAPON],
#endif
        crit,r,0,0);
}
#endif
#ifdef FFT_CHAPTER
// Punch Art strikes 0x31/0x32 (CORE_PUNCH_ART.md): PA, Attack UP, Martial Arts,
// target statuses, zodiac. roll bit 15: execution, low bits the critical draw (0
// none); otherwise roll draws Repeating Fist's hits.
static u16 punch_strike(u8 a,u8 t,AbId ab,u16 r)
{
    const Unit *ua=&st.unit[a],*ut=&st.unit[t];const Ability *b=&abilities[ab];
    s16 xa=ua->pa,ya=(s16)((u16)(ua->pa+b->y)>>1);u16 k;
    // Steal Exp 0x28 (CORE_STEAL_EXP): the hit percent, Speed through the physical stages +
    // zodiac(X) (X in the record's element byte), the Repeating Fist sum with a multiplier of 1.
    // Stat breaks 0x2b (CORE_BREAKS): PA and Y (element byte) the same way. Both apply Martial
    // Arts bare-handed only.
    if((u8)(b->formula-0x26)<=2)xa=ua->sp;
    // Equipment breaks 0x25: Y + the attacker's WP, which the caller passes as r (X = 0: a
    // multiplier of 1 below; Steal Exp and stat breaks pass 0).
    if(b->formula<0x31)ya=b->element+r;
    if(b->formula>=0x31)xa=magic_xa(a,t,b->element,1,b->formula);
    else{
        if(st.support[a]==SUPPORT_ATTACK_UP)xa=magic_third(xa<<2);
        if(st.support[a]==SUPPORT_MARTIAL_ARTS&&hand_weapon(ua,0)==255)xa+=xa>>1;
        xa=unit_status_xa(ut,xa);
        if(core_status_current(&ut->statuses,2)&6)xa+=xa>>1;
        xa=zodiac_xa(ua,ut,xa);
    }
    if(r&0x8000)xa+=(s16)(muls16((s16)(r&0x7fff),xa)>>15);
    if(b->formula==0x31)k=(u16)(xa*ya);
    else{
        k=(u16)(xa+zodiac_xa(ua,ut,ya));
        if(!(r&0x8000))k*=(u16)((muls16((s16)r,cj_range(common,ab)[9])>>15)+1);
    }
    if(b->formula==0x31&&b->element){s16 n=magic_element_value(t,b->element,(s16)k);return n>999?999:(u16)n;}
    return k>999?999:k;
}
u16 charge_query(u8 a,u8 t,AbId ab,u8 flags,u16 roll)
{
    if(jump_ability(ab))return jump_reaches(a,t,flags);
    u8 f=(u8)(abilities[ab].formula-0x25);  // 0x25/26 equipment, 0x28 Steal Exp, 0x2b/2c breaks, 0x31/32
    if(f<=13&&(0x30CFU>>f)&1)return punch_strike(a,t,ab,roll);
    return attack_weapon_damage(a,t,hand_weapon(&st.unit[a],flags&1),
        (flags>>2)&1,roll,(flags>>1)&1,abilities[ab].y);
}
#ifdef FFT_MODULES
#if !defined(FFT_PRODUCTION_CORE) || defined(FFT_WEAPON_TEST_API)
u16 attack_prediction(u8 a,u8 t)
{
    ModCtx *m;mod_call(MOD_WEAPON,255,0);m=mod_context(MOD_WEAPON);m->cast_actor=a;m->cast_ref=t;
    return mod_call(MOD_WEAPON,4,0);
}
#endif
#else
#include "weapon_effects.inc"
#endif

#else
#define attack_prediction(a,t) attack_damage(a,t,0,0)
#endif

// the evades rolled, each source in turn (RE_NOTES.md § Attack): a miss when rand() * base /
// 32768 < the evade (base 100, 50 Defending); the class evade from the front, then a
// shield's from the front or a side (none: no roll)
static u8 roll_hit(u8 a,u8 t)
{
#ifdef FFT_CHAPTER
    return unit_can_act(&st.unit[a])&&magic_hit(a,t,NO_ABIL,1);
#else

#ifdef FFT_CHAPTER
    if(!unit_can_act(&st.unit[a]))return 0;
    if(unit_transparent(&st.unit[a])){u8 n=4;do{rand15();}while(--n);return 1;}
    if(st.unit[t].status_flags&STATUS_NO_EVADE)return 1;
#endif
    const Unit *ut = &st.unit[t];
    u8 side = attack_side(a, t), base = unit_physical_base(&st.unit[a],ut), ev = side == SIDE_FRONT ? jobs[ut->job].cev : 0;
    if (((u32)rand15() * base >> 15) < ev) return 0;
#ifdef FFT_CHAPTER
    if (side != SIDE_BACK && (ev = SHIELD_EV(ut)) != 0) return (u8)(((u32)rand15() * base >> 15) >= ev);
#endif
    return 1;
#endif
}

// the act starts: the user turns to its target (M_ACT)
#ifdef FFT_MODULES
static void act_start(u8 a,u8 t)
{
    st.formula.actor=a;st.formula.target=t;mod_call(MOD_ACTION,1,0);
}
#else
#include "action_start.inc"
#endif
#ifdef FFT_CHAPTER

#endif

// knockback: one tile away along the dominant axis (a tie: z), onto a free targetable tile.
// Source-bound chapters select the highest eligible floor using facing edges/headroom.
// Legacy unbound fixtures retain their dry same-floor rule. A fall of more
// than Jump levels costs (levels - Jump) * max HP / 10
static void knockback(u8 a, u8 t)
{
    Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 dx = (s16)ut->x - ua->x, dz = (s16)ut->z - ua->z, kx = ut->x, kz = ut->z;
    const Tile *s = &map_tiles[UNIT_CELL(ut)], *n;
    u16 cell,drop=0;u8 source_bound=0;
    if (st.dmg >= ut->hp || (!dx && !dz)) return;
    if ((dx < 0 ? -dx : dx) > (dz < 0 ? -dz : dz)) kx += dx > 0 ? 1 : -1;
    else kz += dz > 0 ? 1 : -1;
#ifdef FFT_CHAPTER
    if(W->surface) {
        u8 side,selected;const u8 *src,*dst;
        if((u16)kx>=MAP_W||(u16)kz>=MAP_H)return;
        side=kx>ut->x?0:kx<ut->x?2:kz>ut->z?6:4;
        src=W->surface+((u16)(W->grid_row[ut->z]+ut->x)<<3)+4*node_layer(UNIT_CELL(ut));
        dst=W->surface+((u16)(W->grid_row[kz]+kx)<<3);
        selected=W->surface_flags&(SURFACE_ROOM|SURFACE_GRID_UPPER)?knock_layer(src,dst,side,&drop):knock_lower(src,dst,side,&drop);
        if(!selected)return;
        cell=node_at(kx,kz,(u8)(selected-1));source_bound=1;
    } else
#endif
    {
        cell=node_at(kx,kz,node_layer(UNIT_CELL(ut)));
        if(cell>=MAP_N)cell=node_at(kx,kz,0);
    }
    n=cell<MAP_N?&map_tiles[cell]:RT_NULL;
    if (n && n->walk == 1 && (source_bound||(!n->water&&n->stand<=s->stand)) && !unit_at_cell((u8)cell)) {
        s16 fall = (source_bound?(s16)(drop>>1):(s16)((s->stand-n->stand)>>1)) - ut->jump;
        st.push = 1; st.kx = (u8)kx; st.kz = (u8)kz;
#ifdef FFT_CHAPTER
        st.push_cell=(u8)cell;
#endif
        if (fall > 0) st.dmg += (u16)((u32)fall * ut->max_hp / 10);
        if (st.dmg > 999) st.dmg = 999;
    }
}

// the hit rolled, then the critical (4 %: XA + 0..XA - 1 and a knockback) and the damage
#if !defined(FFT_MODULES)||!defined(FFT_PRODUCTION_CORE)||defined(RT_CYCLES)
#include "weapon_roll.inc"
#endif

#ifdef FFT_CHAPTER
// Engine entry for consecutive ordinary strikes; no equipment/passive producer
// selects two yet. Those producers must supply independent hand/formula policy.
#ifdef FFT_MODULES
// Pure line query may load AI. Complete it before either effect owner executes.
static void hamedo_line(u8 a,u8 t)
{
    if(a<NUNIT&&t<NUNIT&&st.reaction[t]==REACTION_HAMEDO&&WEAPON_REACH(&st.unit[t])>1){
        u8 actor=st.formula.actor,target=st.formula.target;
        W->ai_traj=battle_line_clear(t,a);
        st.formula.actor=actor;st.formula.target=target;
    }
}
u8 attack_sequence(u8 a,u8 t,u8 count)
{
    ModCtx *c;hamedo_line(a,t);mod_call(MOD_RESPONSE,255,0);c=mod_context(MOD_RESPONSE);
    c->cast_actor=a;c->cast_ref=t;return (u8)mod_call(MOD_RESPONSE,4,count);
}
#else
#include "hamedo.inc"
#include "attack_start.inc"
#endif
void attack(u8 a,u8 t){if(a<NUNIT)attack_sequence(a,t,hand_count(a));}
#else
void attack(u8 a, u8 t)
{
    act_start(a, t);
    st.abil = NO_ABIL;
    attack_roll(a,t);
}
#endif

// ---------------------------------------------------------------- abilities
// FFT's abilities at Gariland (RE_NOTES.md § Abilities: read in the decompilation, checked on
// the original's previews and outcomes): what a unit knows is drawn with it (battle_units)
AbId known(u8 who, u8 k)
{
    const Unit *u = &st.unit[who];
    u8 i,n=0,count=k==255;
#ifdef FFT_CHAPTER
    if(u->set==21&&st.mode!=M_STATUS)return math_known(who,k);
    if(u->set==18&&st.mode!=M_STATUS)return count?1:k?NO_ABIL:JUMP_FIRST;
    if(u->set==20){
        AbId ab;for(ab=512;ab<NABIL;ab++)if(throw_available(who,ab)){
            if(count)n++;else if(!k--)return ab;
        }
        return count?n:NO_ABIL;
    }
#endif
    for (i = 0; i < SET_N; i++) if (u->learned & (1U << i)) {
        AbId ab = SET_ABILITY(u->set, i);
#ifdef FFT_CHAPTER
        if(ab&&abilities[ab].kind!=255){if(count)n++;else if(!k--)return ab;}
#else
        if(ab!=NO_ABIL){if(count)n++;else if(!k--)return ab;}
#endif
    }
    return count?n:NO_ABIL;
}

#if MOD_CORE
#include "ability_access.inc"
#endif

// the tiles at most `range` steps away (FFT spreads the range over the map, heights aside),
#ifdef FFT_MODULES
// MOVEMENT owns cold range construction; caller has returned from every image.
#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
void skill_range(u8 who,AbId ab)
{
    st.formula.actor=who;st.formula.ability=ab;mod_call(MOD_MOVEMENT,5,0);
}
#endif
#else
#include "skill_range.inc"
#endif

// Throw Stone is evaded as an attack; Dash, Wish and the items always land; a cure needs one of
// its statuses on the target, Phoenix Down a KO target


// AB_DAMAGE: (r * Y / 32768 + 1) * XA, XA the user's PA and the signs (no critical); AB_HEAL:
// Y x 10; AB_REVIVE: r * Y / 32768 + 1; AB_WISH: twice the user's max HP / 5


// rolled and started (M_ACT); an item leaves the bag whatever the outcome; Dash and Throw Stone
// push the target when rand() * its Brave / 32768 < rand() * the user's Brave / 32768
#ifndef FFT_MODULES
#include "command.inc"
#ifdef FFT_CHAPTER
#include "jump_order.inc"
#include "incantation.inc"
#include "performance_lifecycle.inc"
#endif
#endif
#ifdef FFT_CHAPTER
static u8 batch_commit(CastBatch *b);
static u8 batch_status_commit(CastBatch *b);
#include "performance_commit.inc"
#ifndef FFT_PRODUCTION_CORE   // isolated packet API; gameplay uses owned prepare/commit
u8 cast_batch(u8 op,CastBatch *b)
{
    u8 result;
    if(op>CAST_BATCH_COMMIT||!b)return 0;
#ifdef FFT_MODULES
    u8 status=op&&b->ability<512&&b->ability!=109&&b->ability!=116&&magic_status_formula(abilities[b->ability].formula);
        u8 id=op?(b->ability==109||b->ability==116?MOD_PERFORMANCE:status?MOD_STATUS:(b->ability<512&&performance_formula(abilities[b->ability].formula))?MOD_PERFORMANCE:MOD_SAVE):MOD_TARGETS;
    mod_call(id,255,0);mod_context(id)->cast_batch_out=b;
    result=(u8)mod_call(id,status?12:op?7:1,status?1:0);
#else
    result=op?(b->ability<512&&magic_status_formula(abilities[b->ability].formula)?batch_status_commit(b):batch_commit(b)):batch_prepare(b,0);
#endif
    if(result&2)turn_order();
    return result&1;
}
#endif
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u8 cast_forecast(CastBatch *b)
{
#ifdef FFT_MODULES
    mod_call(MOD_TARGETS,255,0);mod_context(MOD_TARGETS)->cast_batch_out=b;
    return (u8)mod_call(MOD_TARGETS,3,0);
#else
    return batch_forecast(b);
#endif
}
#endif
#endif
#ifdef FFT_CHAPTER
// Called only after COMMAND returns. No image may evict its own executing code.
static u8 cast_dispatch(u8 result)
{
    if(result==CAST_EFFECT_DISPATCH){
#ifdef FFT_MODULES
        hamedo_line(st.formula.actor,st.formula.target);
        result=(u8)mod_call(MOD_COMBAT,0,0);
#else
        result=command_effect(st.formula.actor,st.formula.target,st.formula.ability);
#endif
    }
    else if(result==CAST_AREA_DISPATCH){
#ifdef FFT_MODULES
        result=(u8)mod_call(MOD_TARGETS,2,0);
#else
        result=batch_owned_prepare();
#endif
    }
#ifdef FFT_MODULES
    else if(st.mode==M_ACT)mod_call(MOD_ACTION,255,0);
#endif
    return result;
}
u8 cast_order(u8 a,u8 ref,AbId ab,u8 follow)
{
    u8 result;
#ifdef FFT_MODULES
    ModCtx *c;
    u8 id=ab<512&&performance_formula(abilities[ab].formula)?MOD_PERFORMANCE:MOD_COMMAND;
    mod_call(id,255,0);c=mod_context(id);
    c->cast_actor=a;c->cast_ref=ref;c->cast_ability=ab;
    result=(u8)mod_call(id,3,follow);
#else
    result=ab<512&&performance_formula(abilities[ab].formula)?performance_order(a,ref,ab,follow):command_cast(a,ref,ab,follow);
#endif
    result=cast_dispatch(result);
    if(result==2&&st.ai)ai_next();
    return result;
}
#endif
#if defined(FFT_CHAPTER) && defined(FFT_MODULES)
s16 cast_targets(u8 a,u8 ref,AbId ab,u8 follow)
{
    ModCtx *c;mod_call(MOD_TARGETS,255,0);c=mod_context(MOD_TARGETS);
    c->cast_actor=a;c->cast_ref=ref;c->cast_ability=ab;
    return (s16)mod_call(MOD_TARGETS,0,follow);
}
#endif
#ifdef FFT_CHAPTER
static u8 aim(AbId ab);
static void area_control(u8 op)
{
    u8 result;
#ifdef FFT_MODULES
    result=(u8)mod_call(MOD_AREA_UI,op,0);
#else
    result=area_input(op==AREA_BEGIN);
#endif
    if(result==AREA_QUERY){
#ifdef FFT_MODULES
        result=(u8)mod_call(MOD_TARGETS,4,0);
#else
        result=area_preview();
#endif
        if(!result){aim(st.abil);st.deny=16;}
    }else if(result==AREA_SEND){
        AreaUi *q=&W->area_ui;u8 follow=q->forecast.follow==MATH_BIND?MATH_BIND:q->follow_id!=255&&!q->answer;
        if(!cast_order(st.act,follow==MATH_BIND?q->forecast.ref:follow?q->follow_id:q->centre,st.abil,follow))st.deny=16;
    }else if(result==AREA_BACK){if(st.unit[st.act].set==21){st.mode=M_SKILLS;st.skill=0;}else aim(st.abil);}
}
#endif

void skill(u8 a,u8 t,AbId ab)
{
    u8 charged;
#ifdef FFT_CHAPTER
    if(ab<512&&(abilities[ab].flags&AF_CAST)&&!st.open_resolving){cast_order(a,t,ab,1);return;}
#endif
#ifdef FFT_MODULES
    ModCtx *c;mod_call(MOD_COMMAND,255,0);c=mod_context(MOD_COMMAND);
    c->cast_actor=a;c->cast_ref=t;c->cast_ability=ab;
    charged=(u8)mod_call(MOD_COMMAND,0,0);
    if(charged==5){
        mod_call(MOD_RESPONSE,255,0);c=mod_context(MOD_RESPONSE);
        c->cast_actor=a;c->cast_ref=t;c->cast_ability=ab;charged=(u8)mod_call(MOD_RESPONSE,5,0);
    }
    if(charged==CAST_EFFECT_DISPATCH)charged=cast_dispatch(charged);
    if(charged==4)return;
    // COMMAND has returned. Prepare impact code during wind-up, not at ACT_HIT.
    if(st.mode==M_ACT)mod_call(abilities[ab].kind==AB_CLEANSE?MOD_STATUS:MOD_ACTION,255,0);
#else
    charged=command_skill(a,t,ab);
#ifdef FFT_CHAPTER
    if(charged==5)charged=jump_order(a,t,ab);
#endif
#endif
    if(charged==3){attack(a,t);return;}           // an equipment break with nothing to break
    if(charged==2&&st.ai)ai_next();
}
void defend(u8 a)
{
#ifdef FFT_MODULES
    if(!action_available(a,a))return;
    st.formula.actor=a;mod_call(MOD_COMMAND,1,0);
#else
    command_defend(a);
#endif
}

u8 act_kind(void)
{
#ifdef FFT_CHAPTER
    if(st.abil==NO_ABIL||(st.abil<512&&(abilities[st.abil].flags&AF_CHARGE)))return st.effect_kind&EFFECT_KIND_MASK;
    if(st.abil>=512&&st.abil<NABIL)return st.effect_kind&EFFECT_KIND_MASK;
    if(st.effect_kind==AB_CATCH||st.effect_kind==AB_MP_DAMAGE)return st.effect_kind;
    if(st.effect_kind==AB_GOLEM)return AB_GOLEM;   // a Golem cast or guard
    if(st.abil<512&&((abilities[st.abil].flags&AF_CAST)||restore_ability(st.abil)||reaction_typed(st.abil)))return st.effect_kind&EFFECT_KIND_MASK;
#endif
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

#ifdef FFT_MODULES
#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
void rewards(u8 a,u8 t,u8 kill)
{
    st.formula.actor=a;st.formula.target=t;st.formula.roll=kill;
    mod_call(MOD_ACTION,7,0);
}
#endif
#else
#include "rewards.inc"
#ifdef FFT_CHAPTER
#include "equipment_runtime.inc"
#include "combat_response.inc"
#include "nature_runtime.inc"
static void movement_settle(u8 who){movement_benefit(who);}
#endif
#endif

// victory: every enemy KO (or a crystal or a chest); defeat: Ramza a crystal or a chest, or
// every unit of the player's side KO, the guest included (FFT checks the defeat first)
u8 outcome(void)
{
#ifdef FFT_CHAPTER
#ifdef FFT_MODULES
    return (u8)mod_call(MOD_ORDER,1,0);
#else
    return battle_outcome();
#endif
#else
    u8 i, foes = 0, blue = 0;
    const Unit *u;
    for (i = 0, u = st.unit; i < NUNIT; i++, u++)
        if (u->hp) { if (u->team == TEAM_ENEMY) foes++; else blue++; }
    if (DC_OBJ(st.unit[U_RAMZA].dc) || !blue) return END_DEFEAT;
    return foes ? END_NONE : END_VICTORY;
#endif
}

// a crystal gives back all HP and MP (FFT also offers its abilities: not kept); a chest one of
// the dead unit's equipment, drawn (shown: there is no inventory); then the object is gone
#ifdef FFT_MODULES
void pick_up(u8 who,u8 obj){st.formula.actor=who;mod_call(MOD_PARTY,6,obj);}
#else
#include "loot.inc"
#endif

static void end_battle(u8 e)
{
    st.end = e; st.mode = M_END; st.ai = AI_OFF;
#ifdef FFT_CHAPTER
    if(flow_bank&&e==END_VICTORY){prepare_rewards();st.banner=0;st.open_clock=0;return;}
#endif
    bn_start(e == END_VICTORY ? BN_VICTORY : BN_DEFEAT);   // then the results
}

// M_ACT, once per frame: the outcome lands at ACT_HIT (HP lost or healed, statuses removed, the
// target pushed; a KO or a revival: the turn order again), the menu comes back at ACT_END or,
// after the EXP and the JP shown, ACT_GAIN_END (the turn ends if the user fell: Wish)
#ifndef FFT_MODULES
static u8 status_magic_commit(void);
#ifdef FFT_CHAPTER
#include "allegiance.inc"
#endif
#include "action_resolve.inc"
#ifdef FFT_CHAPTER
#include "batch_commit.inc"
#include "status_commit.inc"
#include "weapon_proc.inc"
#define FFT_STATUS_BATCH
#define batch_commit batch_status_commit
#define action_magic_commit status_magic_commit
#include "batch_commit.inc"
#undef action_magic_commit
#undef batch_commit
#undef FFT_STATUS_BATCH
#endif
#endif
#if defined(FFT_CHAPTER) && !defined(FFT_MODULES)
#define FFT_DIRECT_NAME direct_target
#define FFT_DIRECT_PANEL
#include "direct_trajectory.inc"
#undef FFT_DIRECT_PANEL
#undef FFT_DIRECT_NAME
#include "arc_trajectory.inc"
#include "melee_target.inc"
#include "weapon_freeze.inc"
#include "weapon_prepare.inc"
#include "weapon_commit.inc"
#include "weapon_proc.inc"
#endif
#if defined(FFT_CHAPTER) && !defined(FFT_MODULES)
static u8 area_number_prepare(void);
#endif
static u8 reaction_step(void)
{
#ifdef FFT_CHAPTER
    u8 kind;
    if(st.action.phase==CTX_APPLIED){
#ifdef FFT_MODULES
        mod_call(MOD_RESPONSE,2,0);
#else
        reaction_return();
#endif
    }
    while(st.action.pending){
#ifdef FFT_MODULES
        kind=(u8)mod_call(MOD_RESULTS,3,0);
#else
        kind=reaction_select();
#endif
        if(!kind)return 0;
#ifdef FFT_MODULES
        mod_call(kind==7||kind==10?MOD_TARGETS:kind==9||kind==6?MOD_EQUIPMENT:MOD_RESPONSE,
            kind==10?12:kind==7?11:kind==9?6:kind==6?7:1,kind);
#else
        if(kind==7||kind==10)reaction_area_launch(kind==10);else if(kind==9)combat_response_launch();else reaction_launch(kind);
#endif
        return 1;
    }
#endif
    return 0;
}
#if defined(FFT_CHAPTER) && !defined(FFT_MODULES)
#include "mime_runtime.inc"
#endif
static void attack_step(void)
{
#ifdef FFT_CHAPTER
    if(st.action.phase==CTX_IDLE)goto action_finished;
#endif
#ifdef FFT_CHAPTER
    if(area_impact_root()&&st.act_t<ACT_HIT&&W->area_impact.next<=NUNIT){
#ifdef FFT_MODULES
        mod_call(MOD_AREA_UI,AREA_NUMBERS,0);
#else
        area_number_prepare();
#endif
    }
#endif
    if (st.act_t == ACT_HIT) {
#ifdef FFT_MODULES
        u8 area,id=MOD_ACTION,op=0;
#ifdef FFT_REACH_CONTOUR
        if(st.dbg&DBG_GOD)mod_call(MOD_SCREENS,14,0);   // debug_battle.inc
#endif
        area=W->cast.phase==CAST_BATCH_READY;
        if(st.abil==NO_ABIL||(st.abil<512&&(abilities[st.abil].flags&AF_CHARGE))){id=MOD_WEAPON;op=1;}
        else if(st.abil==109||st.abil==116){id=MOD_PERFORMANCE;op=8;}
        else if(st.abil<512&&(magic_status_formula(abilities[st.abil].formula)||abilities[st.abil].kind==AB_CLEANSE||reaction_self(st.abil))){id=MOD_STATUS;op=area?12:13;}
        else if(area){id=performance_formula(abilities[st.abil].formula)?MOD_PERFORMANCE:MOD_SAVE;op=8;}
        if(mod_call(id,op,0))turn_order();
#else
#ifdef FFT_CHAPTER
#ifdef FFT_REACH_CONTOUR
        {void debug_hit_direct(void);if(st.dbg&DBG_GOD)debug_hit_direct();}
#endif
        if(W->cast.phase==CAST_BATCH_READY){
            if((magic_status_formula(abilities[W->cast.ability].formula)?batch_status_commit(&W->cast):batch_commit(&W->cast))&2)turn_order();
        }else
        if (st.abil<512&&(magic_status_formula(abilities[st.abil].formula)||abilities[st.abil].kind==AB_CLEANSE||reaction_self(st.abil))?status_scalar_commit():action_commit()) turn_order();
#else
        if(action_commit())turn_order();
#endif
#endif
    }
    if (++st.act_t >= (st.gain ? ACT_GAIN_END : ACT_END)) {
#ifdef FFT_CHAPTER
        if((st.action.phase==CTX_IMPACT||st.action.phase==CTX_APPLIED)&&st.action.left){
            u8 target;
#ifdef FFT_MODULES
            target=(u8)mod_call(MOD_PLANNER,5,0);
#else
            target=strike_target();
#endif
            // Trajectory may load AI, only after ACTION has returned.
            if(target){
#ifdef FFT_MODULES
                mod_call(MOD_RESPONSE,7,target-1);
#else
                strike_launch(target-1);
#endif
                if((st.action.phase&254)!=CTX_IMPACT)return;
            }
        }
        if(st.action.phase>=CTX_SECONDARY||st.action.pending)if(reaction_step())return;
        if(st.primary.reward){
#ifdef FFT_MODULES
            mod_call(MOD_RESPONSE,3,0);
#else
            reaction_rewards();
#endif
            if(st.gain)return;
        }
        if(!core_action_close(&st.action)){st.act_t--;return;}
action_finished:
        W->cast.phase=CAST_BATCH_EMPTY;
        if(W->mime.active||W->mime.pending){
            u8 result;
#ifdef FFT_MODULES
            result=(u8)mod_call(MOD_NATURE,9,0);
#else
            result=mime_next();
#endif
            if(result){
                if(result==7){
                    u8 a=st.formula.actor,t=st.formula.target;
                    attack(a,t);
                }else{
                    u8 resolving=st.open_resolving;
                    cast_dispatch(result);st.open_resolving=resolving;
                }
                return;
            }
        }
#endif
        if (outcome()) { end_battle(outcome()); return; }
#ifdef FFT_CHAPTER
        if (st.open_resolving) { st.open_resolving = 0; turn_start(); return; }
#endif
        if (!st.unit[st.act].hp) { turn_end(st.moved, 1); return; }
        st.mode = M_MENU; st.menu = st.moved ? MN_WAIT : MN_MOVE;
        cursor_place(UNIT_CELL(&st.unit[st.act]));
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
#define AI_UNREACHED 125                // FFT's steps to a tile its spread does not reach
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
#if defined(FFT_CHAPTER) && MOD_PART(MOD_PARTY)
// A chapter's cold grid tables after its topology reservation (PARTY10 on the TI).
#if MOD_CORE
static void setup_glyphs(void);
#endif
static void chapter_grid(void)
{
    u16 i;
#if !MOD_CORE
    *M.chapter_story_after=flow_bank&&CD(LOCATION)==7;
    if(CD(MAP_SCRIPT))for(i=0;i<cdesc[CD(MAP_SCRIPT)+1];i++)*M.chapter_story_after|=cdesc[CD(MAP_SCRIPT)+4+8*i]&1;
#endif
    W->surface_flags=(W->surface_flags&SURFACE_TERRAIN)|surface_profile(W->surface,MAP_N);
    W->node_x=W->tx;W->node_z=W->tz;
    grid_tables();
    for (i = 0; i < MAP_N; i++) {
        const Tile *t = &map_tiles[i];
        W->hs[i] = t->stand; W->hl[i] = (u8)(t->stand + (t->water << 1));   // lower()
        W->hw[i] = (u8)((t->walk != 0) | t->water << 1);
        W->top[i] = (u16)(tile_h(t) * 12);
    }
    for(i=0;i<NUNIT;i++)W->cover[i].rot=0xFF;
    W->wrot=0xFF;setup_glyphs();
}
#endif

#define AI_SPREAD 90
#define AI_WORK 12
#if MOD_PART(MOD_AI) || MOD_PART(MOD_AI_SETUP)
// a unit's worth with hp HP and those statuses (Poison -26, Darkness -64: FFT's weights)

// clock ticks before a unit acts (a KO one, a crystal or a chest: never)
#ifdef FFT_CHAPTER
#include "ai_clock.h"
#else
static u16 ai_ticks(const Unit *u)
{
    if (!u->hp || !u->sp
#ifdef FFT_CHAPTER
        ||unit_petrified(u)
#endif
    ) return 0xFFFF;
    return u->ct >= 100 ? 0 : (u16)((u16)(100 - u->ct + u->sp - 1) / u->sp);
}

#endif

// an ability the unit can use now: the enemies' items of their level, the party's in the bag
static u8 ai_usable(u8 a, AbId ab)
{
    const Unit *u = &st.unit[a];
#ifdef FFT_CHAPTER
    if (ab>=NABIL || abilities[ab].kind==255 || !unit_can_act(u) || skill_blocked(a,ab)) return 0;
#endif
#ifdef FFT_CHAPTER
    if(hand_count(a)==2&&(abilities[ab].flags&AF_WEAPON)&&!(abilities[ab].flags&AF_CHARGE))return 0;
#endif
#ifdef FFT_CHAPTER
    if(ab>=512){
        AbId other;if(!throw_available(a,ab))return 0;
        // Strict damage dominance within one element; cheaper/lower id wins ties.
        for(other=512;other<NABIL;other++)if(other!=ab&&abilities[other].element==abilities[ab].element&&
            (abilities[other].y>abilities[ab].y||(abilities[other].y==abilities[ab].y&&other<ab))&&throw_available(a,other))return 0;
        return 1;
    }
    if(abilities[ab].kind==AB_THROW_SELECTOR)return 0;
#endif
    if (!(abilities[ab].flags & AF_ITEM)) return 1;
    return
#ifdef FFT_CHAPTER
        unit_home_team(u)==TEAM_ENEMY
#else
        u->team==TEAM_ENEMY
#endif
        ? abilities[ab].lv<=u->level : st.bag[abilities[ab].bag]!=0;
}

// its range for the AI (an item: at most the unit's Move, as FFT's AI takes it)

// whether ab used from tile o reaches tile c (attack_range and skill_range's rules)

// worth using ab on t (FFT's classify): damage on a living enemy, HP on an ally under half HP,
// a cure on an ally with one of its statuses, a revival on a KO ally
static u8 ai_useful(u8 a, AbId ab, u8 t)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    u8 kind = weapon_order(ab)?weapon_kind(a,t,ab):abilities[ab].kind;
    if (DC_OBJ(ut->dc)) return 0;
#ifdef FFT_CHAPTER
    if(restore_ability(ab)){
        if(!restore_applicable(t,ab))return 0;kind=skill_kind(a,t,ab);
        if(effect_damage(kind))return !blue(ut)!=!blue(ua)&&t!=a;
        if(blue(ut)!=blue(ua))return 0;
        if(kind==AB_REVIVE)return 1;
        if(kind!=AB_MP_HEAL&&(u16)(ut->hp<<1)<ut->max_hp)return 1;
        return (kind==AB_MP_HEAL||kind==AB_HP_MP_HEAL||kind==AB_HP_DAMAGE_MP_HEAL)&&restore_magic_user(t)&&(u16)(ut->mp<<1)<ut->max_mp;
    }
    if(ab!=NO_ABIL&&abilities[ab].kind==AB_CLEANSE)return magic_applicable(t,ab)&&
        cleanse_ai_gain(t,ab,magic_heal(a,t,ab,0),blue(ut)==blue(ua))>0;
    if(unit_petrified(ut))return 0;
    if(ab!=NO_ABIL&&(magic_formula(abilities[ab].formula)||abilities[ab].formula==99))return magic_applicable(t,ab)&&skill_kind(a,t,ab)!=AB_NULL;
#endif
    if (kind == AB_DAMAGE) return ut->hp && blue(ut) != blue(ua) && t != a;
    if (blue(ut) != blue(ua)) return 0;
    if (kind == AB_REVIVE) return !ut->hp;
    if (!ut->hp) return 0;
    if (kind == AB_CURE) return status_curable(ut,abilities[ab].y) != 0;
    return (u16)(ut->hp << 1) < ut->max_hp;
}

// a unit that can give HP to its allies (FFT's CAN_HEAL_ALLIES: Potion, Wish)
static u8 ai_heals(u8 i)
{
    u8 k; AbId ab;
    for (k = 0; (ab = known(i, k)) != NO_ABIL; k++)
        if ((abilities[ab].kind == AB_HEAL || abilities[ab].kind == AB_WISH || abilities[ab].kind == AB_HP_MP_HEAL) && ai_usable(i, ab)) return 1;
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
#ifdef FFT_CHAPTER
        if(unit_hidden_enemy(ua,u))continue;
#endif
        if (mode == NT_HEALER && u->dc == DC_CRYSTAL) ;     // a crystal heals too
        else if (!u->hp) continue;
        else if (mode == NT_HEALER) { if (blue(u) != blue(ua) || !ai_heals(i)) continue; }
        else if (blue(u) == blue(ua)) continue;
        d = (u8)((dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz));
        if (mode == NT_IMMINENT && (ai_ticks(u) > lim || (d < 2 &&
            (s16)tile_h(&map_tiles[UNIT_CELL(u)]) - tile_h(&map_tiles[UNIT_CELL(ua)]) < -2))) continue;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

// the Move points from tile c over the map by unit a's moves (FFT's spread from its focus), as
// far as the tiles it can stop on
static void ai_flood(u8 a, u8 c)
{
    u16 i, n = 0;
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
        if (!DC_OBJ(o->dc) || o->dc == DC_GONE || UNIT_CELL(o) != c) continue;
        if (o->dc == DC_CRYSTAL) g += worth(u, u->max_hp, u->status) - worth(u, u->hp, u->status);
        if (!(u->hp && (u16)(u->hp * 5) <= u->max_hp) && !W->ai_retreat) g += 2;
    }
    return g;
}

// FFT's choice of a tile among those of set bit: the lowest gain-and-danger key, then the
// fewest steps from the focus (dist_first: the fewest steps, then the key); a tie: the later
static u16 ai_select(u8 bit, s16 maxd, u8 dist_first)
{
    s16 best = 0x7FFF, bd = 0x7FFF;
    u16 c, r = 256;
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
    u16 c = ai_select(bit, maxd, 0);
    return (u8)(c < MAP_N ? c : ai_select(bit, 0x7FFF, 1));
}

// FFT's direct trajectory (battle_effect_check_direct_trajectory_between_units: Throw Stone, an
// item thrown 2 tiles or more): a straight line from the user's body to the target's (two
// thirds of a unit's 36-unit height above its feet; a KO target: 12), a point every 2 units,
// blocked inside a tile's column (to its height) or another standing unit's 16 x 16 x 36 box
// (world units: a tile 28, a height level 12)
#include "direct_trajectory.inc"
#ifdef FFT_CHAPTER
#include "weapon_reflect_score.inc"
#endif

// FFT's score of actor a standing on o (already there) using ab on t: the worth changed, the
// pickup at o; its hit chance (AI_BAD: no use)
static s16 __attribute__((noinline)) ai_score(u8 a, AbId ab, u8 t, u8 o, u8 *hit)
{
    Unit *ua = &st.unit[a], *ut = &st.unit[t];
    u8 isweapon=weapon_order(ab);
    u8 kind = isweapon?weapon_kind(a,t,ab):abilities[ab].kind, status = ut->status;
    u16 hp = ut->hp, m;
    s16 d;
#ifdef FFT_CHAPTER
    if(ab!=NO_ABIL&&((abilities[ab].flags&AF_CAST)||abilities[ab].formula==99||restore_ability(ab)))kind=skill_kind(a,t,ab);
#endif
#ifdef FFT_CHAPTER
    // Conservative guest/enemy action policy, before any evaluation (no copy in the AI image).
    if (st.pending[a].state==DEFER_PERFORMING||unit_airborne(ut)||unit_hidden_enemy(ua,ut))return AI_BAD;
    if (kind == AB_BREAK||kind==AB_STEAL_EXP||kind==AB_EQUIP_BREAK||kind==AB_STEAL_EQUIP||kind==AB_STEAL_GIL) return AI_BAD;
#endif
    *hit = ab == NO_ABIL ? hit_chance(a, t) : skill_hit(a, t, ab);
    if (!*hit) return AI_BAD;
    if (ab != NO_ABIL && abilities[ab].range > 1 && (!(abilities[ab].flags & AF_ITEM)
        || (u8)((ua->x > ut->x ? ua->x - ut->x : ut->x - ua->x) + (ua->z > ut->z ? ua->z - ut->z : ut->z - ua->z)) >= 2)
        && !jump_ability(ab) && !isweapon
        && (W->ai_traj = 1, !ai_clear(a, t))) return AI_BAD;
    if (effect_damage(kind)) {
        m = ab == NO_ABIL ? attack_prediction(a,t) : skill_amount(a,t,ab,
            (u16)(0x4000|((abilities[ab].flags&AF_CHARGE)?FORMULA_FORECAST:0)));
        hp =
#ifdef FFT_CHAPTER
            (isweapon||(ab!=NO_ABIL&&magic_formula(abilities[ab].formula)))?magic_hp(kind,hp,ut->max_hp,m):
#endif
            action_hp(kind,hp,ut->max_hp,m);
    } else if (kind == AB_CURE) status = action_status(kind,status,status_curable(ut,abilities[ab].y));
    else {
        m = ab==NO_ABIL?attack_prediction(a,t):skill_amount(a, t, ab, (u16)(0x4000|((abilities[ab].flags&AF_CHARGE)?FORMULA_FORECAST:0)));
        hp =
#ifdef FFT_CHAPTER
            (isweapon||magic_formula(abilities[ab].formula))?magic_hp(kind,hp,ut->max_hp,m):
#endif
            action_hp(kind,hp,ut->max_hp,m);
    }
#ifdef FFT_CHAPTER
    if(kind==AB_CLEANSE)return cleanse_ai_gain(t,ab,skill_amount(a,t,ab,0),blue(ut)==blue(ua))+W->ai_gain[o];
#endif
    d = worth(ut, hp, status) - worth(ut, ut->hp, ut->status);
#ifdef FFT_CHAPTER
    if(restore_ability(ab)&&(kind==AB_MP_HEAL||kind==AB_HP_MP_HEAL||kind==AB_HP_DAMAGE_MP_HEAL)&&restore_magic_user(t)){
        u16 gain=kind!=AB_MP_HEAL?restore_cap(ut->max_mp):skill_amount(a,t,ab,0),space=ut->max_mp-ut->mp;
        if(gain>space)gain=space;d+=(gain+7)>>3;
    }
#endif
    if (blue(ut) != blue(ua)) d = -d;
#ifdef FFT_CHAPTER
    if (kind == AB_RAISE) return 0; // no unlimited self-buffing in guest AI
    if(ab!=NO_ABIL&&(abilities[ab].flags&AF_CHARGE)
        && W->ai_turn_ticks[t]<=charge_ticks[ab-0x196]) d >>= 1;
#endif
#ifdef FFT_CHAPTER
    if(isweapon){u8 row=ab==NO_ABIL?0:ab-0x195;d+=W->ai_panel_bonus[row][t]+weapon_reflect_score(t,row,o);}
#endif
    if (kind == AB_WISH) {
        m = ua->max_hp / 5;
        d += worth(ua, m >= ua->hp ? 0 : ua->hp - m, ua->status) - worth(ua, ua->hp, ua->status);
    }
    return d + W->ai_gain[o];
}

// a candidate into the ranking (FFT: above the fallback tile's score; then by score, hit
// chance, the origin's danger; a tie: the earlier)


// one candidate: a (standing on o) using ab on unit i of tile t, kept if better than *best;
// whether it could be tried (a hit chance, a clear trajectory)
static u8 ai_try(u8 a, AbId ab, u8 o, u8 t, u8 i, AiAct *best)
{
#ifdef FFT_CHAPTER
    if(weapon_order(ab)&&!(W->ai_melee_mask[o]&((u16)1<<i)))return 0;
#endif
    u8 hit, take;
    s16 p = ai_score(a, ab, i, o, &hit);
    if (p == AI_BAD) return 0;
    take = best->pri == AI_BAD || ai_better(p, hit, o, best);
#ifdef FFT_CHAPTER
    // Confusion ignores worth (its ai_base is AI_BAD): a draw keeps a candidate.
    if (best->pri != AI_BAD && (core_status_current(&st.unit[a].statuses, 1) & 16)) take = rand15() & 1;
#endif
    if (take) {
        best->pri = p; best->hit = hit; best->ab = ab; best->o = o; best->t = t;
    }
    return 1;
}

// ability ab's best candidate into the ranking (only on the actor itself: self). Attack from
// every tile; the others try each target once, from the safest tile that reaches it (an item
// the actor could use on itself: from every tile); an ability that can reach the user also from
// its own tile and the tile it would move to. Started by ai_ability, carried on by
// ai_ability_go a number of candidates at a time (1 once done, its best ranked).
static void ai_ability(u8 a, AbId ab, u8 self)
{
    Unit *ua = &st.unit[a];
    u8 home=UNIT_CELL(ua),me=home,i,n;
    AiAct *best = &W->ab_best;
    W->ab = ab; W->ab_k = 0; W->ab_done = 0; W->ab_n = 0;W->ab_walk=0;
    W->ab_all = ab == NO_ABIL || (abilities[ab].flags & AF_ITEM && ai_useful(a, ab, a));
    best->pri = AI_BAD; best->hit = 0; best->f = 0;
#ifdef FFT_CHAPTER
    if((ua->set==21&&ab==414)||(ab<512&&(abilities[ab].flags&(AF_CAST|AF_AREA))==(AF_CAST|AF_AREA))){W->ai_k=4;return;}
#endif
    W->ai_use = 0;
#ifdef FFT_CHAPTER
    n = !self && (core_status_current(&ua->statuses, 1) & 16);   // Confusion: any other unit
#endif
    for (i = 0; i < NUNIT; i++)
        if ((!self || i == a) && (
#ifdef FFT_CHAPTER
            n ? i != a && st.unit[i].hp && !DC_OBJ(st.unit[i].dc) :
#endif
            ai_useful(a, ab, i))) W->ai_use |= (u16)(1 << i);
#ifdef FFT_CHAPTER
    if (core_status_current(&ua->statuses, 2) & 8) W->ai_use &= (u16)(1 << W->ai_focus);   // Berserk
#endif
    if (!W->ai_use) { W->ab_k = W->ai_no; return; }
    if (ab != NO_ABIL && abilities[ab].flags & AF_SELF && W->ai_use & (1 << a)) {
        if (ai_try(a, ab, W->ai_c1, me, a, best)) best->t = me;  // from its tile, then to the tile
        best->f = best->pri != AI_BAD;                           // it would move to (the same:
        unit_place(ua,W->ai_c1);        // first wins)
        if (ai_try(a, ab, W->ai_c1, W->ai_c1, a, best)) best->f = best->t == me;
        unit_place(ua,home);
        if (best->pri != AI_BAD) ai_insert(best);
        best->pri = AI_BAD; best->f = 0;
    }
    for (i = n = 0; i < NUNIT; i++)             // the targets but itself, in map order
        if (i != a && W->ai_use & (1 << i)) {
            u8 j, c2 = (u8)(UNIT_CELL(&st.unit[i]));
            for (j = n++; j > 0 && W->ai_tt[j - 1] > c2; j--) { W->ai_tt[j] = W->ai_tt[j - 1]; W->ai_tu[j] = W->ai_tu[j - 1]; }
            W->ai_tt[j] = c2; W->ai_tu[j] = i;
        }
    W->ab_n = n;
}

static u8 ai_ability_go(u8 a, s16 *work)
{
    Unit *ua = &st.unit[a];
    AbId ab = W->ab; u8 home=UNIT_CELL(ua),n=W->ab_n,i,c,o;
    AiAct *best = &W->ab_best;
    while(W->ab_k<W->ai_no&&*work>0){
        o = ab == NO_ABIL ? W->ai_grid[W->ab_k] : W->ai_ord[W->ab_k];
        unit_place(ua,o);
        if(!W->ab_walk){W->ab_walk=1;W->ab_j=0;W->ab_self=(u8)(W->ai_use>>a&1);*work-=1;}
        while(*work>0){
            u8 j=W->ab_j;
            if(W->ab_self&&(j==n||o<W->ai_tt[j])){
                W->ab_self=0;*work-=1;
                if (ai_reaches(a, ab, o, o)) { *work -= 1; ai_try(a, ab, o, o, a, best); }
                continue;
            }
            if(j==n){W->ab_k++;W->ab_walk=0;break;}
            W->ab_j++;*work-=1; // rejected pairs also consume their range-query work
            c = W->ai_tt[j]; i = W->ai_tu[j];
            if(W->ab_done&(1<<i))continue;
#ifdef FFT_CHAPTER
            // The frozen geometry pass already checked range and the actual
            // recipient for every weapon origin/slot. Do not repeat that query.
            if(weapon_order(ab)?!(W->ai_melee_mask[o]&((u16)1<<i)):!ai_reaches(a,ab,o,c))continue;
#else
            if(!ai_reaches(a,ab,o,c))continue;
#endif
            W->ai_traj = 0;
            if (ai_try(a, ab, o, c, i, best) && !W->ab_all) W->ab_done |= (u16)(1 << i);
            *work -= weapon_order(ab)?4:W->ai_traj?8:1;
        }
    }
    unit_place(ua,home);
    if (W->ab_k < W->ai_no) return 0;
    if (best->pri != AI_BAD) ai_insert(best);
    return 1;
}

// the setup of unit a's decision (acted: it has acted, no ability left), in two frames: its
// abilities, the tiles it can stop on and their danger, the focus; then the steps from the focus
// and the tiles to act from and to fall back to
static void ai_setup(u8 a, u8 acted)
{
#ifdef FFT_CHAPTER
    u8 clock_i;for(clock_i=0;clock_i<NUNIT;clock_i++)W->ai_turn_ticks[clock_i]=ai_ticks(&st.unit[clock_i]);
#endif
    Unit *u = &st.unit[a];
    AbId ab;
    u16 c;
    u8 me = (u8)(UNIT_CELL(u)), i, k, n = 0, er = 0xFF, ar = 0xFF, ranged = 0, ally = 0, weak = 0, ctl = 0;
    u8 ex[UNIT_CAPACITY], ez[UNIT_CAPACITY], ne = 0;
    u16 lim = ai_ticks(u);
    W->ai_n = 0;
#ifdef FFT_CHAPTER
    W->ai_weapon = WEAPON_REACH(u);
#endif
#ifdef FFT_CHAPTER
    acted|=!unit_can_act(u);
    // Source battle_ai_choose_status_action: a Chicken only flees, a Berserk
    // unit only Attacks (its nearest enemy: ai_ability), Confusion draws.
    ctl=core_status_current(&u->statuses,2);
    if(ctl&4)acted=1;
#endif
    if (!acted) W->ai_list[n++] = NO_ABIL;
    if(!acted&&!(ctl&8)&&u->set==21&&known(a,0)!=NO_ABIL)W->ai_list[n++]=414;
    for (k = 0; !acted && !(ctl & 8) && u->set!=21 && (ab = known(a, k)) != NO_ABIL; k++)
        if (n<1+SET_N&&ai_usable(a, ab)) W->ai_list[n++] = ab;
    W->ai_nab = n;
#ifdef FFT_CHAPTER
    W->ai_weapon_left=n?513:0;W->ai_weapon_at=0;W->ai_melee_at=0;
    for(k=0;k<n;k++)if((u16)(W->ai_list[k]-0x196)<8)W->ai_weapon_left|=(u16)2<<(W->ai_list[k]-0x196);
    W->ai_panel_left=W->ai_weapon_left&511;W->ai_panel_at=0;
#endif
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
    if (!st.moved
#ifdef FFT_CHAPTER
        && unit_can_move(u)
#endif
    ) {
        bfs(a, RT_NULL);
        for (c = 0; c < MAP_N; c++) W->ai_m[c] = st.reach[c] != 0xFF && map_tiles[c].walk == 1;
        for (i = 0; i < NUNIT; i++)
            if (!DC_OBJ(st.unit[i].dc)) W->ai_m[UNIT_CELL(&st.unit[i])] = 0;
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
    W->ai_retreat = (ctl & 4) || ((u16)(u->hp * 5) <= u->max_hp && ally >= 2 && ally != weak && !(u->team == TEAM_ENEMY && st.ai_nosup) && !(ctl & 8));
    memset(W->ai_gain, 0, sizeof W->ai_gain);   // a crystal or a chest it can step on
    for (i = 0; i < NUNIT; i++)
        if (DC_OBJ(st.unit[i].dc) && st.unit[i].dc != DC_GONE) {
            c = (u8)(UNIT_CELL(&st.unit[i]));
            if (W->ai_m[c]) W->ai_gain[c] = ai_gain(a, c);
        }
    for (c = 0; c < MAP_N; c++)                 // FFT's tile priority, then the danger
        W->ai_key[c] = (s16)((s16)-W->ai_gain[c] * 128 + W->ai_prox[c]);   // (under 128)
    // the focus, the steps from it (spread over the next frames)
    if (W->ai_retreat || (blue(u) && !(ctl & 8) && divs32_16((long)u->hp << 7, (short)u->max_hp) < 45)) W->ai_focus = ai_nearest(a, NT_HEALER, 0);
    else W->ai_focus = ai_nearest(a, NT_ENEMY, 0);
    if (W->ai_focus == a) { memset(W->ai_dist, 0, MAP_N); W->nav.done = 1; }
    else ai_flood(a, (u8)(UNIT_CELL(&st.unit[W->ai_focus])));
}

// Merge origins by score, then later node first. Reuse idle map-order scratch;
// no extra allocation and no quadratic shifting on256 equally safe origins.
static void ai_origin_order(void)
{
    u8 *src=W->ai_ord,*dst=order;u16 width,n=W->ai_no;
    for(width=1;width<n;width<<=1) {
        u16 base;
        for(base=0;base<n;base+=width+width) {
            u16 left=base,middle=base+width,end=middle+width,right,out=base;
            if(middle>n)middle=n;if(end>n)end=n;right=middle;
            while(left<middle&&right<end) {
                u8 a=src[left],b=src[right];s16 ka=W->ai_key[a],kb=W->ai_key[b];
                if(ka<kb||(ka==kb&&a>b)){dst[out++]=a;left++;}
                else {dst[out++]=b;right++;}
            }
            while(left<middle)dst[out++]=src[left++];
            while(right<end)dst[out++]=src[right++];
        }
        {u8 *swap=src;src=dst;dst=swap;}
    }
    if(src!=W->ai_ord)memcpy(W->ai_ord,src,n);
}

static void ai_setup2(u8 a, u8 acted)
{
    Unit *u = &st.unit[a];
    u16 i,c;
    u8 me = (u8)(UNIT_CELL(u)), k, f = W->ai_focus, er = W->ai_er, ar = W->ai_ar;
    s16 d, r;
    for (i = 0; i < MAP_N; i++) if (W->ai_dist[i] == 0xFF) W->ai_dist[i] = AI_UNREACHED;
    if (W->ai_retreat) {                        // the safest tile, then the nearest to the focus
        W->ai_c0 = W->ai_c1 = ai_select(1, 0xFF, 0);
        W->ai_m[W->ai_c0] |= 2; W->ai_m[me] |= 2;
    } else {
        if ((u16)(u->hp << 1) < u->max_hp)      // under half HP: the crystals draw it
            for (i = 0; i < NUNIT; i++)
                if (st.unit[i].dc == DC_CRYSTAL) W->ai_dist[UNIT_CELL(&st.unit[i])] = 1;
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
#ifdef FFT_CHAPTER
    if (core_status_current(&u->statuses, 1) & 16) W->ai_base = AI_BAD;   // Confusion: any candidate ranks
#endif
    // the origins: in map order (Attack), the safest first (the others; a tie: the later)
    W->ai_no = 0;
    for (c = 0; c < MAP_N; c++) {
        if (!(W->ai_m[c] & 2)) continue;
        W->ai_grid[W->ai_no]=W->ai_ord[W->ai_no]=(u8)c;
        W->ai_no++;
    }
    ai_origin_order();
}

// the ranking drawn: each action taken if rand() % 100 < its hit chance, else the first; none:
// to the fallback tile, Defending if it can
static void ai_choose(u8 a)
{
    Unit *u = &st.unit[a];
    u8 i, me = (u8)(UNIT_CELL(u));
    const AiAct *r = W->ai_rank;
    for (i = 0; i < W->ai_n && rand15() % 100 >= r[i].hit; i++) ;
    if (i >= W->ai_n) i = 0;
#ifdef FFT_CHAPTER
    if (W->ai_n && (core_status_current(&u->statuses, 1) & 16)) i = (u8)(rand15() % W->ai_n);   // Confusion
#endif
    if (W->ai_n) {
        if(r[i].f&128)st.sub=192|((r[i].f>>1)&15);
        st.ai_ab = r[i].ab; st.ai_o = r[i].f ? me : r[i].o; st.ai_tgt = r[i].t;
    } else {
        st.ai_ab = u->defend && !st.acted
#ifdef FFT_CHAPTER
            && unit_can_act(u) && !(u->status_flags & STATUS_NO_CONTROL) && st.pending[a].state!=DEFER_PERFORMING
#endif
            ? ABIL_DEFEND : AI_NONE;
        st.ai_o = W->ai_c0; st.ai_tgt = W->ai_c0;
    }
    st.ai_first = st.ai_ab != AI_NONE && st.ai_ab != ABIL_DEFEND && st.ai_o == me;
}

// the thinking, a step a frame: the setup's first half; the spread from the focus, AI_SPREAD
// tiles a frame; the setup's second half; the abilities' candidates, AI_WORK a frame (a tile to
// act from 1, a candidate 1, with a trajectory 8, the next ability 5; ~4k cycles a unit); the
// choice. 1 once the plan is made
static u8 ai_candidates(void)
{
    u8 a=st.act,done;s16 w=weapon_order(W->ab)?48:AI_WORK;
    while (W->ai_a < W->ai_nab) {
#ifdef FFT_CHAPTER
        if(W->ai_k>=4)return 0;
#endif
        if (w <= 0) return 0;
        done = ai_ability_go(a, &w);
        if (!done) return 0;
        if (++W->ai_a < W->ai_nab) { ai_ability(a, W->ai_list[W->ai_a], W->ai_self); w -= 5; }
    }
    if (W->ai_n || !W->ai_self || !W->ai_nab) {
#ifndef FFT_MODULE
        ai_choose(a);                           // modules: AI_SETUP5, after this image returns
#endif
        return 1;
    }
    W->ai_self = 0; W->ai_a = 0;
    ai_ability(a, W->ai_list[0], 0);
    return 0;
}
static u8 ai_think(void)
{
    u8 a = st.act, done;
    if (W->ai_k == 0) {                             // its abilities, tiles, danger, focus
#ifdef FFT_CHAPTER
        if(!movement_poll(a))return 0;
#endif
        ai_setup(a, st.acted);
        W->ai_k = 1;
    } else if (W->ai_k == 1) {                      // the steps from the focus
        done = spread_go(AI_SPREAD);
        if (done) W->ai_k = 2;
    } else if (W->ai_k == 2) {                      // the tiles to act from and to fall back to
#ifdef FFT_CHAPTER
        if(W->ai_weapon_left){
            if(W->ai_weapon_left==512){if(W->ai_panel_left)weapon_panel_ai();else melee_cache();}
            else weapon_ai_prepare();return 0;
        }
#endif
        ai_setup2(a, st.acted);
        W->ai_k = 3; W->ai_a = 0; W->ai_self = W->ai_retreat;
        if (W->ai_nab) ai_ability(a, W->ai_list[0], W->ai_self);
#ifdef FFT_CHAPTER
    } else if(W->ai_k>=4){
        if(area_ai_step()){
            W->ai_k=3;
            if(++W->ai_a<W->ai_nab)ai_ability(a,W->ai_list[W->ai_a],W->ai_self);
        }
#endif
    } else {                                        // each usable ability's best candidate
        return ai_candidates();
    }
    return 0;
}

#if MOD_CORE                            // tests' hooks (no modules)
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
void ai_plan(u8 a, u8 acted)
{
    u8 act = st.act, ac = st.acted;
    st.act = a; st.acted = acted; W->ai_k = 0;
    while (!ai_think()) ;
    st.act = act; st.acted = ac;
}
#endif

#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
const AiAct *ai_ranked(u8 *n) { *n = W->ai_n; return W->ai_rank; }
#endif

#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u8 ai_fallback(void) { return W->ai_c0; }
#endif
#endif

// FFT's facing at Wait: towards the nearest enemy that acts before its next turn (else the
// nearest enemy) by quadrant, unless the tile that way is off the map, blocked or 3 higher; else
// the next such enemy; at last its own facing, or the last open side
#if MOD_CORE || MOD_PART(MOD_AI_SETUP)
u8 ai_facing(u8 a)
{
    static const s8 fx[4] = { 0, -1, 0, 1 }, fz[4] = { -1, 0, 1, 0 };   // FFT's S W N E
    static const u8 ours[4] = { 3, 1, 2, 0 };                          // FFT's facing <-> ours
    const Unit *u = &st.unit[a];
    u8 ok[4], d, last = 0, h = tile_h(&map_tiles[UNIT_CELL(u)]), t, any = 0;
    u16 skip = 0;
    for (d = 0; d < 4; d++) {
        u16 cell=node_at(u->x+fx[d],u->z+fz[d],node_layer(UNIT_CELL(u)));
        const Tile *n=cell<MAP_N?&map_tiles[cell]:RT_NULL;
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
#endif

#if !MOD_CORE && MOD_PART(MOD_AI)
u16 mod_entry(u8 op, u8 a)
{
    if (!op) return ai_candidates();
    if(op==4)return ai_clear(st.formula.actor,st.formula.target);
    if (op == 1) { if(W->ai_nab)ai_ability(st.act,W->ai_list[W->ai_a],W->ai_self);return 0; }
    return op == 2 ? skill_hit(st.formula.actor, st.formula.target, st.formula.ability)
                   : skill_amount(st.formula.actor, st.formula.target, st.formula.ability, st.formula.roll);
}
#endif
#if !MOD_CORE && MOD_PART(MOD_AI_SETUP)
#include "ai_camera.inc"
u16 mod_entry(u8 op,u8 a)
{
    if(op==2)return ai_facing(a);
    if(op==3){ai_camera();return 0;}
    if(op==4)return view_hides(a,M.cast_actor);
    a=st.act;
    if(op==5){ai_choose(a);return 0;}
    if(!op){ai_setup(a,st.acted);W->ai_k=1;}
    else {ai_setup2(a,st.acted);W->ai_k=3;W->ai_a=0;W->ai_self=W->ai_retreat;}
    return 0;
}
#endif
#if MOD_CORE
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u8 ai_frozen(void) { return W->frz; }
#endif
#ifdef FFT_MODULES
// Two validated FMB3 archive ranges, one execution zone. Cache record pointers
// once at startup so warm/cold calls need no shard selection or file lookup.
#include "module_bank.h"
#include "campaign/data/modules.h"      // MOD_ZONE: the bank's largest image (ffbm)
static u16 mod_zone[MOD_ZONE / 2];
static const u8 *mod_images[MOD_COUNT];
static ModCtx *mod_context(u8 id)
{
    const u8 *m=mod_images[id];
    return (ModCtx *)((u8 *)mod_zone+RD16(m,4));
}
long __attribute__((__stkparm__)) __divsi3(long a, long b);
unsigned long __attribute__((__stkparm__)) __udivsi3(unsigned long a,unsigned long b);
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
#ifdef FFT_CHAPTER
static const u8 *opening_story, *pgfx;
static void opening_battle(u8 direct);
#endif
static void ui_text(s16 x, s16 y, const char *s, u8 color, s16 limit);
static u8 ui_width(const char *s);
static u8 put_s(char *s, u8 n, const char *t);
static u8 put_n(char *s, u8 n, u16 v);
static void text_into(u16 *l, u16 *d, s16 x, s16 y, const char *s, u8 color);
static void portrait(u8 gfx, s16 x, s16 y);
static void draw_hand(s16 x, s16 y);
static void copy_box(const u16 *l, const u16 *d, u8 x, u8 y, u8 words, u8 rows);
static void gauge(s16 x, s16 y, u16 v, u16 max, u8 color, u8 width);
static void unit_card_battle(u8 i,u8 layout,u8 part,u8 context);
static u8 area_paint(u8 who);

#include "module_defaults.inc"
static u16 mod_call(u8 id, u8 op, u8 a)
{
    const u8 *m = mod_images[id];
    if (mod_id != id + 1) {
        u16 offset=RD16(m,4);
        ModCtx *c=(ModCtx *)((u8 *)mod_zone+offset);
        memcpy(mod_zone,m+10,offset);
        memcpy((u8 *)c+sizeof *c,m+12+offset,RD16(m,3)-offset-sizeof *c);
        memcpy(c,&mod_defaults,sizeof *c);
        c->map_w=map_w;
        c->map_h=map_h;
        c->map_n=map_n;
        c->unit_count=encounter_units;
        c->sc_h=sc_h;
        c->sc_top=sc_top;
        c->state=&st;
        c->w=W;
        c->abilities=abilities;
        c->map_tiles=map_tiles;
        c->contour_gfx=contour_gfx;
        c->cdesc=cdesc;
        c->items=items;
        c->cgfx=cgfx;
        c->jobs=jobs;
        c->unit_defs=unit_defs;
        c->common=common;
        c->charge_ticks=charge_ticks;
        c->igfx=igfx;
        c->flow_bank=flow_bank;
        c->flow_ui=flow_ui;
        mod_id = id + 1;
    }
    if(op==255)return 0;                // reserve255 for preload without execution
    {u16 result=((u16 (*)(u8,u8))((u8 *)mod_zone+RD16(m,2)))(op,a);
     if(W){
         static const struct {u8 mask,id,op;} requests[5]={{4,MOD_PLANNER,1},{3,MOD_NATURE,1},{8,MOD_STATUS,2},{16,MOD_NUMBERS,6},{32,MOD_FORMULA,14}};
         const typeof(requests[0]) *p=requests;u8 n=5;
         do{u8 request=W->nature_pending&p->mask;
             if(request&&(p->mask!=3||id!=MOD_NATURE)){
                 u16 value;W->nature_pending&=(u16)~p->mask;
                 value=mod_call(p->id,request==2||request==3?0:p->op,0);
                 if(p->mask==8)result|=value;
             }
             p++;
         }while(--n);
     }
     if(W&&W->rebuild&&id!=MOD_EQUIPMENT)mod_call(MOD_EQUIPMENT,0,0);
     return result;}
}

// the bank found, each module of this build and within the zone
static u8 mod_ready(void)
{
    u16 asize=0,bsize=0,csize=0,id;
    const u8 *a=rt_file("ffbm",&asize),*b=rt_file("ffbn",&bsize),*c=rt_file("ffbo",&csize);
    mod_id=0;mod_images[MOD_RESOURCES]=RT_NULL; // (statics outlive a run: the RAM message's test)
    if(!module_three_valid(a,asize,b,bsize,c,csize,MOD_BANK0_COUNT,MOD_BANK1_END,MOD_COUNT,sizeof(ModCtx),MOD_SIG,MOD_ZONE))return 0;
    for(id=0;id<MOD_COUNT;id++){
        const u8 *bank=id<MOD_BANK0_COUNT?a:id<MOD_BANK1_END?b:c;
        u16 local=id<MOD_BANK0_COUNT?id:id<MOD_BANK1_END?id-MOD_BANK0_COUNT:id-MOD_BANK1_END;
        mod_images[id]=bank+RD16(bank,7+local);
    }
    return 1;
}

static u8 ai_think(void)
{
    if(W->ai_k==0){if(!movement_poll(st.act))return 0;mod_call(MOD_AI_SETUP,0,0);}
    else if(W->ai_k==1){if(spread_go(AI_SPREAD))W->ai_k=2;}
    else if(W->ai_k==2){if(W->ai_weapon_left){
        u8 geometry=W->ai_weapon_left==512,panels=geometry&&W->ai_panel_left;
        mod_call(geometry?(panels?MOD_AI_PREP:MOD_PLANNER):MOD_WEAPON,geometry?(panels?2:0):6,0);return 0;
    }mod_call(MOD_AI_SETUP,1,0);if(W->ai_nab)mod_call(MOD_AI,1,0);}
    else if(W->ai_k>=4){
        if(W->ai_k==4&&st.unit[st.act].set==21)mod_call(MOD_NATURE,5,0);
        if(mod_call(st.unit[st.act].set==21||W->ai_k==4||W->ai_k==8?MOD_AI_PREP:MOD_AREA_AI,0,0)){
            W->ai_k=3;
            if(++W->ai_a<W->ai_nab)mod_call(MOD_AI,1,0);
        }
    }
    else if(mod_call(MOD_AI,0,0)){mod_call(MOD_AI_SETUP,5,0);return 1;}
    return 0;
}
static void draw_turn(void) { mod_call(MOD_TURN, 0, 0); }
static void draw_end(void) { mod_call(MOD_RESULTS, 0, 0); }
static void draw_status(void) { mod_call(MOD_SCREENS, 1, 0); }
#ifdef FFT_CHAPTER
static u8 map_update(void)
{
    u8 active=(u8)mod_call(MOD_BEATS,0,0);
    if(st.open_stage==OPEN_BEGIN)opening_battle(0);
    return active;
}
static void map_render(void) { mod_call(MOD_BEATS, 1, 0); }
static void map_start(u8 after)
{
    mod_call(MOD_BEATS,2,after);
    if(st.open_stage==OPEN_BEGIN)opening_battle(0);
}
static void save_campaign(void) { mod_call(MOD_SAVE, 0, 0); }
static void prepare_rewards(void) { mod_call(MOD_SAVE, 3, 0); }
static void party_load(void) { mod_call(MOD_PARTY, 1, 0);if(W->nature_units)mod_call(MOD_NATURE,1,0); }
static void funds_load(void) { mod_call(MOD_PARTY, 11, 0); }
#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
void battle_units(void) { mod_call(MOD_PARTY, 2, 0); }
#endif
static void fixed_units(void) { mod_call(MOD_PARTY, 4, 0); }
static s16 math_targets(u8 a,u8 code)
{
 st.formula.actor=a;st.formula.roll=code;return (s16)mod_call(MOD_NATURE,6,0);
}
// Keep a resident AI overlay resident: it already contains the identical evaluator.
// Otherwise load the much smaller formula image. No hot AI/formula copy ping-pong.
// Formulas 0x25..0x2c (breaks, steals, Talk) always use FORMULA: the AI/ACTION copies leave the
// breaks and Steal Exp out (the AI rejects them first; ACTION never evaluates them).
static u16 __attribute__((noinline)) formula_call(u8 op)
{
    u16 ab=st.formula.ability;u8 id=mod_id;
    if(ab<512&&(u8)(abilities[ab].formula-0x25)<=7)id=0;
    if(ab==265||ab==NO_ABIL||(ab<512&&(abilities[ab].flags&AF_CHARGE)))id=0;
    if(id==MOD_AI+1)op+=2;
    else id=MOD_FORMULA+1;
    return mod_call((u8)(id-1),op,0);
}
#ifndef FFT_PRODUCTION_CORE
u8 skill_hit(u8 a, u8 t, AbId ab)
{
    st.formula.actor = a; st.formula.target = t; st.formula.ability = ab;
    return (u8)formula_call(0);
}
u16 skill_amount(u8 a, u8 t, AbId ab, u16 r)
{
    st.formula.actor = a; st.formula.target = t; st.formula.ability = ab; st.formula.roll = r;
    return formula_call(1);
}

#endif
#endif
u8 ai_facing(u8 a) { return (u8)mod_call(MOD_AI_SETUP, 2, a); }
#endif

#ifdef FFT_CHAPTER
// Resident preflight for the equipped weapon's trajectory family; no nested image load.
u8 battle_line_clear(u8 a,u8 t)
{
#ifndef FFT_PRODUCTION_CORE
    if(!W||a>=NUNIT||t>=NUNIT)return 0;
#endif
#ifdef FFT_MODULES
    st.formula.actor=a;st.formula.target=t;
    return (u8)mod_call(MOD_PLANNER,2,0);
#else
    return cj_weapon(common,hand_weapon(&st.unit[a],0))[1]&192?melee_target(a,t)!=255:
        (cj_weapon(common,hand_weapon(&st.unit[a],0))[1]&16?arc_target(a,t)!=255:direct_target(a,t)!=255);
#endif
}
#endif

// ---------------------------------------------------------------- units on screen
// frames of walking step i (to path[i + 1]): FFT's climbs, drops and jumps over a tile are slower
static u8 step_frames(u8 i)
{
    return map_tiles[PATH_CELL(i)].stand!=map_tiles[PATH_CELL(i+1)].stand||
        (u8)(PATH_X(i)-PATH_X(i+1)+1)>2||(u8)(PATH_Z(i)-PATH_Z(i+1)+1)>2?CLIMB_FRAMES:WALK_FRAMES;
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
    const Unit *u = &st.unit[i];
    if (DC_OBJ(u->dc) || u->x >= MAP_W || u->z >= MAP_H) return 0;
    if (st.mode == M_WALK && st.sel == i && st.path_i + 1 < st.path_n) {
        u8 a=st.path_i;
        if (step_frames(st.path_i) == CLIMB_FRAMES && st.walk_t) return 0;
        // On a level step, change terrain at the tile edge.
        if (st.walk_t >= (WALK_FRAMES + 1) / 2 && step_frames(st.path_i) == WALK_FRAMES) a++;
        return map_tiles[PATH_CELL(a)].water;
    }
    return map_tiles[UNIT_CELL(u)].water;
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
    f.y = scene_y(u, v) + HH - map_tiles[UNIT_CELL(un)].stand * (HU / 2);
    f.depth = (u8)(u + v);
    if (st.mode == M_WALK && st.sel == i && st.path_i + 1 < st.path_n) {
        u8 a=st.path_i,b=(u8)(a+1);
        s16 u2, v2, x2, y2, t = st.walk_t, hop = 0, q;
        s16 sa=map_tiles[PATH_CELL(a)].stand,sb=map_tiles[PATH_CELL(b)].stand;
        view_of(rot,PATH_X(a),PATH_Z(a),&u,&v);
        view_of(rot,PATH_X(b),PATH_Z(b),&u2,&v2);
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
static u16 unit_feet_key(u8 i,u8 rot)
{
    return (u16)(UNIT_CELL(&st.unit[i])|(u16)rot<<8|0x8000);
}

// Cover mask of a unit: the pixels in front of it (the view's depth: diagonals greater than
// its own) in its sprite's 16 x COVER_H box, one bit per pixel. Kept per unit (W->cover) and
// made again only when its feet, its depth or the view change: a unit standing still costs nothing.
static void cover(u16 *cm, s16 fx, s16 fy, u8 depth)
{
    s16 bx = fx - 8, by = fy - UNIT_FOOT, b0 = bx >> 3, sh = 8 - (bx & 7), r;
#ifdef FFT_VIEW_SPLIT                           // (inline, as before the ragged depth: 13 units'
    const DepthRow *dr = (const DepthRow *)vbuf + by;   // rows after a turn; dr read only in the scene)
    const u8 *p0 = vbuf, *p1 = p0 + vplane, *p2 = p1 + vplane, *p3 = p2 + vplane;
    for (r = 0; r < COVER_H; r++, dr++) {
        u32 m = 0;
        if ((u16)(by + r) < SC_H) {
            u16 o = (u16)(dr->off + b0), j = (u16)(b0 - dr->x0), w = dr->w;
            u8 k = 3;
            do {
                m <<= 8;
                if (j < w) {
                    if (p0[o] > depth) m |= p1[o];
                    if (p2[o] > depth) m |= p3[o];
                }
                j++; o++;
            } while (--k);
        }
#else
    for (r = 0; r < COVER_H; r++) {
        u32 m = cover_bytes(by + r, b0, depth, 3, RT_NULL);
#endif
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
    if (st.mode != M_ACT || st.abil != NO_ABIL || i != st.atk ||
#ifdef FFT_CHAPTER
        current_weapon(i)==255
#else
        st.unit[i].eq[EQ_WEAPON]==255
#endif
        || st.act_t < ACT_WIND || st.act_t >= BLADE_END) return 0;
    return st.act_t < ACT_BLOW ? 1 : st.act_t == ACT_BLOW ? 2 : st.act_t == ACT_BLOW + 1 ? 3 : 4;
}

static u8 hit_flash(u8 i)
{
    u8 t = (u8)(st.act_t - ACT_HIT);
#ifdef FFT_CHAPTER
    if(area_impact_root()){
        if(W->cast.phase!=CAST_BATCH_APPLIED||!(W->cast.mask&((u16)1<<i))||
           !area_impact_harm(i)||t>=FLASH_FRAMES)return 0;
        return t<2||t>=10?1:2;
    }
#endif
    if (st.mode != M_ACT || i != st.tgt || !st.hit || st.abil == ABIL_DEFEND || !effect_damage(act_kind())
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
    return st.mode == M_ACT ? hit_flash(i) : 0;
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
#ifdef FFT_CHAPTER
    if(area_impact_root()){
        if(i==st.atk&&st.act_t>=ACT_WIND&&st.act_t<ACT_HIT)return P_WIND;
        if(W->cast.phase==CAST_BATCH_APPLIED&&(W->cast.mask&((u16)1<<i))&&area_impact_harm(i)&&
           st.act_t>=ACT_HIT&&st.act_t<ACT_HIT+HIT_FRAMES)return P_HIT;
    }else
#endif
    if (st.mode == M_ACT && st.abil != ABIL_DEFEND) {
        if (i == st.atk && st.act_t >= ACT_WIND && st.act_t < ACT_HIT)   // Throw Stone (the evaded
                                                                         // skill) and items:
            return st.act_t < ACT_BLOW ? P_WIND    // FFT's throw after the same wind-up
                : st.abil != NO_ABIL && abilities[st.abil].flags & (AF_EVADE | AF_ITEM) ? P_THROW : P_BLOW;
        if (i == st.tgt && st.act_t >= ACT_HIT && st.act_t < ACT_HIT + HIT_FRAMES && st.hit
            && effect_damage(act_kind())) return P_HIT;
    }
    if (!un->hp) return P_KO;
#ifdef FFT_CHAPTER
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

#define DC_Y 18                         // a death counter's top above the feet

// A covered body's interior is white, preserving its white/black contour: letting a busy
// roof texture show through made the hollow silhouette look like ordinary sprite detail.
// Covered shadows stay transparent. c RT_NULL draws the ordinary unoccluded sprite.
// On the TI the rows it leaves alone are drawn straight from the data file,
// the others copied first.
// Depth-1 shader: a short, constant-height pass. The ordinary dry shader
// stays unchanged; standing units reuse their cached terrain classification.
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

static void draw_unit_covered(s16 sx, s16 sy, u8 rot, u8 i, const Cover *c)
{
#ifdef FFT_CHAPTER
    if(unit_airborne(&st.unit[i]))return;
#endif
    if ((c ? c->wet : unit_in_water(i)) && !DC_OBJ(st.unit[i].dc)) {
        draw_unit_water(sx, sy, rot, i, c); return;
    }
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
#ifdef FFT_CHAPTER
        GFX_CHEMIST(st.unit[i].gfx)
#else
        st.unit[i].gfx >= UG_CHEMIST_M
#endif
        ? 24 : 0)
        + (items[
#ifdef FFT_CHAPTER
        current_weapon(i)
#else
        st.unit[i].eq[EQ_WEAPON]
#endif
        ].knife ? 0 : 12)
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
#ifdef FFT_CHAPTER
    if (WEAPON_REACH(&st.unit[i]) > 1) return;      // a bow: no blade
#endif
    rows = unit_in_water(i) ? WEAPON_FOOT - HU : WEAPON_SH;
    k = weapon_pose(i, phase);
    g = weapon_frame(k, phase - 2);
    bx = sx + weapon_x[k]; by = sy - WEAPON_FOOT;
    b0 = bx >> 3; sh = bx & 7;
    if (W->wx != bx || W->wy != by || W->wrot != st.rot || W->wdepth != depth) {
        for (r = 0; r < WEAPON_SH; r++) {
            u8 tail = 0;
            u32 cm = cover_bytes(by + r, b0, depth, 5, &tail);
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

#ifdef FFT_MODULES
static void setup_glyphs(void){mod_call(MOD_PARTY,5,0);}
#else
#include "setup_glyphs.inc"
#endif

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
    s16 pvx, pvy, cu, cv, i, x, z;
    u16 n = 0;
    // the pivot (cursor tile centre) stays where it is on screen
    view_of(st.rot, st.cx, st.cz, &cu, &cv);
    pvx = scene_x(st.rot, cu, cv) - st.camx;
    pvy = scene_y(cu, cv) + HH - st.camy;
#ifdef FFT_TURN_LOWPOLY_NATIVE
    memset(rt_light,0,RT_PSIZE); memset(rt_dark,0,RT_PSIZE);
    D.l=rt_light; D.d=rt_dark; D.stride=RT_PBYTES;
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
    for (i = 0; i < MAP_N; i++) {
        s16 k; x=TILE_X[i];z=TILE_Z[i];
        k=(gy[x][z]+gy[x+1][z+1]+TURN_SORT_BIAS)>>TURN_SORT_SHIFT;
        key[i]=k<0?0:k>255?255:k;
    }
    tile_order();n=MAP_N;
    for (i = 0; i < n; i++) {
        u8 o = order[i], f;
#ifndef FFT_TURN_NO_EDGES
        u8 ledges=0;
#endif
        const Tile *t = &map_tiles[o];
        s16 tx=TILE_X[o],tz=TILE_Z[o],cx[4],cy[4],px[4],py[4];
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
        sy = pvy + (s16)((muls16(x2, S) + muls16(z2, C)) >> 9) - map_tiles[UNIT_CELL(un)].stand * (HU / 2);
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
#include "math_menu.inc"
#include "math_aim.inc"
#include "attack_range.inc"
#include "target_pick.inc"
u16 mod_entry(u8 op, u8 a) { if(op==4)return aim_ok(a);if(op==5)return pickable(a,M.cast_ref);if(op==6)return twin(a,M.cast_ref);if(op==1)return math_menu();if(op==2){attack_range(st.formula.actor);return 0;}if(op==3){math_aim();return 0;}draw_turn();return 0; }
#endif
#endif
#if MOD_CORE

// ---------------------------------------------------------------- camera
#include "camera_clamp.h"
#ifdef FFT_MODULES
static void cam_target(s16 *tx,s16 *ty)
{
    s16 target[2];mod_call(MOD_BEATS,255,0);mod_context(MOD_BEATS)->camera_out=target;
    mod_call(MOD_BEATS,4,0);*tx=target[0];*ty=target[1];
}
#else
#include "camera_target.inc"
#endif

static s16 ease(s16 c, s16 t)
{
    s16 d = (t - c) >> 2;
    if (!d) d = t > c ? 1 : t < c ? -1 : 0;
    return c + d;
}

// ---------------------------------------------------------------- game
static u8 *sbuf;


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
#ifdef FFT_CHAPTER
    if(navbuf)free(navbuf);
    navbuf=RT_NULL;nav_capacity=0;
#endif
    if (sbuf) free(sbuf);
    if (vbuf) free(vbuf);
    sbuf = vbuf = RT_NULL;
    scene_l = scene_d = RT_NULL;
}

#ifdef FFT_CHAPTER
// The battle releases its large buffers before the supervisor resumes. Keep
// State alive through that handoff and free it only on actual program exit.
static void
#ifdef __m68k__
__attribute__((__stkparm__))
#endif
final_release(void)
{
    release();
    if(battle_state)free(battle_state);
    battle_state=RT_NULL;rt_state=RT_NULL;rt_state_size=0;
}
// The battle program's chapters (FFT_BATTLE, campaign/ENGINE.md M4): each one's banks are "ff",
// its letter, then v0..v3 (views), a (actor frame keys), i (interface), d (descriptor); every
// chapter shares ffcw, ffca, ffcb, ffcu and the modules (ffbm)
static u8 chapter_now;                          // the chapter loaded + 1 (0: none)
static u8 story_after;                          // its script plays beats after the victory

void game_init(void)
{
#ifndef __m68k__
    if(battle_state)free(battle_state);
#endif
    battle_state=RT_NULL;rt_state=RT_NULL;rt_state_size=0;
    chapter_now = 0; encounter_units = 0; W = RT_NULL; igfx = RT_NULL;
    flow_bank=flow_ui=RT_NULL;
#ifdef __m68k__
    navbuf=RT_NULL;nav_capacity=0;
    sbuf=vbuf=RT_NULL;acache=RT_NULL; // reset before any allocation can fail
#endif
#ifndef FFT_MODULES
    battle_state=malloc(BATTLE_STORAGE_BYTES);
    if(!battle_state)return;
    memset(battle_state,0,BATTLE_STORAGE_BYTES);rt_state=battle_state;rt_state_size=sizeof(State);
#endif
#ifdef __m68k__
    atexit(final_release);
#endif
#ifdef FFT_MODULES
    {
        static const CoreBoot boot={&flow_bank,&flow_ui,&wgfx,abank,&cgfx,&common,&charge_ticks,&job_sets,
                                   &abilities,&jobs,&job_names,&items,
                                   &battle_state,&rt_state,&rt_state_size,&vbuf,&sbuf,&navbuf,&nav_capacity,&acache,&ac_key,&ac_age,
                                   &sl_l,&sl_m,&sc_l,&sc_m,
#ifdef FFT_CAMPAIGN_FLOW
                                   1
#else
                                   0
#endif
        };
        if(!mod_ready())return;
        mod_call(MOD_SCREENS,255,0);mod_context(MOD_SCREENS)->boot=&boot;
        if(!mod_call(MOD_SCREENS,11,0))return;
        mod_id=0;
    }
#else
#ifdef FFT_CAMPAIGN_FLOW
    flow_bank=rt_file("ffflow",RT_NULL);
    if(!qvalid(flow_bank))flow_bank=RT_NULL;
    if(flow_bank)flow_ui=rt_file("ffmg",RT_NULL);
#endif
    wgfx = rt_file("ffcw", RT_NULL);
    actor_banks(abank);
    cgfx = rt_file("ffcu", RT_NULL);
    {                                          // the jobs and items every chapter shares
        u16 cj_size;const u8 *cj = rt_file("ffcj", &cj_size);
#ifdef FFT_MODULES
        // Bootstrap has a static executable zone but no Work allocation yet.
        // SAVE5 validates only its supplied catalogue pointer and byte count.
        if(!mod_ready())return;
        mod_call(MOD_SAVE,255,0);
        mod_context(MOD_SAVE)->common=cj;
        mod_context(MOD_SAVE)->desc_size=cj_size;
        if(!mod_call(MOD_SCREENS,12,0))return;
        mod_id=0; // next load must bind the newly allocated Work/map pointers
#else
        if (!cj_valid(cj,cj_size)) return;
#endif
        common = cj; abilities = (const Ability *)cj_section(cj, CJ_BATTLE);
        charge_ticks = (const u8 *)abilities + CJ_EXECUTABLES * 12U;
        jobs = (const Job *)(cj + RD16(cj, 2)); job_names = (const char *)cj + RD16(cj, 3);
        job_sets = cj + RD16(cj, 4); items = (const Item *)(cj + RD16(cj, 5));
    }
    if (!wgfx || !abank[0] || !abank[1] || !cgfx) return;
#endif
#ifndef FFT_MODULES
    if (!sbuf) {                               // two blocks: AMS allocates at most ~64 KB; the
        vbuf = malloc(VIEW_BYTES);              // the core's depth envelope
        sbuf = vbuf ? malloc(2UL * SC_BYTES * SC_HMAX + sizeof(Work)) : RT_NULL;
        acache = sbuf ? malloc(AC_N * (UNIT_FB + 4)) : RT_NULL;
        if (!acache && sbuf) { free(sbuf); sbuf = RT_NULL; }
        if (!sbuf) {                           // not enough memory: a message, ESC quits
            if (vbuf) free(vbuf);
            vbuf = RT_NULL;
            return;
        }
        ac_key = (u16 *)acache[AC_N]; ac_age = ac_key + AC_N;
#ifdef __m68k__
        // final_release owns these buffers and the longer-lived State.
#else
        (void)release;
#endif
    }
#endif
#ifndef FFT_MODULES
    if (sbuf && !navbuf) {                     // the core's map envelope, once: any map fits
        navbuf = malloc(NAV_ENVELOPE);         // (with modules: bootstrap_files.inc)
        nav_capacity = navbuf ? NAV_ENVELOPE : 0;
    }
#endif
}

// chapter c's banks, sizes and tables (1: ready); its work memory starts afresh
#ifndef FFT_MODULES
#include "chapter_validate.h"
#include "chapter_files.inc"
#endif
static u8 chapter_load(u8 c)
{
    u16 i;
    const u8 *surface;
    W = RT_NULL; igfx = RT_NULL; chapter_now = 0; encounter_units = 0; contour_gfx = RT_NULL;
    if (!sbuf || !cgfx) return 0;
#ifdef FFT_MODULES
    // A cold load resets transient inputs; the image adopts the reset names above.
    mod_id=0; // cdesc/igfx change; never reuse the previous chapter's binding
    i=mod_call(MOD_RESOURCES,10,c);
    {
        const ModCtx *m=mod_context(MOD_RESOURCES);
        contour_gfx=m->contour_gfx; ugfx=m->chapter_ugfx; cdesc=m->cdesc; igfx=m->igfx;
        encounter_units=m->unit_count; map_w=m->map_w; map_h=m->map_h; map_n=m->map_n;
        sc_h=m->sc_h; sc_top=m->sc_top; unit_defs=m->unit_defs; map_tiles=m->map_tiles;
    }
#else
    {char letter=chapter_route(c);i=letter?chapter_files(letter):0;}
#endif
    if(!i){igfx=RT_NULL;return 0;}
    surface=cdesc+i;
#ifndef FFT_MODULES
    story_after = 0;
    if(flow_bank&&CD(LOCATION)==7)story_after=1;
    if (CD(MAP_SCRIPT))
        for (i = 0; i < cdesc[CD(MAP_SCRIPT) + 1]; i++) story_after |= cdesc[CD(MAP_SCRIPT) + 4 + 8 * i] & 1;
#ifdef __m68k__
    sl_l = (const PlaneWord *)UI_SHARED(SLOPES);
#else
    for (i = 0; i < 4 * SLOPE_ROWS; i++) slope_pc[i] = RD32(UI_SHARED(SLOPES), 2 * i);
    sl_l = slope_pc;
#endif
    sl_m = sl_l + SLOPE_ROWS; sc_l = sl_m + SLOPE_ROWS; sc_m = sc_l + SLOPE_ROWS;
#endif
    W = (Work *)(sbuf + 2UL * SC_BYTES * SC_HMAX);
    memset(W, 0, sizeof(Work));
    if(!nav_reserve(MAP_N,2)){W=RT_NULL;igfx=RT_NULL;encounter_units=0;return 0;}
    W->surface=surface+SURFACE_HEADER;
    W->surface_flags=surface[3]=='2'?SURFACE_TERRAIN:0;
#ifdef FFT_MODULES
    mod_id=0;mod_call(MOD_PARTY,255,0);mod_context(MOD_PARTY)->chapter_story_after=&story_after;
    mod_call(MOD_PARTY,10,0);         // rebinds to this chapter's Work/map first
#else
    chapter_grid();
#endif
    movement_profile();
    edges(3);                                  // Gariland's units all have Jump 3
    scene_l = sbuf; scene_d = sbuf + SC_PLANE;
    scene_rot = vbuf_rot = 0xFF;
#ifdef FFT_MODULES
    mod_id = 0;                                // the modules' context: this chapter's
#endif
    chapter_now = (u8)(c + 1);
    return 1;
}
#else
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
        char name[6] = "fftv0";                // again each run, the archive may have moved
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
    ugfx = rt_file("fftu", RT_NULL);
    wgfx = rt_file("fftw", RT_NULL);
    igfx = rt_file("ffti", RT_NULL);
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
        if (!sbuf) {                           // not enough memory: a message, ESC quits
            if (vbuf) free(vbuf);
            vbuf = RT_NULL; W = RT_NULL;
            return;
        }
        W = (Work *)(sbuf + 2 * SC_PLANE);
        memset(W, 0, sizeof(Work));
        for (i = 0; i < MAP_N; i++) {
#ifndef FFT_TABLES_ARCHIVED
            W->tx[i] = (u8)(i % MAP_W); W->tz[i] = (u8)(i / MAP_W);
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
        setup_glyphs();
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

#endif

// a target for the weapon or the ability: a living unit (Phoenix Down: a KO one; unit_at never
// gives a crystal or a chest)
#ifdef FFT_MODULES
static u16 pick_query(u8 op,u8 cell,u8 aiming);
static u8 aim_ok(u8 t){return (u8)pick_query(4,t,0);}
#else
static u8 aim_ok(u8 t);
#endif

#ifdef FFT_MODULES
static u8 aim(AbId ab)
{
    ModCtx *ctx;mod_call(MOD_MOVEMENT,255,0);ctx=mod_context(MOD_MOVEMENT);
    ctx->cast_ability=ab;{u8 result=(u8)mod_call(MOD_MOVEMENT,10,0);if(result){st.formula.actor=st.act;mod_call(MOD_TURN,result,0);}}
    return 0;
}
#else
#ifdef FFT_CHAPTER
#include "math_aim.inc"
#endif
#include "aim_menu.inc"
#endif

#ifdef FFT_CHAPTER
static u8 scenario_prepare(u8 n);
#endif
void game_scenario(u16 n)
{
    u8 bag_i;
#ifdef FFT_CHAPTER
    if(W)memset(&W->mime,0,sizeof W->mime);
    u8 manual;
    u8 c = (u8)(n < 100 ? FFT_DEFAULT_CHAPTER : n / 100 - 1);   // the chapter, then its door
    n %= 100;
    if (chapter_now != c + 1 && !chapter_load(c)) return;
    if (CD(START)) {                            // a story start: beats before the battle
        opening_start(n);
#ifdef FFT_CONTOUR_DEFAULT
        st.reach_contour=1; // isolated benchmark control, ordinary battles default to full tiles
#endif
        return;
    }
#endif
#if !defined(FFT_EXPLORATION) && !defined(FFT_CHAPTER)
    u8 i;
#endif
#ifdef FFT_EXPLORATION
    if (!n) n = 61;                            // exploration launcher: Ramza immediately
#endif
#ifndef FFT_MODULES
    memset(&st, 0, sizeof st);
#ifdef FFT_CONFIG
    st.pzoom = 1; st.psolid = 1;      // unchecked options: low-poly, wide, filled
#endif
#ifdef FFT_CONTOUR_DEFAULT
    st.reach_contour=1;
#endif
    st.want = st.prot = st.ai_view = 0xFF;
#endif
    if (!W) return;
#ifdef FFT_ACTOR_BANK
    actor_reset();                                // every actor decoded again, at once
#endif
    rt_seed = n ? n : rt_ticks();                 // a new draw each battle; the tests: fixed
#ifdef FFT_EXPLORATION
    if (n >= 61 && n <= 68) rt_seed = 61;         // identical actors/equipment for all cameras
#endif
    rand_seed(rt_seed);
#ifdef FFT_CHAPTER
#ifdef FFT_MODULES
    manual=(u8)mod_call(MOD_PARTY,13,(u8)n);
    if(W->nature_units)mod_call(MOD_NATURE,1,0);
#ifdef FFT_CONTOUR_DEFAULT
    st.reach_contour=1;
#endif
#else
    manual=scenario_prepare((u8)n);
#endif
#else
    battle_units();                               // Gariland's eleven units at their start tiles
    for (bag_i = 0; bag_i < NBAG; bag_i++) st.bag[BAG_SLOT(bag_i)] = bag_init[bag_i];
#endif
#ifdef FFT_CHAPTER
    if(manual){cam_target(&st.camx,&st.camy);map_start(0);return;}
#endif
#ifndef FFT_CHAPTER
    st.ai_all = n == 16;                          // the AI plays both sides to the end
#endif
    turn_start();
    while (n && n != 16 && !st.end && st.act != U_RAMZA) turn_end(0, 0); // terminal doors cannot wait for Ramza
    if(st.end)return;                            // do not overwrite the terminal mode with a test door
#if !defined(FFT_EXPLORATION) && !defined(FFT_CHAPTER)
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
        cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));
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
            cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));
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
#if defined(FFT_MODULES) && defined(FFT_CHAPTER)
    mod_call(MOD_PARTY,14,(u8)n);
#else
    if (n >= 61 && n <= 68) {                 // direct Ramza; the three shoulder views
        st.ai = AI_OFF; st.mode = M_MENU; st.act = st.sel = U_RAMZA;
        cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));
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
            cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));
            st.menu = MN_ACT;
        }
#endif
#endif
    }
#ifndef FFT_CHAPTER
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
        cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));
    }
#endif
    W->status_i = 0; W->pan_k[0][0] = W->pan_k[1][0] = 0; W->pan_ok[0] = W->pan_ok[1] = 0; W->fc_key = 0; W->win_k[0] = 0;
#endif
    if (!n) bn_start(BN_START);                  // a battle: its objective first
    cam_target(&st.camx, &st.camy);
    hl_seq++;
}

#ifdef FFT_MODULES
static u8 view_input(u8 op,s16 du,s16 dv)
{
    ModCtx *c;mod_call(MOD_SCREENS,255,0);c=mod_context(MOD_SCREENS);
    c->cast_actor=(u8)du;c->cast_ref=(u8)dv;return (u8)mod_call(MOD_SCREENS,op,0);
}
static void move_cursor(s16 du,s16 dv){view_input(8,du,dv);}
static u8 face_of(s16 du,s16 dv){return view_input(9,du,dv);}
#else
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
    {
        u16 cell=node_at(x,z,node_layer(CURSOR_CELL));
        if(cell>=MAP_N)cell=node_at(x,z,0);
        if(cell<MAP_N)cursor_place((u8)cell);
    }
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

#endif

static void to_menu(void)
{
    st.mode = M_MENU;
    cursor_place(UNIT_CELL(&st.unit[st.act]));
}

// Manual / this unit / all allies. Never reset Move/Act or advance the CT clock.
// Cancellation lets a committed walk, blow or loot message complete normally.
#ifdef FFT_MODULES
static void auto_control(u8 choice){mod_call(MOD_SCREENS,7,choice);}
#else
static void auto_control(u8 choice)
{
    u8 i;
    if (!choice) {
        st.auto_units=0; st.ai_all=0;
        if (st.unit[st.act].team!=TEAM_PLAYER || !st.ai) return;
#ifdef FFT_CHAPTER
        if(st.unit[st.act].status_flags&STATUS_NO_CONTROL)return;
#endif
    } else if (choice==2) {
        for (i=0;i<NUNIT;i++) if (st.unit[i].team==TEAM_PLAYER) st.auto_units|=(u16)(1U<<i);
    } else st.auto_units|=(u16)(1U<<st.act);
    st.ai=choice ? AI_THINK:AI_OFF; st.ai_t=0;
    W->ai_k=W->frz=W->frz_n=W->hold=0; W->frz_h=0;
    if (choice) st.mode=M_BROWSE;
    else if (st.mode!=M_WALK && st.mode!=M_ACT && st.mode!=M_PICK) to_menu();
    hl_seq++;
}

#endif

// the turn menu: Move and Act once each (a move undone with [shift] before acting), Wait
// (then the facing), Status
#ifdef FFT_MODULES
static void menu_keys(void)
{
    if(input_pressed(K_UP|K_DOWN|K_LEFT|K_RIGHT|K_A|K_ENTER|K_ESC|K_B)&&mod_call(MOD_SCREENS,5,0))compute_reach(st.act);
}
static u8 modal_keys(void){return (u8)mod_call(MOD_SCREENS,13,0);}
static void acts_keys(void)
{
    if(!unit_can_act(&st.unit[st.act])){to_menu();return;}
    u8 math=st.unit[st.act].set==21&&(st.mode==M_SKILLS||(st.sub==1&&input_pressed(K_A|K_ENTER)));
    u8 result=(u8)mod_call(math?MOD_TURN:MOD_SCREENS,math?1:6,0);
    if(result==2){
        if(st.unit[st.act].set==21&&st.abil!=NO_ABIL){W->upd_k+=AIM_K;area_control(AREA_BEGIN);}
        else aim(st.abil);
    }else if(result==3)defend(st.act);
    else if(result==4)equipment_control(1);
}
#else
static void menu_keys(void)
{
    Unit *u = &st.unit[st.act];
    st.menu=choice_move(st.menu,MN_N,rt_keys&~rt_prev);
    if (input_pressed(K_A | K_ENTER)) {
        if (st.menu == MN_MOVE && !st.moved
#ifdef FFT_CHAPTER
            && unit_can_move(u)
#endif
        ) {
            st.mode = M_TARGET; st.sel = st.act;
            compute_reach(st.act);
        } else if (st.menu == MN_ACT && !st.acted
#ifdef FFT_CHAPTER
            && unit_can_act(u)
#endif
        ) {
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
        if (st.moved && !st.acted && !st.pick_t
#ifdef FFT_CHAPTER
            && !movement_is_teleport(st.act)
#endif
            ) {   // the move undone (not after a pick-up)
#ifdef FFT_CHAPTER
            unit_place(u,st.move_cell);u->face=st.mface;
#else
            u->x = st.mx; u->z = st.mz; u->face = st.mface;
#endif
            st.moved = 0; st.menu = MN_MOVE;
            to_menu();
        } else st.mode = M_BROWSE;                 // looking around
    }
}

// Act's list (Attack, the unit's skill set) and the abilities it knows (an item without any
// left greyed); [shift] goes back a level
#ifdef FFT_CHAPTER
#include "math_menu.inc"
#endif
static void acts_keys(void)
{
    u8 n = 0;
#ifdef FFT_CHAPTER
    if(!unit_can_act(&st.unit[st.act])){to_menu();return;}
    if(st.unit[st.act].set==21&&(st.mode==M_SKILLS||(st.sub==1&&input_pressed(K_A|K_ENTER)))){
        if(math_menu()==2){W->upd_k+=AIM_K;area_control(AREA_BEGIN);}return;
    }
#endif
    if (st.mode == M_ACTS) {
        u8 m = (u8)(2 + st.unit[st.act].defend
#ifdef FFT_CHAPTER
            +cj_support(common,st.unit[st.act].job,st.support[st.act],480)
#endif
        );  // Attack, the skill set, Defend
        st.sub=choice_move(st.sub,m,rt_keys&~rt_prev);
        if (input_pressed(K_A | K_ENTER)) {
            if (!st.sub) aim(NO_ABIL);
            #ifdef FFT_CHAPTER
            else if(st.sub==2+st.unit[st.act].defend){equipment_control(1);}
#endif
            else if (st.sub == 2) { st.acted = 1; defend(st.act); }
            else if (known(st.act, 0) != NO_ABIL) { st.mode = M_SKILLS; st.skill = 0; }
        } else if (input_pressed(K_ESC | K_B)) st.mode = M_MENU;
        return;
    }
    n=(u8)known(st.act,255);
    st.skill=choice_move(st.skill,n,rt_keys&~rt_prev);
    if (input_pressed(K_A | K_ENTER)) {
        AbId ab = known(st.act, st.skill);
        if (can_use(st.act, ab)) aim(ab);
    } else if (input_pressed(K_ESC | K_B)) st.mode = M_ACTS;
}
#include "modal_input.inc"

#endif

#ifdef FFT_MODULES
#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
u8 view_hides(u8 rot,u8 i)
{
    mod_call(MOD_AI_SETUP,255,0);mod_context(MOD_AI_SETUP)->cast_actor=i;
    return (u8)mod_call(MOD_AI_SETUP,4,rot);
}
#endif
static void ai_camera(void){mod_call(MOD_AI_SETUP,3,0);}
#else
#include "ai_camera.inc"
#endif

// an AI turn on screen, as a player's without the menus: what comes next in its plan (st.ai_*):
// act (from its tile, or once moved: the view turned if needed, the cursor gliding to the
// target, the preview, an ability's name), move (the cursor gliding to the tile, its range
// shown, then the walk), after acting from its tile think again where to go, then the facing
static void ai_next(void)
{
    Unit *u = &st.unit[st.act];
    u8 me = (u8)(UNIT_CELL(u));
    st.ai_t = 0;
    st.mode = M_BROWSE;
    W->frz = W->frz_n = 0; W->frz_h = 0;
    cursor_place(UNIT_CELL(u));
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
        st.ai_t = AI_HOLD_SKIP;                 // thinking again after acting: no hold
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
#ifdef FFT_MODULES
static u8 ai_shown(void){return (u8)mod_call(MOD_SCREENS,10,0);}
#else
static u8 ai_shown(void)
{
    u8 c=st.ai==AI_MOVE?st.ai_o:st.ai_tgt,x=TILE_X[c],z=TILE_Z[c];
    if (st.cx != x || st.cz != z || CURSOR_CELL!=c) {
        if (++st.ai_t >= AI_GLIDE) {
            st.ai_t = 0;
            s16 nextx=st.cx,nextz=st.cz;u16 cell;
            if (nextx != x) nextx+=nextx<x?1:-1;
            else if(nextz!=z)nextz+=nextz<z?1:-1;
            cell=node_at(nextx,nextz,node_layer(CURSOR_CELL));
            if(cell>=MAP_N)cell=node_at(nextx,nextz,0);
            if(cell<MAP_N)cursor_place((u8)cell);
            if(nextx==x&&nextz==z)cursor_place(c);
        }
        return 0;
    }
    if (!W->hud_ok) return 0;                   // the time shown counts once the HUD is drawn
    return ++st.ai_t >= (st.ai == AI_MOVE ? AI_SHOW : st.ai_ab == NO_ABIL ? AI_PREVIEW : AI_PREVIEW + AI_NAME);
}

#endif

// Resident-only clock: no battle/save/overlay layout growth. ai_t=0 arms a new
// turn; the normal replan path carries AI_HOLD_SKIP. Unsigned subtraction wraps.
static u16 ai_hold_start;
static void ai_step(void)
{
    if (st.ai == AI_THINK) {                    // once the picture stands still (game_render)
        if (!st.ai_t) { ai_hold_start=rt_ticks(); st.ai_t=1; }
        if (W->frz || ++W->frz_n > 40) {        // (40 frames at most: the picture no longer
            u8 done;                            // drawn anyway)
            ZB(9); done = ai_think(); ZE(9);
            if (done) st.ai = AI_GO;            // (this frame keeps the picture)
        }
    } else if (st.ai == AI_GO) {                // thinking counts toward the turn's pause
        if (st.ai_t != AI_HOLD_SKIP && !(st.dbg & DBG_FAST) && (u16)(rt_ticks()-ai_hold_start)<AI_HOLD_TICKS) return;
        ai_camera(); ai_next(); W->hold = 1;    // then the plan shown from the next
    }                                           // frame (this one keeps the picture)
    else if (st.ai == AI_FACE ? ++st.ai_t >= AI_SHOW / 2 : ai_shown()) {
        if (st.ai == AI_MOVE) {
            Unit *u = &st.unit[st.act];
            if (make_path_cell(st.act,st.ai_o)) {
                st.mx = u->x; st.mz = u->z; st.mface = u->face;
#ifdef FFT_CHAPTER
                st.move_cell=UNIT_CELL(u);
#endif
                st.mode = M_WALK; st.path_i = 0; st.walk_t = 0;
            } else { st.moved = 1; ai_next(); }
        } else if (st.ai == AI_AIM) {
            u8 o = unit_at_cell(st.ai_tgt);
#ifdef FFT_CHAPTER
            if(st.ai_ab<512&&(st.unit[st.act].set==21||(abilities[st.ai_ab].flags&(AF_CAST|AF_AREA))==(AF_CAST|AF_AREA))){
                if(!cast_order(st.act,st.unit[st.act].set==21?st.sub&15:st.ai_tgt,st.ai_ab,st.unit[st.act].set==21?MATH_BIND:0)){
                    W->ai_k=0;st.ai=AI_THINK;st.mode=M_BROWSE;
                }
                return;
            }
#endif
            st.acted = 1;
            if (!o) ai_next();
            else if (st.ai_ab == NO_ABIL) attack(st.act, o - 1);
            else skill(st.act, o - 1, st.ai_ab);
        } else turn_end(st.moved, st.acted);
    }
}

// can tile c be picked: a reachable tile to walk to, or (aim) a unit in reach to act on?
#ifndef FFT_MODULES
#include "target_pick.inc"
#else
static u16 pick_query(u8 op,u8 cell,u8 aiming)
{
    mod_call(MOD_TURN,255,0);mod_context(MOD_TURN)->cast_ref=aiming;
    return mod_call(MOD_TURN,op,cell);
}
static u8 pickable(u8 c,u8 aiming){return (u8)pick_query(5,c,aiming);}
static u16 twin(u8 c,u8 aiming){return pick_query(6,c,aiming);}
#endif

#ifdef FFT_MODULES
static void status_keys(void){mod_call(MOD_SCREENS,4,0);}
#else
#include "status_input.inc"
#endif

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
    {                                           // the units stay animated while an AI thinks
#ifdef FFT_CHAPTER
        if(!flow_bank||st.mode!=M_END)
#endif
        st.tick++;
    }
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
#if !defined(FFT_CHAPTER) || !defined(FFT_REACH_CONTOUR)
    if (!fast && input_pressed(K_MODE)) { st.over ^= 1; hl_seq++; }
#endif
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

#ifdef FFT_CHAPTER
    if(st.mode==M_CLOCK){
        if(!fast&&input_pressed(K_ESC)){st.back=M_CLOCK;st.mode=M_QUIT;st.quit=1;}
        else if(!st.turn)turn_start();
        return 1;
    }
    if((st.unit[st.act].status_flags&STATUS_CT_FROZEN)&&st.mode!=M_WALK&&st.mode!=M_ACT&&st.mode!=M_PICK&&st.mode!=M_END&&!(st.mode==M_QUIT&&st.back==M_CLOCK)){
        turn_end(st.moved,st.acted);return 1;
    }
#endif
    if (st.mode == M_WALK) {
        u8 a=st.path_i,b=(u8)(a+1);
        st.unit[st.sel].face=PATH_X(b)>PATH_X(a)?0:PATH_X(b)<PATH_X(a)?1:PATH_Z(b)>PATH_Z(a)?2:3;
        if (++st.walk_t >= step_frames(st.path_i)) {
            st.walk_t = 0;
            st.path_i++;
            unit_place(&st.unit[st.sel],PATH_CELL(st.path_i));
            cursor_place(UNIT_CELL(&st.unit[st.sel]));
            if (st.path_i + 1 >= st.path_n) {      // back to the menu, on Act (Wait once acted),
                u8 i;                              // or a crystal or a chest there picked up
                st.mode = M_MENU; st.moved = 1; st.menu = st.acted ? MN_WAIT : MN_ACT;
#ifdef FFT_CHAPTER
                movement_settle(st.sel);
#endif
                for (i = 0; i < NUNIT; i++)
                    if (UNIT_CELL(&st.unit[i])==CURSOR_CELL && DC_OBJ(st.unit[i].dc)&&st.unit[i].dc!=DC_GONE) pick_up(st.sel, i);
                if (st.ai && st.mode == M_MENU) ai_next();
            }
        }
    } else if (st.mode == M_ACT) {
        attack_step();
    } else if (st.mode == M_PICK) {                // the message, then the menu (no undo)
        if (++st.pick_t >= PICK_FRAMES) { st.mode = M_MENU; if (st.ai) ai_next(); }
    } else if (st.mode == M_END) {                 // the results: the battle again, or quit
#ifdef FFT_CHAPTER                                 // (a chapter: back to the campaign)
        if(flow_bank&&st.end==END_VICTORY&&st.reward_ready) {
            if(input_pressed(K_ESC))return 0;
            if(st.open_clock<250)st.open_clock++;
            if(st.reward_page==2) {
                if(input_pressed(K_UP)&&st.reward_scroll)st.reward_scroll--;
                if(input_pressed(K_DOWN)&&st.reward_scroll+4<st.reward_n)st.reward_scroll++;
            }
            if(input_pressed(K_A|K_ENTER)) {
                st.open_clock=0;
                if(++st.reward_page>=3)return 0;
            }
            return 1;
        }
        if(input_pressed(K_A|K_ENTER|K_ESC))return 0;
#else
        if (input_pressed(K_A | K_ENTER)) game_scenario(0);
        else if (input_pressed(K_ESC)) return 0;
#endif
#ifdef FFT_CHAPTER
    } else if(st.mode==M_EQUIP){
        equipment_control(2);
    } else if(st.mode==M_AREA){
        if(!st.turn)area_control(AREA_INPUT);
#endif
    } else if (st.ai) {                            // an AI unit's turn
        if (!st.turn) ai_step();
    } else if (!st.turn) {
        if (input_pressed(K_APPS) && (st.mode == M_BROWSE || st.mode == M_MENU || st.mode == M_TARGET
            || st.mode == M_AIM || st.mode == M_CAST_RANGE || st.mode == M_ACTS || st.mode == M_SKILLS)) {
            st.back = st.mode; st.mode = M_ORDER;  // the turn order (modal_input.inc)
        } else if ((u8)(st.mode - M_ORDER) < 3) {  // turn order, Quit?, Auto-battle (modal_input.inc)
            if (modal_keys()) return 0;
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
                u8 o, c = (u8)(CURSOR_CELL);
                if (
                    (st.mode == M_TARGET || st.mode == M_AIM || st.mode==M_CAST_RANGE) && !pickable(c, st.mode != M_TARGET)) {
                    u16 t = twin(c, st.mode != M_TARGET);
                    if (t) { c = t - 1; cursor_place(c); }
                }
                o = unit_at_cell(CURSOR_CELL);
                if (st.mode == M_TARGET) {
                    Unit *u = &st.unit[st.sel];
                    if (st.reach[c] != 0xFF && st.reach[c] > 0 && make_path_cell(st.sel,c)) {
                        st.mx = u->x; st.mz = u->z; st.mface = u->face;
#ifdef FFT_CHAPTER
                        st.move_cell=UNIT_CELL(u);
#endif
                        st.mode = M_WALK; st.path_i = 0; st.walk_t = 0;
                    } else st.deny = DENY_FRAMES;
                #ifdef FFT_CHAPTER
                } else if(st.mode==M_CAST_RANGE){
                    area_control(AREA_BEGIN);
#endif
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
                if (st.mode == M_BROWSE && input_pressed(K_ESC)) { st.back=M_BROWSE;st.mode = M_QUIT; st.quit = 1; }
                else if (st.mode == M_AIM || st.mode==M_CAST_RANGE) {            // back to the list it came from
                    st.mode = st.abil == NO_ABIL ? M_ACTS : M_SKILLS;
                    cursor_place(UNIT_CELL(&st.unit[st.act]));
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

static u8 battle_update(void);
// leaving after a victory: the campaign save written (save.inc), then on the TI the buffers
// freed, for AMS to write it as the program exits (rt_save)
u8 game_update(void)
{
#ifdef FFT_CHAPTER
    if(!battle_state)return !input_pressed(K_ESC);
#endif
    if (battle_update()) return 1;
#ifdef FFT_CHAPTER
    if (W && (st.end == END_VICTORY || st.open_handoff)) {
        st.open_handoff = 1;                    // the campaign goes on (fftbat returns it)
        save_campaign();
#ifdef __m68k__
        release(); W = RT_NULL;
#endif
    }
#endif
    return 0;
}

static u8 battle_update(void)
{
    u8 r, k = st.ai == AI_THINK ? AI_FAST : 1;   // (thinking this frame: its last step too)
    FRAME_BEGIN();
#ifdef FFT_ACTOR_BANK
#ifndef ACTOR_BUDGET
#define ACTOR_BUDGET 1                     // new poses decoded per frame (255: all, the raw-bank look)
#endif
    if(W){W->actor_budget=ACTOR_BUDGET;W->actor_misses=0;W->ac_now++;}
#endif
#ifdef FFT_CHAPTER
    if (st.open_stage || st.open_fade || (st.end == END_VICTORY && !st.banner && story_after&&!st.story_done)) return map_update();
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
    for (; r && k < AI_FAST && !st.morph_mask && !st.banner && st.ai && st.ai != AI_THINK && (rt_keys & (K_A | K_ENTER) || st.dbg & DBG_FAST) && !W->upd_k
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
    s16 y = st.camy < 0 ? -st.camy : 0, sh = st.camx & 15, rs = 16 - sh;
    s16 source_y = st.camy < 0 ? 0 : st.camy;
    const u8 *sl = scene_l + source_y * SC_BYTES + ((st.camx >> 4) << 1);
    const u8 *sd = scene_d + source_y * SC_BYTES + ((st.camx >> 4) << 1);
    u8 *dl = rt_light, *dd = rt_dark;
    if (y) {
        u16 bytes = (u16)y * RT_PBYTES;
        memset(dl, 0, bytes); memset(dd, 0, bytes);
        dl += bytes; dd += bytes;
    }
#ifdef __m68k__                                 // a row: 5 longs, each a long and the next word
    for (; y < RT_H; y++, sl += SC_BYTES, sd += SC_BYTES, dl += RT_PBYTES, dd += RT_PBYTES) {
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
    for (; y < RT_H; y++, sl += SC_BYTES, sd += SC_BYTES, dl += RT_PBYTES, dd += RT_PBYTES) {
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
#ifdef FFT_CHAPTER
    if(unit_monster(&st.unit[i]))return JOB_NAME(st.unit[i].job);
#endif
#ifdef FFT_CHAPTER
    if (CD(UNIT_NAMES)) return (const char *)(igfx+RD16(igfx+OPENING_NAMES,i));
#endif
    const Unit *u = &st.unit[i];
    if (u->gfx == UG_RAMZA) return "Ramza";
    if (u->gfx == UG_DELITA) return "Delita";
    {
#ifdef FFT_CHAPTER
        return identity_name(pgfx,(u16)u->name+(u->female?255:0));
#else
        const u8 *names = igfx + (u->female ? UI_FEMALE : UI_MALE);
        return (const char *)names + RD16(names, u->name);
#endif
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

#include "portrait32.inc"
// Original portrait with palette-zero transparency and an offline white silhouette outline.
static void portrait(u8 gfx, s16 x, s16 y)
{
#ifdef FFT_CHAPTER
    gfx = GFX_PORTRAIT(gfx);
#endif
    const u8 *p =
#ifdef FFT_CHAPTER
        identity_portrait(pgfx,pgfx[UI_PORTRAITS+gfx]);
#else
        igfx + UI_PORTRAITS + (u16)gfx * 348;
#endif
    portrait32(p,x,y);
}

static void unit_card_bar(s16 x,s16 y,u16 v,u16 max,u8 width,u8 color,u8 style,u8 *opacity);
static void gauge(s16 x,s16 y,u16 v,u16 max,u8 color,u8 width)
{unit_card_bar(x,y,v,max,width,color,UC_BAR_FLAT,0);}

// Outlined small text built into the transparent panel, never redrawn glyph by glyph on screen.
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

#ifdef FFT_PACKED_PANELS
#define PAN_CACHE(side,buffer,plane) ((u16 *)((u8 *)W->pan[side][buffer]+10*(plane)))
#define TAG_CACHE(team,plane) ((u16 *)((u8 *)W->tag[team]+10*(plane)))
#else
#define PAN_CACHE(side,buffer,plane) W->pan[side][buffer][plane]
#define TAG_CACHE(team,plane) W->tag[team][plane]
#endif
#define PAN_MAKE(side,plane) PAN_CACHE(side,W->pan_f[side]^1,plane)   // the panel being made
static void panel_text(u8 side, s16 x, s16 y, const char *s, s16 limit)
{
    outlined_text((u8 *)PAN_MAKE(side,0), (u8 *)PAN_MAKE(side,1), (u8 *)PAN_MAKE(side,2), x, y, s, limit);
}

static u8 ui_width(const char *s)
{
    u8 width = 0;
    while (*s) width += UI_SHARED(ADV)[(u8)*s++ - 32];
    return width;
}

static void battle_card_text(s16 x,s16 y,const char *s,u8 color,s16 limit,u8 context)
{
    if(context<2)panel_text(context,x,y,s,limit);else ui_text(x,y,s,color,limit);
}
#define CARD_TEXT(x,y,s,c,l,context) battle_card_text(x,y,s,c,l,context)
#define CARD_WIDTH(s) ui_width(s)
#include "unit_card.inc"
#define CHOICE_TEXT(x,y,s,c,l) ui_text(x,y,s,c,l)
#define CHOICE_WIDTH(s) ui_width(s)
#include "choice_menu.inc"


static void unit_card_battle(u8 i,u8 layout,u8 part,u8 context)
{
    const Unit *u=&st.unit[i];UnitCard d;u8 gfx=u->gfx;
    u8 *opacity=layout==UC_BATTLE_LEFT||layout==UC_BATTLE_RIGHT?(u8 *)PAN_MAKE(context,2):0;
#ifdef FFT_CHAPTER
    gfx=GFX_PORTRAIT(gfx);d.job=JOB_NAME(u->job);
#else
    d.job=GFX_CHEMIST(gfx)?"Chemist":"Squire";
#endif
#ifdef FFT_CHAPTER
    d.portrait=identity_portrait(pgfx,pgfx[UI_PORTRAITS+gfx]);
#else
    d.portrait=igfx+UI_PORTRAITS+(u16)gfx*348;
#endif
    d.name=unit_name(i);
    d.hp=u->hp;d.max_hp=u->max_hp;d.mp=u->mp;d.max_mp=u->max_mp;
    d.level=u->level;d.exp=u->exp;d.brave=u->brave;d.faith=u->faith;
    unit_card_draw(&d,layout,0,part,context,opacity);
    if(part==UC_PORTRAIT&&opacity) {
        const u8 *p=d.portrait+232;u8 r;
        for(r=0;r<29;r++) {
            u8 *m=opacity+(u16)r*RT_PBYTES;u32 bits=~RD32(p,r*2);
#ifdef FFT_PACKED_PANELS
            // The right portrait extends beyond its80-pixel displayed crop.
            // Discard its spill into the blank prefix of the next row slot.
            if(context){WR32((u8 *)PAN_MAKE(context,1)+(u16)r*RT_PBYTES,0,0);WR32(m,0,0);}
#endif
            if(context){WR16(m,3,(u16)(bits>>29));WR16(m,4,(u16)(bits>>13));}
            else WR32(m,0,bits);
        }
    }
}

#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define st_names CONTOUR_REF_st_names
#else
static const char *const st_names[4] = { "Defend", "Poison", "Dark", "Silence" };
#endif   // ST_*

#ifdef FFT_MODULES
#ifndef FFT_PRODUCTION_CORE // resident test/tool adapter
u8 forecast_text(u8 a,u8 t,AbId ab,char *s)
{
    ModCtx *c;
    st.formula.actor=a;st.formula.target=t;st.formula.ability=ab;
    mod_call(MOD_PLANNER,3,0);
    mod_call(MOD_NUMBERS,255,0);c=mod_context(MOD_NUMBERS);c->forecast_out=s;
    return (u8)mod_call(MOD_NUMBERS,5,0);
}
#endif
#else
#include "forecast_text.inc"
#endif

#ifdef FFT_MODULES
static void make_forecast(u8 a,u8 t,AbId ab)
{
    st.formula.actor=a;st.formula.target=t;st.formula.ability=ab;mod_call(MOD_PLANNER,4,0);
}
#else
#include "forecast_panel.inc"
#endif

static void make_panel(u8 side, u8 i)
{
    void *sl = rt_light, *sd = rt_dark;
    u8 k = W->pan_n[side]++;
    rt_light = PAN_MAKE(side,0); rt_dark = PAN_MAKE(side,1);
    if(!k)memset(PAN_MAKE(side,0),0,sizeof W->pan[side][0]);
    unit_card_battle(i,side?UC_BATTLE_RIGHT:UC_BATTLE_LEFT,k,side);
    if(k==UC_LEVEL) {
        W->pan_f[side] ^= 1; W->pan_ok[side] = 1;  // made: shown from now on
        memcpy(W->pan_fk[side], W->pan_k[side], sizeof W->pan_fk[side]);
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
    const u8 *l = (const u8 *)PAN_CACHE(side,W->pan_f[side],0), *d = (const u8 *)PAN_CACHE(side,W->pan_f[side],1);
    const u8 *m = (const u8 *)PAN_CACHE(side,W->pan_f[side],2);
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
#ifdef FFT_MODULES
static void make_window(void){mod_call(MOD_RESULTS,2,0);}
#else
#include "window_cache.inc"
#endif

static void draw_hand(s16 x, s16 y)
{
    #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define sway CONTOUR_REF_sway
#else
static const u8 sway[8] = { 0, 1, 2, 3, 3, 2, 1, 0 };
#endif
    choice_hand_at(x+sway[(st.tick>>2)&7],y);
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
        outlined_text((u8 *)TAG_CACHE(t,0), (u8 *)TAG_CACHE(t,1), (u8 *)TAG_CACHE(t,2), 1, 1, team_names[t], 31);
        W->tag_w[t] = ui_width(team_names[t]);
    }
    W->tag_ok = 1;
}

// the tag over the outer edge of a side's portrait: left-aligned on the left, right on the right
static void draw_tag(u8 side, u8 t)
{
    u8 r, sh = side ? (u8)(30 - W->tag_w[t]) : 0;
    const u8 *l = (const u8 *)TAG_CACHE(t,0), *d = (const u8 *)TAG_CACHE(t,1), *m = (const u8 *)TAG_CACHE(t,2);
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

#ifdef FFT_CHAPTER
static u8 area_paint(u8 who)
{
    u8 pk[2],side;pk[0]=st.act+1;pk[1]=who;
    if(!W->tag_ok)make_tags();else panel_part(pk);
    for(side=0;side<2;side++)if(pk[side]&&panel_shown(side,pk[side])){
        u8 team=st.unit[pk[side]-1].team;draw_panel(side);
        if(team!=TEAM_PLAYER)draw_tag(side,team==TEAM_GUEST);
    }
    if(who&&panel_shown(0,pk[0]))draw_forecast();
    W->hud_ok=panel_shown(0,pk[0])&&(!who||panel_shown(1,who));
    return W->hud_ok;
}
#endif

// The HUD's parts are made one a frame at least (more while the budget has room): a menu, then
// the panels; the active unit (or the attacker) on the left, the unit under the cursor (or the
// target) on the right, during moves and blows too
static void draw_hud(void)
{
#ifdef FFT_CHAPTER
    if(st.mode==M_AREA){
#ifdef FFT_MODULES
        mod_call(MOD_AREA_UI,AREA_DRAW,0);
#else
        area_draw(0);
#endif
        return;
    }
#endif
    u8 o, i, pk[2], forecast = 0, hidden = st.turn, parts;
    u16 k[3];
    u8 pre = st.mode == M_WALK && !st.ai, ahead = 0;   // the menu after the walk made during it
    u16 forecast_key;
    o = unit_at_cell(CURSOR_CELL);
    pk[0] = hidden ? 0 : st.mode == M_ACT ? st.atk + 1 : st.act + 1;
    pk[1] = hidden ? 0 : st.mode == M_ACT ? (st.tgt != st.atk ? st.tgt + 1 : 0)
        : o && o - 1 != st.act && (st.mode == M_AIM || st.mode==M_CAST_RANGE || st.mode == M_BROWSE || st.mode==M_CLOCK || st.mode == M_TARGET) ? o : 0;
    k[0] = st.mode == M_MENU || st.mode == M_AUTO || st.mode == M_ACTS || st.mode == M_SKILLS
        || st.mode == M_PICK || st.mode == M_ORDER || st.mode == M_QUIT ? st.mode : 0;
    if (pre) k[0] = M_MENU;
    k[1] = k[0] == M_MENU ? (u8)(pre | st.moved | st.acted << 1) : k[0] == M_ACTS ? known(st.act, 0) == NO_ABIL
        : k[0] == M_PICK ? st.pick : k[0] == M_ORDER ? st.turns[1] : k[0] == M_SKILLS ? SKILL_TOP : 0;
#ifdef FFT_CHAPTER
    if(k[0]==M_MENU)k[1]|=(!unit_can_move(&st.unit[st.act]))|(!unit_can_act(&st.unit[st.act])<<1);
#endif
    k[2] = st.act;
    if ((st.mode == M_AIM||st.mode==M_CAST_RANGE) && o && st.reach[CURSOR_CELL] == 1 && aim_ok(o - 1)) {
        k[0] = M_AIM; k[1] = st.act; k[2] = o - 1;
        if (st.ai == AI_AIM && st.abil != NO_ABIL && st.ai_t >= AI_PREVIEW && CURSOR_CELL == st.ai_tgt) {
            k[0] = M_ACT; k[1] = st.abil; k[2] = st.act;
        }
    }
    if (k[0] == M_AIM) { forecast = k[2] + 1; k[0] = 0; }
    forecast_key = forecast ? (u16)((u16)(AbId)(st.abil + 1) << 4 | forecast) : 0;
#ifdef FFT_CHAPTER
    if(forecast&&W->fc_key==forecast_key)pk[1]=(u8)(W->preview_target+1);
#endif
    // one part even without room (the budget reckons high), but not in a frame already full,
    // unless it waited WAIT_MAX frames
#ifdef LEFT_TRACE
    BENCH_VALUE((u32)(u16)W->left);
#endif
    if (W->left >= 0 || (++W->hud_wait > WAIT_MAX && W->bg_drawn)) W->hud_wait = 0;
#ifdef FFT_CHAPTER
    if(k[0]==M_SKILLS&&(st.sub&128))W->hud_wait=0; // mandatory first-frame Math feedback
#endif
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
            ZB(13); make_forecast(pk[0] - 1, forecast - 1, (AbId)((forecast_key >> 4) - 1)); ZE(13);
            W->fc_key = forecast_key;
#ifdef FFT_CHAPTER
            pk[1]=(u8)(W->preview_target+1);
#endif
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
        if (k[0] == M_MENU || k[0] == M_AUTO || k[0] == M_ACTS || k[0] == M_SKILLS || k[0] == M_QUIT || k[0] == M_ORDER) {
            u8 e = k[0] == M_MENU ? st.menu : k[0] == M_ACTS || k[0] == M_AUTO ? st.sub : k[0] == M_SKILLS ? st.skill - W->win_k[1]
                : k[0] == M_ORDER ? st.status_row : st.quit + 1;
            draw_hand(80, y + 7 + 6 * e);
        }
    }
    W->hud_ok = (!pk[0] || panel_shown(0, pk[0])) && (!pk[1] || panel_shown(1, pk[1]))
        && (!forecast || W->fc_key == forecast_key)
        && (hidden || pre || !k[0] || (W->win_k[0] == k[0] && W->win_k[1] == k[1] && W->win_k[2] == k[2] && W->win_n == W->win_m));
#ifdef FFT_CHAPTER
    if(st.mode==M_CAST_RANGE){
#ifdef FFT_MODULES
        mod_call(MOD_AREA_UI,AREA_HEADER,0);
#else
        area_draw(1);
#endif
    }
#endif
}

#ifndef __m68k__
#ifndef FFT_PRODUCTION_CORE   // test/tool entry, absent from fftbat
u8 hud_ready(void) { return W && W->hud_ok; }
#endif
#endif

#endif
#if MOD_CORE || MOD_PART(MOD_SCREENS) || MOD_PART(MOD_RESULTS)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define help_lines CONTOUR_REF_help_lines
#else
static const char *const help_lines[2] = { "ENTER: stats/equip   SHIFT: back",
#ifdef FFT_CHAPTER          // fftstory returns to the title after a lost battle (original FFT)
    "[ENTER] title"         // (tools/contour_assets.py archives help_lines[1]: this one first)
#else
    "[ENTER] again   [ESC] quit"
#endif
};
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
#if !MOD_CORE
#include "status_input.inc"
#endif
#if !MOD_CORE && defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define st_names CONTOUR_REF_st_names
#endif
// Details describe the equipped item or learned ability; browsing never changes the loadout.
static void status_detail(u8 k)
{
    char s[64];
    u8 n = 0; AbId id = st.status_col ? status_ability() : st.unit[st.sel].eq[st.status_row];
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
        if (k == 3) { ui_text(6, 38, id == NO_ABIL ? "No ability" : id == ABIL_DEFEND ? "Defend" : ABIL_NAME(id), C_BLACK, 159); return; }
        if (id == NO_ABIL) return;
        if (id == ABIL_DEFEND) { if (k == 4) ui_text(6, 47, "Raises evasion until next turn", C_BLACK, 159); return; }
        ab = &abilities[id];
        if (k == 4) {
            n = put_s(s, n, "Range "); n = put_n(s, n,
#ifdef FFT_CHAPTER
                ab->formula==99?st.unit[st.sel].move:
#endif
                ab->range); n = put_s(s, n, "   Height ");
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
            n = put_s(s, n, ab->kind==AB_MATH_SELECTOR?"Selects targets":effects[ab->kind]);
        } else if (k == 7 && ab->flags & AF_ITEM) { n = put_s(s, n, "In stock "); n =
#ifdef FFT_CHAPTER
            unit_home_team(&st.unit[st.sel])!=TEAM_ENEMY
#else
            st.unit[st.sel].team==TEAM_PLAYER
#endif
            ? put_n(s,n,
#ifdef FFT_CHAPTER
                id>=512?battle_save.inv[ab->bag]:
#endif
                st.bag[ab->bag]):put_s(s,n,"Unlimited"); }
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
            unit_card_battle(st.sel,UC_STATUS,UC_PORTRAIT,2);
            unit_card_battle(st.sel,UC_STATUS,UC_NAME,2);
            unit_card_battle(st.sel,UC_STATUS,UC_JOB,2);
            unit_card_battle(st.sel,UC_STATUS,UC_LEVEL,2);
            ui_text(24, 25, st.page ? "EQUIP / SKILLS" : "STATUS", C_DGRAY, 92);
            ui_text(4, 95, st.status_info ? "[ESC] back" : st.page ? "[2nd] details" : "< > page", C_DGRAY, 81);
            if (!st.status_info) ui_text(94, 95, "[ESC] return", C_DGRAY, 159);
        } else if (k <= 2) {
            unit_card_battle(st.sel,UC_STATUS,k==1?UC_HP:UC_MP,2);
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
                u8 slot = k - 4; AbId ab = known(st.sel, slot);
                ui_text(23, 44 + 8 * slot, u->eq[slot] == 0xFF ? "-" : items[u->eq[slot]].name, C_BLACK, 81);
                ui_text(103, 44 + 8 * slot, ab == NO_ABIL ? u->defend && slot == status_ability_count() - 1 ? "Defend" : "-" : ABIL_NAME(ab), C_BLACK, 159);
                if (!slot && u->defend && status_ability_count() == 6) ui_text(103, 84, "Defend", C_BLACK, 159);
            }
        }
    }
    rt_light = sl; rt_dark = sd;
    copy_box(W->status[0], W->status[1], 0, 0, 10, RT_H);
    if (st.page && !st.status_info && W->status_n == 9)
        draw_hand(st.status_col ? 81 : 1, 41 + 8 * st.status_row);
}
#if !MOD_CORE
#if defined(FFT_CHAPTER) && defined(FFT_REACH_CONTOUR)
#include "config.inc"
#endif

#include "menu_input.inc"
#ifdef FFT_CHAPTER
#include "bootstrap_files.inc"
#endif
u16 mod_entry(u8 op,u8 a)
{
#ifdef FFT_CHAPTER
    if(op==11)return bootstrap_files();
    if(op==12)return cj_valid(common,M.desc_size);
#endif
    if(op==10)return ai_shown();
    if(op==5)return menu_keys();if(op==6)return acts_keys();
    if(op==13)return modal_keys();
    if(op==7){auto_control(a);return 0;}
    if(op==8){move_cursor((s8)M.cast_actor,(s8)M.cast_ref);return 0;}
    if(op==9)return face_of((s8)M.cast_actor,(s8)M.cast_ref);
#if defined(FFT_CHAPTER) && defined(FFT_REACH_CONTOUR)
    if(op==2)return config_update();
    if(op==3){config_draw();return 0;}
    if(op==4){status_keys();return 0;}
    if(op==14){debug_hit();return 0;}
#endif
    draw_status();return 0;
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
#ifdef FFT_MODULES
static void draw_number(void)
{
    if(area_impact_root()&&W->cast.phase==CAST_BATCH_READY)return;
    mod_call(MOD_AREA_UI,AREA_RESULTS,0);
}
#else
#include "numbers.inc"
static void draw_number(void){numbers_draw();}
#endif

#ifdef FFT_MODULES
// ORDER is never needed while the Wait facing is chosen: no image ping-pong.
static void draw_arrows(void){mod_call(MOD_ORDER,2,0);}
#else
#include "facing_arrows.inc"
#endif

// Lato Black's exact former vector fill is sampled offline into four small text strips
// in ffti. Reveal those packed rows at runtime: no glyph rasterization can overrun the
// banner timer, and the same four greys/white outline remain. No full-screen images.
#define BN_K 20                         // packed masked strips (measured with ti-cycles)


static void __attribute__((noinline)) bn_start(u8 kind)
{
#ifdef FFT_MODULES
    // Once per objective/result, after any battle/scene image has returned.
    mod_call(MOD_PARTY,7,kind);
#else
    banner_setup(kind);
#endif
}

#ifdef FFT_MODULES
static void bn_shade(void){mod_call(MOD_ORDER,6,0);}
static void bn_draw(void){mod_call(MOD_ORDER,7,0);}
#else
#include "banners.inc"
#endif

#endif
#if MOD_PART(MOD_RESULTS)
#if !MOD_CORE

#define CHOICE_TEXT(x,y,s,c,l) ui_text(x,y,s,c,l)
#define CHOICE_WIDTH(s) ui_width(s)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define hand_gfx CONTOUR_REF_hand_gfx
#endif
#include "choice_menu.inc"
#include "window_cache.inc"
#endif
#ifdef FFT_CHAPTER
#include "campaign/result_ui.inc"
#endif
// the results (M_END): the outcome, then each of the player's units with its level (before >
// after), the EXP and JP it earned, and the keys; drawn into the scene buffer one line a frame
// (AMS text), copied to the screen
static void draw_end(void)
{
#ifdef FFT_CHAPTER
    if(flow_bank&&st.end==END_VICTORY&&st.reward_ready){result_draw();return;}
#endif
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
#include "reaction_live.h"

#include "reaction_select.inc"
u16 mod_entry(u8 op, u8 a)
{
    (void)a;if(op==3)return reaction_select();if(op==1)return window_label(M.window_out);if(op==2){make_window();return 0;}draw_end();return 0;
}
#endif
#endif
#if MOD_CORE

// The battle cannot start: the RAM its buffers need (RESOURCES op 12 says how much), else a
// missing bank
static void missing_message(void)
{
    draw_rect(0,0,RT_W,RT_H,C_WHITE);
#if defined(FFT_MODULES) && defined(__m68k__)
    if (mod_images[MOD_RESOURCES]) {             // AMS's free RAM in 16-byte units (no ROM calls there)
        mod_call(MOD_RESOURCES, 255, 0); mod_context(MOD_RESOURCES)->desc_size = (u16)(HeapAvail() >> 4);
        if (mod_call(MOD_RESOURCES, 12, (u8)((battle_state ? 1 : 0) | (sbuf ? 2 : 0) | (navbuf ? 4 : 0)))) return;
    }
#endif
    draw_text(4,40,"Missing data",F_SMALL,C_BLACK);
}

void game_render(void)
{
#if defined(FFT_CHAPTER) && defined(FFT_COMPACT_UI)
    if(!battle_state) { missing_message(); return; }
#endif
#ifndef FFT_COMPACT_UI
    u8 ord[UNIT_CAPACITY], vis[UNIT_CAPACITY], i, j, cached, tg;
    s16 n;
    Feet f[UNIT_CAPACITY];
    s16 u, v;
    if(
#ifdef FFT_CHAPTER
       !battle_state||
#endif
       !W||!ugfx||!wgfx||!igfx||!views[0]||!views[1]||!views[2]||!views[3]) {
        missing_message();
#ifdef FFT_CHAPTER
        if(battle_state)
#endif
        { FRAME_END(); }
        return;
    }
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
        (!st.morph_mask && (W->hold || (!st.banner && st.ai == AI_THINK && W->frz && (rt_frame & 3))))) {   // an AI
        W->hold = 0;                                // thinking: one picture in four, the units' animation
        FRAME_END(); return;                        // kept (its pause, AI_GO, draws every frame)
    }
#ifdef FFT_EXPLORATION
#ifdef FFT_CINEMA
    if (st.cphase && st.perspective) { cinema_render(); FRAME_END(); return; }
#endif
    if (st.perspective) { perspective_render(); FRAME_END(); return; }
#endif
#ifdef FFT_CHAPTER
    if (st.open_stage || st.open_fade) { map_render(); FRAME_END(); return; }
#endif
    if (st.mode == M_END && !st.banner && !st.morph_mask
#ifdef FFT_CHAPTER
        &&!(flow_bank&&st.end==END_VICTORY&&st.reward_ready)
#endif
    ) {
#ifdef FFT_CHAPTER
        // The kill's first rendered frame precedes map_update's scene entry.
        // Keep the battlefield visible until the post-battle dialogue has run.
        if(!(flow_bank&&st.end==END_VICTORY&&story_after&&!st.story_done))
#endif
        { draw_end(); FRAME_END(); return; }
    }
    #ifdef FFT_CHAPTER
    if(st.mode==M_EQUIP){equipment_control(3);FRAME_END();return;}
#endif
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
        u16 fk=unit_feet_key(i,st.rot);
        if (st.unit[i].x >= MAP_W || st.unit[i].z >= MAP_H) {
            f[i].x = f[i].y = 0; f[i].depth = 0;   // removed unit / test injection: no map access
        } else if (st.mode == M_WALK && i == st.sel) f[i] = unit_feet(st.rot, i), W->feet_k[i] = 0;
        else if (W->feet_k[i] == fk) f[i] = W->feet[i];   // kept while it stands
        else { f[i] = W->feet[i] = unit_feet(st.rot, i); W->feet_k[i] = fk; }
        if (st.mode == M_ACT && hit_flash(i)) {   // visual recoil only
            static const s8 dx[4] = { -1, -1, 1, 1 }, dy[4] = { 1, -1, 1, -1 };
            u8 dir = unit_direction(st.rot, st.atk), t = (u8)(st.act_t - ACT_HIT);
            u8 step = t < 6 ? 2 : t < 10 ? 1 : 0;
            f[i].x += dx[dir] * step; f[i].y += dy[dir] * (step != 0);
        }
        for (j = i; j > 0 && f[ord[j - 1]].depth > f[i].depth; j--) ord[j] = ord[j - 1];
        ord[j] = i;
        bx = f[i].x - st.camx - 8; by = f[i].y - st.camy - UNIT_FOOT;
        vis[i] = st.unit[i].x < MAP_W &&
#ifdef FFT_CHAPTER
            !unit_airborne(&st.unit[i]) &&
#endif
            bx > -16 && bx < RT_W && by > -UNIT_SH && by - FOE_H < RT_H;
#ifdef FFT_CHAPTER
        if(flow_bank&&st.scene_hide&(1U<<i))vis[i]=0;
#endif
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
    if (AIMING(st.mode) && st.tick & 8 && (tg = unit_at_cell(CURSOR_CELL)) && vis[tg - 1]) n += UNIT_K;
    else tg = 0;
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
        draw_unit_covered(f[tg].x - st.camx, f[tg].y - st.camy, st.rot, tg, &W->cover[tg]);
    }
#ifdef FFT_ACTOR_BANK
    W->left-=(s16)W->actor_misses*32;
#endif
    ZE(4); ZB(5);
#ifdef FFT_CHAPTER
    if(st.mode==M_END&&flow_bank&&st.end==END_VICTORY&&st.reward_ready&&st.story_done) {
        draw_end();ZE(5);FRAME_END();return;
    }
#endif
#ifdef FFT_CHAPTER
    weapon_effect();
#endif
    // the cursor, above everything (blinking), dashed behind the scenery
    dst_screen(st.camx, st.camy);
    view_of(st.rot, st.cx, st.cz, &u, &v);
#ifdef FFT_CHAPTER
    if(area_impact_root()&&W->area_impact.centre<MAP_N&&
       st.act_t>=ACT_BLOW&&st.act_t<ACT_HIT+FLASH_FRAMES){
        Marker mk;u16 centre=W->area_impact.centre;u8 white=(st.act_t&4)!=0;
        marker(st.rot,(u8)centre,white,&mk);
        blit24(mk.x,mk.y,mk.rows,mk.l,mk.d,cursor_mask(&mk,centre|(white?0:0x1000)));
    }
#endif
    if (st.mode == M_ACT && st.act_t >= ACT_HIT) draw_number();
    if (st.mode == M_FACE) draw_arrows();
    // During AI thinking the active-unit pointer identifies the actor. The
    // tile cursor returns when the plan is shown; a new turn already spends
    // its frame on the clock, camera and unit covers.
    else if (!st.banner && !st.morph_mask && st.ai != AI_THINK && st.mode != M_ACT && (st.tick & 8 || RINGS(st.mode)) && !(st.deny & 4)
#ifdef FFT_SOURCE_CAMERA
        && map_tiles[CURSOR_CELL].walk
#endif
    ) {
        Marker mk;
        u32 fl[24];                            // alive through blit24, including return from inspection
        u8 c = (u8)(CURSOR_CELL), ring = RINGS(st.mode);
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
    if (st.ai == AI_THINK && !st.banner && !st.morph_mask) {      // the camera settled: thinking may
        s16 tx, ty;                                                  // take the frames (game_render
        cam_target(&tx, &ty);                                        // draws one in four)
        if (tx == st.camx && ty == st.camy) W->frz = 1;
    }
#ifdef FFT_CONFIG
    if(W->left>=60) pidle_tick(48);
#endif
    FRAME_END();
#endif
}

#ifdef FFT_CHAPTER
#include "rules.inc"
#endif
#ifdef FFT_CHAPTER
#include "opening/opening.inc"
#endif
#endif
#if !MOD_CORE && MOD_PART(MOD_BEATS) && defined(FFT_CHAPTER)
#include "camera_target.inc"
#include "opening/opening.inc"                  // its beats only (MOD_PART there)
#endif
#if (MOD_PART(MOD_SAVE) || MOD_PART(MOD_PARTY)) && defined(FFT_CHAPTER)
#include "save.inc"
#endif

#if !MOD_CORE && MOD_PART(MOD_MOVEMENT)
static const s8 dir_x[4]={1,-1,0,0},dir_z[4]={0,0,1,-1};
#define EDGE_ROW(id) (W->eg+((u16)(id)<<W->edge_shift))
#include "movement_runtime.inc"
#include "movement_edges.inc"
#include "tactical_path.h"
#include "reach_menu.inc"
#include "path_menu.inc"
#include "skill_range.inc"
#include "aim_menu.inc"
u16 mod_entry(u8 op,u8 a)
{
    if(op==10)return aim(M.cast_ability);
    if(op==9){movement_profile();return 0;}
    if(op==12){movement_ready(a);return 0;}
    if(op==13)return movement_poll(a);
    if(op==8){compute_reach(M.cast_actor);return 0;}
    if(op==7)return make_path_cell(M.cast_actor,M.cast_ref);
    if(op==5){skill_range(st.formula.actor,st.formula.ability);return 0;}
    if(!op){W->move_kind=0;edges((u8)M.cast_ability);return 0;}
    movement_prepare(a);return 0;
}
#endif

#if defined(FFT_CHAPTER) && (MOD_PART(MOD_EQUIPMENT))
#if !MOD_CORE
#include "rewards.inc"
#include "equipment_runtime.inc"
#include "combat_response.inc"
#include "reaction_self.inc"
#define CHOICE_TEXT(x,y,s,c,l) ui_text(x,y,s,c,l)
#define CHOICE_WIDTH(s) ui_width(s)
#include "choice_menu.inc"
#endif
#include "equipment_ui.inc"
static void equipment_local_control(u8 op)
{
    if(op==1){if(equipment_ready(st.act)){if(W->edge_phase){W->edge_phase=0;W->ej=255;}memcpy(W->ai_tt,st.unit[st.act].eq,5);W->status_i=0;W->pan_ok[0]=W->pan_ok[1]=W->pan_n[0]=W->pan_n[1]=0;st.mode=M_EQUIP;st.status_col=st.sub=0;}}
    if(op==2)equipment_input();
    if(op==3)equipment_draw();
}
#if MOD_CORE
static void equipment_control(u8 op){equipment_local_control(op);}
#else
u16 mod_entry(u8 op,u8 a)
{
    if(!op){equipment_rebuild_pending();return 0;}
    if(op==4)return movement_benefit(a);
    if(op==5)return equipment_commit(a,W->ai_tt);
    if(op==6){combat_response_launch();return 0;}
    if(op==7){reaction_self_launch(6);return 0;}
    equipment_local_control(op);return 0;
}
#endif
#endif

#if !MOD_CORE && MOD_PART(MOD_COMBAT)
#include "magic_prepare.inc"
#include "hamedo.inc"
#include "command_effect.inc"
u16 mod_entry(u8 op,u8 a)
{
    (void)op;(void)a;
    return command_effect(st.formula.actor,st.formula.target,st.formula.ability);
}
#endif

#if !MOD_CORE && MOD_PART(MOD_NATURE)
#include "nature_runtime.inc"
#include "discovery_move.inc"
#include "reaction_live.h"
#include "mime_runtime.inc"
u16 mod_entry(u8 op,u8 a)
{
 u8 i;
 if(op==9)return mime_next();
 if(op==0){nature_hit_commit();W->nature_pending&=(u16)~3;return 0;}
 if(op==1){monster_refresh();return 0;}
 if(op==3)return discovery_find(a,(u8)st.formula.roll);
 if(op==6)return (u16)math_targets(st.formula.actor,(u8)st.formula.roll);
 if(op==5){AreaAi *q=&W->area_ai;memset(q,0,sizeof *q);q->next=1;
  for(i=0;i<16;i++)q->math_mask[i]=math_filters(st.act,i)?(u16)math_targets(st.act,i):0;
  W->ai_k=7;return 0;}
 return 0;
}
#endif

#if !MOD_CORE && MOD_PART(MOD_RESOURCES)
#include "chapter_validate.h"
#define ugfx (M.chapter_ugfx)
#include "chapter_files.inc"
#undef ugfx
// The battle could not start (op 12; held: 1 State, 2 the view/scene/cache blocks, 4 the map
// envelope; M.desc_size: AMS's free RAM / 16, read by the resident): "Not enough RAM" when that
// free RAM and the blocks held are less than the battle allocates (BATTLE_RAM_BYTES), so a user
// knows how much to free. 0: a bank is missing instead.
static u8 ram_message(u8 held)
{
    u32 have = ((u32)M.desc_size << 4) + (held & 1 ? BATTLE_STORAGE_BYTES : 0)
        + (held & 4 ? NAV_ENVELOPE : 0) + (held & 2 ? BATTLE_RAM_BYTES - BATTLE_STORAGE_BYTES - NAV_ENVELOPE : 0);
    char s[32];                                 // (no initializer: a module has no memcpy)
    const char *t = "need ";
    u8 n = 0, i;
    if (have >= BATTLE_RAM_BYTES) return 0;
    for (i = 0; i < 2; i++) {                   // "need N KB, free N KB"
        u16 v = i ? (u16)(have >> 10) : (u16)((BATTLE_RAM_BYTES + 1023) >> 10), d = 10000;
        while (*t) s[n++] = *t++;
        while (d > v && d > 1) d /= 10;
        for (; d; d /= 10) s[n++] = (char)('0' + v / d % 10);
        t = i ? " KB" : " KB, free ";
    }
    while (*t) s[n++] = *t++;
    s[n] = 0;
    draw_text(4, 34, "Not enough RAM", F_SMALL, C_BLACK);
    draw_text(4, 44, s, F_SMALL, C_BLACK);
    return 1;
}
u16 mod_entry(u8 op,u8 a)
{
    if(op==12)return ram_message(a);
    if(op==11)return (u8)chapter_route(a);
    if(op==10){char letter=chapter_route(a);return letter?chapter_files(letter):0;}
    return 0;
}
#endif
