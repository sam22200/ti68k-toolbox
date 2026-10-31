// Source casting resource/timing policy. No target lookup, mutation or RNG.
#ifndef FFT_CAST_RULES_H
#define FFT_CAST_RULES_H
#include "../../runtime/core/rt.h"
static inline u8 magic_status_formula(u8 f){return (u8)(f-10)<=1||f==18||f==20||f==97||f==0x33||f==0x2a;}
// Punch Art support 0x33..0x35: PA instead of MA, no Faith (CORE_PUNCH_ART.md).
static inline u8 punch_formula(u8 f){return (u8)(f-0x33)<=2;}
static inline u8 magic_percent_formula(u8 f){return f==9||(u8)(f-15)<=1;}
static inline u8 magic_transfer_formula(u8 f){return (u8)(f-15)<=1;}
static inline u8 magic_formula(u8 f){return (u8)(f-8)<=8||f==18||f==20||f==97||f==0x2a||punch_formula(f);}
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
