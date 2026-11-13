#ifndef FFT_SAVE_TYPES_H
#define FFT_SAVE_TYPES_H
// Serialized save layout only; no file calls before module context bindings.
#include "../../../runtime/core/rt.h"
#define SAVE_MAGIC "FSV5"
#define SAVE_V4_BYTES 3202U
#define SAVE_V3_BYTES 3138U
#define SAVE_DISCOVERY_BYTES 64U
#define SAVE_FLAGS 4                    // byte offset of the story flags (fftstory reads them)
#define FLAG_N 256                      // story flags: bit k of flags[k >> 3]
#define PARTY_MAX 16                    // FFT's roster
#define SAVE_V2_BYTES 3110U
#define SAVE_FLOW_OFFSET 3114U
#define CAREER_N 20                     // FFT's generic jobs, Squire 0x4A .. Mime 0x5D: a unit's
                                        // JP and abilities per job (a special job: the Squire's)
typedef struct {
    u8 job, gfx, type, name;            // FFT job id; the battle's sprite (units.h UG_*, 0xFF: none
                                        // for its job); generation table (0 male, 1 female, 2
                                        // Ramza, 3 source monster); generic name index
    u8 level, exp, brave, faith, zodiac;
    u8 secondary;                       // the secondary skill set: FFT id, 0 none
    u8 raw[5][3];                       // FFT's raw stats HP MP SP PA MA (24 bits)
    u8 eq[5];                           // head/body/accessory/right/left item; left weapon or shield, 0 none
    u8 reaction[2], support[2], movement[2];   // the abilities equipped: FFT ids, 0 none
    u8 jp[CAREER_N][2], tjp[CAREER_N][2];   // per job: JP left, earned in all (its job level)
    u8 learned[CAREER_N][3];            // per job, FFT's bits: 0x800000 >> its list's k-th (the 16
                                        // actions, then the 6 reactions, supports, movements)
} SaveUnit;
typedef struct {
    u8 magic[4];
    u8 flags[FLAG_N / 8];               // story progress: a chapter's flag set by its victory
    u8 n, loc;                          // units in the party; the map location (FFT id + 1, 0 none)
    u8 inv[256];                        // items owned (bag and spare equipment), per FFT item id
    SaveUnit unit[PARTY_MAX];
    u8 money[4];                       // big-endian gil, source cap99,999,999
    u8 flow;                           // Academy1, deployment2, Gariland title3, complete4
    u8 deploy_n, deploy_battle[2];
    u8 deploy[5][4];                   // roster slot, world x/z, native facing
    u8 discovered[SAVE_DISCOVERY_BYTES]; //128 source maps, four once-only item locations each
    u8 raw_current[2];                  // party-slot bitmap: current raw stats instead of level-one seeds
} Save;
static u8 save_raw_current(const Save *s,u8 slot)
{return slot<PARTY_MAX&&(s->raw_current[slot>>3]&((u8)1<<(slot&7)))!=0;}
static void save_raw_set(Save *s,u8 slot,u8 current)
{
    u8 bit=(u8)1<<(slot&7);if(slot>=PARTY_MAX)return;
    if(current)s->raw_current[slot>>3]|=bit;else s->raw_current[slot>>3]&=(u8)~bit;
}
#endif
