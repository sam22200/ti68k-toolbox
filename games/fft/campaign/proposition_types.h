#ifndef FFT_PROPOSITION_TYPES_H
#define FFT_PROPOSITION_TYPES_H
// Byte-only durable service payload. No files, drawing or arithmetic helpers.
enum { PROP_N=96,PROP_JOBS=8,PROP_WORKERS=3,PROP_FIRST=2428 };
enum { PROP_SENT=1,PROP_RECALLED=2,PROP_RETURNED=4 };
enum { PROP_PROGRESS=2,PROP_COMPLETE=4,PROP_ATTEMPTED=8 };
enum { PROP_OK,PROP_LOCKED,PROP_UNAVAILABLE,PROP_FULL,PROP_TEAM,PROP_DAYS,PROP_GIL,PROP_CORRUPT };
typedef struct {u8 flags,id,elapsed,days,location,n,unit[PROP_WORKERS];} PropositionJob;
typedef struct {
    u8 n;
    PropositionJob job[PROP_JOBS];
    u8 status[PROP_N/2],retry[PROP_N];
} PropositionJobs;
enum { PROP_SUCCESS,PROP_FAILURE,PROP_BAD_FAILURE };
enum { PROP_NO_BONUS,PROP_TREASURE,PROP_LAND,PROP_RANDOM };
typedef struct {
    u8 ready,id,location,n,unit[3],result,kind,index,speaker;
    u8 career[3],jp[3][2],gil[4];
} PropositionReport;
typedef struct {
    PropositionJobs dispatches;
    u8 completed_date[108],treasure_date[35],land_date[18];
    PropositionReport report;
} PropositionState;
#endif
