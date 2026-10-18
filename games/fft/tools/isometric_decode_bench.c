// Headless 68000: time the project's existing verified ZX0 decoder, not edge drawing.
#include <stdlib.h>
#include "../../../runtime/core/rt.h"
#include "../../../tools/m68kbench/bench.h"
#include "../../../lib/unpack68k.h"
static const u8 *packed;
static u8 *output;
void game_init(void)
{
    packed = rt_file("isocost",RT_NULL); output = malloc(52320);
}
void game_scenario(u16 n) { (void)n; }
u8 game_update(void)
{
    u8 *end; u16 a = 0, b = 0, n; const u8 *p;
    if (!packed || !output) { BENCH_VALUE(0xBAD); return 0; }
    BENCH_NAME(4,"ZX0 view decode"); BENCH_BEGIN(4);
    end = zx0_asm(packed,output);
    BENCH_END(4); BENCH_VALUE(end-output);
    for (p = output,n = 52320; n; n--) {
        a += *p++; if (a >= 255) a -= 255;
        b += a; if (b >= 255) b -= 255;
    }
    BENCH_VALUE((u16)((b << 8)|a));
    free(output); output = RT_NULL;
    return 0;
}
void game_render(void) { }
