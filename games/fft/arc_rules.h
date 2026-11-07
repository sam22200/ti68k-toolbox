// Bounded ballistic model: world units, upward heights, gravity168.
#ifndef FFT_ARC_RULES_H
#define FFT_ARC_RULES_H
static u16 core_arc_root(u32 value)
{
    u16 root=0,bit=1024;
    while(bit){u16 next=root|bit;if((u32)muls16(next,next)<=value)root=next;bit>>=1;}
    return root;
}
// Endpoints fit native source heights. The discriminant needs only16x16
// products; the flight itself advances by additions and shifts.
static s16 core_arc_curve(s16 dx,s16 dz,s16 rise,u8 high)
{
    s32 disc=28224-muls16(336,rise)-muls16(dx,dx)-muls16(dz,dz);
    s16 root;
    if((!dx&&!dz)||disc<0)return -1;
    root=(s16)core_arc_root((u32)disc);
    return 168-rise+(high?root:-root);
}
typedef struct {s32 y,advance;s16 bend;} CoreArcStep;
static void core_arc_begin(CoreArcStep *s,s16 start,s16 rise,s16 curve)
{
    s->y=muls16(start,4096);s->advance=muls16(rise,64)+muls16(curve,63);s->bend=curve+curve;
}
static s16 core_arc_next(CoreArcStep *s)
{
    s->y+=s->advance;s->advance-=s->bend;return (s16)(s->y>>12);
}
// Retail arc_b/arc_a choice, after both traces failed.255 means terrain.
static u8 core_arc_recipient(u8 low,u8 high,u8 low_enemy,u8 high_enemy)
{
    if(low!=255&&low_enemy)return low;
    if(high!=255&&high_enemy)return high;
    return low!=255?high:255;
}
#endif
