// Shared archive catalogue (tools/common_tables.py). Byte reads keep the same
// endian/layout on the PC and 68000; records remain in Flash, without a RAM copy.
#ifndef FFT_COMMON_H
#define FFT_COMMON_H
#include "../../runtime/core/rt.h"
#include <string.h>

enum { CJ_JOBS, CJ_JOB_NAMES, CJ_JOB_SETS, CJ_ITEMS, CJ_PRIMARY, CJ_RANGE,
       CJ_ITEM_FORMULA, CJ_SPECIAL, CJ_HUMAN, CJ_MONSTER, CJ_ABILITY_NAMES,
       CJ_SET_NAMES, CJ_STATUS, CJ_BATTLE, CJ_SECTIONS };
enum { CJ_LENGTH = 4 + 2 * CJ_SECTIONS, CJ_HEADER = CJ_LENGTH + 2, CJ_ABILITIES = 512, CJ_DEFAULT = 0x170,
       CJ_HUMANS = 0xB0, CJ_SETS = 0xE0, CJ_NAMED_SETS = 64 };
enum { CJ_AFFINITY = 168 + CJ_SETS + 20, CJ_AFFINITY_INDEX = 6,
       CJ_AFFINITY_ROWS = CJ_AFFINITY_INDEX + 160 + 256 };

static inline u16 cj_word(const u8 *p) { return (u16)((u16)p[0] << 8 | p[1]); }
static inline const u8 *cj_section(const u8 *b, u8 section)
{
    return b + cj_word(b + 4 + (u16)section * 2);
}

static inline u8 cj_valid(const u8 *b, u16 size)
{
    static const u16 minimum[CJ_SECTIONS] = {
        160 * 14, 160 * 14, 160, 256 * 30, 512 * 8, 0x170 * 14,
        14 * 3, CJ_AFFINITY + CJ_AFFINITY_ROWS + 5, 0xB0 * 25, 48 * 5, 512 * 2, 64 * 2, 128 * 6 + 40 * 16 + 10, 512 * 12 + 8 + 20
    };
    u16 end, previous = CJ_HEADER;
    u8 k;
    if (!b || size < CJ_HEADER || memcmp(b, "FCJA", 4)) return 0;
    end = cj_word(b + CJ_LENGTH);          // excludes AMS's optional six-byte OTH trailer
    if (end > size || end < CJ_HEADER) return 0;
    for (k = 0; k < CJ_SECTIONS; k++) {
        u16 at = cj_word(b + 4 + 2 * k);
        u16 next = k + 1 == CJ_SECTIONS ? end : cj_word(b + 6 + 2 * k);
        if (at < previous || at > end || next < at || next > end || next - at < minimum[k]) return 0;
        previous = next;
    }
    {
        const u8 *p=cj_section(b,CJ_SPECIAL)+CJ_AFFINITY;
        u16 n=cj_word(b+4+2*(CJ_SPECIAL+1))-cj_word(b+4+2*CJ_SPECIAL)-CJ_AFFINITY,i;
        if(memcmp(p,"FEA1",4)||!p[4]||p[5]||n<CJ_AFFINITY_ROWS+(u16)p[4]*5)return 0;
        for(i=0;i<416;i++)if(p[CJ_AFFINITY_INDEX+i]>=p[4])return 0;
        for(i=0;i<5;i++)if(p[CJ_AFFINITY_ROWS+i])return 0;
    }
    return 1;
}

// Cold rebuild: job base masks, then OR equipment. ID0 skips neutral items.
// Callers supply validated source job IDs and ordinary-human equipment slots.
static inline void cj_affinity(const u8 *b,u8 job,const u8 *eq,u8 *out)
{
    const u8 *p=cj_section(b,CJ_SPECIAL)+CJ_AFFINITY,*rows=p+CJ_AFFINITY_ROWS,*r;
    u8 i,k,id=p[CJ_AFFINITY_INDEX+job];
    memcpy(out,rows+(u16)id*5,5);
    for(i=0;i<5;i++)if(eq[i]!=255&&(id=p[CJ_AFFINITY_INDEX+160+eq[i]])!=0){
        r=rows+(u16)id*5;
        for(k=0;k<5;k++)out[k]|=r[k];
    }
}

static inline const u8 *cj_primary(const u8 *b, u16 id)
{
    return id < CJ_ABILITIES ? cj_section(b, CJ_PRIMARY) + (id << 3) : RT_NULL;
}

static inline const u8 *cj_range(const u8 *b, u16 id)
{
    return id < CJ_DEFAULT ? cj_section(b, CJ_RANGE) + id * 14U : RT_NULL;
}

static inline u8 cj_action_menu(const u8 *b,u8 set)
{
    return set<CJ_SETS?cj_section(b,CJ_SPECIAL)[168U+set]:255;
}
static inline const u8 *cj_status_rules(const u8 *b)
{
    return cj_section(b,CJ_STATUS)+128U*6;
}
enum { HAND_DUAL = 1, ITEM_MAGIC_SHIELD=128, ITEM_MAGIC_SHIFT=1, ITEM_MAGIC_MASK=63, SUPPORT_TWO_SWORDS = 0x1dd };
static inline u8 cj_two_swords(const u8 *b,u8 job,u16 support)
{
    return support==SUPPORT_TWO_SWORDS||(job<160&&
        (cj_section(b,CJ_SPECIAL)[168U+CJ_SETS+(job>>3)]&(1U<<(job&7))));
}

// Positions 0..15 are actions; 16..21 are R/S/M, matching the save's learned
// bits. Monster sets have four entries. ID zero is an empty entry, never a sentinel
// for the end of the list: holes and actions above 255 are preserved.
static inline u16 cj_set_ability(const u8 *b, u8 set, u8 pos)
{
    const u8 *p;
    if (set < CJ_HUMANS) {
        if (pos >= 22) return 0;
        p = cj_section(b, CJ_HUMAN) + (u16)set * 25;
        return (u16)(p[3 + pos] | (p[pos >> 3] & (0x80 >> (pos & 7)) ? 0x100 : 0));
    }
    if (set >= CJ_SETS || pos >= 4) return 0;
    p = cj_section(b, CJ_MONSTER) + (u16)(set - CJ_HUMANS) * 5;
    return (u16)(p[1 + pos] | (p[0] & (0x80 >> pos) ? 0x100 : 0));
}

static inline const char *cj_name(const u8 *b, u8 section, u16 id, u16 count)
{
    const u8 *p;
    u16 at;
    if (id >= count) return "";
    p = cj_section(b, section);
    at = cj_word(p + (id << 1));
    return at && at < cj_word(b + CJ_LENGTH) ? (const char *)b + at : "";
}
static inline const char *cj_ability_name(const u8 *b, u16 id)
{
    return cj_name(b, CJ_ABILITY_NAMES, id, CJ_ABILITIES);
}
static inline const char *cj_set_name(const u8 *b, u8 id)
{
    return cj_name(b, CJ_SET_NAMES, id, CJ_NAMED_SETS);
}
static inline const char *cj_battle_name(const u8 *b, u16 id)
{
    // Preserve the reviewed battle spacing; management retains FFT's source labels.
    if (id == 0x196 || id == 0x197)
        return (const char *)cj_section(b, CJ_BATTLE) + 512 * 12 + 8 + (id - 0x196) * 10;
    return cj_ability_name(b, id);
}
#endif
