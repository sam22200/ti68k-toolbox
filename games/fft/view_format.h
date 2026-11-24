#ifndef FFT_VIEW_FORMAT_H
#define FFT_VIEW_FORMAT_H
#include "core_limits.h"
// R4 stores320-pixel scenes with40-byte rows; R5 stores352px/44-byte rows.
// Both retain the same eight-byte header: two raw color planes,
// an optional one-plane width mask, then a ZX0 ragged-depth stream.
// Return the row count. Byte reads also accept unaligned/truncated test inputs.
static u16 core_view_rows(const u8 *p,u16 size,u16 max_rows)
{
    u16 colors,mask,depth,rows,stride;
    if(!p||size<9||p[0]!='R'||(p[1]!='4'&&p[1]!='5'))return 0;
    colors=(u16)((u16)p[2]<<8|p[3]);mask=(u16)((u16)p[4]<<8|p[5]);
    stride=p[1]=='5'?88U:80U;
    depth=(u16)((u16)p[6]<<8|p[7]);rows=colors/stride;
    if(!rows||rows>max_rows||colors>FFT_CORE_SCENE_MAX||colors%stride||(mask&&mask!=(colors>>1))||
       depth<4U*rows||depth>FFT_CORE_DEPTH_MAX||(depth&3)||
       (u32)8+colors+mask>=size)return 0;
    return rows;
}
#endif
