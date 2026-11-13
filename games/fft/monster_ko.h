// FKO1: optional chapter-owned, raw archived monster Dead poses. No heap growth.
#ifndef FFT_MONSTER_KO_H
#define FFT_MONSTER_KO_H
#define KO_JOBS 160
#define KO_HEADER 328
#define KO_ROWS 40
#define KO_FRAME_ROWS 26
#define KO_CACHE_BASE 0xff00U
static u8 monster_ko_valid(const u8 *b,u16 size)
{
    u16 pairs,frames,i,last,base;const u8 *p;
    if(!b||size<KO_HEADER||b[0]!='F'||b[1]!='K'||b[2]!='O'||b[3]!='1')return 0;
    pairs=RD16(b,2);frames=RD16(b,3);
    if(!pairs||pairs>KO_JOBS||!frames||frames>4*KO_JOBS)return 0;
    base=(u16)(KO_HEADER+8*pairs);last=(u16)(base+2*(frames+1));
    if(last>size)return 0;
    for(i=0;i<KO_JOBS;i++){u16 v=RD16(b+8,i);if(v!=65535U&&v>=pairs)return 0;}
    for(i=0;i<4*pairs;i++)if((RD16(b+KO_HEADER,i)&32767U)>=frames)return 0;
    p=b+base;
    for(i=0;i<frames;i++){
        u16 start=RD16(p,i),end=RD16(p,i+1),r;u8 h;
        const u8 *g;
        if(start!=last||(start&1)||end<=start||end>size||end-start<4)return 0;
        g=b+start;h=g[0];
        if(!h||h>KO_FRAME_ROWS||g[1]!=16||g[2]>28||(s16)h-g[2]>12||g[3]||end-start!=4+16*h)return 0;
        g+=4;
        for(r=0;r<h;r++){
            u32 mask=RD32(g,4*h+2*r),inner=RD32(g,6*h+2*r);
            if((inner|RD32(g,2*r)|RD32(g,2*h+2*r))&mask)return 0;
        }
        last=end;
    }
    return last==size;
}
// Validated cold; lookup is direct by source job and camera-relative direction.
static const u8 *monster_ko_frame(const u8 *b,u8 job,u8 dir,u8 *mirror)
{
    u16 pair,key,base;
    if(job>=KO_JOBS)return RT_NULL;
    pair=RD16(b+8,job);if(pair==65535U)return RT_NULL;
    key=RD16(b+KO_HEADER,4*pair+dir);
    base=(u16)(KO_HEADER+8*RD16(b,2));*mirror=(u8)(key>>15);
    return b+RD16(b+base,key&32767U);
}
#endif
