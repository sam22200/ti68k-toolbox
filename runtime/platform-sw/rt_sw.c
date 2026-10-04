// Software renderer and headless frame driver, see rt_sw.h. Pixel by pixel on purpose: speed does
// not matter on the PC, matching ExtGraph's semantics exactly does. Clipping is ExtGraph's
// (240x128 planes); only the visible 160x100 is compared with the calculator.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rt_sw.h"
#include "amsfont.h"

u8 sw_planes[2][RT_PSIZE];

u16 rt_ticks(void) { return (u16)(((u32)rt_frame * RT_FRAME_TICKS2) >> 1); }   // virtual clock: deterministic

static void put(u8 *p, s16 x, s16 y, u8 on)
{
    u8 *b, m;
    if ((u16)x >= RT_PW || (u16)y >= RT_PH) return;
    b = p + y * RT_PBYTES + (x >> 3);
    m = 0x80 >> (x & 7);
    if (on) *b |= m; else *b &= ~m;
}

static u8 get(const u8 *p, s16 x, s16 y)
{
    return p[y * RT_PBYTES + (x >> 3)] >> (7 - (x & 7)) & 1;
}

static u32 row_bits(const void *data, u8 w, u16 r)   // row r, left-aligned in 32 bits
{
    if (w == 8) return (u32)((const u8 *)data)[r] << 24;
    if (w == 16) return (u32)((const u16 *)data)[r] << 16;
    return ((const u32 *)data)[r];
}

// dest = (dest & mask) | data, or dest = data without a mask
static void blit(u8 *p, s16 x, s16 y, u8 w, u8 h, const void *data, const void *mask)
{
    u16 r, c;
    for (r = 0; r < h; r++) {
        u32 d = row_bits(data, w, r), m = mask ? row_bits(mask, w, r) : 0;
        for (c = 0; c < w; c++) {
            s16 px = x + c, py = y + r;
            u32 bit = 0x80000000UL >> c;
            if ((u16)px >= RT_PW || (u16)py >= RT_PH) continue;
            put(p, px, py, (d & bit) || ((m & bit) && get(p, px, py)));
        }
    }
}

void draw_clear(void)                  // like every draw_*: into rt_light / rt_dark (the TI's)
{
    memset(rt_light, 0, RT_PSIZE);
#ifndef RT_MONO
    memset(rt_dark, 0, RT_PSIZE);
#endif
}

void rt_fill(u16 x1, u16 y1, u16 x2, u16 y2, u8 color)
{
    u16 x, y;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++) {
#ifdef RT_MONO
            put((u8 *)rt_light, x, y, color >> 1);
#else
            put((u8 *)rt_light, x, y, color & 1);
            put((u8 *)rt_dark, x, y, color >> 1);
#endif
        }
}

void draw_sprite(s16 x, s16 y, const RtSprite *s)
{
#ifdef RT_MONO
    blit((u8 *)rt_light, x, y, s->w, s->h, s->dark, s->mask);
#else
    blit((u8 *)rt_light, x, y, s->w, s->h, s->light, s->mask);
    blit((u8 *)rt_dark, x, y, s->w, s->h, s->dark, s->mask);
#endif
}

void rt_clamp_cam(const RtTilemap *m, s16 *cx, s16 *cy);

static void tile16(s16 x, s16 y, const u16 *t)     // opaque, rows (dark, light) interlaced
{
    u16 r, c;
    for (r = 0; r < 16; r++, t += 2)
        for (c = 0; c < 16; c++) {
            u16 bit = 0x8000 >> c;
#ifdef RT_MONO
            put((u8 *)rt_light, x + c, y + r, (t[0] & bit) != 0);
#else
            put((u8 *)rt_light, x + c, y + r, (t[1] & bit) != 0);
            put((u8 *)rt_dark, x + c, y + r, (t[0] & bit) != 0);
#endif
        }
}

void draw_tilemap(const RtTilemap *m, s16 camx, s16 camy)
{
    s16 tx, ty;
    rt_clamp_cam(m, &camx, &camy);
    for (ty = camy >> 4; ty <= (camy + RT_H - 1) >> 4; ty++)
        for (tx = camx >> 4; tx <= (camx + RT_W - 1) >> 4; tx++)
            tile16((tx << 4) - camx, (ty << 4) - camy, m->tiles + ((u16)m->map[ty * m->w + tx] << 5));
}

void tilemap_dirty(void) { }

