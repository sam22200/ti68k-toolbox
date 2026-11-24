// Composed human rows versus an independent pixel oracle, including warm-cache
// ownership, cover invalidation, pose changes, oversize fallback and clipping.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#include <string.h>
#endif
#include "../../../runtime/core/rt.h"
#include "../base.inc"
#define UNIT_SH 26
#define UNIT_FOOT 21
typedef struct {
    s16 x,y;u8 depth,rot,r0,r1,wet;u16 cm[UNIT_SH];
    s16 bx,by;u8 bd,brot,b0,b1,bvalid;u8 bc[32][8];
} Cover;
static u16 planes[2][RT_DRAW_BYTES/2],expected[2][RT_DRAW_BYTES/2],picture[4*UNIT_SH];
void *rt_light=planes[0],*rt_dark=planes[1];
#include "../unit_blit.inc"
static u32 checks,failures,digest=17;
static void check(u16 ok){checks++;if(!ok)failures++;digest=(digest<<1|digest>>31)^ok;}
static void fixture(u8 rows,u8 seed)
{
    u16 r;
    for(r=0;r<UNIT_SH;r++){
        u16 bits=r<rows?(u16)(0x7ffeU^(seed*17+r*257)):0;
        WR16((u8 *)picture,r,bits&(u16)(0x5555U+r*31));
        WR16((u8 *)picture,UNIT_SH+r,bits&(u16)(0xaaaaU+seed*13));
        WR16((u8 *)picture,2*UNIT_SH+r,(u16)~bits);
        WR16((u8 *)picture,3*UNIT_SH+r,bits&0x1ff8U);
    }
}
static void trial(Cover *c,s16 x,s16 y,u16 pose_key,u8 accepted)
{
    u16 p,r,col,at;const u8 *g=(const u8 *)picture;
    for(p=0;p<2;p++)for(at=0;at<RT_DRAW_BYTES;at++)((u8 *)planes[p])[at]=(u8)(at*13+p*71);
    memcpy(expected,planes,sizeof planes);
    check((unit_blit_covered(x,y,g,c,pose_key)!=0)==accepted);
    if(accepted)for(r=0;r<UNIT_SH;r++){
        u16 k=r>=c->r0&&r<=c->r1?(u16)(c->cm[r]&RD16(g,3*UNIT_SH+r)):0;
        u16 mask=(u16)(RD16(g,2*UNIT_SH+r)|(r>=UNIT_FOOT?k:0));
        if((u16)(y+r)>=RT_DRAW_ROWS)continue;
        for(col=0;col<16;col++){
            u16 bit=(u16)(32768U>>col);u8 dstbit=(u8)(128U>>((x+col)&7));
            at=(u16)(y+r)*RT_PBYTES+((x+col)>>3);
            for(p=0;p<2;p++){
                if(!(mask&bit))((u8 *)expected[p])[at]&=(u8)~dstbit;
                if((RD16(g,p*UNIT_SH+r)&~k)&bit)((u8 *)expected[p])[at]|=dstbit;
            }
        }
    }
    for(p=0;p<2;p++)check(memcmp(planes[p],expected[p],RT_DRAW_BYTES)==0);
}
static void trials(void)
{
    static const s16 ys[]={-30,-25,-8,0,81,99,100};
    Cover c;u16 phase,n;u8 i;
    memset(&c,0,sizeof c);c.r0=2;c.r1=23;
    for(n=0;n<UNIT_SH;n++)c.cm[n]=(u16)(0xaaaaU+n*71);
    for(phase=0;phase<16;phase++)for(i=0;i<sizeof ys/sizeof ys[0];i++){
        fixture(25,(u8)phase);trial(&c,(s16)(32+phase),ys[i],(u16)(phase|0x8000),1);
        // The second draw uses warm composed rows.
        trial(&c,(s16)(32+phase),ys[i],(u16)(phase|0x8000),1);
        fixture(19,(u8)(phase+1));trial(&c,(s16)(32+phase),ys[i],(u16)(phase+1),1);
        // A new scenery cover explicitly invalidates the human scratch owner.
        c.cm[7]^=65535U;c.bvalid=0;trial(&c,(s16)(32+phase),ys[i],(u16)(phase+1),1);
    }
    fixture(26,1);c.bvalid=0;trial(&c,32,0,9,0);
    fixture(25,1);trial(&c,-1,0,9,0);trial(&c,224,0,9,0);
    trial(&c,223,0,9,1);
    // A monster overwrites this same scratch with its own owner tag.
    memset(c.bc,0xa5,sizeof c.bc);c.bvalid=2;trial(&c,223,0,9,1);
    fixture(0,0);trial(&c,32,0,10,1);
}
#ifdef __m68k__
void _main(void){trials();BENCH_VALUE(checks);BENCH_VALUE(failures);BENCH_VALUE(digest);}
#else
int main(void){trials();printf("unit-blit %lu checks failures %lu digest %08lX\n",(unsigned long)checks,(unsigned long)failures,(unsigned long)digest);return failures!=0;}
#endif
