// Byte-only archive envelope boundaries; no emulator or asset decompression.
#include <assert.h>
#include <stdio.h>
#include "../view_format.h"
static void word(u8 *p,u16 value){p[0]=(u8)(value>>8);p[1]=(u8)value;}
int main(void)
{
    u8 p[9]={'R','4'};u16 n;
    word(p+2,16160);word(p+6,18488);assert(core_view_rows(p,32000,202)==202);
    for(n=0;n<9;n++)assert(!core_view_rows(p,n,202));
    assert(!core_view_rows(p,16168,202));assert(!core_view_rows(0,32000,202));
    p[1]='5';word(p+2,16544);word(p+6,17280);assert(core_view_rows(p,32000,202)==188);
    word(p+2,16632);assert(!core_view_rows(p,32000,202)); // one extra wide row
    word(p+2,17776);assert(!core_view_rows(p,32000,202)); // invalid product of maxima
    word(p+2,16544);word(p+4,8272);assert(core_view_rows(p,32000,202)==188);
    word(p+4,8271);assert(!core_view_rows(p,32000,202));word(p+4,0);
    word(p+6,18492);assert(!core_view_rows(p,32000,202));word(p+6,17281);
    assert(!core_view_rows(p,32000,202));word(p+6,748);assert(!core_view_rows(p,32000,202));
    word(p+6,17280);p[1]='3';assert(!core_view_rows(p,32000,202));
    puts("R4/R5 row/color/depth/truncation boundaries pass");return 0;
}
