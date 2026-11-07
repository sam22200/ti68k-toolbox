// Cold coordinate-cache setup, identical on either side of the image boundary.
static void grid_tables(void)
{
    u8 x,z,*col=W->col,*row=W->row;u16 i=0,offset=0;
    W->edge_phase=0;
    for(x=0;x<MAP_W;x++,col+=28)memset(col,x,28);
    for(z=0;z<MAP_H;z++,row+=28) {
        W->grid_row[z]=(u8)offset;memset(row,(u8)offset,28);offset+=MAP_W;
        for(x=0;x<MAP_W;x++){W->tx[i]=x;W->tz[i++]=z;}
    }
}
