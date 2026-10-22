// Beats: a scripted scene step (actors, a dialogue or a silent hold), shared by the battle
// engine's map scenes (opening.inc) and the scene player (campaign/scene.c). The includer
// provides st.open_*, W->dialog*/feet*/cover/status, cover(), actor_frame(), ui_text().
// BEATS_PICTURE (the scene player): beats over a picture with depth (vbuf, CHAPEL_*), panned;
// otherwise over the battle map, its camera and its cover masks.
#ifdef BEATS_PICTURE
#define ON_PICTURE 1
#else
#define ON_PICTURE 0
#endif
static const u8 *opening_beat(void)
{
    return opening_story + RD16(opening_story + 2, st.open_beat);
}

static const char *opening_speaker(void)
{
    const u8 *b = opening_beat();
    return (const char *)(b + 4 + b[1] * 8);
}

static const char *opening_text(void)
{
    const char *s = opening_speaker();
    while (*s++) ;
    return s + st.open_offset;
}

static void opening_dialog_reset(void)
{
    st.open_reveal=0;st.open_offset=0;st.open_clock=0;W->dialog_key=0xFFFF;
}

static u8 opening_find(u8 event)
{
    u8 i;
    for(i=0;i<OPENING_BEATS;i++){
        st.open_beat=i;
        if(opening_beat()[0]==event){opening_dialog_reset();return 1;}
    }
    return 0;
}

// A known speaker's portrait (ffcp, tools/common_portraits.py: names, then 32x29 portraits),
// at the bottom left of the dialogue; pgfx is the includer's (RT_NULL: none). 0xFF: none.
#define DLG_FACE_W 20                   // its outlined art is 19 pixels wide; 22 with frame and gap
static u8 opening_face(const char *name)
{
    const u8 *p;u8 n;
    if(!pgfx||!*name)return 0xFF;
    for(p=pgfx+7,n=pgfx[6];n;n--){
        const char *a=(const char *)p+1,*b=name;
        while(*a&&*a==*b)a++,b++;
        if(!*a&&!*b)return p[0];
        p++;while(*p++);               // the next entry: its portrait, its name
    }
    return 0xFF;
}
static void opening_face_draw(u8 face,s16 x)   // x: its frame's left, beside the box
{
    const u8 *q=pgfx+RD16(pgfx,2)+(u16)face*348;
    {
#ifdef __m68k__
        RtSprite sp={32,29,q,q+116,q+232};
#else
        u32 rows[3][29];u8 k,r;
        RtSprite sp={32,29,rows[0],rows[1],rows[2]};
        for(k=0;k<3;k++)for(r=0;r<29;r++)rows[k][r]=RD32(q+k*116,r*2);
#endif
        draw_rect(x,RT_H-31,DLG_FACE_W+1,31,st.open_white?C_BLACK:C_WHITE);   // the box's frame
        draw_rect(x+1,RT_H-30,DLG_FACE_W-1,30,st.open_white?C_WHITE:C_BLACK);
        draw_sprite(x,RT_H-29,&sp);
    }
}

// Fit words with the actual canonical glyph advances; six lines maximum, with
// a continuation page instead of truncation. The box has no spare bottom rows.
static void opening_layout(void)
{
    ZB(23);
    const char *s=opening_text();u16 at=0;u8 row=0,maxw=0,limit;
    W->dialog_face=opening_face(opening_speaker());
    limit=W->dialog_face==0xFF?146:146-DLG_FACE_W-2;
    memset(W->dialog,0,sizeof W->dialog);W->dialog_total=0;W->dialog_drawn=0xFFFF;W->dialog_part=0;
    while(s[at] && row<6){
        u16 start=at,last=at;u8 width=0,col=0,lastcol=0,lastwidth=0;
        while(s[at] && col<47){
            u8 ch=(u8)s[at],adv=UI_SHARED(ADV)[ch>=32&&ch<=126?ch-32:'?'-32];
            if(width+adv>limit)break;
            if(ch==' '){last=at;lastcol=col;lastwidth=width;}
            width+=adv;at++;col++;
        }
        if(s[at] && last>start){at=last;col=lastcol;width=lastwidth;}
        memcpy(W->dialog[row],s+start,col);W->dialog_total+=col;
        if(width>maxw)maxw=width;
        while(s[at]==' ')at++;
        row++;
    }
    W->dialog_n=row;W->dialog_w=maxw+8;W->dialog_h=6+7*row;
    W->dialog_next=(u8)at;W->dialog_key=st.open_beat;W->dialog_white=st.open_white;
    ZE(23);
}

