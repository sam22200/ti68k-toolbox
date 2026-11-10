// Battle-engine modules (campaign/ENGINE.md, core milestone): parts of fft.c compiled apart as
// position-independent images (tools/module.py), kept in two matching archive banks and copied into one zone
// of the core program when used (the only RAM an unpatched Titanium runs code from:
// ti68k-c-patterns.md § interrupts). A module reaches the core only through this context,
// written into the module's copy (its symbol M) after each load.
// Included by fft.c after the Work type, with FFT_MODULES (core: the loader) or FFT_MODULE
// (a module's own build: the core's names become the context's).
// Include shared rule/data expectations, so an older AI cannot bypass new rules.
#define MOD_ABI 82U
#define MOD_SIG ((u16)(sizeof(State) * 3 + sizeof(Work) + sizeof(ModCtx) + sizeof(Save) + 257U*MOD_ABI))
// Cold bootstrap allocates existing blocks and binds resident archive/cache pointers.
typedef struct {
    const u8 **p_flow_bank,**p_flow_ui,**p_wgfx,**p_abank,**p_cgfx,**p_common,**p_charge_ticks,**p_job_sets;
    const Ability **p_abilities;const Job **p_jobs;const char **p_job_names;const Item **p_items;
    State **p_battle_state;void **p_rt_state;u16 *p_rt_state_size;
    u8 **p_vbuf,**p_sbuf,**p_navbuf;u16 *p_nav_capacity;u16 (**p_acache)[UNIT_FB/2];u16 **p_ac_key,**p_ac_age;
    const PlaneWord **p_sl_l,**p_sl_m,**p_sc_l,**p_sc_m;
    u16 campaign_flow;                   // resident build policy, shared images serve both entries
} CoreBoot;
typedef struct {
    u16 sig;                            // MOD_SIG, from the module's own build
    union {                             // transient input, never gameplay/save state
        struct {u16 cast_ability;u8 cast_actor,cast_ref;};
        u8 *chapter_story_after;         // PARTY10 cold script flags
        const CoreBoot *boot;          // SCREENS11: cold pointer bindings, no gameplay state
        Unit *stats_unit;                // resident -> PARTY9 only
        char *window_out;               // resident -> RESULTS1 only
        s16 *camera_out;                 // BEATS4: two camera target coordinates
        char *forecast_out;              // resident -> NUMBERS5 only
        CastBatch *cast_batch_out;        // resident -> TARGETS1/3 or SAVE7 only
        struct {u16 desc_size,interface_size;}; // SAVE4: FCD3; SCREENS12: catalogue size in desc_size;
                                        // RESOURCES12: AMS's free RAM / 16 in desc_size
        const u8 *chapter_ugfx;          // SAVE6 -> resident: the chapter's unit frames
    };
    u8 map_w, map_h;
    u16 map_n;                         // the chapter's sizes, as they are at load (fft.h)
    u8 unit_count;                      // validated encounter count, storage remains13 slots
    s16 sc_h, sc_top;
    State *state;
    Work *w;
    const Ability *abilities;
    const Tile *map_tiles;
    const u8 *contour_gfx, *cdesc;
    const u8 **views;
    u8 *highlight_seq;
    u16 (*rand15)(void);
    AbId (*known)(u8 who, u8 k);
    void (*spread_start)(u8 who, u8 c, u8 limit, u8 *dist, u8 *from, u8 out, const u8 *need, u16 left);
    u8 (*spread_go)(u16 n);
    void (*bfs)(u8 who, u8 *from);
    u16 (*charge_query)(u8 a,u8 t,AbId ab,u8 flags,u16 roll);
    u8 (*skill_cost)(u8 who,AbId ab);
    u16 (*magic_amount)(u8 a,u8 t,u16 spec);
    u16 (*magic_heal)(u8 a,u8 t,AbId ab,u8 query);
    u8 (*skill_kind)(u8 a,u8 t,AbId ab);
    u8 (*magic_hit)(u8 a,u8 t,AbId ab,u8 execute);
    u16 (*target_query)(u8 who,u16 criterion); // bit15: ability applicability;255/254/253/252: validity/reaction/critical/Jump
    u16 (*weapon_amount)(u8 a,u8 t,u8 weapon,u8 critical,u16 roll,u8 awake,u8 charge,u16 base_roll);
    void (*knockback)(u8 a,u8 t);
    s16 (*zodiac_xa)(const Unit *a, const Unit *t, s16 xa);
    const Tile *(*tile_at)(s16 x, s16 z);
    u16 (*node_at)(s16 x,s16 z,u8 layer);
    u8 (*unit_place)(Unit *u,u8 cell);
    u8 (*unit_at_cell)(u8 cell);
    u8 (*cursor_place)(u8 cell);
    Dst *D;
    void **rt_light, **rt_dark;
    void (*world_of)(u8 rot,s16 u,s16 v,s16 *x,s16 *z);
    void (*view_of)(u8 rot, s16 x, s16 z, s16 *u, s16 *v);
    s16 (*scene_x)(u8 rot, s16 u, s16 v);
    s16 (*scene_y)(s16 u, s16 v);
    void (*draw_unit_covered)(s16 sx,s16 sy,u8 rot,u8 i,const Cover *c);
    u8 **scene_l;
    const Item *items;
    void (*ui_text)(s16 x, s16 y, const char *s, u8 color, s16 limit);
    u8 (*ui_width)(const char *s);
    u8 (*put_s)(char *s, u8 n, const char *t);
    u8 (*put_n)(char *s, u8 n, u16 v);
    const char *(*unit_name)(u8 i);
    void (*text_into)(u16 *l, u16 *d, s16 x, s16 y, const char *s, u8 color);
    void (*unit_card_battle)(u8 i,u8 layout,u8 part,u8 context);
    u8 (*area_paint)(u8 who);             // pure card-cache preparation/blits, never an image load
    void (*draw_hand)(s16 x, s16 y);
    void (*copy_box)(const u16 *l, const u16 *d, u8 x, u8 y, u8 words, u8 rows);
    void (*draw_rect)(s16 x, s16 y, s16 w, s16 h, u8 color);
    void (*draw_text)(s16 x, s16 y, const char *s, u8 font, u8 color);
    u8 (*job_level)(const Unit *u);
    u8 (*hand_weapon)(const Unit *u,u8 hand);   // resident helpers shared instead of copied per image
    u8 (*restore_ability)(AbId ab);
#ifdef FFT_CHAPTER
    const u8 **opening_story, **pgfx;
    u8 (*can_use)(u8 who,AbId ab);        // reused former unused opening_after binding
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
    u8 (*ranged_reach)(u8 a, u8 o, u8 c); // a255: fill manual mask for actor o/origin c
    u8 (*rt_load)(const char *name, void *data, u16 size);
    u8 (*rt_save)(const char *name, const void *data, u16 size);
    const void *(*rt_file)(const char *name, u16 *size);   // SAVE6: the chapter's banks
    const Job *jobs;
    const UnitDef *unit_defs;
    const u8 *common, *charge_ticks, *igfx,*flow_bank,*flow_ui;
    u16 *rt_seed;
#endif
    long __attribute__((__stkparm__)) (*divsi3)(long a, long b);   // GCC's helpers: stack
    long __attribute__((__stkparm__)) (*mulsi3)(long a, long b);   // arguments
    unsigned long __attribute__((__stkparm__)) (*udivsi3)(unsigned long a,unsigned long b);
} ModCtx;

