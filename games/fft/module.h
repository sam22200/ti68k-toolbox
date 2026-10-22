// Battle-engine modules (campaign/ENGINE.md, core milestone): parts of fft.c compiled apart as
// position-independent images (tools/module.py), kept in the archive and copied into a zone
// of the core program when used (the only RAM an unpatched Titanium runs code from:
// ti68k-c-patterns.md § interrupts). A module reaches the core only through this context,
// written into the module's copy (its symbol M) after each load.
// Included by fft.c after the Work type, with FFT_MODULES (core: the loader) or FFT_MODULE
// (a module's own build: the core's names become the context's).
#define MOD_SIG ((u16)(sizeof(State) * 3 + sizeof(Work)))   // both sides built alike
typedef struct {
    u16 sig;                            // MOD_SIG, from the module's own build
    u8 map_w, map_h, map_n;             // the chapter's sizes, as they are at load (fft.h)
    s16 sc_h, sc_top;
    State *st;
    Work *w;
    const Ability *abilities;
    const Tile *map_tiles;
    const u8 *contour_gfx, *cdesc;
    u16 (*rand15)(void);
    u8 (*known)(u8 who, u8 k);
    void (*spread_start)(u8 who, u8 c, u8 limit, u8 *dist, u8 *from, u8 out, const u8 *need, u8 left);
    u8 (*spread_go)(u16 n);
    void (*bfs)(u8 who, u8 *from);
    u8 (*hit_chance)(u8 a, u8 t);
    u16 (*attack_damage)(u8 a, u8 t, u8 crit, u16 r);
    u8 (*skill_hit)(u8 a, u8 t, u8 ab);
    u16 (*skill_amount)(u8 a, u8 t, u8 ab, u16 r);
    const Tile *(*tile_at)(s16 x, s16 z);
    Dst *D;
    void **rt_light, **rt_dark;
    void (*view_of)(u8 rot, s16 x, s16 z, s16 *u, s16 *v);
    s16 (*scene_x)(u8 rot, s16 u, s16 v);
    s16 (*scene_y)(s16 u, s16 v);
    void (*draw_unit_covered)(s16 sx, s16 sy, u8 rot, u8 i, const Cover *c);
    u8 **scene_l;
    const Item *items;
    void (*ui_text)(s16 x, s16 y, const char *s, u8 color, s16 limit);
    u8 (*put_s)(char *s, u8 n, const char *t);
    u8 (*put_n)(char *s, u8 n, u16 v);
    const char *(*unit_name)(u8 i);
    void (*text_into)(u16 *l, u16 *d, s16 x, s16 y, const char *s, u8 color);
    void (*portrait)(u8 gfx, s16 x, s16 y);
    void (*draw_hand)(s16 x, s16 y);
    void (*copy_box)(const u16 *l, const u16 *d, u8 x, u8 y, u8 words, u8 rows);
    void (*draw_rect)(s16 x, s16 y, s16 w, s16 h, u8 color);
    void (*draw_text)(s16 x, s16 y, const char *s, u8 font, u8 color);
    u8 (*job_level)(const Unit *u);
    u8 (*status_ability)(void);
    u8 (*status_ability_count)(void);
    void (*gauge)(s16 x, s16 y, u16 v, u16 max, u8 color, u8 width);
#ifdef FFT_CHAPTER
    const u8 **opening_story, **pgfx;
    void (*opening_battle)(u8 direct);
    void (*opening_after)(void);
#endif
#ifdef FFT_ACTOR_BANK
    const u8 *(*actor_frame)(u16 logical, u8 slot);
#endif
    void (*cover)(u16 *cm, s16 fx, s16 fy, u8 depth);
    void (*ensure_scene)(u8 rot);
    void (*copy_view)(void);
    Feet (*unit_feet)(u8 rot, u8 i);
    u8 (*unit_direction)(u8 rot, u8 i);
    void (*draw_sprite)(s16 x, s16 y, const RtSprite *s);
    u32 *rt_keys, *rt_prev;
    u8 *scene_rot, *vbuf_rot;
    const u8 *cgfx;
#ifdef FFT_CHAPTER
    u8 (*ranged_reach)(u8 a, u8 o, u8 c);
#endif
    long __attribute__((__stkparm__)) (*divsi3)(long a, long b);   // GCC's helpers: stack
    long __attribute__((__stkparm__)) (*mulsi3)(long a, long b);   // arguments
} ModCtx;

#ifdef FFT_MODULE
ModCtx M = { MOD_SIG };                 // in the image (tools/module.py: one section)
#define map_w (M.map_w)
#define map_h (M.map_h)
#define map_n (M.map_n)
#define sc_h (M.sc_h)
#define sc_top (M.sc_top)
#define st (*M.st)
#define W (M.w)
#define abilities (M.abilities)
#define map_tiles (M.map_tiles)
#define contour_gfx (M.contour_gfx)
#define cdesc (M.cdesc)
#define rand15 M.rand15
#define known M.known
#define spread_start M.spread_start
#define spread_go M.spread_go
#define bfs M.bfs
#define hit_chance M.hit_chance
#define attack_damage M.attack_damage
#define skill_hit M.skill_hit
#define skill_amount M.skill_amount
#define tile_at M.tile_at
#define ranged_reach M.ranged_reach
#define D (*M.D)
#define rt_light (*M.rt_light)
#define rt_dark (*M.rt_dark)
#define view_of M.view_of
#define scene_x M.scene_x
#define scene_y M.scene_y
#define draw_unit_covered M.draw_unit_covered
#define scene_l (*M.scene_l)
#define items (M.items)
#define ui_text M.ui_text
#define put_s M.put_s
#define put_n M.put_n
#define unit_name M.unit_name
#define text_into M.text_into
#define portrait M.portrait
#define gauge M.gauge
#define draw_hand M.draw_hand
#define copy_box M.copy_box
#define draw_rect M.draw_rect
#define draw_text M.draw_text
#define job_level M.job_level
#define status_ability M.status_ability
#define status_ability_count M.status_ability_count
#define opening_story (*M.opening_story)
#define pgfx (*M.pgfx)
#define opening_battle M.opening_battle
#define opening_after M.opening_after
#define actor_frame M.actor_frame
#define cover(cm, fx, fy, depth) M.cover(cm, fx, fy, depth)   // (W->cover: a field too)
#define ensure_scene M.ensure_scene
#define copy_view M.copy_view
#define unit_feet M.unit_feet
#define unit_direction M.unit_direction
#define draw_sprite M.draw_sprite
#define rt_keys (*M.rt_keys)
#define rt_prev (*M.rt_prev)
#define scene_rot (*M.scene_rot)
#define vbuf_rot (*M.vbuf_rot)
#define cgfx (M.cgfx)
long __attribute__((__stkparm__)) __divsi3(long a, long b) { return M.divsi3(a, b); }
long __attribute__((__stkparm__)) __mulsi3(long a, long b) { return M.mulsi3(a, b); }
#endif
