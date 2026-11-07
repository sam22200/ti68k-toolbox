// Runtime-projection experiment: shared 8x8 face textures projected through
// shared per-shape texel maps. No retail data is compiled into this library.
#ifndef FFT_SCENERY_PROJ_H
#define FFT_SCENERY_PROJ_H
#include "../../../runtime/core/rt.h"

typedef struct {
    const u8 *bank, *map, *textures;
    const u8 *piece[255];          // row data start per piece (directory walked once at bind)
    u8 width, height, piece_count, patterns;
    u16 rows, plane;
    u16 shapes_off, tops_off, units_off, caps_off, directory;
    u32 pieces;
} SpScene;

u8 sp_bind(SpScene *s, const u8 *bank, u16 bank_size, const u8 *map, u16 map_size);
// Output: two color planes, four coverage planes, and one temporary seen plane.
// Same plane layout as bank.h's sb_compose; color may be NULL (coverage only).
void sp_compose(SpScene *s, u8 rotation, u8 *color, u8 *coverage, u8 *seen);

// Parameter block of proj68k.s (offsets fixed there): one piece's rows, all
// pointers at the first row's first byte.
typedef struct {
    const u8 *p, *tex;
    u8 *seen, *light, *dark, *cov0, *cov1;
    u16 twoplane;
    u8 depth, shift;
    u16 rows;
} SpRows;
#ifdef __m68k__
void sp_rows_asm(SpRows *rows asm("%a0")) __attribute__((__regparm__(1)));
#endif
#endif
