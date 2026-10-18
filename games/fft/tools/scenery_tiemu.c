// Actual TiEmu timer benchmark, grayscale enabled. The ordinary backend main is unused.
#define USE_TI89
#define MIN_AMS 200
#define SAVE_SCREEN
#include <tigcclib.h>
#undef _main
#define _main rt_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off // AMS off() macro conflicts with local offsets in the included game implementation
#include "fixed_scenery_bench.c"

typedef struct { u16 cold, raw, capped; } EmuResult;
static EmuResult result[8];

void _main(void)
{
    INT_HANDLER old1 = GetIntVec(AUTO_INT_1);
    short oldfont = FontGetSys();
    void *dbuf = RT_NULL;
    u16 c, n, t0, last;
    u8 ok = 0;
    memset(result, 0, sizeof result);
    tm_big = 0; tm_cur.map = 0;
    rt_frame = 0; rt_keys = rt_prev = 0; rt_seed = 1; rt_state = 0; rt_state_size = 0;
    if (!get_fonts()) return;
    dbuf = malloc(GRAYDBUFFER_SIZE); if (!dbuf) return;
    SetIntVec(AUTO_INT_1, DUMMY_HANDLER);
    if (!GrayOn()) goto done;
    GraySetInt1Handler(tick_handler); GrayDBufInit(dbuf);
    rt_light = GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark = GrayDBufGetHiddenPlane(DARK_PLANE);
    game_init();
    if (!W || !pbank) goto grey_done;
    for (c = 0; c < 8; c++) {
        game_scenario(c); if (!bench_texture) goto grey_done;
        t0 = ticks; game_update(); game_render(); result[c].cold = ticks-t0;
        for (n = 0; n < 32; n++) { game_update(); game_render(); rt_frame++; }
        t0 = ticks;
        for (n = 0; n < 512; n++) { game_update(); game_render(); rt_frame++; }
        result[c].raw = ticks-t0;
        last = t0 = ticks;
        for (n = 0; n < 128; n++) {
            rt_prev = rt_keys; rt_keys = read_keys();
            game_update(); game_render(); GrayDBufToggleSync();
            rt_light = GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark = GrayDBufGetHiddenPlane(DARK_PLANE);
            while ((u16)(ticks-last) < 8) pokeIO(0x600005, 0x1D);
            last += 8;
            if ((u16)(ticks-last) > 16) last = ticks;
            rt_frame++;
        }
        result[c].capped = ticks-t0;
    }
    ok = 1;
grey_done:
    GraySetInt1Handler(DUMMY_HANDLER); GrayOff();
done:
    SetIntVec(AUTO_INT_1, old1); free(dbuf);
    ClrScr(); FontSetSys(F_4x6);
    if (!ok) printf_xy(0, 0, "missing data or not enough RAM");
    else {
        printf_xy(0, 0, "TIEMU HW%u: actors in both", (u16)HW_VERSION);
        printf_xy(0, 8, "view tex coldms rawFPS capFPS");
        for (c = 0; c < 8; c++) {
            const char *view = (c & 3) == 0 ? "CF" : (c & 3) == 1 ? "CB" : (c & 3) == 2 ? "WF" : "WB";
            u32 raw = 512UL*2560/result[c].raw, cap = 128UL*2560/result[c].capped;
            printf_xy(0, 17+c*8, "%s %c %4lu %3lu.%lu %2lu.%lu", view, c < 4 ? 'T' : 'F',
                      (u32)result[c].cold*1000/256, raw/10, raw%10, cap/10, cap%10);
        }
        printf_xy(0, 83, "raw:512 anim frames; cap:128");
        printf_xy(0, 91, "cached scenery; TIEMU timings");
    }
    GKeyFlush(); ngetchx(); FontSetSys(oldfont);
}
