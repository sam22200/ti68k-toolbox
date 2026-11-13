// The cold part of chapter_load: the chapter's archived banks found and checked, its sizes
// and tables set (campaign/ENGINE.md). Resident on the PC; on the TI the SAVE image's op 6,
// where these names are the image's context (module.h) and the resident adopts them after
// the call. Returns the surface section's descriptor offset, 0: a missing or invalid bank.
#include "campaign/pack_format.h"
#include "view_format.h"
static const u8 *chapter_contours(const u8 *p,u16 size)
{
    const u8 *f;u16 off;
    size=resource_size(p,size);
    if(!p||(size&1)||size<CONTOUR_BYTES+10)return RT_NULL;
    f=p+size-10;off=RD16(f,3);
    return RD32(f,0)==0x46433032UL&&!(off&1)&&
        off+CONTOUR_BYTES==size-10&&RD16(f,4)==CONTOUR_BYTES?p+off:RT_NULL;
}
static char chapter_route(u8 encounter)
{
    u16 size;const u8 *pack=rt_file("ffpack",&size);
    if(pack)return fp_size(pack,size,0)?(char)fp_letter(pack,encounter):0;
    /* Old headless fixtures have no package metadata. Installed distributions
       require a pack, including when fftbat is invoked directly from HOME. */
    if(rt_file("ffcore",RT_NULL))return 0;
    return encounter<2?"og"[encounter]:0;
}
static u16 chapter_files(char letter)
{
    char name[6];                              // "ff?v0" (no initializer: a module has no memcpy)
    u16 i, view_size = 0, desc_size = 0, interface_size = 0,identity_size=0;
    pgfx=rt_file("ffcp",&identity_size);
    if(!identity_valid(pgfx,identity_size))return 0;
    name[0] = name[1] = 'f'; name[2] = letter; name[3] = 'v'; name[5] = 0;
    for (i = 0; i < 4; i++) {                  // read in place (archived on the TI): fetched
        name[4] = (char)('0' + i);             // again each load, the archive may have moved
        u16 size;const u8 *v=resource_file(name,&size);
        views[i]=v;if(!i)view_size=size;
        if(!v||size<9||v[0]!='R'||RD16(v,3)>FFT_CORE_DEPTH_MAX)return 0;
#ifndef FFT_LEGACY_REACH_BACKUPS
        if(!core_view_rows(v,size,SC_HMAX))return 0;
#else
        if(v[1]!='2'&&v[1]!='3')return 0;
#endif
    }
    /* Production stores these identical engine tables once in ffcu. Old
       extraction fixtures retain the same validated tail in their first view. */
    {u16 size;const u8 *shared=rt_file("ffcu",&size);
     contour_gfx=chapter_contours(shared,size);}
    if(!contour_gfx)contour_gfx=chapter_contours(views[0],view_size);
    name[4] = 0;
    name[3] = 'a'; ugfx = resource_file(name, RT_NULL);
    name[3] = 'd'; cdesc = resource_file(name, &desc_size);
    name[3] = 'i'; igfx = resource_file(name, &interface_size);
    if (!contour_gfx || !ugfx || !igfx || !cdesc) return 0;
    i=chapter_surface_offset(cdesc,desc_size,interface_size);
    if(!i)return 0;
    encounter_units = (u8)CD(UNIT_COUNT);
    map_w = (u8)CD(MAP_W); map_h = (u8)CD(MAP_H); map_n = (u16)map_w * map_h; sc_h = (s16)CD(SC_H); sc_top = (s16)CD(SC_TOP);
#ifndef FFT_LEGACY_REACH_BACKUPS
    {u16 rot;for(rot=0;rot<4;rot++)if(RD16(views[rot],1)!=2U*SC_BYTES*(u16)sc_h)return 0;}
#endif
    unit_defs = (const UnitDef *)(cdesc + CD(UNIT_DEFS));
    map_tiles = (const Tile *)(igfx + UI_TILES);
    return i;
}
