// Portable visible-byte hash: planes_sum intentionally reads host-native words.
static u32 impact_picture_hash(void)
{
    const u8 *l=rt_light,*d=rt_dark;u16 x,y;u32 h=17;
    for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){
        h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];
    }
    return h;
}
