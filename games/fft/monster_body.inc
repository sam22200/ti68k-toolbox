#define BODY_BLIT_CODE
#include "body_blit.h"
// Wide monster bodies (monster_body.h), drawn in 16-column strips straight from the
// archive. Same rules as the human sprite: covered interior white, covered shadow
// transparent, the depth-1 water crop with its light edge, the hit flash.
// The bank that draws this job, or none: a chapter's families may need two.
static const u8 *monster_body_of(u8 job)
{
    u8 b;
    for(b=0;b<MB_BANKS;b++)
        if(body_gfx[b]&&monster_body_box(body_gfx[b],job)!=RT_NULL)return body_gfx[b];
    return RT_NULL;
}
static u8 monster_body_active(u8 i)
{
    return (u8)(!DC_OBJ(st.unit[i].dc)&&monster_body_of(st.unit[i].job)!=RT_NULL);
}
// The current frame: Dead, hurt, the attacker's blow (held on its last frame), the
// walker, otherwise the idle loop, staggered per unit like the human idle.
static const u8 *monster_body_pose(const u8 *bank,u8 rot,u8 i)
{
    u8 dir=unit_direction(rot,i),pose=unit_pose(i),anim=MB_IDLE,hold=0;
    u16 t=(u16)(st.tick+11*i);
    if(pose==P_KO)anim=MB_DEAD;
    else if(pose==P_HIT)anim=MB_HURT;
    else if(pose==P_WIND||pose==P_BLOW||pose==P_THROW){
        anim=MB_ATTACK;hold=1;t=st.mode==M_ACT&&i==st.atk?(u16)(st.act_t-ACT_WIND):0;
    }else if(st.mode==M_WALK&&i==st.sel)anim=MB_WALK;
    return monster_body_frame(bank,st.unit[i].job,(u8)(2*anim+(dir&1)),t,hold);
}
static const u8 rev_nibble[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
// One 16-column word, mirrored: p is the row inside a plane's strip block (2 bytes, word
// aligned). The row loops read four words a strip, so the plain read is inlined and only
// the mirror stays a call (-Os would otherwise call out for both).
static u16 body_mirrored(const u8 *p)
{
    u8 a=p[1],b=p[0];
    return (u16)((rev_nibble[a&15]<<12)|(rev_nibble[a>>4]<<8)|(rev_nibble[b&15]<<4)|rev_nibble[b>>4]);
}
#define body_word(p,mirror) ((mirror)?body_mirrored(p):(u16)((p)[0]<<8|(p)[1]))
// The depth planes of the current view, read once per body: in a chapter SC_H is a
// runtime value, so VDEPTH and DEPTH_STRIDE each cost a multiply every time they are named.
typedef struct { const u8 *base; u16 stride, h; } BodyDepth;
#ifdef FFT_VIEW_SPLIT
#define BODY_BASE vbuf
#define BODY_ROW(v,y,t_,x0_,w_) do{const DepthRow *r_=(const DepthRow *)(v)->base+(y); \
    t_=(v)->base+r_->off;x0_=r_->x0;w_=r_->w;}while(0)