// AMS font data read in place on the TI; glyphs ORed, drawn only when the cell is in the plane
static void text_plane(u8 *p, s16 x, s16 y, const char *s, u8 font)
{
    for (; *s; s++) {
        u8 c = (u8)*s, rows, adv, r, k;
        const u8 *g;
        if (font == F_SMALL) { g = ams_f4x6 + c * 6; adv = *g++; rows = 5; }
        else { g = ams_f6x8 + c * 8; adv = 6; rows = 8; }
        if ((u16)x <= RT_PW - 16 && (u16)y <= RT_PH - 1 - rows)
            for (r = 0; r < rows; r++)
                for (k = 0; k < 8; k++)
                    if (g[r] & (0x80 >> k)) put(p, x + k, y + r, 1);
        x += adv;
    }
}

void draw_text(s16 x, s16 y, const char *s, u8 font, u8 color)
{
#ifdef RT_MONO                                  // through rt_light / rt_dark like the TI:
    if (color & 2) text_plane(rt_light, x, y, s, font);   // a game may point them at its own
#else                                           // plane-format buffer to render text once
    if (color & 1) text_plane(rt_light, x, y, s, font);
    if (color & 2) text_plane(rt_dark, x, y, s, font);
#endif
}

// ---------------------------------------------------------------- data files
const void *rt_file(const char *name, u16 *size)   // NAME.bin, loaded once, kept
{
    static struct { char name[12]; void *data; u16 size; } cache[8];
    char path[32];
    FILE *f;
    long n;
    u8 k;
    for (k = 0; k < 8 && cache[k].data; k++)
        if (!strcmp(cache[k].name, name)) { if (size) *size = cache[k].size; return cache[k].data; }
    if (k == 8 || strlen(name) > 10) return RT_NULL;
    snprintf(path, sizeof path, "%s.bin", name);
    if (!(f = fopen(path, "rb"))) return RT_NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 65518 || !(cache[k].data = malloc(n)) || fread(cache[k].data, 1, n, f) != (size_t)n) {
        fclose(f);
        return RT_NULL;
    }
    fclose(f);
    strcpy(cache[k].name, name);
    cache[k].size = (u16)n;
    if (size) *size = (u16)n;
    return cache[k].data;
}

// ---------------------------------------------------------------- save files
u8 rt_load(const char *name, void *data, u16 size)
{
    char path[32];
    FILE *f;
    u8 ok;
    snprintf(path, sizeof path, "%s.sav", name);
    if (!(f = fopen(path, "rb"))) return 0;
    ok = fread(data, 1, size, f) == size && fgetc(f) == EOF;
    fclose(f);
    return ok;
}

u8 rt_save(const char *name, const void *data, u16 size)
{
    char path[32];
    FILE *f;
    u8 ok;
    snprintf(path, sizeof path, "%s.sav", name);
    if (!(f = fopen(path, "wb"))) return 0;
    ok = fwrite(data, 1, size, f) == size;
    fclose(f);
    return ok;
}

// ---------------------------------------------------------------- frame driver
void sw_init(u16 scenario)
{
    rt_light = sw_planes[0];
#ifndef RT_MONO
    rt_dark = sw_planes[1];
#endif
    rt_frame = 0;
    rt_keys = rt_prev = 0;
    draw_clear();
    game_init();
    game_scenario(scenario);
}

u8 sw_step(u32 keys)
{
    u8 go;
    rt_prev = rt_keys;
    rt_keys = keys;
    go = game_update();
    if (go) game_render();
    rt_frame++;
    return go;
}

u8 sw_level(s16 x, s16 y)
{
#ifdef RT_MONO
    return get(sw_planes[0], x, y) * 3;
#else
    return get(sw_planes[0], x, y) | get(sw_planes[1], x, y) << 1;
#endif
}

u16 sw_checksum(void)
{
    u16 a = 0, b = 0, p, y, k;
    for (p = 0; p < 2; p++)
        for (y = 0; y < RT_H; y++)
            for (k = 0; k < RT_W / 8; k++) {
                a += sw_planes[p][y * RT_PBYTES + k]; if (a >= 255) a -= 255;
                b += a; if (b >= 255) b -= 255;
            }
    return b << 8 | a;
}

// ---------------------------------------------------------------- PNG (stored deflate, greyscale)
static u32 crc_tab[256];
static u32 crc(u32 c, const u8 *p, size_t n)
{
    if (!crc_tab[1]) {
        u32 k, j, v;
        for (k = 0; k < 256; k++) { for (v = k, j = 0; j < 8; j++) v = v & 1 ? 0xEDB88320UL ^ (v >> 1) : v >> 1; crc_tab[k] = v; }
    }
    while (n--) c = crc_tab[(c ^ *p++) & 0xFF] ^ (c >> 8);
    return c;
}

