#ifndef YOSHI_ART_H
#define YOSHI_ART_H
#include "../../runtime/core/rt.h"
u8 art_init(void);
u8 art_ready(void);
void art_hero(void);
void art_shy(s16 x, s16 y, u8 facing, u8 moving);
void art_egg(s16 x, s16 y);
void art_coin(s16 x, s16 y);
void art_aim(s16 x, s16 y, u8 locked);
void art_hit(s16 x, s16 y);
void art_baby(s16 x, s16 y, u8 bubble);
void art_counter(u16 seconds, u8 alert);
#ifndef __m68k__
extern u8 art_reference;
#endif
#endif
