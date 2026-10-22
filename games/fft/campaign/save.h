// The campaign save (campaign/ENGINE.md, core milestone): one variable every program of the
// campaign shares (fftbat writes it after a victory, fftstory reads where to resume; the world
// map and the management menus next). Bytes only, u16 big-endian: the same file on the PC and
// the TI. Every id is FFT's own (jobs, items, abilities), never a chapter's table index, so any
// chapter reads it.
#define SAVE_NAME "ffsave"
#define SAVE_MAGIC "FSV1"
#define SAVE_FLAGS 4                    // byte offset of the story flags (fftstory reads them)
#define FLAG_N 256                      // story flags: bit k of flags[k >> 3]
#define PARTY_MAX 16                    // FFT's roster
#define CAREER_N 20                     // FFT's generic jobs, Squire 0x4A .. Mime 0x5D: a unit's
                                        // JP and abilities per job (special jobs: the Squire's)
#ifndef SAVE_OFFSETS_ONLY
typedef struct {
    u8 job, gfx, type, name;            // FFT job id; sprite (units.h UG_*); generation table
                                        // (0 male, 1 female, 2 Ramza); generic name index
    u8 level, exp, brave, faith, zodiac;
    u8 raw[5][3];                       // FFT's raw stats HP MP SP PA MA (24 bits)
    u8 eq[5];                           // head, body, accessory, weapon, shield: FFT item ids, 0 none
    u8 support[2];                      // the support ability equipped: FFT id, 0 none
    u8 jp[CAREER_N][2], tjp[CAREER_N][2];   // per job: JP left, earned in all (its job level)
    u8 learned[CAREER_N][3];            // per job, FFT's bits: 0x800000 >> its list's k-th
    u8 pad;
} SaveUnit;
typedef struct {
    u8 magic[4];
    u8 flags[FLAG_N / 8];               // story progress: a chapter's flag set by its victory
    u8 n, pad;                          // units in the party
    u8 inv[256];                        // items owned (bag and spare equipment), per FFT item id
    SaveUnit unit[PARTY_MAX];
} Save;
#endif
