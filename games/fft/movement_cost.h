#ifndef FFT_MOVEMENT_COST_H
#define FFT_MOVEMENT_COST_H
#include "core_limits.h"
// Source-policy prototype, not yet the battle's terrain-cost producer.
// A zero table entry selects weather BEFORE Any Ground can override nonzero
// traversable costs. Impassable255 remains impassable; Any Weather is separate.
static u8 movement_terrain_cost(u8 raw,u8 weather,u8 any_ground)
{return !raw?weather:any_ground&&raw!=255?1:raw;}
// Original forward frontier uses remaining points, with one positive point
// required after entry. Counts/IDs are unrelated;255 here is a cost sentinel.
static u8 movement_gap_remaining(u8 available,u8 terrain,u8 gap)
{
    u16 cost=(u16)terrain+gap;
    return terrain==255||cost>=available?0:(u8)(available-cost);
}
#endif
