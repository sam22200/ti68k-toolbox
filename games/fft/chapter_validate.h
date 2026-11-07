#ifndef FFT_CHAPTER_VALIDATE_H
#define FFT_CHAPTER_VALIDATE_H
// Pure FCD3/FGS1/FGS2 span checks shared by portable loading and the cold SAVE image.
// Return the geometry section's descriptor offset, or zero before any binding.
// This preserves the existing supported-format checks; it is not map/scene v1.
#define FIELD(n) RD16(desc,CD_##n)
static u16 chapter_surface_offset(const u8 *desc,u16 desc_size,u16 interface_size)
{
    const u8 *surface;
    if(!desc||desc_size<CD_HEADER_BYTES||
       RD16(desc,0)!=0x4643||RD16(desc,1)!=0x4434||
       (u16)(FIELD(MAP_W)-1)>=MAP_WMAX||(u16)(FIELD(MAP_H)-1)>=MAP_HMAX||
       (u16)(FIELD(SC_H)-1)>=SC_HMAX||
       FIELD(MAP_W)*FIELD(MAP_H)>MAP_NMAX||
       FIELD(TILES)>interface_size||
       FIELD(MAP_W)*FIELD(MAP_H)*(u16)sizeof(Tile)>interface_size-FIELD(TILES)||
       !FIELD(UNIT_COUNT)||FIELD(UNIT_COUNT)>UNIT_CAPACITY||
       FIELD(UNIT_DEFS)<CD_HEADER_BYTES||(FIELD(UNIT_DEFS)&1)||
       FIELD(UNIT_DEFS)>desc_size||
       FIELD(UNIT_COUNT)*(u16)sizeof(UnitDef)>desc_size-FIELD(UNIT_DEFS)||
       FIELD(UNIT_REWARDS)<CD_HEADER_BYTES||FIELD(UNIT_REWARDS)>desc_size||
       FIELD(UNIT_COUNT)*2U>desc_size-FIELD(UNIT_REWARDS)||
       (FIELD(UNIT_RAW)&&(FIELD(UNIT_RAW)>interface_size||
           FIELD(UNIT_COUNT)*20U>interface_size-FIELD(UNIT_RAW)))||
       (FIELD(UNIT_NAMES)&&(FIELD(UNIT_NAMES)>interface_size||
           FIELD(UNIT_COUNT)*2U>interface_size-FIELD(UNIT_NAMES)))||
       (FIELD(UNIT_ZODIAC)&&(FIELD(UNIT_ZODIAC)>desc_size||
           FIELD(UNIT_COUNT)>desc_size-FIELD(UNIT_ZODIAC))))return 0;
    surface=surface_descriptor(desc,desc_size,(u8)FIELD(MAP_W),(u8)FIELD(MAP_H),CD_HEADER_BYTES);
    return surface?(u16)(surface-desc):0;
}
#undef FIELD
#endif
