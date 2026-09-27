// TI-89 / Titanium backend: NOSTUB _main, 256 Hz tick on auto-int 1, keyboard matrix read once
// per frame, drawing with ExtGraph 2 into GrayDBuf hidden planes (RT_MONO: one offscreen plane
// copied to LCD_MEM). Run as prog() or prog(N) to start in game_scenario(N).
// -DRT_BENCH=N: non-UI benchmark, N updates then N renders from scenario prog(S), prints the
// cycles per frame and waits for a key.
#define USE_TI89
#define MIN_AMS 200                                // fonts read in place (OO_CondGetAttr)
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"
#include "tilemap.h"
#include "../core/rt.h"

#if C_WHITE != COLOR_WHITE || C_LGRAY != COLOR_LIGHTGRAY || C_DGRAY != COLOR_DARKGRAY || C_BLACK != COLOR_BLACK
#error rt.h colours must equal ExtGraph's COLOR_*
#endif

void rt_fill(u16 x1, u16 y1, u16 x2, u16 y2, u8 color);
void rt_clamp_cam(const RtTilemap *m, s16 *cx, s16 *cy);

static volatile u16 ticks;
#ifdef RT_BENCH
static u16 bench_upd, bench_rnd;
#endif
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
u16 rt_ticks(void) { return ticks; }

// TI-89 matrix (c-patterns §5): row 0 = up left down right 2nd shift diamond alpha = K_UP..K_D;
// ENTER row 1 b0, ESC row 6 b0; digits in rows 4, 3, 2 (columns 1-4-7, 2-5-8, 3-6-9) bits 1-3.
static u32 read_keys(void)
{
    static const u16 spread[8] = { 0, 1, 8, 9, 64, 65, 72, 73 };   // bits 0 1 2 -> 0 3 6
    u16 r0 = _rowread(~1), r1 = _rowread(~2), r2 = _rowread(~4), r3 = _rowread(~8),
        r4 = _rowread(~16), r6 = _rowread(~64);
    u16 pad = spread[(r4 >> 1) & 7] | spread[(r3 >> 1) & 7] << 1 | spread[(r2 >> 1) & 7] << 2;
    return (r0 & 0xFF) | (r1 & 1) << 8 | (r6 & 1) << 9 | (u32)pad << 16;
}

void draw_clear(void)
{
#ifdef RT_MONO
    FastClearScreen_R(rt_light);
#else
    GrayClearScreen2B_R(rt_light, rt_dark);
#endif
}

void rt_fill(u16 x1, u16 y1, u16 x2, u16 y2, u8 color)
{
#ifdef RT_MONO
    FastFillRect_R(rt_light, x1, y1, x2, y2, color & 2 ? A_NORMAL : A_REVERSE);
#else
    GrayFastFillRect_R(rt_light, rt_dark, x1, y1, x2, y2, color);
#endif
}

void draw_sprite(s16 x, s16 y, const RtSprite *s)
{
    void *l = rt_light;
#ifdef RT_MONO
    if (s->mask) switch (s->w) {
        case 8:
            // ExtGraph 2 bug: ClipSprite8_MASK_R draws garbage when x & 15 = 9..15 (its long
            // path does swap %d1 instead of swap %d0; xcheck scene 1). The grey routine with
            // the same plane twice is correct and idempotent.
            if ((u16)(x - 1) < 231 && (x & 15) > 8)
                GrayClipSprite8_MASK_R(x, y, s->h, s->dark, s->dark, s->mask, s->mask, l, l);
            else ClipSprite8_MASK_R(x, y, s->h, s->dark, s->mask, l);
            break;
        case 16: ClipSprite16_MASK_R(x, y, s->h, s->dark, s->mask, l); break;
        default: ClipSprite32_MASK_R(x, y, s->h, s->dark, s->mask, l);
    } else switch (s->w) {
        case 8:  ClipSprite8_RPLC_R(x, y, s->h, s->dark, l); break;
        case 16: ClipSprite16_RPLC_R(x, y, s->h, s->dark, l); break;
        default: ClipSprite32_RPLC_R(x, y, s->h, s->dark, l);
    }
#else
    void *d = rt_dark;
    if (s->mask) switch (s->w) {
        case 8:  GrayClipSprite8_MASK_R(x, y, s->h, s->light, s->dark, s->mask, s->mask, l, d); break;
        case 16: GrayClipSprite16_MASK_R(x, y, s->h, s->light, s->dark, s->mask, s->mask, l, d); break;
        default: GrayClipSprite32_MASK_R(x, y, s->h, s->light, s->dark, s->mask, s->mask, l, d);
    } else switch (s->w) {
        case 8:  GrayClipSprite8_RPLC_R(x, y, s->h, s->light, s->dark, l, d); break;
        case 16: GrayClipSprite16_RPLC_R(x, y, s->h, s->light, s->dark, l, d); break;
        default: GrayClipSprite32_RPLC_R(x, y, s->h, s->light, s->dark, l, d);
    }
#endif
}

