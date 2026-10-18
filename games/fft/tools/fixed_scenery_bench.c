// Shared PC/TI benchmark harness. Prepared on PC; run TI only with user authorization.
// Cases 0..3: textured scenery; cases 4..7: flat scenery. Actors stay drawn in every case.
#define game_scenario fft_original_scenario
#define game_render fft_original_render
#define game_update fft_original_update
#include "../fft.c"
#undef game_scenario
#undef game_render
#undef game_update
#ifndef __m68k__
#include <stdio.h>
static u8 bench_texture_pc[6000]; // PC rt_file has only eight cache slots, all used by FFT
#endif

static u8 bench_flat, bench_view;
static const u8 *bench_texture;

u8 game_update(void) { st.tick++; return 1; } // same idle inspection update; no tactical work

void game_scenario(u16 n)
{
    char name[6] = "fftb0";
    bench_view = n & 3; bench_flat = (n & 4) != 0;
    fft_original_scenario(62);
    if (bench_view >= 2) { st.pdist = 80; st.ppitch = 6; }
    if (bench_view & 1) st.pyaw ^= 32;
    name[4] += bench_view;
#ifdef __m68k__
    bench_texture = rt_file(name, RT_NULL);
#else
    { char path[16]; FILE *file;
      snprintf(path, sizeof path, "%s.bin", name); file = fopen(path, "rb");
      bench_texture = RT_NULL;
      if (file) {
          if (fread(bench_texture_pc, 1, sizeof bench_texture_pc, file) == sizeof bench_texture_pc)
              bench_texture = bench_texture_pc;
          fclose(file);
      } }
#endif
}

void game_render(void)
{
    u8 side = st.pyaw != st.pbase;
#ifdef RT_CYCLES
    u32 start = BENCH_CYCLES;
#endif
    if (!pvalid) { memset(FW, 0, sizeof *FW); pvalid = 1; }
    if (!FW->view[side].valid) {
        fprepare(side);
        if (!bench_flat && bench_texture) memcpy(scene_l+side*6000, bench_texture, 6000);
    }
    perspective_render(); // settled inspection only; same renderer, excludes unused tactical UI
#ifdef RT_CYCLES
    BENCH_VALUE(0x78000000UL | rt_frame); BENCH_VALUE(BENCH_CYCLES-start);
#endif
}

#ifndef __m68k__
#include <stdio.h>
#include <stdlib.h>
#include "../../../runtime/platform-sw/rt_sw.h"
int main(int argc, char **argv)
{
    unsigned n, frames;
    if (argc != 3) return 2;
    frames = (unsigned)atoi(argv[2]); sw_init((u16)atoi(argv[1]));
    if (!bench_texture) return 3;
    for (n = 0; n < frames; n++) {
        if (!sw_step(0)) return 4;
        printf("%u %04X\n", n, sw_checksum());
    }
    return 0;
}
#endif
