#ifndef FFT_PORTRAIT32_INC
#define FFT_PORTRAIT32_INC
// One bank format everywhere: 32x29, two big-endian colour planes and transparency.
static void portrait32(const u8 *p,s16 x,s16 y)
{
#ifdef __m68k__
    RtSprite sp={32,29,p,p+116,p+232};
#else
    u32 rows[3][29];u8 k,r;
    RtSprite sp={32,29,rows[0],rows[1],rows[2]};
    for(k=0;k<3;k++)for(r=0;r<29;r++) {
        const u8 *q=p+(u16)k*116+(u16)r*4;
        rows[k][r]=(u32)q[0]<<24|(u32)q[1]<<16|(u16)q[2]<<8|q[3];
    }
#endif
    draw_sprite(x,y,&sp);
}
#endif