// ---------------------------------------------------------------- tile map: ExtGraph TileMap
// A 272x160 buffer (per plane) holds the tiles around the view, rebuilt only when the view
// leaves its margin; each frame it is blitted with the sub-tile offset (performance §6).
static Plane tm_pl;
static char *tm_big;
static RtTilemap tm_cur;                           // the map the plane was set up for (a copy:
                                                   // games may pass one struct refilled per room)
#ifdef RT_MONO
static u16 *tm_mono;                               // dark rows only, 16 per tile
#endif

void tilemap_dirty(void) { tm_pl.force_update = 1; }

static char *symstr(char *sym, const char *name)   // "\0name\0" -> the SYMSTR pointer
{
    char *d = sym;
    *d++ = 0;
    while (*name && d < sym + 10) *d++ = *name++;
    *d = 0;
    return d;
}

#define SAV_EXTRA 6                                // 0, "sav", 0, OTH_TAG
u8 rt_load(const char *name, void *data, u16 size)
{
    char sym[12];
    SYM_ENTRY *e = SymFindPtr(symstr(sym, name), 0);
    const unsigned char *p;
    if (!e) return 0;
    p = HeapDeref(e->handle);
    if (*(const u16 *)p != size + SAV_EXTRA || p[2 + size + SAV_EXTRA - 1] != OTH_TAG) return 0;
    memcpy(data, p + 2, size);
    return 1;
}

static const void *sv_data;
static u16 sv_size;
static char sv_name[10];

u8 rt_save(const char *name, const void *data, u16 size)
{
    u8 k;
    for (k = 0; name[k] && k < 8; k++) sv_name[k] = name[k];
    sv_name[k] = 0;
    sv_data = data;
    sv_size = size;
    return 1;
}

static void save_write(void)                       // after the teardown: c-patterns §10
{
    char sym[12], *s = symstr(sym, sv_name);
    SYM_ENTRY *e = SymFindPtr(s, 0);
    HANDLE h;
    HSym hs;
    unsigned char *p;
    if (e && e->flags.bits.archived) EM_moveSymFromExtMem(s, HS_NULL);   // unarchive FIRST
    h = HeapAlloc(2 + sv_size + SAV_EXTRA);
    if (h == H_NULL) return;
    hs = SymAdd(s);
    if (hs.folder == 0) { HeapFree(h); return; }
    DerefSym(hs)->handle = h;
    p = HeapDeref(h);
    *(u16 *)p = sv_size + SAV_EXTRA;
    memcpy(p + 2, sv_data, sv_size);
    p += 2 + sv_size;
    *p++ = 0; *p++ = 's'; *p++ = 'a'; *p++ = 'v'; *p++ = 0; *p = OTH_TAG;
    EM_moveSymToExtMem(s, HS_NULL);                // archived: survives a RAM reset
}

