// Portable proportional bitmap fonts in an archived big-endian BMF1 bank.
#ifndef TI_BITMAP_FONT_H
#define TI_BITMAP_FONT_H
#include "../runtime/core/rt.h"

typedef struct {
    const u8 *bank,*glyphs,*kern;
    u16 size,kerns;
} BfFont;
typedef struct {s16 left,top;u16 width,sweep;u8 cell;} BfLayout;
typedef struct {s16 left,top,right,bottom,advance;} BfMetrics;

// No heap or runtime scaling. cell0:proportional;1..32:fixed pixel advance.
// Printable ASCII, missing glyphs use '?'. At most128 chars /256px of advance.
u8 bf_open(BfFont *font,const u8 *bank,u16 bytes,u8 face);
u8 bf_measure(const BfFont *font,const char *text,u8 cell,BfMetrics *out);
void bf_draw(const BfFont *font,const char *text,const BfLayout *layout);
#endif
