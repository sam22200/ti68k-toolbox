// Actual TiEmu timer run: grayscale, buffer swaps and the normal 32 FPS limiter.
#define USE_TI89
#define MIN_AMS 200
#define SAVE_SCREEN
#include <tigcclib.h>
#undef _main
#define _main rt_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#include "camera_audit.inc"

typedef struct { u16 total, worst, frames, phase[4]; u8 ok; } AuditResult;
static AuditResult audit_result[AUDIT_N];

void _main(void)
{
    INT_HANDLER old1 = GetIntVec(AUTO_INT_1);
    short oldfont = FontGetSys();
    void *dbuf = RT_NULL;
    u16 c, last, start, elapsed, k;
    u8 ready = 0;
    memset(audit_result, 0, sizeof audit_result);
    tm_big = 0; tm_cur.map = 0;
    rt_frame = 0; rt_keys = rt_prev = 0; rt_seed = 1; rt_state = 0; rt_state_size = 0;
    if (!get_fonts()) return;
    dbuf = malloc(GRAYDBUFFER_SIZE); if (!dbuf) return;
    SetIntVec(AUTO_INT_1, DUMMY_HANDLER);
    if (!GrayOn()) goto done;
    GraySetInt1Handler(tick_handler); GrayDBufInit(dbuf);
    rt_light = GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark = GrayDBufGetHiddenPlane(DARK_PLANE);
    game_init();
    for (c = 0; c < AUDIT_N; c++) {
        AuditResult *r = &audit_result[c];
        audit_setup((u8)c); if (!audit_ok) goto grey_done;
        // Show the case before its first expensive render, outside the timed interval.
        draw_rect(0,0,160,8,C_WHITE); draw_text(2,0,audit_name[c],F_SMALL,C_BLACK);
        GrayDBufToggleSync();
        rt_light = GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark = GrayDBufGetHiddenPlane(DARK_PLANE);
        last = ticks;
        do {
            start = ticks;
            if (!audit_next()) goto grey_done;
            GrayDBufToggleSync();
            rt_light = GrayDBufGetHiddenPlane(LIGHT_PLANE); rt_dark = GrayDBufGetHiddenPlane(DARK_PLANE);
            while ((u16)(ticks-last) < 8) pokeIO(0x600005, 0x1D);
            last += 8; if ((u16)(ticks-last) > 16) last = ticks;
            elapsed = ticks-start; r->total += elapsed; r->phase[audit_phase] += elapsed;
            if (elapsed > r->worst) r->worst = elapsed;
        } while (!audit_done() && audit_frame < 128);
        r->frames = audit_frame; r->ok = audit_verify();
    }
    ready = 1;
grey_done:
    GraySetInt1Handler(DUMMY_HANDLER); GrayOff();
done:
    SetIntVec(AUTO_INT_1, old1); free(dbuf);
    ClrScr(); FontSetSys(F_4x6);
    if (!ready) printf_xy(0,0,"AUDIT FAILED: missing data/RAM");
    else {
        printf_xy(0,0,"TIEMU HW%u 256Hz; 32fps cap",(u16)HW_VERSION);
        printf_xy(0,7,"case    total ms  n max ms");
        for (c = 0; c < AUDIT_N; c++) {
            const AuditResult *r = &audit_result[c];
            printf_xy(0,14+c*6,"%s %6lu %3u %5lu%c",audit_name[c],
                (u32)r->total*1000/256,r->frames,(u32)r->worst*1000/256,r->ok ? ' ' : '!');
        }
        // A second page is available after ENTER; no additional run is required.
        GKeyFlush(); ngetchx(); ClrScr();
        printf_xy(0,0,"TIEMU cinematic stage ms");
        for (c = 7; c <= 8; c++) {
            printf_xy(0,12+(c-7)*32,"%s: enter / action / return",audit_name[c]);
            for (k = 1; k <= 3; k++) printf_xy((k-1)*50,24+(c-7)*32,"%lu",(u32)audit_result[c].phase[k]*1000/256);
        }
    }
    GKeyFlush(); ngetchx(); FontSetSys(oldfont);
}
