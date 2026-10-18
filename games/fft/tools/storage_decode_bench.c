// Read-only audit: existing verified ZX0 decoder, datasheet cycles and output checksum.
#include <stdlib.h>
#include "../../../runtime/core/rt.h"
#include "../../../tools/m68kbench/bench.h"
#include "../../../lib/unpack68k.h"
static const u8 *packed;
static u8 *output;
static u16 length;
void game_init(void)
{
    packed = rt_file("audpack", RT_NULL);
    length = packed ? (u16)((u16)packed[0] << 8 | packed[1]) : 0;
    output = length ? malloc(length) : RT_NULL;
}
void game_scenario(u16 n) { (void)n; }
u8 game_update(void)
{
    const u8 *p; u8 *end; u16 a = 0, b = 0, n;
    if (!packed || !output) { BENCH_VALUE(0xBAD); return 0; }
    BENCH_NAME(4, "Audit ZX0 decode"); BENCH_BEGIN(4);
    end = zx0_asm(packed + 2, output);
    BENCH_END(4); BENCH_VALUE(end - output);
    for (p = output, n = length; n; n--) {
        a += *p++; if (a >= 255) a -= 255;
        b += a; if (b >= 255) b -= 255;
    }
    BENCH_VALUE((u16)((b << 8) | a));
    free(output); output = RT_NULL;
    return 0;
}
void game_render(void) { }
