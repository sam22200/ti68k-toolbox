// The Game Boy as the port sees it: the ROM (the data file "bgrom", read in place), WRAM, HRAM
// and VRAM as flat byte arrays addressed like on the GB, the few I/O registers the game writes,
// and protothreads for the GB's blocking code (a wait for N VBlanks becomes a yield).
// The game's routines are translated by hand on this memory: same addresses, same layouts, so
// pointers stored in RAM (scripts, mask addresses) keep working and the traces compare memory.
#ifndef GB_H
#define GB_H
#include "../../runtime/core/rt.h"
#include <stdlib.h>

extern const u8 *gb_rom;               // 0000-7FFF
extern u8 *gb_wram;                    // C000-DFFF (malloc: 8 KB; statics live in the TI program)
extern u8 *gb_vram;                    // 8000-9FFF
extern u8 gb_hram[0x80];               // FF80-FFFF
extern u8 gb_io[0x50];                 // FF00-FF4F: LCDC, SCY, BGP... (the renderer reads them)

#define R8(a) gb_rom[(u16)(a)]
#define R16(a) (u16)(R8(a) | (u16)R8((u16)(a) + 1) << 8)
#define W8(a) gb_wram[(u16)(a) - 0xC000]
#define W16(a) (u16)(W8(a) | (u16)W8((u16)(a) + 1) << 8)
#define H8(a) gb_hram[(u16)(a) - 0xFF80]
#define IO(a) gb_io[(u16)(a) - 0xFF00]
static inline void w16(u16 a, u16 v) { W8(a) = (u8)v; W8(a + 1) = (u8)(v >> 8); }

u8 rd_slow(u16 a);
void wr_slow(u16 a, u8 v);
// any address (ROM, VRAM, WRAM, HRAM, I/O); WRAM and ROM inline (the hot paths: pointers into
// the RAM tables and the ROM scripts); VRAM writes mark the BG dirty
static inline u8 rd(u16 a)
{
    if ((u16)(a - 0xC000) < 0x2000) return gb_wram[a - 0xC000];
    if (a < 0x8000) return gb_rom[a];
    return rd_slow(a);
}
static inline void wr(u16 a, u8 v)
{
    if ((u16)(a - 0xC000) < 0x2000) gb_wram[a - 0xC000] = v;
    else wr_slow(a, v);
}

// I/O registers the game uses
#define LCDC 0xFF40
#define SCY 0xFF42
#define BGP 0xFF47
#define OBP0 0xFF48
#define OBP1 0xFF49
#define WY 0xFF4A
#define WX 0xFF4B

// small ROM helpers, by address (0747 fill, 074C/0753 copy, 3341 word table, 334C byte table)
void fill(u16 dst, u8 v, u16 n);
void copy(u16 dst, u16 src, u16 n);
static inline u16 tab16(u16 t, u8 i) { return (u16)(rd((u16)(t + 2 * i)) | rd((u16)(t + 2 * i + 1)) << 8); }
static inline u16 mul16(u16 a, u16 b) { return (u16)(a * b); }   // 042B

// ---------------------------------------------------------------- protothreads
// A blocking GB routine becomes `u8 name(void)` returning 1 when done, 0 when it yields to
// the next VBlank. Its locals that cross a yield live in static storage (reset by gb_reset).
typedef u16 Pt;
#define PT_BEGIN(pt) switch (pt) { case 0:
#define PT_WAIT1(pt) do { (pt) = __LINE__; return 0; case __LINE__:; } while (0)
#define PT_CALL(pt, call) do { (pt) = __LINE__; case __LINE__: if (!(call)) return 0; } while (0)
#define PT_END(pt) } (pt) = 0; return 1
#define PT_EXIT(pt) do { (pt) = 0; return 1; } while (0)

extern u8 gb_ret;                      // a blocking routine's result (register A / the Z flag)

// flow.c: the VBlank helpers every blocking routine uses
u8 wait_vbl(void);                     // 0390: one VBlank
u8 wait_frames(u8 n);                  // 0387: n VBlanks (n read when it starts)
u8 wait_start(u8 n);                   // 04F6: up to n VBlanks; gb_ret = 1 if Start was pressed
void read_keys(void);                  // 04A4: FF8B held, FF8C pressed, C0F1 direction
void oam_dma(void);                    // FF80: the OAM shadow C000 into the OAM the screen shows
void r_033a(void);                     // 033A the VBlank interrupt's jobs (the secret runs it
                                       // in place of a wait)
void r_0379(u16 de);                   // 0379: run routine DE at the next VBlank instead of the
                                       // usual VBlank jobs (FF80 = the OAM DMA, 1B01 = the bar)
#endif