#ifdef FFT_MODULE
// The archive builder reads this symbol and entry even without C callers.
const u16 mod_context_size __attribute__((used))=sizeof(ModCtx);
u16 mod_entry(u8 op,u8 a) __attribute__((externally_visible));
ModCtx M = { MOD_SIG };                 // in the image (tools/module.py: one section)
#define map_w (M.map_w)
#define map_h (M.map_h)
#define map_n (M.map_n)
#define encounter_units (M.unit_count)
#define sc_h (M.sc_h)
#define sc_top (M.sc_top)
#undef st
#define st (*M.state)
#define W (M.w)
#define abilities (M.abilities)
#define map_tiles (M.map_tiles)
#define contour_gfx (M.contour_gfx)
#define cdesc (M.cdesc)
#define views (M.views)
#define hl_seq (*M.highlight_seq)
#define rand15 M.rand15
#define known M.known
#define can_use M.can_use
#define spread_start M.spread_start
#define spread_go M.spread_go
#define bfs M.bfs
#define hit_chance(a,t) magic_hit(a,t,NO_ABIL,0)
#define charge_query M.charge_query
#define skill_cost M.skill_cost
#define magic_amount M.magic_amount
#define magic_heal M.magic_heal
#define skill_kind M.skill_kind
#define magic_hit M.magic_hit
#define magic_target(t) M.target_query(t,255)
#define magic_applicable(t,ab) ((u8)M.target_query(t,(u16)(ab)|32768U))
#define weapon_amount M.weapon_amount
#define roll_hit(a,t) magic_hit(a,t,NO_ABIL,1)
#define knockback M.knockback
#if MOD_PART(MOD_AI) || MOD_PART(MOD_FORMULA) || MOD_PART(MOD_ACTION) || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_COMBAT) || MOD_PART(MOD_NUMBERS)
#define skill_hit formula_hit
#define skill_amount formula_amount
#endif
#define zodiac_xa M.zodiac_xa
#define tile_at M.tile_at
#define node_at M.node_at
#define unit_place M.unit_place
#define unit_at_cell M.unit_at_cell
#define cursor_place M.cursor_place
#define ranged_reach M.ranged_reach
#define D (*M.D)
#define rt_light (*M.rt_light)
#define rt_dark (*M.rt_dark)
#define world_of M.world_of
#define view_of M.view_of
#define scene_x M.scene_x
#define scene_y M.scene_y
#define draw_unit_covered M.draw_unit_covered
#define scene_l (*M.scene_l)
#define items (M.items)
#define ui_text M.ui_text
#define ui_width M.ui_width
#define put_s M.put_s
#define put_n M.put_n
#define unit_name M.unit_name
#define text_into M.text_into
#define unit_card_battle M.unit_card_battle
#define area_paint M.area_paint
#define draw_hand M.draw_hand
#define copy_box M.copy_box
#define draw_rect M.draw_rect
#define draw_text M.draw_text
#define job_level M.job_level
#define opening_story (*M.opening_story)
#define pgfx (*M.pgfx)
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
#define rt_load M.rt_load
#define rt_save M.rt_save
#define rt_file M.rt_file
#define job_sets cj_section(common,CJ_JOB_SETS) // one cold PARTY lookup, no duplicate binding
#define jobs (M.jobs)
#define unit_defs (M.unit_defs)
#define common (M.common)
#define charge_ticks (M.charge_ticks)
#define igfx (M.igfx)
#define flow_bank (M.flow_bank)
#define flow_ui (M.flow_ui)
#define rt_seed (*M.rt_seed)
#ifndef FFT_MODULE_ARITH_MASK
#define FFT_MODULE_ARITH_MASK 7
#endif
#if FFT_MODULE_ARITH_MASK & 1
long __attribute__((__stkparm__,externally_visible)) __divsi3(long a, long b) { return M.divsi3(a, b); }
#endif
#if FFT_MODULE_ARITH_MASK & 2
unsigned long __attribute__((__stkparm__,externally_visible)) __udivsi3(unsigned long a,unsigned long b) { return M.udivsi3(a,b); }
#endif
#if FFT_MODULE_ARITH_MASK & 4
long __attribute__((__stkparm__,externally_visible)) __mulsi3(long a, long b) { return M.mulsi3(a, b); }
#endif
#endif
