// The pinned graphics companion owns this strided cached-window compositor.
// A fitted frame leaves scenery outside it intact, including partial words.
// No zone switch, allocation, extra plane, or second width/padding policy.
static void window_blit(u8 y)
{
    u8 i,r;u16 pitch=RT_PBYTES/2;
    for(i=0;i<4;i++) {
        const u8 *src0=(const u8 *)W->win[0]+2*i,*src1=(const u8 *)W->win[1]+2*i;
        u8 *dst0=(u8 *)rt_light+(u16)y*RT_PBYTES+12+2*i,*dst1=(u8 *)rt_dark+(u16)y*RT_PBYTES+12+2*i;
        u16 mask=RD16((const u8 *)W->win[0],i);
        for(r=0;r<3;r++) {
            WR16(dst0,0,(RD16(dst0,0)&~mask)|RD16(src0,0));
            WR16(dst1,0,(RD16(dst1,0)&~mask)|RD16(src1,0));
            src0+=RT_PBYTES;src1+=RT_PBYTES;dst0+=RT_PBYTES;dst1+=RT_PBYTES;
        }
        mask=RD16((const u8 *)W->win[0],(W->win_h-1)*pitch+i);
        if(!mask)continue;
        if(mask==65535)for(;r<W->win_h;r++) {
            WR16(dst0,0,RD16(src0,0));WR16(dst1,0,RD16(src1,0));
            src0+=RT_PBYTES;src1+=RT_PBYTES;dst0+=RT_PBYTES;dst1+=RT_PBYTES;
        } else for(;r<W->win_h;r++) {
            WR16(dst0,0,(RD16(dst0,0)&~mask)|RD16(src0,0));
            WR16(dst1,0,(RD16(dst1,0)&~mask)|RD16(src1,0));
            src0+=RT_PBYTES;src1+=RT_PBYTES;dst0+=RT_PBYTES;dst1+=RT_PBYTES;
        }
    }
}