static void opening_positions(void)
{
    const u8 *b=opening_beat(),*a=b+4;u8 i;
    for(i=0;i<b[1];i++,a+=8){
        u8 slot=a[0]>>4,gfx=a[0]&15;
        s16 x=(s16)RD16(a+2,0),y=(s16)RD16(a+4,0);
        if(!(st.open_seen&(1U<<slot))){
            W->actor_key[slot]=0xFFFF; // a scripted role never borrows a battle actor's body
            W->feet[slot].x=x;W->feet[slot].y=y;
            if(ON_PICTURE&&gfx<9){W->feet[slot].x-=40;W->feet[slot].y+=20;}
            if(!ON_PICTURE&&gfx==11)W->feet[slot].x-=64;
            st.open_seen|=(u16)(1U<<slot);W->cover[slot].rot=255;
        }
        if(!ON_PICTURE && b[3] && !(st.open_seen&0x8000) && (gfx==1||gfx==9)){
            W->feet[slot].x=x-64;W->feet[slot].y=y;
        }
        W->feet_k[slot]=W->feet[slot].x!=x||W->feet[slot].y!=y;
        if(W->feet[slot].x!=x)W->feet[slot].x+=x>W->feet[slot].x?1:-1;
        if(W->feet[slot].y!=y)W->feet[slot].y+=y>W->feet[slot].y?1:-1;
    }
    if(!ON_PICTURE&&b[3])st.open_seen|=0x8000;
    if(ON_PICTURE && !(st.tick&3)){
        if(st.open_pan<b[3])st.open_pan+=8;
        else if(st.open_pan>b[3])st.open_pan-=8;
    }
}

static void opening_actor(u8 gfx,u8 slot,s16 x,s16 y,u8 face,u16 logical)
{
    u8 pose=W->feet_k[slot]?walk_seq[st.tick&15]:0;
    const u8 *g=actor_frame(W->feet_k[slot]&&gfx<11&&!(!ON_PICTURE&&opening_beat()[3])?(u16)(gfx*44+(face&1?5:0)+pose+(face&2?22:0)):logical,slot);
    RtSprite sp;u16 l[UNIT_SH],d[UNIT_SH],m[UNIT_SH];u8 r;
    Cover *c=&W->cover[slot<NUNIT?slot:0];u8 depth=face>>2;
    // Cached per-byte depth sampling, the tactical cover algorithm at stride 20.
    // Six actors x 26 x 16 per-pixel tests cost ~0.9M cycles in the first version.
    if(c->x!=x||c->y!=y||c->depth!=depth||c->rot!=(ON_PICTURE?42:43)){
#ifdef BEATS_PICTURE
        s16 bx=(x-8)>>3,shift=(x-8)&7;
#endif
#ifndef BEATS_PICTURE
        cover(c->cm,x,y,depth);
#else
        for(r=0;r<UNIT_SH;r++){
            s16 sy=y-UNIT_FOOT+r;u32 bits=0;u8 j;
            for(j=0;j<3;j++){
                u8 bit=0;s16 sx=bx+j;
                if((u16)sy<100&&(u16)sx<CHAPEL_STRIDE){
                    u16 at=(u16)(sy*CHAPEL_STRIDE+sx);
                    if(vbuf[2*CHAPEL_PLANE+at]>depth)bit|=vbuf[3*CHAPEL_PLANE+at];
                    if(vbuf[4*CHAPEL_PLANE+at]>depth)bit|=vbuf[5*CHAPEL_PLANE+at];
                }
                bits=(bits<<8)|bit;
            }
            c->cm[r]=(u16)(bits>>(8-shift));
        }
#endif
        c->x=x;c->y=y;c->depth=depth;c->rot=ON_PICTURE?42:43;
    }
    for(r=0;r<UNIT_SH;r++){
        // The rear abduction is a staged foreground shot: the static battle
        // mesh lacks the original event's moving/hidden parts. Preserve the
        // tactical terrain shader in battle and the chapel's own cover here.
        u16 covered=ON_PICTURE?c->cm[r]&RD16(g,3*UNIT_SH+r):0;
        l[r]=RD16(g,r)&~covered;d[r]=RD16(g,UNIT_SH+r)&~covered;m[r]=RD16(g,2*UNIT_SH+r);
    }
    sp.w=16;sp.h=UNIT_SH;sp.light=l;sp.dark=d;sp.mask=m;draw_sprite(x-8-(ON_PICTURE?st.open_pan:st.camx),y-UNIT_FOOT-(ON_PICTURE?0:st.camy),&sp);
}

