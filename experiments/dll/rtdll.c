// Can a runtime game (greys, interrupts, its own heap) load a module (TIGCC DLL) and let it
// draw through a context? Unpacked program, plain loader: the Titanium configuration
// (ti68k-c-patterns.md: DLLs). ESC quits; the screen shows the frames the module counted.
#include "../../runtime/core/rt.h"
#include "rtmod.h"
#ifdef __m68k__
#include <dll.h>
#define mod_frame _DLL_call(u16, (const ModCtx *), 0)
#endif
static u16 counter;
static short loaded;
static ModCtx ctx;
static void rect(s16 x, s16 y, s16 w, s16 h, u8 c) { draw_rect(x, y, w, h, c); }
static void text(s16 x, s16 y, const char *s, u8 f, u8 c) { draw_text(x, y, s, f, c); }
void game_init(void)
{
    counter = 0; ctx.counter = &counter; ctx.rect = rect; ctx.text = text;
#ifdef __m68k__
    loaded = LoadDLL("rtmod", RTMOD_ID, 1, 0);
#endif
}
void game_scenario(u16 n) { (void)n; }
u8 game_update(void)
{
#ifdef __m68k__
    if (input_pressed(K_ESC)) { if (loaded == DLL_OK) UnloadDLL(); return 0; }
#else
    if (input_pressed(K_ESC)) return 0;
#endif
    return 1;
}
void game_render(void)
{
    draw_rect(0, 0, 160, 100, C_LGRAY);
#ifdef __m68k__
    if (loaded == DLL_OK) mod_frame(&ctx);
    else { char s[] = "LoadDLL ?"; s[8] = (char)('0' + loaded); draw_text(4, 4, s, F_SMALL, C_BLACK); }
#endif
}