const void *rt_file(const char *name, u16 *size)   // c-patterns §1: data read in place
{
    char sym[12], *d = sym;
    SYM_ENTRY *e;
    const unsigned char *p;
    *d++ = 0;
    while (*name && d < sym + 10) *d++ = *name++;
    *d = 0;
    e = SymFindPtr(d, 0);                          // SYMSTR form: a pointer to the final 0
    if (!e) return RT_NULL;
    if (!e->flags.bits.archived) HLock(e->handle); // a RAM variable must not move
    p = HeapDeref(e->handle);
    if (size) *size = *(const u16 *)p;             // data + 0 + "dat" + 0 + OTH tag
    return p + 2;
}

void draw_tilemap(const RtTilemap *m, s16 camx, s16 camy)
{
    if (!tm_big && !(tm_big = malloc(GRAY_BIG_VSCREEN_SIZE))) return;
    rt_clamp_cam(m, &camx, &camy);
    if (m->map != tm_cur.map || m->tiles != tm_cur.tiles || m->w != tm_cur.w) {
#ifdef RT_MONO
        u16 k, *d;
        const u16 *t = m->tiles;
        if (tm_mono) free(tm_mono);                // AMS free(NULL) is not safe
        if (!(d = tm_mono = malloc(m->ntiles * 32))) { tm_cur.map = 0; return; }
        for (k = m->ntiles * 16; k--; t += 2) *d++ = *t;
        tm_pl.sprites = tm_mono;
#else
        tm_pl.sprites = (void *)m->tiles;
#endif
        tm_pl.matrix = (void *)m->map;
        tm_pl.width = m->w;
        tm_pl.big_vscreen = tm_big;
        tm_pl.force_update = 1;
        tm_cur = *m;
    }
#ifdef RT_MONO
    DrawPlane(camx, camy, &tm_pl, rt_light, TM_RPLC89, TM_16B);
#else
    DrawPlane(camx, camy, &tm_pl, rt_dark, TM_GRPLC89, TM_G16B);   // dark plane first (GrayDBuf)
#endif
}

// ---------------------------------------------------------------- text: AMS fonts in place
// One aligned long OR per glyph row (performance §6, 6.8x faster than DrawStr).
static const u8 *font[2];

static void text_plane(u8 *plane, s16 x, s16 y, const u8 *s, u8 f)
{
    u16 rows = f == F_SMALL ? 5 : 8;
    u8 *row;
    if ((u16)y > RT_PH - 1 - rows) return;
    row = plane + y * RT_PBYTES;
    for (; *s; s++) {
        const u8 *g;
        u16 adv, r;
        if (f == F_SMALL) { g = font[0] + *s * 6; adv = *g++; }
        else { g = font[1] + (*s << 3); adv = 6; }
        if ((u16)x <= RT_PW - 16) {
            u32 *p = (u32 *)(row + ((x >> 3) & ~1));
            u16 sh = 24 - (x & 15);
            for (r = rows; r--; p = (u32 *)((u8 *)p + RT_PBYTES)) *p |= (u32)*g++ << sh;
        }
        x += adv;
    }
}

void draw_text(s16 x, s16 y, const char *s, u8 f, u8 color)
{
#ifdef RT_MONO
    if (color & 2) text_plane(rt_light, x, y, (const u8 *)s, f);
#else
    if (color & 1) text_plane(rt_light, x, y, (const u8 *)s, f);
    if (color & 2) text_plane(rt_dark, x, y, (const u8 *)s, f);
#endif
}

static short get_fonts(void)                       // c-patterns / performance §6
{
    pFrame fr = 0xFF000000UL;                      // no running app: the system frame
    short app = EV_runningApp, k;
    if (app) fr = *(pFrame *)((char *)HeapDeref(app) + 20);   // ACB.pFrame
    for (k = 0; k < 2; k++)
        if (!OO_CondGetAttr(fr, OO_SFONT + k, (void **)&font[k])) return 0;
    return 1;
}

