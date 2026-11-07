#ifndef FFT_AREA_IMPACT_H
#define FFT_AREA_IMPACT_H
// Only live during an owned area action, after AI candidate evaluation finishes.
typedef struct {
    u32 glyph[FFT_CORE_UNIT_MAX][7],oil[7];
    u8 width[FFT_CORE_UNIT_MAX],oil_width;
    u16 centre;                       // MAP_N means absent; tile255 remains valid
    u8 next,oil_ready;
    u16 ready;
} AreaImpact;
typedef char AreaImpactFitsGainScratch[(sizeof(AreaImpact)<=512)?1:-1];
#endif
