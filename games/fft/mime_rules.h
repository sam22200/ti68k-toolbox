// Source world rotation and integer-height layer choice; no camera inputs.
#ifndef FFT_MIME_RULES_H
#define FFT_MIME_RULES_H
static void core_mime_rotate(s16 *dx,s16 *dz,u8 rotation)
{
    s16 swap;if(rotation&1){swap=*dx;*dx=*dz;*dz=-swap;}
    if(rotation&2){*dx=-*dx;*dz=-*dz;}
}
static u8 core_mime_layer(u8 h,u8 lo,u8 hi,u8 blocked,u8 jump)
{
    s16 dl,dh;
    if(blocked&1)return blocked&2?0:2;
    if(blocked&2)return 1;
    if(jump)return hi>=lo?2:1;
    dl=(s16)lo-h;if(dl<0)dl=-dl;dh=(s16)hi-h;if(dh<0)dh=-dh;
    return dh<dl||(dh==dl&&hi>=lo)?2:1;
}
#endif
