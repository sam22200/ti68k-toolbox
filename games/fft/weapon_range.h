// Source weapon panel shapes, independent of projectile obstruction and damage.
#ifndef FFT_WEAPON_RANGE_H
#define FFT_WEAPON_RANGE_H
enum { WEAPON_ARC=16,WEAPON_DIRECT=32,WEAPON_LUNGE=64,WEAPON_STRIKE=128 };
static u8 core_weapon_reaches(u8 flags,u8 range,s16 dx,s16 dz,u8 height,u16 target)
{
    u16 d=(dx<0?-dx:dx)+(dz<0?-dz:dz);s16 h=(s16)target-height;
    if(flags&WEAPON_STRIKE)return d==1&&h>=-6&&h<=5;
    if(flags&WEAPON_LUNGE)return d>=1&&d<=2&&(!dx||!dz)&&h>=(s16)(d<<1)-10&&h<=7;
    if(flags&WEAPON_DIRECT)return d>=3&&d<=range;
    h=(s16)(u8)target-height;
    if(h<0)return d>=3&&d<=(u16)range+((u16)-h>>2);
    return d>=3&&d+(h>>2)<=range;
}
#endif
