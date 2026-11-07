// Canonical PS1 status IDs/bitmaps. This stores policy, not every status's gameplay.
#ifndef FFT_STATUS_POLICY_H
#define FFT_STATUS_POLICY_H
enum { STATUS_BYTES=5, STATUS_COUNT=40, STATUS_ROW=16,
       STATUS_RIDER=1, STATUS_IMMORTAL=2, STATUS_CONTEXT=3,
       STATUS_CT_FROZEN=4, STATUS_NO_MOVE=8, STATUS_NO_ACT=16, STATUS_NO_EVADE=32, STATUS_NO_CONTROL=64, STATUS_AIRBORNE=128,
       STATUS_REJECTED=0xffffU };
enum { SID_CRYSTAL=1,SID_DEAD=2,SID_UNDEAD=3,SID_CHARGING=4,SID_JUMP=5,
       SID_DEFEND=6,SID_PERFORM=7,SID_PETRIFY=8,SID_INVITE=9,SID_DARK=10,SID_CONFUSION=11,SID_SILENCE=12,SID_BERSERK=20,
       SID_FLOAT=17,SID_TRANSPARENT=19,SID_CHICKEN=21,SID_FROG=22,SID_POISON=24,SID_REGEN=25,SID_PROTECT=26,SID_SHELL=27,SID_HASTE=28,SID_SLOW=29,
       SID_FAITH=32,SID_INNOCENT=33,
       SID_STOP=30,SID_CHARM=34,SID_SLEEP=35,SID_DONT_MOVE=36,SID_DONT_ACT=37,SID_REFLECT=38,SID_SENTENCE=39 };
typedef struct { u8 inflicted[5],innate[5],immune[5]; } CoreStatus;
typedef struct { u8 add[5],remove[5]; } CoreStatusDelta;
typedef char CoreStatusSize[(sizeof(CoreStatus)==15)?1:-1];
typedef char CoreStatusDeltaSize[(sizeof(CoreStatusDelta)==10)?1:-1];
static inline u8 core_status_current(const CoreStatus *s,u8 byte)
{
    return s->inflicted[byte]|s->innate[byte];
}
static inline u8 core_status_removable(const CoreStatus *s,u8 byte)
{
    return s->inflicted[byte]&(u8)~s->innate[byte];
}
// The first four native flags remain a small derived hot-path cache.
static inline u8 core_status_compact_bytes(u8 b0,u8 b1,u8 b3)
{
    return (u8)(((b0&2)>>1)|((b3&128)>>6)|((b1&32)>>3)|(b1&8));
}
static inline u8 core_status_compact(const CoreStatus *s)
{
    return core_status_compact_bytes(core_status_current(s,0),core_status_current(s,1),core_status_current(s,3));
}
static inline u8 core_status_curable(const CoreStatus *s)
{
    return core_status_compact_bytes(core_status_removable(s,0),
        core_status_removable(s,1),core_status_removable(s,3));
}
static inline void core_status_compact_mask(u8 *out,u8 compact)
{
    out[0]=(u8)((compact&1)<<1);out[1]=(u8)(((compact&4)<<3)|(compact&8));
    out[2]=out[4]=0;out[3]=(u8)((compact&2)<<6);
}
#endif
