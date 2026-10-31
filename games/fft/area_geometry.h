// Source radial vertical choice, independent of compact node availability.
#ifndef FFT_AREA_GEOMETRY_H
#define FFT_AREA_GEOMETRY_H
static u16 core_area_height(const u8 *p)
{
    return ((u16)p[0]<<1)+(p[1]&31)+((p[1]>>5)<<1);
}
// Returns0 none,1 lower,2 upper. Ref is source-narrowed to u8; tolerance doubles.
// A blocked layer's finite -255 sentinel participates in selection, then stays clear.
static u8 core_area_layer(const u8 *p,u8 ref,u8 tolerance,u8 high_only,u8 upper_present)
{
    s16 lo=p[3]&128?-255:(s16)core_area_height(p),hi=-255,ld,hd;
    u8 selected;
    if(upper_present&&!(p[7]&128))hi=(s16)core_area_height(p+4);
    if(high_only){if(lo>=hi)hi=-255;else lo=-255;}
    ld=lo-ref;if(ld<0)ld=-ld;hd=hi-ref;if(hd<0)hd=-hd;
    selected=hd<ld?2:1;
    if(selected==1)return lo>=0&&ld<=((u16)tolerance<<1)?1:0;
    return hi>=0&&hd<=((u16)tolerance<<1)?2:0;
}
#endif
