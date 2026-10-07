#ifndef YOSHI_TERRAIN_H
#define YOSHI_TERRAIN_H
#include "../../runtime/core/rt.h"
#define YT_TOP 1536
#define YT_BOTTOM 2048
#define YT_END 1264
#define YT_NONE 32767
u8 terrain_init(void);
u8 terrain_ready(void);
s16 terrain_floor(u16 x, s16 low, s16 high, u8 *angle);
s16 terrain_actor_floor(u16 center, s16 low, s16 high);
u8 terrain_solid(u16 x, u16 y);
void terrain_render(u16 camx, u16 camy);
void terrain_render_reference(u16 camx, u16 camy);
#endif