static void opening_copy_rect(s16 x,s16 y,u8 w,u8 h)
{
    u8 a=(u8)(x>>3),b=(u8)((x+w-1)>>3),r;
    u8 left=(u8)(0xFF>>(x&7)),right=(u8)(0xFF<<(7-((x+w-1)&7)));
    for(r=0;r<h;r++){
        u16 at=(u16)((y+r)*RT_PBYTES);u8 p;
        for(p=0;p<2;p++){
            u8 *dst=(u8*)(p?rt_dark:rt_light)+at;const u8 *src=(const u8*)W->status[p]+at;
            if(a==b){u8 mask=left&right;dst[a]=(dst[a]&~mask)|(src[a]&mask);}
            else{dst[a]=(dst[a]&~left)|(src[a]&left);dst[b]=(dst[b]&~right)|(src[b]&right);if(b>a+1)memcpy(dst+a+1,src+a+1,b-a-1);}
        }
    }
}

static void opening_dialog_draw(void)
{
    u8 row,col;u16 n=0;u8 w=W->dialog_w,h=W->dialog_h;
    // with a portrait, the frame and the box are centred together
    s16 x=W->dialog_face==0xFF?(RT_W-w)>>1:DLG_FACE_W+2+((RT_W-DLG_FACE_W-2-w)>>1),y=RT_H-h;
    const char *name=opening_speaker();s16 nw=8;
    void *saved_l=rt_light,*saved_d=rt_dark;
    u8 cold=W->dialog_drawn==0xFFFF;
    u16 visible=st.open_reveal,from=cold?0:W->dialog_drawn;
    if(visible>from+8)visible=from+8;
    rt_light=W->status[0];rt_dark=W->status[1];
    for(col=0;name[col];col++)nw+=UI_SHARED(ADV)[(u8)name[col]-32];
    if(nw>156)nw=156;
    if(cold){
        draw_rect(x,y,w,h,st.open_white?C_BLACK:C_WHITE);
        draw_rect(x+1,y+1,w-2,h-2,st.open_white?C_WHITE:C_BLACK);
        // Word masks apply the ordered gradient without a per-pixel loop.
        for(row=2;row<5;row++){
            u16 pat=row<2?0xFFFF:row==2?0xAAAA:row==3?0x8888:0;
            u8 first=(u8)((x+2)>>4),last=(u8)((x+w-3)>>4),word;
            u16 left=(u16)(0xFFFFU>>((x+2)&15)),right=(u16)(0xFFFFU<<(15-((x+w-3)&15)));
            u8 *l=(u8*)rt_light+(y+row)*RT_PBYTES;
            for(word=first;word<=last;word++){
                u16 mask=pat&(word==first?left:0xFFFF)&(word==last?right:0xFFFF);
                RMW16(l+2*word,mask,st.open_white?mask:0);
            }
        }
        if(*name){                      // (an anonymous line: no name box)
            draw_rect(x,y-11,nw,11,st.open_white?C_BLACK:C_WHITE);
            draw_rect(x+1,y-10,nw-2,10,st.open_white?C_WHITE:C_BLACK);
            ui_text(x+4,y-8,name,st.open_white?C_BLACK:C_WHITE,156);
        }
    }
    if(cold||W->dialog_drawn!=st.open_reveal||W->dialog_part<W->dialog_n){
        for(row=0;row<W->dialog_n;row++){
            s16 sx=x+4;char ch[2]={0,0};
            for(col=0;W->dialog[row][col];col++,n++){
                ch[0]=W->dialog[row][col];
                if(row==W->dialog_part||(W->dialog_drawn!=0xFFFF&&n>=W->dialog_drawn&&n<visible+3)){
                    u8 color=n<visible?(st.open_white?C_BLACK:C_WHITE):n<visible+3?(st.open_white?C_DGRAY:C_LGRAY):(st.open_white?C_LGRAY:C_DGRAY);
                    ui_text(sx,y+4+row*7,ch,color,156);
                }
                sx+=UI_SHARED(ADV)[(u8)ch[0]-32];
            }
        }
        if(W->dialog_part<W->dialog_n)W->dialog_part++;
        W->dialog_drawn=visible;
    }
    rt_light=saved_l;rt_dark=saved_d;
    // Only the fitted window rectangle is copied, not a second whole screen.
    opening_copy_rect(x,y,w,h);
    if(*name)opening_copy_rect(x,y-11,(u8)nw,11);
    if(W->dialog_face!=0xFF)opening_face_draw(W->dialog_face,x-DLG_FACE_W-2);
    if(st.open_reveal>=W->dialog_total && (st.tick&16))draw_rect(x+w-5,96,3,2,st.open_white?C_BLACK:C_WHITE);
}
