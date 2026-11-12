// Source casting resource/timing policy. No target lookup, mutation or RNG.
#ifndef FFT_CAST_RULES_H
#define FFT_CAST_RULES_H
#include "../../runtime/core/rt.h"
static inline u8 performance_formula(u8 f){return (u8)(f-0x1c)<=1;}
static inline u8 draw_out_formula(u8 f){return (u8)(f-0x20)<=3;}
// Speed/PA/MA losses by the status commit: Dances, and the Ruins (0x1A). PA by ability id
// (Polka 89, Last Dance 96, Power Ruin 0xC6, Beaking 0x135), else MA; AB_SPEED is Speed.
static inline u8 stat_loss_formula(u8 f){return performance_formula(f)||f==0x1A||f==0x55;}
static inline u8 stat_loss_pa(u16 ab){return ab==89||ab==96||ab==0xC6||ab==0x135;}
// Punch Art support 0x33..0x35: PA instead of MA, no Faith (CORE_PUNCH_ART.md).
static inline u8 punch_formula(u8 f){return (u8)(f-0x33)<=2;}
// Guts 0x39..0x3b: Speed/Brave/Scream gains that always hit (CORE_GUTS.md).
static inline u8 guts_formula(u8 f){return (u8)(f-0x39)<=2;}
// Talk Skill and Guts: the record's signed change, no Faith or support stage.
static inline u8 speech_formula(u8 f){return (u8)(f-0x29)<=1||guts_formula(f);}
// Formula sets as bitmaps over ids 0..103 (a few instructions in the resident and in every image).
// magic: 8..16, 18, 20, 97, Geomancy0x24, Talk0x2a, Punch Art0x33..0x35, Guts0x39..0x3b,
// 0x4E (formula08 without Faith: magic_amount's power bit7), Holy Sword 0x2D (physical, weapon element),
// Mighty Sword 0x2E (Holy Sword's strike plus an equipment break), Dark Sword 0x2F (physical
// PA x WP, MP drained to the caster);
// Secret Fist0x50 uses the shared status commit and a physical-status accuracy;
// Ruins 0x1A (stat loss, the status commit) and 0x1B (max-MP percent) (campaign/RUINS.md).
// Monster 0x3D (Mind Blast, Death Sentence): formula 10's (MA+X)% without Faith, a random-one set;
// Beaking 0x55: the same accuracy after magical evasion, PA - Y by the status commit;
// Look of Fright 0x62: Beaking's evasion and accuracy, Foxbird's (97) Brave - Y.
// Goblin Punch 0x4F (0x50's evade and accuracy) and Self Destruct 0x52 (always, Oil at 100%):
// damage = the caster's missing HP; Self Destruct's caster takes its own HP.
// Core id 101 (FORMULA_AREA_CLEANSE): source 0x38 on the batch (Odd Soundwave: a cancel set, always
// applied, no draw); the item cleanses keep 0x38 outside the magic pipeline.
// status (the status batch commit): 10, 11, 18, 20, 0x1A, 97, 101, 0x2a, 0x33, 0x39..0x3b, 0x3D, 0x50, 0x55, 0x62.
static const u8 magic_formula_bits[13]={0x00,0xff,0x15,0x3c,0x1f,0xe6,0x38,0x2e,0x00,0xd0,0x27,0x00,0x2e};
static const u8 magic_status_bits[13]={0x00,0x0c,0x14,0x04,0x04,0x06,0x08,0x2e,0x00,0x00,0x23,0x00,0x26};
static inline u8 magic_status_formula(u8 f){return f<104&&(magic_status_bits[f>>3]>>(f&7)&1);}
static inline u8 magic_percent_formula(u8 f){return f==9||(u8)(f-15)<=1;}
static inline u8 magic_transfer_formula(u8 f){return (u8)(f-15)<=1||f==0x2F;}
static inline u8 magic_formula(u8 f){return f<104&&(magic_formula_bits[f>>3]>>(f&7)&1);}
enum { FORMULA_AREA_CLEANSE=101 };   // not 100: Jump's core id (jump_rules.h JUMP_FORMULA)
// A raw record's formula as the batch knows it.
static inline u8 batch_formula(u8 raw){return raw==0x38?FORMULA_AREA_CLEANSE:raw;}
// Formula08's damage path; 0x4E only drops Faith (magic_amount).
static inline u8 magic_damage_formula(u8 f){return f==8||f==0x4E;}
enum { SUPPORT_HALF_MP=0x1ce, SUPPORT_SHORT_CHARGE=0x1e2,
       SUPPORT_NON_CHARGE=0x1e3, SOURCE_JOB_MIME=0x5d,
       SUPPORT_MAGIC_ATTACK_UP=0x1d3,SUPPORT_MAGIC_DEFENSE_UP=0x1d4,
       SUPPORT_ATTACK_UP=0x1d1,SUPPORT_DEFENSE_UP=0x1d2,SUPPORT_MARTIAL_ARTS=0x1d8,SUPPORT_MAINTENANCE=0x1db };
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
