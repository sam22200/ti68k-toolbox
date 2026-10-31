// Shared terrain-piece experiment. No retail data is compiled into this library.
#ifndef FFT_SCENERY_BANK_H
#define FFT_SCENERY_BANK_H
#include "../../../runtime/core/rt.h"

typedef struct {
    const u8 *bank, *map;
    u8 width, height, materials, shapes;
    u16 rows, plane, sprite_count;
    u16 shapes_off, tops_off, units_off, caps_off, directory;
    u32 pieces, sprite_rows, touched_bytes;
} SbScene;

u8 sb_bind(SbScene *s, const u8 *bank, u16 bank_size, const u8 *map, u16 map_size);
// Output: two color planes, four coverage planes, and one temporary seen plane.
// Color can be NULL when rebuilding coverage for an already cached orientation.
void sb_compose(SbScene *s, u8 rotation, u8 *color, u8 *coverage, u8 *seen);
// Optional exact-source control: sparse, independently compressed 256-byte XOR
// patches. Caller provides only a 256-byte scratch, never another complete view.
// A NULL color or coverage pointer preserves that already patched cache.
u8 sb_patch(const u8 *patches, u8 rotation, u8 *color, u8 *coverage, u16 plane, u8 *scratch);
u16 sb_checksum(const u8 *p, u16 size);
#endif
