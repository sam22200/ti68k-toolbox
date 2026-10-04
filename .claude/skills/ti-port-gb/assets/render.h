// render.c: the GB screen rebuilt from VRAM, I/O and OAM
#ifndef RENDER_H
#define RENDER_H
#include "gb.h"
extern u8 in_play;                     // a hall is on screen: playfield 1:1 + the port's HUD
// other screens: which one (screens.c sets it), for the 100 of the 144 GB rows shown
enum { SK_LOGO1, SK_LOGO2, SK_TITLE, SK_TABLE, SK_OVER, SK_END };
extern u8 screen_kind;
extern u8 gb_oam[0xA0];                // the OAM as the last DMA left it
extern u8 secret_blink;                // > 0: the HUD score blinks (a secret found, decision 12)
void vram_dirty(u16 a);
void render_reset(void);
#endif
