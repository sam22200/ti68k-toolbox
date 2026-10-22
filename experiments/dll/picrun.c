// A position-independent module (an archived data variable: "FMD1", entry, size, image)
// copied into a zone reserved inside the program and called with a context, in a runtime
// game that allocated its buffers first.
#include "../../runtime/core/rt.h"
#include <string.h>
typedef struct { u16 *counter; void (*rect)(s16, s16, s16, s16, u8); void (*text)(s16, s16, const char *, u8, u8); } ModCtx;
static u16 zone[512] = { 1 };                    // the modules' zone, inside the program
static u16 counter, entry; static u8 ok;
static ModCtx ctx;
static void rect(s16 x, s16 y, s16 w, s16 h, u8 c) { draw_rect(x, y, w, h, c); }
static void text(s16 x, s16 y, const char *s, u8 f, u8 c) { draw_text(x, y, s, f, c); }
void game_init(void)
{
    const u8 *m = rt_file("picmod", RT_NULL);
    ctx.counter = &counter; ctx.rect = rect; ctx.text = text;
    ok = m && m[0] == 'F' && m[3] == '1' && (u16)(m[6] << 8 | m[7]) <= sizeof zone;
    if (ok) { entry = (u16)(m[4] << 8 | m[5]); memcpy(zone, m + 8, (u16)(m[6] << 8 | m[7])); }
}
void game_scenario(u16 n) { (void)n; }
u8 game_update(void) { return !input_pressed(K_ESC); }
void game_render(void)
{
    draw_rect(0, 0, 160, 100, C_LGRAY);
    if (ok) ((u16 (*)(const ModCtx *))((u8 *)zone + entry))(&ctx);
    else draw_text(4, 4, "no module", F_SMALL, C_BLACK);
}
