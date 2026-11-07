// Sweep the shared archive through the actual PC/68000 reader. All 512 IDs,
// sparse 16-action lists, six R/S/M slots and monster sets must survive unchanged.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main catalogue_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "../common.h"

static const u8 *catalogue;
static u16 catalogue_size;
static u32 result;
static u16 failure;

static u32 hash_byte(u32 h, u16 v) { return (h << 1 | h >> 31) ^ v; }

#define REQUIRE(test, code) do { if (!(test)) { failure = code; return 0; } } while (0)
static u32 check(void)
{
    u32 h = 17;
    u16 id, k;
    u8 set;
    const u8 *p;
    u8 header[CJ_HEADER];
    REQUIRE(cj_valid(catalogue, catalogue_size), 1);
    REQUIRE(cj_valid(catalogue, (u16)(cj_word(catalogue + CJ_LENGTH) + 6)), 2); // AMS OTH trailer
    REQUIRE(!cj_valid(catalogue, (u16)(cj_word(catalogue + CJ_LENGTH) - 1)), 3);
    REQUIRE(!cj_valid(catalogue, 31) && !cj_valid(RT_NULL, 0), 4);
    memcpy(header, catalogue, sizeof header);
    header[3] = '9';
    REQUIRE(!cj_valid(header, catalogue_size), 31); // reject previous Item metadata contract
    memcpy(header, catalogue, sizeof header);
    header[4] = header[5] = 0;
    REQUIRE(!cj_valid(header, catalogue_size), 5); // overlapping header
    memcpy(header, catalogue, sizeof header);
    header[CJ_LENGTH] = header[CJ_LENGTH + 1] = 0;
    REQUIRE(!cj_valid(header, catalogue_size), 6);
    REQUIRE(!cj_primary(catalogue, 512) && !cj_range(catalogue, 0x170), 7);
    REQUIRE(!*cj_ability_name(catalogue, 512) && !*cj_set_name(catalogue, 64), 8);
    REQUIRE(cj_word(cj_primary(catalogue, 0x92)) == 300, 9);
    REQUIRE(cj_word(cj_primary(catalogue, 0x170)) == 30, 10);
    REQUIRE(cj_word(cj_primary(catalogue, 0x17D)) == 90, 11);
    REQUIRE(!memcmp(cj_ability_name(catalogue, 0x1E0), "Equip Change", 13), 12);
    REQUIRE(!memcmp(cj_set_name(catalogue, 0x19), "Guts", 5), 13);
    REQUIRE(cj_set_ability(catalogue, 6, 13) == 0x17D, 14);
    REQUIRE(!cj_set_ability(catalogue, 5, 4) && !cj_set_ability(catalogue, 5, 22), 15);
    REQUIRE(!cj_set_ability(catalogue, 224, 0) && !cj_set_ability(catalogue, 176, 4), 16);
    REQUIRE(cj_range(catalogue, 0x93)[8] == 0x37, 17);  // Dash's formula
    REQUIRE(!memcmp(cj_battle_name(catalogue, 0x196), "Charge +1", 10), 18);
    REQUIRE(!memcmp(cj_battle_name(catalogue, 0x197), "Charge +2", 10), 19);
    REQUIRE(!memcmp(cj_section(catalogue,CJ_ITEMS)+30*0x91+10,"Bronze Helmet",14),20);
    for(id=0;id<256;id++) {
        p=cj_section(catalogue,CJ_ITEMS)+30*id+10;
        for(k=0;k<17;k++)h=hash_byte(h,p[k]);
    }
    for (id = 0; id < CJ_ABILITIES; id++) {
        p = cj_primary(catalogue, id);
        h = hash_byte(h, cj_word(p));
        for (k = 2; k < 8; k++) h = hash_byte(h, p[k]);
        p = (const u8 *)cj_ability_name(catalogue, id);
        do { h = hash_byte(h, *p); } while (*p++);
        if (id < CJ_DEFAULT) {
            p = cj_range(catalogue, id);
            for (k = 0; k < 14; k++) h = hash_byte(h, p[k]);
        }
    }
    REQUIRE(cj_action_menu(catalogue,5)==0 && cj_action_menu(catalogue,6)==1 &&
            cj_action_menu(catalogue,8)==10 && cj_action_menu(catalogue,224)==255,21);
    memcpy(header,catalogue,sizeof header);header[3]='7';
    REQUIRE(!cj_valid(header,catalogue_size),22);
    memcpy(header,catalogue,sizeof header);
    id=cj_word(header+4+2*CJ_SPECIAL)+168;
    header[4+2*CJ_HUMAN]=(u8)(id>>8);header[5+2*CJ_HUMAN]=(u8)id;
    REQUIRE(!cj_valid(header,catalogue_size),23); // missing dispatch table
    memcpy(header,catalogue,sizeof header);
    id=cj_word(header+4+2*CJ_STATUS)+128*6;
    header[4+2*CJ_BATTLE]=(u8)(id>>8);header[5+2*CJ_BATTLE]=(u8)id;
    REQUIRE(!cj_valid(header,catalogue_size),24); // missing status policy
    p=cj_status_rules(catalogue);
    for(id=0;id<40*16+10;id++)h=hash_byte(h,p[id]);
    for (set = 0; set < CJ_SETS; set++) {
        h=hash_byte(h,cj_action_menu(catalogue,set));
        u8 n = set < CJ_HUMANS ? 22 : 4;
        for (k = 0; k < n; k++) h = hash_byte(h, cj_set_ability(catalogue, set, (u8)k));
    }
    for (id = 0; id < CJ_NAMED_SETS; id++) {
        p = (const u8 *)cj_set_name(catalogue, (u8)id);
        do { h = hash_byte(h, *p); } while (*p++);
    }
    return h;
}
#undef REQUIRE

void game_init(void)
{
    failure = 0; result = 0;
    catalogue = rt_file("ffcj", &catalogue_size);
}
void game_scenario(u16 n) { (void)n; }
u8 game_update(void) { result = check(); return 0; }
void game_render(void) {}

#ifndef __m68k__
int main(void)
{
    sw_init(0); sw_step(0);
    printf("catalogue %08lX failure %u\n", (unsigned long)result, failure);
    return failure != 0;
}
#else
void _main(void)
{
    catalogue_runtime_main();
    BENCH_VALUE(result); BENCH_VALUE(failure);
}
#endif