void _main(void)
{
    INT_HANDLER old_int1 = GetIntVec(AUTO_INT_1);
    short old_font = FontGetSys();
    u16 scenario = 0;
#ifndef RT_BENCH
    u16 last;
#endif
    ESI ap;
#ifdef RT_MONO
    void *scr;
#else
    void *dbuf;
#endif

    // statics keep their values between runs (the program runs from its own file, c-patterns §1)
    tm_big = 0; tm_cur.map = 0;
#ifdef RT_MONO
    tm_mono = 0;
#endif
    rt_frame = 0; rt_keys = rt_prev = 0; rt_seed = 1; rt_state = 0; rt_state_size = 0;
    if (!get_fonts()) return;
#ifdef RT_MONO
    scr = malloc(RT_PSIZE);
#else
    dbuf = malloc(GRAYDBUFFER_SIZE);
#endif
    InitArgPtr(ap);
    if (GetArgType(ap) == POSINT_TAG) scenario = GetIntArg(ap);
#ifdef RT_MONO
    if (!scr) return;
    rt_light = scr;
    SetIntVec(AUTO_INT_1, tick_handler);
#else
    if (!dbuf) return;
    SetIntVec(AUTO_INT_1, DUMMY_HANDLER);          // before GrayOn (KB rule)
    if (!GrayOn()) goto out;
    GraySetInt1Handler(tick_handler);
    GrayDBufInit(dbuf);
    rt_light = GrayDBufGetHiddenPlane(LIGHT_PLANE);
    rt_dark = GrayDBufGetHiddenPlane(DARK_PLANE);
#endif

    game_init();
    game_scenario(scenario);
#ifdef RT_BENCH
    {                                              // RT_BENCH updates (no keys), then RT_BENCH
        u16 k, t0;                                 // renders of the final state, no frame wait
        t0 = ticks;
        for (k = 0; k < RT_BENCH; k++) { rt_prev = rt_keys = 0; game_update(); rt_frame++; }
        bench_upd = (u16)(ticks - t0);
        t0 = ticks;
        for (k = 0; k < RT_BENCH; k++) game_render();
        bench_rnd = (u16)(ticks - t0);
    }
#else
    rt_keys = read_keys();                         // keys held at launch are not presses
    last = ticks;
    for (;;) {
        rt_prev = rt_keys;
        rt_keys = read_keys();
        if (!game_update()) break;
        game_render();
#ifdef RT_MONO
        FastCopyScreen_R(scr, LCD_MEM);
#else
        GrayDBufToggleSync();                      // swap at a plane switch: no tearing
        rt_light = GrayDBufGetHiddenPlane(LIGHT_PLANE);
        rt_dark = GrayDBufGetHiddenPlane(DARK_PLANE);
#endif
        rt_frame++;
        while ((u16)(ticks - last) < RT_FRAME_TICKS) pokeIO(0x600005, 0x1D);   // sleep
        last += RT_FRAME_TICKS;
        if ((u16)(ticks - last) > 2 * RT_FRAME_TICKS) last = ticks;   // too slow: no catch-up
    }
    while (_rowread(0)) ;                          // do not leak keys to AMS
#endif

#ifdef RT_MONO
    SetIntVec(AUTO_INT_1, old_int1);
    free(scr);
#else
    GraySetInt1Handler(DUMMY_HANDLER);
    GrayOff();
out:
    SetIntVec(AUTO_INT_1, old_int1);
    free(dbuf);
#endif
#ifdef RT_BENCH
    // 12 MHz / 256 Hz = 46875 cycles per tick. TiEmu undercounts movem and multi-bit shifts
    // (ExtGraph, grey planes): render figures are optimistic, see performance §1.
    ClrScr();
    FontSetSys(F_4x6);
    printf_xy(0, 0, "bench %u frames, scenario %u", (u16)RT_BENCH, scenario);
    printf_xy(0, 8, "update %lu cyc/frame", bench_upd * 46875UL / RT_BENCH);
    printf_xy(0, 16, "render %lu cyc/frame", bench_rnd * 46875UL / RT_BENCH);
    printf_xy(0, 24, "budget %lu cyc/frame", RT_FRAME_TICKS * 46875UL);
    GKeyFlush();
    ngetchx();
#endif
    if (tm_big) free(tm_big);
#ifdef RT_MONO
    if (tm_mono) free(tm_mono);
#endif
    FontSetSys(old_font);
    if (sv_data) { save_write(); sv_data = 0; }    // statics survive between runs
    GKeyFlush();
}
