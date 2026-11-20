// FMB1: optional archived wide monster bodies (idle, walk, attack, hurt, Dead),
// raw rows read in place. Format: games/fft/campaign/MONSTER_BODY.md.
#ifndef FFT_MONSTER_BODY_H
#define FFT_MONSTER_BODY_H
#define MB_JOB0 94
#define MB_JOBS 48
#define MB_FAMILIES 48
#define MB_ANIMS 10                     // five poses x front/back
#define MB_HEADER (8 + MB_JOBS)
#define MB_ENV(f) (MB_HEADER + 4 * (f))   // the family's box: rows, row bytes, feet
#define MB_ANIM(families, i) (MB_HEADER + 4 * (families) + 4 * (i))
#define MB_TABLE(families) MB_ANIM(families, MB_ANIMS * (families))
#define MB_ROWS 40
#define MB_FRAMES 1024
#define MB_BANKS 3        // ffmb, ffmc, ffmd: the families past one archive variable
enum { MB_IDLE, MB_WALK, MB_ATTACK, MB_HURT, MB_DEAD };
// A box (rows, row bytes, rows above the feet, columns left of them) holds another.
static u8 monster_box_holds(const u8 *e,const u8 *g)
{
    return (u8)(g[3]<=e[3]&&g[2]<=e[2]&&
                (u16)(8*g[1]-g[3])<=(u16)(8*e[1]-e[3])&&
                (s16)(g[0]-g[2])<=(s16)(e[0]-e[2]));   // the feet may sit below a picture
}
// Frame slots may share their pixels (the source repeats frames inside a sequence),
// so offsets are not chained; every slot is validated from its own header.
static u8 monster_body_valid(const u8 *b,u16 size)
{
    u16 families,frames,i,base,waits,first,top=0;const u8 *p,*e;
    if(!b||size<MB_HEADER||b[0]!='F'||b[1]!='M'||b[2]!='B'||b[3]!='1')return 0;
    families=RD16(b,2);frames=RD16(b,3);
    if(!families||families>MB_FAMILIES||!frames||frames>MB_FRAMES)return 0;
    base=(u16)MB_TABLE(families);
    waits=(u16)(base+2*(frames+1));
    first=(u16)(waits+frames+(frames&1));
    if(waits<base||first<waits||first>size)return 0;
    for(i=0;i<MB_JOBS;i++)if(b[8+i]!=255&&b[8+i]>=families)return 0;
    for(i=0;i<families;i++){
        e=b+MB_ENV(i);
        if(!e[0]||e[0]>MB_ROWS||(e[1]!=2&&e[1]!=4&&e[1]!=6&&e[1]!=8)||!e[2]||e[2]>MB_ROWS||
           (s16)e[0]-e[2]>12||e[3]>8*e[1])return 0;   // a probe box, wider than a picture
    }
    for(i=0;i<frames;i++)if(!b[waits+i])return 0;
    for(i=0;i<MB_ANIMS*families;i++){
        u16 total=0,k,start;
        p=b+MB_ANIM(families,i);
        start=RD16(p,0);
        if(!p[2]||!p[3]||start+p[2]>frames)return 0;
        for(k=0;k<p[2];k++)total=(u16)(total+b[waits+start+k]);
        if(total!=p[3])return 0;        // the animation's whole loop, in ticks
    }
    p=b+base;
    for(i=0;i<frames;i++){
        u16 start=RD16(p,i),r,n,end;const u8 *g;u8 h,wb;
        if((start&1)||start<first||(u16)(size-start)<4)return 0;
        g=b+start;h=g[0];wb=g[1];
        if(!h||h>MB_ROWS||(wb!=2&&wb!=4&&wb!=6)||!g[2]||g[2]>MB_ROWS||
           (s16)h-g[2]>12||g[3]>8*wb)return 0;
        n=(u16)(h*wb);
        if((u16)(size-start)<(u16)(4+4*n))return 0;
        end=(u16)(start+4+4*n);
        if(end>top)top=end;
        g+=4;
        for(r=0;r<n;r++)if((g[r]|g[n+r]|g[3*n+r])&g[2*n+r])return 0;
    }
    // Every picture an animation shows must sit inside its family's box: the
    // renderer probes the scenery in front of that box once, for every pose.
    for(i=0;i<MB_ANIMS*families;i++){
        u16 k,start;
        p=b+MB_ANIM(families,i);
        start=RD16(p,0);e=b+MB_ENV(i/MB_ANIMS);
        for(k=0;k<p[2];k++)if(!monster_box_holds(e,b+RD16(b+base,start+k)))return 0;
    }
    return top==size&&RD16(b+base,frames)==size;
}
// The family's box, or null: validated cold, so the renderer reads it directly.
static const u8 *monster_body_box(const u8 *b,u8 job)
{
    u8 family;
    if((u8)(job-MB_JOB0)>=MB_JOBS||(family=b[8+job-MB_JOB0])==255)return RT_NULL;
    return b+MB_ENV(family);
}
// Validated cold; the frame of an animation at tick t: the slot its own wait holds.
static const u8 *monster_body_frame(const u8 *b,u8 job,u8 anim,u16 t,u8 hold)
{
    const u8 *a,*w;u8 family,n,i;u16 k,base,first,families=RD16(b,2);
    if((u8)(job-MB_JOB0)>=MB_JOBS||(family=b[8+job-MB_JOB0])==255)return RT_NULL;
    a=b+MB_ANIM(families,MB_ANIMS*family+anim);
    base=(u16)MB_TABLE(families);
    w=b+base+2*(RD16(b,3)+1);
    n=a[2];first=RD16(a,0);
    k=hold||!t?t:(u16)(t%a[3]);
    for(i=0;i+1<n;i++){
        u8 d=w[first+i];
        if(k<d)break;
        k=(u16)(k-d);
    }
    return b+RD16(b+base,(u16)(first+i));
}
#endif
