// Reuse offline serif strips: 8px columns, light/dark/mask, at most16 rows.
// The advancing grey leading edge has no timer or storage of its own.
static void serif_reveal(const u8 *p,u8 columns,u8 height,s16 left,s16 top,u16 sweep)
{
    u8 col;
    for(col=0;col<columns;col++,p+=(u16)height*3) {
        s16 age=(s16)sweep-col*8;
        if(age>=20) {
            RtSprite sp={8,height,p,p+height,p+2*height};
            draw_sprite(left+col*8,top,&sp);
        } else if(age>=0) {
            u8 l[16],d[16],m[16],solid=0,mid=0,dim=0,x,r;
            for(x=0;x<8;x++) {
                s16 t=age-x;u8 bit=(u8)(0x80>>x);
                if(t>=12)solid|=bit;else if(t>=6)mid|=bit;else if(t>=0)dim|=bit;
            }
            for(r=0;r<height;r++) {
                u8 ink=(u8)~p[2*height+r];
                m[r]=(u8)~(ink&(solid|mid|dim));
                l[r]=(p[r]&solid)|(ink&mid);d[r]=(p[height+r]&solid)|(ink&dim);
            }
            {RtSprite sp={8,height,l,d,m};draw_sprite(left+col*8,top,&sp);}
        }
    }
}
