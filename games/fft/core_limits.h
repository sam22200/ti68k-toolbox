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
typedef u8 CoreTileId;                  // compact node0..255, includes both floors
typedef u16 CoreTileCount;              // can represent count256 and loop endpoint
typedef struct { u8 x, z_layer; } CoreTilePosition; // z in bits0..4; layer in bit5
// Tile255 is valid. Absence belongs to an explicit state/validity flag or mask,
// never a reserved CoreTileId. Source (layer*256+y*width+x) is an OFFLINE address,
// not the compact runtime identity. Row offsets/topology are generated offline.
#endif