static void be32(u8 *p, u32 v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

static void chunk(FILE *f, const char *type, const u8 *data, u32 n)
{
    u8 h[8];
    u32 c;
    be32(h, n); memcpy(h + 4, type, 4);
    fwrite(h, 1, 8, f); fwrite(data, 1, n, f);
    c = crc(0xFFFFFFFFUL, h + 4, 4);
    c = crc(c, data, n) ^ 0xFFFFFFFFUL;
    be32(h, c); fwrite(h, 1, 4, f);
}

int sw_write_png(const char *path, int scale)
{
    static const u8 grey[4] = { 0xD8, 0x98, 0x58, 0x18 };   // LCD-like: white .. black
    u32 w = RT_W * scale, h = RT_H * scale, stride = w + 1, raw_n = stride * h, a = 1, b = 0, k;
    u8 *raw = malloc(raw_n), *z = malloc(raw_n + raw_n / 65535 * 5 + 16), ihdr[13], *q;
    u32 x, y, off = 0;
    FILE *f;
    if (!raw || !z) return -1;
    for (y = 0; y < h; y++) {
        raw[y * stride] = 0;
        for (x = 0; x < w; x++) raw[y * stride + 1 + x] = grey[sw_level(x / scale, y / scale)];
    }
    q = z; *q++ = 0x78; *q++ = 0x01;
    while (off < raw_n) {
        u32 n = raw_n - off > 65535 ? 65535 : raw_n - off;
        *q++ = off + n == raw_n; *q++ = n; *q++ = n >> 8; *q++ = ~n; *q++ = ~n >> 8;
        memcpy(q, raw + off, n); q += n; off += n;
    }
    for (k = 0; k < raw_n; k++) { a = (a + raw[k]) % 65521; b = (b + a) % 65521; }
    be32(q, b << 16 | a); q += 4;
    f = fopen(path, "wb");
    if (!f) { free(raw); free(z); return -1; }
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    be32(ihdr, w); be32(ihdr + 4, h);
    ihdr[8] = 8; ihdr[9] = 0; ihdr[10] = ihdr[11] = ihdr[12] = 0;
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z, q - z);
    chunk(f, "IEND", RT_NULL, 0);
    fclose(f);
    free(raw); free(z);
    return 0;
}

// ---------------------------------------------------------------- state and input scripts
int sw_save_state(const char *path)
{
    FILE *f = fopen(path, "wb");
    size_t n;
    if (!f || !rt_state) { if (f) fclose(f); return -1; }
    n = fwrite(rt_state, 1, rt_state_size, f);
    fclose(f);
    return n == rt_state_size ? 0 : -1;
}

int sw_load_state(const char *path)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f || !rt_state) { if (f) fclose(f); return -1; }
    n = fread(rt_state, 1, rt_state_size, f);
    fclose(f);
    return n == rt_state_size ? 0 : -1;
}

u32 sw_parse_keys(const char *s)
{
    static const struct { const char *n; u32 k; } names[] = {
        { "UP", K_UP }, { "LEFT", K_LEFT }, { "DOWN", K_DOWN }, { "RIGHT", K_RIGHT }, { "A", K_A },
        { "B", K_B }, { "C", K_C }, { "D", K_D }, { "ENTER", K_ENTER }, { "ESC", K_ESC },
    };
    u32 keys = 0;
    char w[16];
    int n;
    while (sscanf(s, " %15s%n", w, &n) == 1) {
        size_t k;
        s += n;
        if (w[0] == '#') break;
        if (w[0] >= '1' && w[0] <= '9' && !w[1]) { keys |= K_DIGIT(w[0] - '0'); continue; }
        for (k = 0; k < sizeof(names) / sizeof(names[0]); k++)
            if (!strcmp(w, names[k].n)) { keys |= names[k].k; break; }
        if (k == sizeof(names) / sizeof(names[0])) fprintf(stderr, "unknown key %s\n", w);
    }
    return keys;
}

int sw_load_script(SwScript *s, const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256];
    s->n = s->pos = 0;
    if (!f) return -1;
    while (fgets(line, sizeof(line), f) && s->n < SW_SCRIPT_MAX) {
        int fr, n;
        if (sscanf(line, " %d%n", &fr, &n) != 1) continue;   // blank or comment
        s->frame[s->n] = fr;
        s->keys[s->n++] = sw_parse_keys(line + n);
    }
    fclose(f);
    return 0;
}

u32 sw_script_keys(SwScript *s, u16 frame)
{
    while (s->pos + 1 < s->n && s->frame[s->pos + 1] <= frame) s->pos++;
    return s->n && s->frame[s->pos] <= frame ? s->keys[s->pos] : 0;
}
