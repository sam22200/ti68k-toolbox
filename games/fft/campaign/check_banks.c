// Exact old binding contract after cold loads across both archive ranges.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main bank_runtime_main
#define game_render bank_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void bank_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
static u16 checks,failures;static u32 digest=17;
static void check(u16 got,u16 want,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^got;
    if(got!=want){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("line%u: %u expected%u\n",line,got,want);
#endif
    }
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
#ifdef FFT_MODULES
static ModCtx expected;
static void expected_context(void)
{
    ModCtx *c=&expected;memset(c,0,sizeof *c);c->sig=MOD_SIG;
        c->map_w = map_w; c->map_h = map_h; c->map_n = map_n; c->unit_count = encounter_units;
        c->sc_h = sc_h; c->sc_top = sc_top;
        c->state = &st; c->w = W; c->abilities = abilities; c->map_tiles = map_tiles;
        c->contour_gfx = contour_gfx; c->cdesc = cdesc;
        c->views=views;c->highlight_seq=&hl_seq;
        c->rand15 = rand15; c->known = known; c->spread_start = spread_start; c->spread_go = spread_go;
        c->bfs = bfs; c->hit_chance = hit_chance; c->attack_prediction = attack_prediction;
        c->charge_query=charge_query;c->skill_cost=skill_cost;
        c->magic_amount=magic_amount;
        c->magic_heal=magic_heal;
        c->skill_kind=skill_kind;
        c->magic_hit=magic_hit;c->magic_target=magic_target;
        c->attack_roll=attack_roll;c->roll_hit=roll_hit;c->knockback=knockback;
        c->skill_hit = skill_hit; c->skill_amount = skill_amount; c->zodiac_xa = zodiac_xa; c->tile_at = tile_at;
        c->node_at=node_at;c->unit_place=unit_place;c->unit_at_cell=unit_at_cell;c->cursor_place=cursor_place;
        c->D = &D; c->rt_light = &rt_light; c->rt_dark = &rt_dark; c->world_of=world_of;c->view_of = view_of;
        c->scene_x = scene_x; c->scene_y = scene_y; c->draw_unit_covered = draw_unit_covered;
        c->ui_width=ui_width;c->scene_l = &scene_l; c->items = items; c->ui_text = ui_text; c->put_s = put_s; c->put_n = put_n;
        c->unit_name = unit_name; c->text_into = text_into; c->portrait = portrait;
        c->unit_card_battle=unit_card_battle;c->area_paint=area_paint;
        c->draw_hand = draw_hand; c->copy_box = copy_box; c->draw_rect = draw_rect;
        c->draw_text = draw_text; c->job_level = job_level;
        c->hand_weapon = hand_weapon; c->restore_ability = restore_ability;   // shared resident helpers (ABI 46)
#ifdef FFT_CHAPTER
        c->opening_story = &opening_story; c->pgfx = &pgfx; c->opening_battle = opening_battle;
        // opening_after compiles into BEATS (CORE_CONTROL_STATUS): its slot stays zero.
#endif
#ifdef FFT_ACTOR_BANK
        c->actor_frame = actor_frame;
#endif
        c->cover = cover; c->ensure_scene = ensure_scene; c->copy_view = copy_view;
        c->unit_feet = unit_feet; c->unit_direction = unit_direction; c->draw_sprite = draw_sprite;
        c->rt_keys = &rt_keys; c->rt_prev = &rt_prev; c->scene_rot = &scene_rot; c->vbuf_rot = &vbuf_rot;
        c->cgfx = cgfx;
        c->gauge = gauge;
        c->ranged_reach = ranged_reach; c->rt_load = rt_load; c->rt_save = rt_save; c->job_names = job_names;
        c->job_sets = job_sets;
        c->jobs = jobs; c->unit_defs = unit_defs; c->common = common; c->charge_ticks = charge_ticks;
        c->igfx = igfx; c->flow_bank=flow_bank;c->flow_ui=flow_ui;c->rt_seed = &rt_seed;
        c->divsi3 = __divsi3; c->mulsi3 = __mulsi3;
        c->udivsi3=__udivsi3;
}
static void bindings(void)
{
    u8 id,pass;u32 seed=rng32;u16 size;const u8 *bank;
    u8 actual_w=map_w,actual_h=map_h,actual_units=encounter_units;
    u16 actual_n=map_n;s16 actual_sc_h=sc_h,actual_sc_top=sc_top;
    CHECK(sizeof mod_images,4*MOD_COUNT);CHECK(sizeof expected,384);
    for(pass=0;pass<2;pass++){
        if(pass){map_w=1;map_h=1;map_n=1;encounter_units=13;sc_h=200;sc_top=1;}
        expected_context();
        for(id=0;id<MOD_COUNT;id++){
            ModCtx *c;ZB(61);mod_call(id,255,0);ZE(61);c=mod_context(id);
            CHECK(mod_id,id+1);CHECK(memcmp(c,&expected,sizeof expected)==0,1);
            CHECK(rng32==seed,1);
            c->cast_ability=0x123;c->cast_actor=12;c->cast_ref=255;
            ZB(62);mod_call(id,255,0);ZE(62);CHECK(c->cast_ability,0x123);
        }
    }
    // Both directories are resolved into one stable cache in original ID order.
    bank=rt_file("ffbm",&size);CHECK(MOD_BANK0_COUNT>0&&MOD_BANK0_COUNT<MOD_COUNT,1);
    CHECK(mod_images[0]==bank+RD16(bank,7),1);
    bank=rt_file("ffbn",&size);CHECK(mod_images[MOD_BANK0_COUNT]==bank+RD16(bank,7),1);
    CHECK(mod_images[MOD_COUNT-1]==bank+RD16(bank,7+MOD_COUNT-1-MOD_BANK0_COUNT),1);
    // The1x1 context above is not a playable map. Restore real dimensions before
    // an injection door, which now checks on-field team survival correctly.
    map_w=actual_w;map_h=actual_h;map_n=actual_n;encounter_units=actual_units;
    sc_h=actual_sc_h;sc_top=actual_sc_top;mod_id=0;
    // A same-chapter retry may leave SAVE warm with obsolete inputs/bindings.
    // Loading must force a fresh context before asking the cold validator.
    game_scenario(61);
    for(pass=0;pass<3;pass++) {
        ModCtx *c;
        mod_call(MOD_SAVE,255,0);c=mod_context(MOD_SAVE);
        c->cdesc=RT_NULL;c->w=RT_NULL;c->desc_size=c->interface_size=65535;
        CHECK(chapter_load(1),1);CHECK(chapter_now,2);CHECK(W!=RT_NULL,1);
        mod_call(MOD_SAVE,255,0);c=mod_context(MOD_SAVE);
        CHECK(c->cdesc==cdesc,1);CHECK(c->w==W,1);
        c->desc_size=0;c->interface_size=0;
        ZB(63);CHECK(mod_call(MOD_SAVE,4,0),0);ZE(63);
    }
    // Restore real unit state and test the next launch's reset path.
    game_scenario(61);CHECK(mod_ready(),1);CHECK(mod_id,0);
}
#endif
#ifndef __m68k__
int main(void){puts("Native loader binding fixture; parser semantics have a separate portable checker.");return 0;}
#else
void _main(void){bank_runtime_main();BENCH_VALUE(W!=RT_NULL);if(W)bindings();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
