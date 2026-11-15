// Wide monster bodies (monster_body.h), drawn in 16-column strips straight from the
// archive. Same rules as the human sprite: covered interior white, covered shadow
// transparent, the depth-1 water crop with its light edge, the hit flash.
static u8 monster_body_active(u8 i)
{
    return body_gfx&&!DC_OBJ(st.unit[i].dc)&&
        monster_body_frame(body_gfx,st.unit[i].job,MB_IDLE,0,0)!=RT_NULL;
}
// The current frame: Dead, hurt, the attacker's blow (held on its last frame), the
// walker, otherwise the idle loop, staggered per unit like the human idle.
static const u8 *monster_body_pose(u8 rot,u8 i)
{
    u8 dir=unit_direction(rot,i),pose=unit_pose(i),anim=MB_IDLE,hold=0;
    u16 t=(u16)(st.tick+11*i);
    if(pose==P_KO)anim=MB_DEAD;
    else if(pose==P_HIT)anim=MB_HURT;
    else if(pose==P_WIND||pose==P_BLOW||pose==P_THROW){
        anim=MB_ATTACK;hold=1;t=st.mode==M_ACT&&i==st.atk?(u16)(st.act_t-ACT_WIND):0;
    }else if(st.mode==M_WALK&&i==st.sel)anim=MB_WALK;
    return monster_body_frame(body_gfx,st.unit[i].job,(u8)(2*anim+(dir&1)),t,hold);
}
static const u8 rev_nibble[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
static u16 body_word(const u8 *row,u8 wb,u8 c,u8 mirror)
{
    if(mirror){u8 a=row[wb-1-2*c],b=row[wb-2-2*c];
        return (u16)((rev_nibble[a&15]<<12)|(rev_nibble[a>>4]<<8)|(rev_nibble[b&15]<<4)|rev_nibble[b>>4]);}
    return (u16)(row[2*c]<<8|row[2*c+1]);
}
// The top row of the drawn body relative to the feet (for the markers above it).
static u8 draw_monster_body(s16 sx,s16 sy,u8 rot,u8 i,const Cover *c)
{
    const u8 *g;u8 h,wb,ay,mirror,c0,r,rows,wet,flash=unit_flash(i),depth;
    u16 n,l[MB_ROWS],d[MB_ROWS],m[MB_ROWS];s16 x,y,wy;RtSprite sp;
    if(!monster_body_active(i))return 0;
    g=monster_body_pose(rot,i);
    h=g[0];wb=g[1];ay=g[2];mirror=(u8)(unit_direction(rot,i)>>1);
    x=sx-(mirror?(s16)(8*wb-g[3]):g[3]);y=sy-ay;
    g+=4;n=(u16)(h*wb);rows=h;
    wet=(u8)(c?c->wet:unit_in_water(i));depth=c?c->depth:255;
    wy=sy-UNIT_FOOT+WATER_ROWS;                // the human crop line, from the same feet
    if(wet){if(y>=wy)return ay;if(y+rows>wy)rows=(u8)(wy-y);}
    sp.w=16;sp.h=rows;sp.light=l;sp.dark=d;sp.mask=m;
    for(c0=0;c0<wb>>1;c0++){
        s16 bx=x+16*c0+st.camx,b0=bx>>3;u8 shift=(u8)(bx&7);
        for(r=0;r<rows;r++){
            const u8 *row=g+r*wb;
            u16 a=body_word(row,wb,c0,mirror),b=body_word(row+n,wb,c0,mirror),
                mk=body_word(row+2*n,wb,c0,mirror),in=body_word(row+3*n,wb,c0,mirror),k=0;
            if(depth!=255){u8 tail;u32 v=cover_bytes(y+r+st.camy,b0,depth,3,&tail);
                k=(u16)(((v<<shift)|((u32)tail>>(8-shift)))&in);}
            if(flash){u16 f=r+1<ay?in:0;a=(a&~f)|(flash==1?f:0);b&=~f;}
            l[r]=a&~k;d[r]=b&~k;m[r]=mk|(r>=ay?k:0);
            if(wet&&r+1==rows){l[r]=(u16)~(mk|k);d[r]=0;m[r]=mk|k;}
        }
        draw_sprite(x+16*c0,y,&sp);
    }
    return ay;
}
