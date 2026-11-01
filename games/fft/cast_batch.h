// Effect-stage storage. Work owns one live packet; isolated callers may own one.
#ifndef FFT_CAST_BATCH_H
#define FFT_CAST_BATCH_H
#include "cast_rules.h"
typedef struct { u16 amount;u8 hit,effect; } CastVictim;
typedef struct {
    AbId ability;u16 mask;
    u8 actor,ref,follow,phase;
    CastVictim victim[FFT_CORE_UNIT_MAX];
} CastBatch;
// FORECAST hit is a percentage; READY/APPLIED hit is an execution boolean.
enum { CAST_BATCH_EMPTY,CAST_BATCH_READY,CAST_BATCH_APPLIED,CAST_BATCH_FORECAST };
enum { CAST_BATCH_PREPARE,CAST_BATCH_COMMIT };
enum { CAST_AREA_DISPATCH=3 }; // internal COMMAND -> resident -> TARGETS handoff
typedef char CastVictimSize[(sizeof(CastVictim)==4)?1:-1];
// Ordinary damage/healing/resurrection/status families. Other producers stay gated.
static u8 cast_area_supported(const Ability *b,const u8 *r)
{
    return r&&(magic_formula(b->formula)||b->formula==0x31)&&
        (speech_formula(b->formula)||b->kind==(magic_status_formula(b->formula)?(b->formula==18?AB_QUICK:b->formula==97?AB_BRAVE:b->formula==20?AB_GOLEM:b->kind==AB_CLEANSE?AB_CLEANSE:AB_STATUS):b->formula==12?AB_HEAL:b->formula==13||b->formula==0x35?AB_REVIVE:b->formula==0x34?AB_HP_MP_HEAL:b->formula==15?AB_MP_DAMAGE:b->formula==14?AB_DEATH:AB_DAMAGE))&&
        (b->flags&(AF_CAST|AF_AREA))==(AF_CAST|AF_AREA)&&
        !(b->flags&~(AF_CAST|AF_AREA|AF_SELF|AF_EVADE))&&
        r[8]==b->formula&&!(r[3]&32)&&!(r[4]&14)&&!(r[5]&4)&&!(r[6]&32);
}
#endif
