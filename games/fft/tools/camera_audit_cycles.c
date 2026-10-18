// Same case driver, no hardware paths; datasheet counts and per-frame PC/TI checks.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main rt_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "camera_audit.inc"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
static u16 audit_state_hash(void)
{
    const u8 *p=(const u8 *)&st; unsigned n; u16 a=0,b=0;
    for (n=0;n<sizeof st;n++) { a=(a+p[n])%255; b=(b+a)%255; }
    return (u16)(b<<8 | a);
}
int main(int argc,char **argv)
{
    unsigned c;
    sw_init(65);
    for (c = 0; c < AUDIT_N; c++) {
        audit_setup((u8)c);
        do {
            if (!audit_next()) return 2;
            if (argc>1) { char path[1024];
                snprintf(path,sizeof path,"%s/%02u-%03u.png",argv[1],c,audit_frame);
                if (sw_write_png(path,1)) return 4; }
            printf("%u,%u,%u,%04X,%04X,%lu,%u\n",c,audit_frame,audit_phase,sw_checksum(),
                audit_state_hash(),(unsigned long)rng32,rt_seed);
        } while (!audit_done() && audit_frame < 128);
        if (!audit_verify()) return 3;
    }
    return 0;
}
#else
void _main(void)
{
    void *planes = malloc(2*RT_PSIZE);
    u16 c;
    rt_keys = rt_prev = 0; rt_seed = 1; rt_frame = 0; rt_state = 0; rt_state_size = 0;
    if (!planes || !get_fonts()) return;
    rt_dark = planes; rt_light = (u8 *)planes+RT_PSIZE; BENCH_DARK_FIRST(1);
    game_init();
    for (c = 0; c < AUDIT_N; c++) {
        audit_setup((u8)c);
        BENCH_VALUE(0x61000000UL | c);
        do {
            u32 start = BENCH_CYCLES;
            if (!audit_next()) break;
            BENCH_VALUE(0x62000000UL | audit_phase);
            BENCH_VALUE(BENCH_CYCLES-start);
            BENCH_SHOT(rt_dark);
        } while (!audit_done() && audit_frame < 128);
        BENCH_VALUE(0x63000000UL | audit_verify());
    }
    free(planes);
}
#endif