#else
#define BODY_BASE VDEPTH
#define BODY_ROW(v,y,t_,x0_,w_) do{t_=(v)->base+(u16)((y)*SC_BYTES);x0_=0;w_=SC_BYTES;}while(0)
#endif
// The cover bytes of one body row, every strip at once: one row lookup instead of one per
// strip. Measured: a lookup per strip and per row cost 43% of a13-body frame.
static void body_cover_row(const BodyDepth *v,s16 y,s16 b0,u8 depth,u8 nb,u8 *out)
{
    const u8 *t,*t1,*t2,*t3;u16 x0,w;u8 j,c;
    if((u16)y>=v->h){for(j=0;j<nb;j++)out[j]=0;return;}
    BODY_ROW(v,y,t,x0,w);
    t+=b0;t1=t+v->stride;t2=t1+v->stride;t3=t2+v->stride;   // the four planes walked, not indexed
    for(j=0;j<nb;j++,t++,t1++,t2++,t3++,b0++){
        c=0;
        if((u16)(b0-x0)<w){
            if(*t>depth)c=*t1;
            if(*t2>depth)c|=*t3;
        }
        out[j]=c;
    }
}
// A slot goes to the first unit that asks for one and stays with it; the claimed ones
// come first, so finding a free slot means this unit owns none.
static BodyShift *body_shift_slot(u8 i)
{
    u8 k,me=(u8)(i+1);BodyShift *b=W->bshift;
    for(k=0;k<MB_SHIFT_N;k++,b++){
        if(b->owner==me)return b;
        if(!b->owner){b->owner=me;return b;}   // Work is zeroed, so its picture is null
    }
    return RT_NULL;
}
// The top row of the drawn body relative to the feet (for the markers above it).
static u8 draw_monster_body(s16 sx,s16 sy,u8 rot,u8 i,const Cover *c)
{
    const u8 *g,*e;u8 h,wb,ay,mirror,strips,c0,sc,r,rows,wet,flash=unit_flash(i),depth;
    u8 r0[3],r1[3],first=0,last=0;u16 n,bh,l[2*MB_ROWS],d[2*MB_ROWS],m[2*MB_ROWS],kw[3*MB_ROWS];u8 cb[10];
    s16 x,y,wy;RtSprite sp;BodyShift *bshift;u8 bkey;
    {const u8 *bank=DC_OBJ(st.unit[i].dc)?RT_NULL:monster_body_of(st.unit[i].job);
     if(!bank)return 0;
     e=monster_body_box(bank,st.unit[i].job);
     g=monster_body_pose(bank,rot,i);}
    h=g[0];wb=g[1];ay=g[2];mirror=(u8)(unit_direction(rot,i)>>1);
    x=sx-(mirror?(s16)(8*wb-g[3]):g[3]);y=sy-ay;
    g+=4;n=(u16)(h*wb);rows=h;strips=(u8)(wb>>1);bh=(u16)(2*h);
    wet=(u8)(c?c->wet:unit_in_water(i));depth=c?c->depth:255;
    wy=sy-UNIT_FOOT+WATER_ROWS;                // the human crop line, from the same feet
    if(wet){if(y>=wy)return ay;if(y+rows>wy)rows=(u8)(wy-y);}
    // Two strips, not mirrored: keep this body composed and shifted to where it stands,
    // and a frame that finds it unmoved blits it on word boundaries without reading the
    // scenery, the picture or ExtGraph's shift again. A 16-column body is already inside
    // the budget thirteen at a time (248,346 cycles against 458,284), so it stays on the
    // ExtGraph path and spares both a slot and the code.
    bshift=(strips<=2&&rows<=MB_SHIFT_ROWS&&
            (u16)x<=(u16)(RT_PBYTES*8-32))?body_shift_slot(i):RT_NULL;
    bkey=(u8)(flash|(u8)(wet<<2));
    if(bshift&&bshift->g==g&&bshift->x==x&&bshift->y==y&&bshift->d==depth&&
       bshift->rot==rot&&bshift->flags==bkey&&          // the height follows from them
       body_rows_blit(x,y,bshift->row,rows))return ay;
    for(c0=0;c0<3;c0++){r0[c0]=rows;r1[c0]=0;}  // per strip: the rows the scenery really bites
    if(depth!=255){
        u8 eh=e[0],ewb=e[1],eay=e[2],eax=e[3],enb=(u8)(ewb+1),f0,f1,kept;
        s16 ebx=(s16)(sx-(mirror?(s16)(8*ewb-eax):(s16)eax)+st.camx),eby=(s16)(sy-eay+st.camy);
        s16 bx=x+st.camx,by=(s16)(y+st.camy),b0=bx>>3,off,into=(s16)(b0-(ebx>>3));
        u8 shift=(u8)(bx&7),nb=(u8)(wb+1);BodyDepth v;
        v.base=BODY_BASE;v.stride=DEPTH_STRIDE;v.h=SC_H;
        // The family's box holds every one of its poses, so what stands in front of it is
        // the same whatever the body is doing: read once per place, and keep those rows'
        // bytes, so a new picture of the same animation reads nothing at all.
        if(c&&c->bvalid&&c->bx==ebx&&c->by==eby&&c->bd==depth&&c->brot==rot){
            f0=c->b0;f1=c->b1;kept=(u8)(c->bvalid==2);
        }else{
            Cover *keep=(Cover *)c;      // W->cover is the unit's own mutable record
            u8 r2,j,any;
            f0=1;f1=0;kept=(u8)(enb<=MB_CACHE_BYTES);
            if(keep)memset(keep->bc,0,sizeof keep->bc);
            for(r2=0;r2<eh;r2++){
                body_cover_row(&v,(s16)(eby+r2),ebx>>3,depth,enb,cb);
                any=0;for(j=0;j<enb;j++)any|=cb[j];
                if(!any)continue;
                if(f0>f1)f0=r2;
                f1=r2;
                if(r2-f0>=MB_CACHE_ROWS||!keep)kept=0;
                else if(kept)memcpy(keep->bc[r2-f0],cb,enb);
            }
            if(keep){keep->bvalid=(u8)(kept?2:1);keep->bx=ebx;keep->by=eby;
                keep->bd=depth;keep->brot=rot;keep->b0=f0;keep->b1=f1;}
        }
        off=(s16)(eay-ay);                   // pose row r is box row r + off
        if(f0<=f1){
            s16 lo=(s16)f0-off,hi=(s16)f1-off+1;
            if(lo<0)lo=0;
            if(hi>(s16)rows)hi=(s16)rows;
            if(lo<hi){first=(u8)lo;last=(u8)hi;}
        }
        {   // the interior plane of each strip, once: the row loop only indexes it
            const u8 *in[3];
            for(c0=0;c0<strips;c0++){
                sc=mirror?(u8)(strips-1-c0):c0;
                in[c0]=g+3*n+sc*bh;
            }
            for(r=first;r<last;r++){         // only these rows are read again below
                const u8 *row;u16 *kwp=kw+r;
                if(kept)row=c->bc[r+off-f0]+into;
                else{body_cover_row(&v,(s16)(by+r),b0,depth,nb,cb);row=cb;}
                for(c0=0;c0<strips;c0++,kwp+=MB_ROWS){
                    u16 k=(u16)((u16)((u16)(row[2*c0]<<8|row[2*c0+1])<<shift)|(u16)(row[2*c0+2]>>(8-shift)));
                    if(k){
                        k&=body_word(in[c0]+2*r,mirror);
                        if(k){if(r<r0[c0])r0[c0]=r;r1[c0]=r;}
                    }
                    *kwp=k;
                }
            }
        }
    }
    sp.w=16;
    for(c0=0;c0<strips;c0++){
        const u8 *pl,*pd,*pm,*pi;u8 top=0,bottom=rows;
        u16 *lp=l+(bshift?(u16)c0*MB_ROWS:0),*dp=d+(bshift?(u16)c0*MB_ROWS:0),
            *mp=m+(bshift?(u16)c0*MB_ROWS:0);
        sc=mirror?(u8)(strips-1-c0):c0;
        pl=g+sc*bh;pd=pl+n;pm=pd+n;pi=pm+n;
        for(r=top;r<bottom;r++){
            u16 a=body_word(pl+2*r,mirror),b=body_word(pd+2*r,mirror),
                mk=body_word(pm+2*r,mirror),k=r>=first&&r<last?kw[c0*MB_ROWS+r]:0;
            if(flash){u16 in=r+1<ay?body_word(pi+2*r,mirror):0;
                a=(u16)((a&~in)|(flash==1?in:0));b&=(u16)~in;}
            lp[r]=(u16)(a&~k);dp[r]=(u16)(b&~k);mp[r]=(u16)(mk|(r>=ay?k:0));
            if(wet&&r+1==rows){lp[r]=(u16)~(mk|k);dp[r]=0;mp[r]=(u16)(mk|k);}
        }
        if(bshift)continue;                    // kept for the shift below, drawn after it
        sp.h=(u8)(bottom-top);sp.light=lp+top;sp.dark=dp+top;sp.mask=mp+top;
        draw_sprite(x+16*c0,(s16)(y+top),&sp);
    }
    // Both strips composed: shift each row once into the slot, which the next frames
    // blit untouched, and draw it.
    if(bshift){
        u8 sh=(u8)(x&15);
        if(strips<2)for(r=0;r<rows;r++){l[MB_ROWS+r]=d[MB_ROWS+r]=0;m[MB_ROWS+r]=0xFFFF;}
        for(r=0;r<rows;r++)
            body_row_shift(&bshift->row[r],l[r],d[r],m[r],
                           l[MB_ROWS+r],d[MB_ROWS+r],m[MB_ROWS+r],sh);
        bshift->g=g;bshift->x=x;bshift->y=y;bshift->d=depth;bshift->rot=rot;
        bshift->flags=bkey;
        body_rows_blit(x,y,bshift->row,rows);
    }
    return ay;
}
