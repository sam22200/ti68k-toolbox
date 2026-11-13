#ifndef FFT_CAMPAIGN_SAVE_H
#define FFT_CAMPAIGN_SAVE_H
// The campaign save: the session (ffsave) shared by every program. fftbat writes it after a
// victory, fftmng edits the party, fftmap moves on the world map, and fftstory reads where to
// resume. Nothing writes the player's slots ffsave1..3 but the title: Continue copies a slot
// into the session, Save (world map, a chapter's save prompt) copies the session into a slot,
// as in FFT; ffslot only remembers the last slot used. Bytes only, u16 big-endian:
// the same file on the PC and TI. Every id is
// FFT's own (jobs, items, abilities, skill sets), never a chapter's table index, so any chapter
// reads it.
#include "save_slots.h"
#include "save_types.h"
#ifndef SAVE_OFFSETS_ONLY
#include <string.h>
#define SAVE_NAME SAVE_SESSION_NAME
// FSV2 had no funds or deployment. Preserve its entire party/flags/inventory;
// initialize the newly introduced balance to the source's starting2000gil.
static u8 save_load_from(const char *name,Save *s)
{
    memset(s,0,sizeof *s);
    if(rt_load(name,s,sizeof *s)&&!memcmp(s->magic,SAVE_MAGIC,4))return s->n<=PARTY_MAX;
    memset(s,0,sizeof *s);
    if(rt_load(name,s,SAVE_V4_BYTES)&&!memcmp(s->magic,"FSV4",4)&&s->n<=PARTY_MAX){memcpy(s->magic,SAVE_MAGIC,4);return 1;}
    memset(s,0,sizeof *s);
    if(rt_load(name,s,SAVE_V3_BYTES)&&!memcmp(s->magic,"FSV3",4)&&s->n<=PARTY_MAX){memcpy(s->magic,SAVE_MAGIC,4);return 1;}
    memset(s,0,sizeof *s);
    if(rt_load(name,s,SAVE_V2_BYTES)&&!memcmp(s->magic,"FSV2",4)&&s->n<=PARTY_MAX) {
        memcpy(s->magic,SAVE_MAGIC,4);s->money[2]=7;s->money[3]=208;
        if(s->flags[0]&2)s->flow=4;
        return 1;
    }
    memset(s,0,sizeof *s);return 0;
}
static u8 save_load(Save *s){return save_load_from(SAVE_NAME,s);}
#else
#define SAVE_NAME SAVE_SESSION_NAME
#endif
#endif
