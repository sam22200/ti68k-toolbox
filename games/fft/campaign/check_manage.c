// The management menus (manage.c) run on the PC and as the TI binary (ti-cycles): the same
// screen and the same edited party, per key script (check.py).
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main manage_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "manage.c"
#ifndef __m68k__
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
static u32 state_hash(void)
{
    u32 h = 17;
    u16 i;
    const u8 *p = (const u8 *)&save;
    for (i = 0; i < sizeof save; i++) h = (h << 1 | h >> 31) ^ p[i];
    h = (h << 1 | h >> 31) ^ ((u32)mg.screen << 16 | (u16)mg.unit << 8 | (u8)(mg.top + mg.row));
    return h;
}
#ifndef __m68k__
int main(int argc, char **argv)
{
    SwScript script; unsigned f, limit;
    if (argc != 5) return 2;
    limit = (unsigned)atoi(argv[2]); if (sw_load_script(&script, argv[3])) return 3;
    sw_init((u16)atoi(argv[1]));
    for (f = 0; f < limit; f++) if (!sw_step(sw_script_keys(&script, rt_frame))) break;
    printf("executed %u checksum %04X state %08lX\n", rt_frame, sw_checksum(), (unsigned long)state_hash());
    return sw_write_png(argv[4], 1);
}
#else
void _main(void) { manage_runtime_main(); BENCH_VALUE(0x4D4E4701UL); BENCH_VALUE(state_hash()); }
#endif
