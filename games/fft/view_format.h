#ifndef FFT_VIEW_FORMAT_H
#define FFT_VIEW_FORMAT_H
#include "core_limits.h"
// R4 stores320-pixel scenes with40-byte rows: two raw color planes,
// an optional one-plane width mask, then a ZX0 ragged-depth stream.
// Return the row count. Byte reads also accept unaligned/truncated test inputs.
static u16 core_view_rows(const u8 *p,u16 size,u16 max_rows)
{
    u16 colors,mask,depth,rows;
    if(!p||size<9||p[0]!='R'||p[1]!='4')return 0;
    colors=(u16)((u16)p[2]<<8|p[3]);mask=(u16)((u16)p[4]<<8|p[5]);
    depth=(u16)((u16)p[6]<<8|p[7]);rows=colors/80U;
    if(!rows||rows>max_rows||colors%80U||(mask&&mask!=(colors>>1))||
       depth<4U*rows||depth>FFT_CORE_DEPTH_MAX||(depth&3)||
       (u32)8+colors+mask>=size)return 0;
    return rows;
}
#endif
