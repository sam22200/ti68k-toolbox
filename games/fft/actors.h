// The common actor bank (ffca/ffcb, tools/common_actors.py) and its frame cache, shared by the
// battle engine (fft.c) and the scene player (campaign/scene.c). The includer defines
// ACTOR_SLOTS (actors drawn at once, + 2), ACTOR_UNITS (slots that may wait a frame for a new
// pose), ACTOR_KEY(logical) (its frame key: bank frame, bit 15 mirrored), and a W pointer
// whose struct holds ACTOR_FIELDS; actors.inc holds the functions.
#define ACTOR_FIELDS \
    u8 actor_budget, actor_misses, actor_next; \
    u16 actor_key[ACTOR_SLOTS], ac_now;  /* each slot's pose; frames drawn so far */ \
    u8 actor_ent[ACTOR_SLOTS];           /* its acache entry */
// The chapter's actor frames (tools/common_actors.py): ffca, ffcb (both required), then ffcc..
// while present, each at most one archive variable; frames are numbered across them in order.
#define ACTOR_BANKS 6                   // ffca..ffcf: ~390 KB of frames
static const u8 *abank[ACTOR_BANKS + 1];   // then NULL: the frame lookup stops there
// The banks found in the archive into b (the missing ones and the one after: NULL); 1: ffca and
// ffcb are there. (A module builds the name without an initializer: it has no memcpy.)
static u8 actor_banks(const u8 **b)
{
    char name[5];
    u8 k;
    name[0] = name[1] = 'f'; name[2] = 'c'; name[4] = 0;
    for (k = 0; k < ACTOR_BANKS; k++) {
        name[3] = (char)('a' + k);
        b[k] = k && !b[k - 1] ? RT_NULL : rt_file(name, RT_NULL);
    }
    b[ACTOR_BANKS] = RT_NULL;
    return b[0] && b[1];
}
// Decoded frames shared by every actor: 48 hold Gariland's AI battle at 0.06 decodes
// a frame (1.8 with one pose per actor). Its own block: Gariland's Work + planes
// block (57,006 bytes) cannot grow by 7 KB under AMS's ~64 KB block limit.
#ifndef AC_N
#define AC_N 48
#endif
static u16 (*acache)[UNIT_FB / 2];      // AC_N frames, then ac_key and ac_age
static u16 *ac_key, *ac_age;            // frame key (0xFFFF: free), last frame drawn
                                        // (in the block: -mno-bss globals cost program bytes)
#ifdef __m68k__                         // ExtGraph (verified, ti68k-c-patterns.md: sprites)
void SpriteX8_MIRROR_H_R(unsigned short height asm("%d0"), const unsigned char *src asm("%a0"),
                         unsigned short bytewidth asm("%d1"), unsigned char *dest asm("%a1"))
                         __attribute__((__regparm__(4)));
#endif
static void actor_reset(void);
