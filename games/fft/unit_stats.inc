#ifndef FFT_UNIT_STATS_INC
#define FFT_UNIT_STATS_INC
// The campaign's one derivation of a saved unit's statistics, as the battle derives them
// (fft.c unit_stats): raw stats grown by level with the job's growth, then its multipliers,
// then what it wears. Management shows them on its Status page and the store's fitting bench
// shows what a piece of its stock would make of them.
// The includer defines:
//   UST_JOB(u)      -> the unit's 32-byte job record (ffmg section 0, or a monster's)
//   UST_ITEM(id)    -> an item's 14-byte record (ffmg section 1)
//   UST_CURRENT(u)  -> 1 when raw[] already holds current stats, 0 when they are level-one seeds
//   UST_MOVE(u,m,j) -> adds the unit's movement ability bonus; a caller without ffcj may skip it
#include "raw_stats.h"
typedef struct { u16 hp, mp; u8 sp, pa, ma, move, jump; } Stats;
#define UST_SLOTS 5
// job record fields, as management names them
#define UST_GROWTH 4
#define UST_MULT 9
#define UST_MOVE_F 14
#define UST_JUMP_F 15
#define UST_ITEM_HP 5                   // then MP PA MA Speed Move Jump

static Stats unit_stats(const SaveUnit *u)
{
    const u8 *j = UST_JOB(u);
    Stats s;
    u16 v16[5];
    u8 i, current = UST_CURRENT(u);
    for (i = 0; i < 5; i++) {
        u32 v = (u32)u->raw[i][0] << 16 | (u16)u->raw[i][1] << 8 | u->raw[i][2];
        if (!current) v = core_raw_grown(v, j[UST_GROWTH + i], u->level);
        v = v * j[UST_MULT + i] / 100 >> 14;
        v16[i] = v ? (u16)v : 1;
    }
    s.hp = v16[0]; s.mp = v16[1]; s.move = j[UST_MOVE_F]; s.jump = j[UST_JUMP_F];
    for (i = 0; i < UST_SLOTS; i++) if (u->eq[i]) {
        const u8 *it = UST_ITEM(u->eq[i]);
        s.hp += it[UST_ITEM_HP]; s.mp += it[UST_ITEM_HP + 1];
        v16[2] += it[UST_ITEM_HP + 4]; v16[3] += it[UST_ITEM_HP + 2]; v16[4] += it[UST_ITEM_HP + 3];
        s.move += it[UST_ITEM_HP + 5]; s.jump += it[UST_ITEM_HP + 6];
    }
    if (s.hp > 999) s.hp = 999;
    if (s.mp > 999) s.mp = 999;
    UST_MOVE(u, &s.move, &s.jump);
    if (s.jump > 7) s.jump = 7;
    s.sp = (u8)(v16[2] > 50 ? 50 : v16[2]);
    s.pa = (u8)(v16[3] > 99 ? 99 : v16[3]);
    s.ma = (u8)(v16[4] > 99 ? 99 : v16[4]);
    return s;
}
#endif
