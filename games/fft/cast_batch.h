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
enum { CAST_AREA_DISPATCH=3,CAST_EFFECT_DISPATCH=6 }; // internal COMMAND -> resident -> TARGETS handoff
typedef char CastVictimSize[(sizeof(CastVictim)==4)?1:-1];
// Ordinary damage/healing/resurrection/status families. Other producers stay gated.
#if defined(FFT_MODULE) && MOD_PART(MOD_COMMAND)
#define CAST_PERFORMANCE(f) 0 // resident routes persistent orders to PERFORMANCE, never COMMAND
#else
#define CAST_PERFORMANCE(f) performance_formula(f)
#endif
static u8 cast_area_supported(const Ability *b,const u8 *r)
{
    return r&&(magic_formula(b->formula)||b->formula==0x31)&&
        (CAST_PERFORMANCE(b->formula)||speech_formula(b->formula)||((b->formula==0x1A||b->formula==0x55)&&(b->kind==AB_SPEED||b->kind==AB_BREAK))||b->kind==(magic_status_formula(b->formula)?(b->formula==18?AB_QUICK:(u8)(b->formula-97)<=1?AB_BRAVE:b->formula==20?AB_GOLEM:b->kind==AB_CLEANSE?AB_CLEANSE:AB_STATUS):b->formula==12||b->formula==76||b->formula==0x23?AB_HEAL:b->formula==13||b->formula==0x35?AB_REVIVE:b->formula==0x34?AB_HP_MP_HEAL:b->formula==15||b->formula==0x1B||b->formula==0x21||b->formula==0x2F?AB_MP_DAMAGE:b->formula==14?AB_DEATH:AB_DAMAGE))&&
        (b->flags&(AF_CAST|AF_AREA))==(AF_CAST|AF_AREA)&&
        !(b->flags&~(AF_CAST|AF_AREA|AF_SELF|AF_EVADE|((b->formula|2)==0x2F?AF_WEAPON:0)))&&   // Holy/Dark Sword: a physical strike
        batch_formula(r[8])==b->formula&&!(r[3]&32)&&!(r[4]&10)&&(!(r[4]&4)||b->formula==0x31||b->formula==0x20||b->formula==0x2D||(r[6]&32))&&(!(r[5]&4)||CAST_PERFORMANCE(b->formula));
}
#endif
