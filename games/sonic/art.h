#ifndef SONIC_ART_H
#define SONIC_ART_H
#include "sonic.h"
#include "generated/art_ids.h"
u8 sonic_art_init(void);
void sonic_art_world(void);
void sonic_art_draw(u16 id, s16 x, s16 y);
void sonic_art_player(void);
void sonic_art_ring(u8 frame, s16 x, s16 y);
u8 sonic_art_burst(void);
#endif
