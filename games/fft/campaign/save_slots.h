#ifndef FFT_SAVE_SLOTS_H
#define FFT_SAVE_SLOTS_H
#define SAVE_LEGACY_NAME "ffsave"
// The session: the working copy every program reads and writes (party, story flags, flow,
// deployment, rewards). The player's slots change only through the title's Save.
#define SAVE_SESSION_NAME SAVE_LEGACY_NAME
#define SAVE_SELECT_NAME "ffslot"
#define SAVE_SELECT_MAGIC "FSL1"
#define SAVE_SELECT_BYTES 6
#define SAVE_SLOT_COUNT 3
static const char *save_slot_name(unsigned char slot)
{
    static const char names[3][8]={"ffsave1","ffsave2","ffsave3"};
    return slot>=1&&slot<=3?names[slot-1]:SAVE_LEGACY_NAME;
}
#endif
