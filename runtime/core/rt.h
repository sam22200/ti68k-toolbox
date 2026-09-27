// Portable Game Runtime: the one API a game sees, identical on the PC (platform-sdl, tests via
// platform-sw) and on the TI-89 / Titanium (platform-ti68k, ExtGraph 2 underneath).
// A game is plain C using only this header: it defines the hooks below and never includes
// tigcclib.h or SDL.h. Screen 160x100, 4 grey levels (define RT_MONO for black and white).
// Plane format = ExtGraph's: 240x128 bits, 30-byte rows, MSB = leftmost pixel; light and dark
// planes, grey level = light + 2 * dark. Sprites and tiles are ExtGraph data as is.
#ifndef RUNTIME_RT_H
#define RUNTIME_RT_H

#ifdef __m68k__
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef long s32;
#define RT_NULL ((void *)0)
#else
#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
#define RT_NULL ((void *)0)
#endif

#define RT_W 160
#define RT_H 100
#define RT_PW 240                      // plane geometry (ExtGraph, LCD_MEM)
#define RT_PH 128
#define RT_PBYTES 30
#define RT_PSIZE (RT_PBYTES * RT_PH)   // 3840 bytes per plane

// Frame rate: the game loop runs once every RT_FRAME_TICKS ticks of the 256 Hz clock
// (8 = 32 fps). A game may define it before including rt.h, both platforms honour it.
#ifndef RT_FRAME_TICKS
#define RT_FRAME_TICKS 8
#endif
#define RT_HZ 256

// ---------------------------------------------------------------- game hooks (the game defines)
// On the calculator, globals and statics keep their values between runs (the program runs from
// its own file): game_init must set every one of them, not rely on initialisers.
void game_init(void);                  // once, before the first frame
void game_scenario(u16 n);             // injection door: jump to a precise state n (0 = normal
                                       // start). PC: --scenario N, TI: prog(N) from HOME
u8 game_update(void);                  // one fixed step; returns 0 to quit
void game_render(void);                // draw the whole frame into the hidden planes
// Raw state for save/load on the PC (F2/F3, --load/--save): the game sets these in game_init
// to its POD state struct (no pointers). Same-platform only (byte order, padding).
extern void *rt_state;
extern u16 rt_state_size;

// ---------------------------------------------------------------- input
// One snapshot per frame, taken by the runtime before game_update. TI-89 keys in brackets.
#define K_UP     0x0001UL
#define K_LEFT   0x0002UL
#define K_DOWN   0x0004UL
#define K_RIGHT  0x0008UL
#define K_A      0x0010UL              // [2nd]      PC: Ctrl, Space, Z
#define K_B      0x0020UL              // [shift]    PC: Shift, X
#define K_C      0x0040UL              // [diamond]  PC: C
#define K_D      0x0080UL              // [alpha]    PC: V
#define K_ENTER  0x0100UL              // [ENTER]    PC: Enter
#define K_ESC    0x0200UL              // [ESC]      PC: Escape, window close
#define K_DIGIT(n) (0x8000UL << (n))   // [1]..[9] = bits 16..24, keypad grid for gestures
#define K_DIGITS 0x01FF0000UL          //            PC: keypad or number row 1..9
extern u32 rt_keys, rt_prev;           // this frame, previous frame
#define input_held(k)    (rt_keys & (k))
#define input_pressed(k) (rt_keys & ~rt_prev & (k))
#define input_released(k) (~rt_keys & rt_prev & (k))
extern u16 rt_frame;                   // frames since start (wraps)

// ---------------------------------------------------------------- drawing (hidden planes)
enum { C_WHITE, C_LGRAY, C_DGRAY, C_BLACK };   // = ExtGraph COLOR_* (checked at compile time)
enum { F_SMALL, F_MEDIUM };                    // AMS F_4x6 (variable width), F_6x8

// Masked sprite, ExtGraph layout: w = 8, 16 or 32; rows are u8, u16 or u32 by w.
// dest = (dest & mask) | data on each plane: mask bit 1 = transparent. mask RT_NULL = opaque
// (replace the w x h block). Mono builds draw the dark plane (dark grey and black → black).
typedef struct {
    u8 w, h;
    const void *light, *dark, *mask;
} RtSprite;

// 16x16 grey tiles in ExtGraph TileMap order: tile t at tiles + 32 t = 16 (dark row, light row)
// pairs. map: w x h tile indices, row-major, at least 11 x 7 (the screen). On the TI it is drawn
// by the TileMap engine, which also reads (never shows) up to 17 x 10 cells from the camera.
typedef struct {
    const u8 *map;
    u16 w, h;
    const u16 *tiles;
    u16 ntiles;                        // mono builds convert the tile set once
} RtTilemap;

void draw_clear(void);                                           // hidden planes to white
void draw_rect(s16 x, s16 y, s16 w, s16 h, u8 color);            // filled, clipped
void draw_sprite(s16 x, s16 y, const RtSprite *s);               // clipped
void draw_text(s16 x, s16 y, const char *s, u8 font, u8 color);  // OR, AMS fonts; a glyph
                                       // is drawn only if its cell is inside the plane:
                                       // 0 <= x <= 224, 0 <= y <= 127 - height (5 or 8)
void draw_tilemap(const RtTilemap *m, s16 camx, s16 camy);       // opaque, whole screen; the
                                       // camera is clamped so that the view stays in the map
void tilemap_dirty(void);              // call after changing map cells of the current map
extern void *rt_light, *rt_dark;       // hidden planes this frame (rt_dark only in grey)

// ---------------------------------------------------------------- data files
// Big read-only data (tile sets, sprites, maps) kept outside the program (64 KB per variable):
// TI: the variable NAME (type OTH, made with ttbin2oth; archive it: it is read in place, from
// Flash, never copied; a RAM one is locked). PC: the file NAME.bin in the current directory
// (the same bytes in the host's byte order). Returns the data (after the TI size word) or
// RT_NULL when missing; *size gets its length in bytes when size is not RT_NULL.
const void *rt_file(const char *name, u16 *size);

// ---------------------------------------------------------------- utilities
u16 rt_rand(void);                     // wyhash16, deterministic (rt_seed)
extern u16 rt_seed;
u16 rt_ticks(void);                    // 256 Hz clock (wraps)

#endif
