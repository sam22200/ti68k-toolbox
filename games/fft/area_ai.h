#ifndef FFT_AREA_AI_H
#define FFT_AREA_AI_H
// Lives in the focus-distance union only after setup/search has completed.
typedef struct {
    s16 gain[FFT_CORE_UNIT_MAX];
    u8 hit[FFT_CORE_UNIT_MAX];
    u16 useful;
    u8 open[32];
    u16 next,origin;
    u8 flags,self_kind,x,z,layer;
} AreaAi;
typedef char AreaAiFitsIdleDistance[(sizeof(AreaAi)<=256)?1:-1];
#endif
