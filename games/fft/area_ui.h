#ifndef FFT_AREA_UI_H
#define FFT_AREA_UI_H
// Manual modal targeting owns this idle AI-distance storage, never a live cast.
#define AREA_UI_ROWS 4
typedef struct {
    CastBatch forecast;
    u8 ids[FFT_CORE_UNIT_MAX],count,row,top,answer,centre,follow_id;
    u8 panels[32];
    char line[AREA_UI_ROWS][28];
    u8 confirm,ready,dirty,forecast_dirty,weather;
} AreaUi;
typedef char AreaUiFitsAiScratch[(sizeof(AreaUi)<=256)?1:-1];
enum { AREA_INPUT,AREA_BEGIN,AREA_DRAW,AREA_HEADER,AREA_NUMBERS,AREA_RESULTS };
enum { AREA_NONE,AREA_QUERY,AREA_SEND,AREA_BACK };
#endif
