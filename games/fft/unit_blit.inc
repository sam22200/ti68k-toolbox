// A human reuses its monster-only cover scratch for composed rows. Each row
// keeps shifted light/dark longs and the original16-bit opaque mask (10 bytes).
// Up to25 nontransparent rows fit the existing256 bytes; larger poses fall back.
// The pose key is the actor cache's actual key, including budget deferral/mirror.
#include <stddef.h>
typedef char UnitBlitAlignment[(offsetof(Cover,bc)&1)?1:-1];
typedef char UnitBlitCapacity[(UNIT_SH==26&&sizeof(((Cover *)0)->bc)>=251)?1:-1];
static u8 __attribute__((noinline)) unit_blit_covered(s16 x,s16 y,const u8 *g,Cover *c,u16 pose_key)
{
    u16 r,first,last,shift=(u16)(16-(x&15));u8 *row,*dl,*dd,changed=0;
    if(!c||(u16)x>=RT_PBYTES*8-16)return 0;
    if(c->bvalid!=3||c->bx!=(x&15)||RD16(&c->bd,0)!=pose_key){
        c->bvalid=0;first=0;last=UNIT_SH;
        // One transparent endpoint is enough to fit25 rows. Avoid a full trim
        // scan; oversized poses retain the ordinary compositor.
        if(RD16(g,0)==0&&RD16(g,UNIT_SH)==0&&RD16(g,2*UNIT_SH)==65535U)first=1;
        else if(RD16(g,UNIT_SH-1)==0&&RD16(g,2*UNIT_SH-1)==0&&RD16(g,3*UNIT_SH-1)==65535U)last--;
        else return 0;
        row=(u8 *)c->bc+1;                 // bc starts at an odd byte in Cover
        for(r=first;r<last;r++,row+=10){
            u16 k=r>=c->r0&&r<=c->r1?(u16)(c->cm[r]&RD16(g,3*UNIT_SH+r)):0;
            WR32(row,0,(u32)(u16)(RD16(g,r)&~k)<<shift);
            WR32(row,2,(u32)(u16)(RD16(g,UNIT_SH+r)&~k)<<shift);
            WR16(row,4,(u16)~(RD16(g,2*UNIT_SH+r)|(r>=UNIT_FOOT?k:0)));
        }
        c->bx=x&15;WR16(&c->bd,0,pose_key);
        c->b0=(u8)first;c->b1=(u8)last;c->bvalid=3;
        changed=1;
    }
    first=c->b0;last=c->b1;
    if(y+(s16)first<0)first=(u16)-y;
    if(y+(s16)last>RT_DRAW_ROWS)last=(u16)(RT_DRAW_ROWS-y);
    if(first>=last||y>=RT_DRAW_ROWS)return (u8)(1+changed);
    row=(u8 *)c->bc+1+10*(first-c->b0);
    dl=(u8 *)rt_light+(u16)(y+first)*RT_PBYTES+2*(x>>4);
    dd=(u8 *)rt_dark+(u16)(y+first)*RT_PBYTES+2*(x>>4);
#define HUMAN_ROW() do {u32 keep=~((u32)RD16(row,4)<<shift); \
    WR32(dl,0,(RD32(dl,0)&keep)|RD32(row,0)); \
    WR32(dd,0,(RD32(dd,0)&keep)|RD32(row,2)); \
    row+=10;dl+=RT_PBYTES;dd+=RT_PBYTES;} while(0)
    if((last-first)&1)HUMAN_ROW();
    for(r=(u16)((last-first)>>1);r;r--){HUMAN_ROW();HUMAN_ROW();}
#undef HUMAN_ROW
    return (u8)(1+changed);
}
