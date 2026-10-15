/* Contextual perspective research. World units: 64 per grid cell, Z points up.
 * Rendering never owns gameplay state. Synthetic fixtures are NOT ROM maps. */
#ifndef TENCHU_ENGINE_H
#define TENCHU_ENGINE_H
#include "../../runtime/core/rt.h"
#define TE_GRID 16
#define TE_MAX_SPRITES 10
#define TE_MAX_ANCHORS 8
#define TE_MAX_BOXES 24
#define TE_SEGMENTS 6
#ifdef __m68k__
#define TE_DATA_EXTRA 6 /* rt_file size includes the OTH dat trailer on TI */
#else
#define TE_DATA_EXTRA 0
#endif
typedef struct { s16 x,y,z; u8 yaw; s8 pitch; } TeCamera;
typedef struct { s16 x0,y0,z0,x1,y1,z1; u8 material,cell,roof,flags; } TeBox;
typedef struct { s16 x,y,z; u8 pose,pad; } TeActor;
typedef struct { s16 x,y,z; } TeAnchor;
typedef struct {
    u8 floor[256], door[256];
    TeBox box[TE_MAX_BOXES]; TeActor actor[TE_MAX_SPRITES];
    TeAnchor anchor[TE_MAX_ANCHORS];
    u8 nbox,nactor,nanchor,scene;
} TeWorld;
typedef struct { u8 top,bottom; u16 depth; } TeSegment;
typedef struct {
    u16 walls,polygons,rejected,sprite_pixels,segment_overflow,cache_hits,cache_misses;
    u16 cells,portals;
} TeMetrics;
typedef struct {
    TeCamera camera;
    u8 scene,renderer,resolution,view,orientation,sprite_mode,nsprites,moving;
    s8 target,hit; u8 door_open,scope,style;
    s16 feet_z,grapple_ticks;
    s16 hook_x,hook_y,hook_z,jump_base;
    u8 training,missing,jump_frame,pad;
} TeState;
typedef struct {u16 nv,nf,na;const u8 *vertices,*faces,*anchors;} TeMesh;
extern TeMesh te_mesh;
u8 te_training_load(void);
void te_training_scene(void);
u8 te_training_move(s16 dx,s16 dy);
s16 te_ground(s16 x,s16 y,s16 maximum);
u8 te_training_los(s16 x,s16 y,s16 z,s16 tx,s16 ty,s16 tz);
void te_training_anchors(void);
u8 te_training_barrier(s16 x,s16 y,s16 z,s16 tx,s16 ty,s16 tz);
static inline u16 te_rd16(const u8 *p) {
#ifdef __m68k__
    return *(const u16 *)p;
#else
    return ((u16)p[0]<<8)|p[1];
#endif
}
extern const short te_sin[256];
extern const unsigned short te_recip[2048];
extern const unsigned short te_row[100];
extern TeState te_state;
#ifdef TE_ONLY_RENDERER
#define te_renderer ((u8)TE_ONLY_RENDERER)
#else
#define te_renderer te_state.renderer
#endif
extern TeWorld *te_world_ptr;
#define te_world (*te_world_ptr)
extern u16 te_revision;
u8 te_world_init(void);
void te_world_finish(void);
extern TeMetrics te_metrics;
extern u8 *te_fb;
extern u16 te_width,te_height,te_focal;
extern TeSegment (*te_cover)[TE_SEGMENTS];
extern u8 *te_ncover;
u8 te_render_init(void);
void te_render_finish(void);
u16 te_render_bytes(void);
u16 te_poly_bytes(void);
void te_scene(u8 scene);
void te_camera_defaults(u8 scene);
void te_begin(void);
u8 te_background_get(void);
void te_background_put(void);
void te_present(void);
void te_pixel(s16 x,s16 y,u8 black);
void te_span(s16 x0,s16 x1,s16 y,u8 pattern);
void te_line(s16 x0,s16 y0,s16 x1,s16 y1,u8 black);
s16 te_project(s16 value,u16 depth);
void te_transform(s16 x,s16 y,s16 z,s16 *side,s16 *up,s16 *depth,u8 pitch);
void te_ray(void);
void te_sprites(void);
void te_actor(u16 n);
u8 te_sprites_init(void);
void te_sprites_finish(void);
u16 te_sprite_cache_bytes(void);
u16 te_sprite_flash_bytes(void);
u8 te_poly_init(void);
void te_finish(void);
void te_poly(void);
void te_floor(void);
void te_floor_span(s16 y,s16 x0,s16 x1,s16 z);
extern const s16 *te_clip_left,*te_clip_right;
s16 te_project_y(s16 value,u16 depth);
void te_markers(void);
u8 te_occluded(u16 x,u16 y,u16 depth);
u8 te_los(s16 x,s16 y,s16 z,s16 tx,s16 ty,s16 tz);
s8 te_target(void);
s16 te_ray_box(const TeCamera *camera,s16 dx,s16 dy,s16 dz,const TeBox *box);
s8 te_aim_guard(void);
u8 te_move(s16 dx,s16 dy);
u16 te_tables_bytes(void);
/* Verified primitive, reused from the project knowledge base. No new assembly. */
static inline s32 te_mul(s16 a,s16 b) {
#ifdef __m68k__
    s32 r; __asm__("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r;
#else
    return (s32)a*b;
#endif
}
#ifdef RT_CYCLES
#include "../../tools/m68kbench/bench.h"
#define TE_BEGIN(n) BENCH_BEGIN(n)
#define TE_END(n) BENCH_END(n)
#else
#define TE_BEGIN(n) ((void)0)
#define TE_END(n) ((void)0)
#endif
#endif
