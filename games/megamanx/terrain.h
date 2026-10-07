#ifndef MMX_TERRAIN_H
#define MMX_TERRAIN_H
#include "../../runtime/core/rt.h"
#define MX_WIDTH 1024
#define MX_TOP 256
#define MX_HEIGHT 256
u8 terrain_init(void);
u8 terrain_ready(void);
u8 terrain_class(s16 x,s16 y);
s16 terrain_surface(s16 x,s16 y);
u8 terrain_solid(s16 x,s16 y);
void terrain_render(u16 camx,u16 camy);
#endif
