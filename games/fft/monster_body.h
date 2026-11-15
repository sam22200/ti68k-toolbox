// FMB1: optional archived wide monster bodies (idle, walk, attack, hurt, Dead),
// raw rows read in place. Format: games/fft/campaign/MONSTER_BODY.md.
#ifndef FFT_MONSTER_BODY_H
#define FFT_MONSTER_BODY_H
#define MB_JOB0 94
#define MB_JOBS 48
#define MB_FAMILIES 48
#define MB_ANIMS 10                     // five poses x front/back
#define MB_HEADER (8 + MB_JOBS)
#define MB_ROWS 40
#define MB_FRAMES 1024
enum { MB_IDLE, MB_WALK, MB_ATTACK, MB_HURT, MB_DEAD };
static u8 monster_body_valid(const u8 *b,u16 size)
{
    u16 families,frames,i,last,base;const u8 *p;
    if(!b||size<MB_HEADER||b[0]!='F'||b[1]!='M'||b[2]!='B'||b[3]!='1')return 0;
    families=RD16(b,2);frames=RD16(b,3);
    if(!families||families>MB_FAMILIES||!frames||frames>MB_FRAMES)return 0;
    base=(u16)(MB_HEADER+4*MB_ANIMS*families);last=(u16)(base+2*(frames+1));
    if(last>size)return 0;
    for(i=0;i<MB_JOBS;i++)if(b[8+i]!=255&&b[8+i]>=families)return 0;
    for(i=0;i<MB_ANIMS*families;i++){
        p=b+MB_HEADER+4*i;
        if(!p[2]||!p[3]||RD16(p,0)+p[2]>frames)return 0;
    }
    p=b+base;
    for(i=0;i<frames;i++){
        u16 start=RD16(p,i),end=RD16(p,i+1),r,n;const u8 *g;u8 h,wb;
        if(start!=last||(start&1)||end<=start||end>size||end-start<4)return 0;
        g=b+start;h=g[0];wb=g[1];
        if(!h||h>MB_ROWS||(wb!=2&&wb!=4&&wb!=6)||!g[2]||g[2]>MB_ROWS||
           (s16)h-g[2]>12||g[3]>8*wb||end-start!=4+4*h*wb)return 0;
        g+=4;n=(u16)(h*wb);
        for(r=0;r<n;r++)if((g[r]|g[n+r]|g[3*n+r])&g[2*n+r])return 0;
        last=end;
    }
    return last==size;
}
// Validated cold; the frame of an animation, direct: family record, then offsets.
static const u8 *monster_body_frame(const u8 *b,u8 job,u8 anim,u16 t,u8 hold)
{
    const u8 *a;u8 family,n;u16 k;
    if((u8)(job-MB_JOB0)>=MB_JOBS||(family=b[8+job-MB_JOB0])==255)return RT_NULL;
    a=b+MB_HEADER+4*(MB_ANIMS*family+anim);
    n=a[2];k=(u16)(t/a[3]);
    k=hold?(k<n?k:(u16)(n-1)):(u16)(k%n);
    return b+RD16(b+MB_HEADER+4*MB_ANIMS*RD16(b,2),RD16(a,0)+k);
}
#endif
