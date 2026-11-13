#ifndef FFT_ARCHIVE_ART_H
#define FFT_ARCHIVE_ART_H
// Shared engine-art tail, present once in shipping ffcu. Extraction controls
// may still attach the same bytes to a first view; never copy them into code.
static const u8 *archive_art_tail(const u8 *p,u16 size)
{
    const u8 *f;u16 off;
    size=resource_size(p,size);
    if(!p||(size&1)||size<CONTOUR_BYTES+10)return RT_NULL;
    f=p+size-10;off=RD16(f,3);
    return RD32(f,0)==0x46433032UL&&!(off&1)&&
        off+CONTOUR_BYTES==size-10&&RD16(f,4)==CONTOUR_BYTES?p+off:RT_NULL;
}
#endif
