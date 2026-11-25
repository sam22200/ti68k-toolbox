#ifndef FFT_CORE_LIMITS_H
#define FFT_CORE_LIMITS_H
// Accepted TI port envelope,2026-10-24. Storage uses this cap; each FCD3
// encounter declares its actual count. Existing battles still contain eleven.
// See campaign/PORT_LIMITS.md before extending descriptors/arrays.
#include "../../runtime/core/rt.h"
#define FFT_CORE_UNIT_MAX 13
#define FFT_CORE_TILE_MAX 256U
#define FFT_CORE_AXIS_MAX 18
#define FFT_CORE_LAYER_MAX 2
// Validated color storage: 320x202 or 352x188, never the product352x202.
#define FFT_CORE_SCENE_MAX 16544U
// RAM of one view's unpacked ragged depth (extract.ragged_depth, fft.c vbuf), 2026-11-10:
// Orbonne 16,400..18,488, Gariland 16,968..18,304; a bigger view is refused when packaged
// (chapter_desc.py) and loaded (chapter_files.inc). Full planes would take 32,320.
//
// 2026-11-22: the envelope WAS 20,480, which reserved 1,992 bytes above the biggest view
// that exists. The battle is short of RAM on the Titanium (campaign/CORE_PACKAGES.md), so
// it is now the measured maximum itself. The contract is unchanged -- a bigger view is
// still refused, at packaging and at load -- it simply has no slack: raise this with the
// view that needs it, as a core change.
//
// 2026-11-25: raised 18,488 -> 18,520 for North Wall of Bethla Garrison (map 67), the one
// map of the census whose crop would have taken ground a unit stands on -- two rows of it,
// to save 32 bytes. The map measures 18,520 at its full 202 rows, so it now fits exactly,
// with no slack, which is the same footing as Orbonne's 8 bytes. The cost is 32 bytes of
// the battle's RAM, measured live against the worst case: the Zeklaus 13-unit diagnostic
// has 436 free (campaign/INSTALLED_RAM_ABI99.md), so 404 remain. Slums in Dorter, over by
// 408, was refused the same trade: it would have left 28.
#define FFT_CORE_DEPTH_MAX 18520U
// The companion image's block (campaign/COMPANION.md), reserved at battle start whatever its
// current size: tools/module.py refuses a bigger image, fft.c a bigger loaded one. Raise it
// with the image (a core change); the battle's RAM requirement counts it.
// 2026-11-22: was 5,376 against an image of 5,046, so it reserved 328 bytes nothing used.
// Like the depth envelope above, it is now the image itself plus its even-size pad.
// Bar/FSV8 migration (2026-11-22) adds94 image bytes:5046 ->5140.
// Reserve the measured image, +92 RAM bytes; retain all reviewed cache capacities.
// Town services/FSV9 (2026-11-23): measured graphics companion7264, +2124
// versus5140. It now also composes cached menus through its pinned252 entry.
// ABI97: graphics4128 and cold Save reader3542 share one4128-byte block.
// Saves3136 bytes versus7264; no play-time allocation or warm-frame switch.
// Preserve42 actor frames/depth18488.
// ABI98 adds four context bytes to each image: graphics4132, cold reader3546.
#define FFT_CORE_COMPANION_MAX 4132U
typedef u8 CoreTileId;                  // compact node0..255, includes both floors
typedef u16 CoreTileCount;              // can represent count256 and loop endpoint
typedef struct { u8 x, z_layer; } CoreTilePosition; // z in bits0..4; layer in bit5
// Tile255 is valid. Absence belongs to an explicit state/validity flag or mask,
// never a reserved CoreTileId. Source (layer*256+y*width+x) is an OFFLINE address,
// not the compact runtime identity. Row offsets/topology are generated offline.
#endif
