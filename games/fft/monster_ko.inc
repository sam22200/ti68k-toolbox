// Source-specific fixed Dead poses. Raw archive rows prepare two existing cache entries.
// The existing 32-column blade-cover primitive and verified mirror routine are reused.
static u8 monster_ko_active(u8 i)
{
    u8 mirror;
    if(st.unit[i].hp||(!DC_OBJ(st.unit[i].dc)&&unit_pose(i)!=P_KO)||
       (DC_OBJ(st.unit[i].dc)&&!(st.morph_mask&((u16)1<<i))))return 0;
    return monster_ko_frame(ko_gfx,st.unit[i].job,0,&mirror)!=RT_NULL;
}
#ifdef FFT_ACTOR_BANK
// Two existing cache entries hold the shaded 16-column halves. Cover.cm is
// otherwise unused in raw-KO mode; keep the two remembered indexes and height
// there. Every remembered entry is validated after another consumer can evict it.
static u8 ko_cache_claim(u16 frame_key)
{
    u8 e,victim=0;u16 best=0;
    for(e=0;e<AC_N;e++)if(ac_key[e]==frame_key)goto found;
    for(e=0;e<AC_N;e++){
        u16 age=(u16)(W->ac_now-ac_age[e]);
        if(ac_key[e]==65535U){victim=e;break;}
        if(age>=best){best=age;victim=e;}
    }
    e=victim;ac_key[e]=frame_key;W->actor_misses++;
found:
    ac_age[e]=W->ac_now;return e;
}
static void ko_cache_draw(u8 a,u8 b,s16 x,s16 y,u8 rows)
{
    RtSprite sp;const u16 *g=acache[a];
    ac_age[a]=ac_age[b]=W->ac_now;
    sp.w=16;sp.h=rows;sp.light=g;sp.dark=g+UNIT_SH;sp.mask=g+2*UNIT_SH;
    draw_sprite(x,y,&sp);g=acache[b];
    sp.light=g;sp.dark=g+UNIT_SH;sp.mask=g+2*UNIT_SH;draw_sprite(x+16,y,&sp);
}
#endif
static u8 draw_monster_ko(s16 sx,s16 sy,u8 rot,u8 i,u8 depth)
{
    const u8 *frame,*g,*obj=RT_NULL;
    u8 mirror,h,r,rows,morph=0,wet;
    s16 ay,by,bx,b0,shift,source_y,obj_y;
    u32 flipped[4*KO_ROWS],l[KO_ROWS],d[KO_ROWS],m[KO_ROWS];
    RtSprite sp;
    if(!monster_ko_active(i))return 0;
    frame=monster_ko_frame(ko_gfx,st.unit[i].job,unit_direction(rot,i),&mirror);
    h=frame[0];ay=(s8)frame[2];g=frame+4;
    wet=(u8)(!DC_OBJ(st.unit[i].dc)&&unit_in_water(i));
#ifdef FFT_ACTOR_BANK
    if(!DC_OBJ(st.unit[i].dc)||morph_first()!=i){
        Cover *c=&W->cover[i];u16 a=c->cm[0],b=c->cm[1],frame_key=(u16)(KO_CACHE_BASE+2*i);
        if(c->r0==(u8)(128|unit_direction(rot,i))&&c->r1==st.unit[i].job&&
           c->x==sx+st.camx&&c->y==sy+st.camy&&c->depth==depth&&c->rot==rot&&c->wet==wet&&
           a<AC_N&&b<AC_N&&ac_key[a]==frame_key&&ac_key[b]==frame_key+1){
            ko_cache_draw((u8)a,(u8)b,sx-16,sy-ay,(u8)c->cm[2]);return 1;
        }
    }
#endif
    if(mirror){
#if defined(__m68k__) && defined(FFT_ACTOR_BANK)
        SpriteX8_MIRROR_H_R(4*h,g,4,(u8 *)flipped);
#else
        static const u8 rev[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
        const u8 *p=g;u8 *out=(u8 *)flipped;u16 n;
        for(n=0;n<4*h;n++,p+=4,out+=4){u8 k;
            for(k=0;k<4;k++){u8 v=p[3-k];out[k]=(u8)(rev[v&15]<<4|rev[v>>4]);}}
#endif
        g=(const u8 *)flipped;
    }
    by=sy+st.camy-ay;bx=sx+st.camx-16;rows=h;
    source_y=0;obj_y=0;
    if(DC_OBJ(st.unit[i].dc)&&morph_first()==i){
        s16 bottom=by+h,ob=sy+st.camy-UNIT_FOOT;
        obj=object_frame(i);morph=1;
        if(ob<by){source_y=ob-by;by=ob;}
        if(ob+UNIT_SH>bottom)bottom=ob+UNIT_SH;
        rows=(u8)(bottom-by);obj_y=by-ob;
    }
    if(wet){
        s16 end=sy+st.camy-UNIT_FOOT+WATER_ROWS;
        if(by>=end)return 1;
        if(by+rows>end)rows=(u8)(end-by);
    }
    b0=bx>>3;shift=bx&7;
    for(r=0;r<rows;r++){
        s16 sr=source_y+r,orr=obj_y+r;
        u32 a=0,b=0,mask=0xffffffffUL,in=0,k;u8 tail=0;
        if((u16)sr<h){
            a=RD32(g,2*sr);b=RD32(g,2*(h+sr));
            mask=RD32(g,2*(2*h+sr));in=RD32(g,2*(3*h+sr));
        }
        if(morph){
            u32 oa=0,od=0,om=0xffffffffUL,oi=0,bits=morph_bits((u8)orr);
            bits|=bits<<16;
            if((u16)orr<UNIT_SH){
                oa=(u32)RD16(obj,orr)<<8;od=(u32)RD16(obj,UNIT_SH+orr)<<8;
                om=((u32)RD16(obj,2*UNIT_SH+orr)<<8)|0xff0000ffUL;
                oi=(u32)RD16(obj,3*UNIT_SH+orr)<<8;
            }
            a^=(a^oa)&bits;b^=(b^od)&bits;mask^=(mask^om)&bits;in^=(in^oi)&bits;
            if(st.morph_t>=4&&st.morph_t<24&&orr<UNIT_FOOT-1){a&=~in;b&=~in;}
        }
        k=cover_bytes(by+r,b0,depth,5,&tail);
        k=((k<<shift)|((u32)tail>>(8-shift)))&in;
        l[r]=a&~k;d[r]=b&~k;m[r]=mask|((s16)(by+r)>=sy+st.camy?k:0);
        if(wet&&r+1==rows){l[r]=~(mask|k);d[r]=0;m[r]=mask|k;}
    }
    sp.w=32;sp.h=rows;sp.light=l;sp.dark=d;sp.mask=m;
#ifdef FFT_ACTOR_BANK
    if(!morph){
        Cover *c=&W->cover[i];u8 a,b;
        a=ko_cache_claim((u16)(KO_CACHE_BASE+2*i));b=ko_cache_claim((u16)(KO_CACHE_BASE+2*i+1));
        for(r=0;r<rows;r++){
            acache[a][r]=(u16)(l[r]>>16);acache[b][r]=(u16)l[r];
            acache[a][UNIT_SH+r]=(u16)(d[r]>>16);acache[b][UNIT_SH+r]=(u16)d[r];
            acache[a][2*UNIT_SH+r]=(u16)(m[r]>>16);acache[b][2*UNIT_SH+r]=(u16)m[r];
        }
        c->x=sx+st.camx;c->y=sy+st.camy;c->depth=depth;c->rot=rot;
        c->r0=(u8)(128|unit_direction(rot,i));c->r1=st.unit[i].job;
        c->cm[0]=a;c->cm[1]=b;c->cm[2]=rows;c->wet=wet;
        ko_cache_draw(a,b,sx-16,by-st.camy,rows);return 1;
    }
#endif
    draw_sprite(bx-st.camx,by-st.camy,&sp);
    return 1;
}
