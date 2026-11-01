// Source casting resource/timing policy. No target lookup, mutation or RNG.
#ifndef FFT_CAST_RULES_H
#define FFT_CAST_RULES_H
#include "../../runtime/core/rt.h"
// Punch Art support 0x33..0x35: PA instead of MA, no Faith (CORE_PUNCH_ART.md).
static inline u8 punch_formula(u8 f){return (u8)(f-0x33)<=2;}
// Guts 0x39..0x3b: Speed/Brave/Scream gains that always hit (CORE_GUTS.md).
static inline u8 guts_formula(u8 f){return (u8)(f-0x39)<=2;}
// Talk Skill and Guts: the record's signed change, no Faith or support stage.
static inline u8 speech_formula(u8 f){return f==0x2a||guts_formula(f);}
// Formula sets as bitmaps over ids 0..103 (a few instructions in the resident and in every image).
// magic: 8..16, 18, 20, 97, Talk 0x2a, Punch Art 0x33..0x35, Guts 0x39..0x3b;
// status (the status batch commit): 10, 11, 18, 20, 97, 0x2a, 0x33, 0x39..0x3b.
static const u8 magic_formula_bits[13]={0x00,0xff,0x15,0x00,0x00,0x04,0x38,0x0e,0x00,0x00,0x00,0x00,0x02};
static const u8 magic_status_bits[13]={0x00,0x0c,0x14,0x00,0x00,0x04,0x08,0x0e,0x00,0x00,0x00,0x00,0x02};
static inline u8 magic_status_formula(u8 f){return f<104&&(magic_status_bits[f>>3]>>(f&7)&1);}
static inline u8 magic_percent_formula(u8 f){return f==9||(u8)(f-15)<=1;}
static inline u8 magic_transfer_formula(u8 f){return (u8)(f-15)<=1;}
static inline u8 magic_formula(u8 f){return f<104&&(magic_formula_bits[f>>3]>>(f&7)&1);}
enum { SUPPORT_HALF_MP=0x1ce, SUPPORT_SHORT_CHARGE=0x1e2,
       SUPPORT_NON_CHARGE=0x1e3, SOURCE_JOB_MIME=0x5d,
       SUPPORT_MAGIC_ATTACK_UP=0x1d3,SUPPORT_MAGIC_DEFENSE_UP=0x1d4,
       SUPPORT_ATTACK_UP=0x1d1,SUPPORT_MARTIAL_ARTS=0x1d8 };
static inline u8 core_cast_cost(u8 cost,u8 menu,u8 mime,u8 half_mp)
{
    if(menu||mime)return 0;
    return half_mp?cost>>1:cost;
}
static inline u8 core_cast_ticks(u8 ticks,u8 persevere,u8 short_charge,u8 non_charge)
{
    ticks&=0x7f;
    if(!persevere){
        if(short_charge)ticks=(ticks+1)>>1;
        if(non_charge)ticks=0;
    }
    return ticks;
}
#endif
