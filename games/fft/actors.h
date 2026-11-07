// The common actor bank (ffca/ffcb, tools/common_actors.py) and its frame cache, shared by the
// battle engine (fft.c) and the scene player (campaign/scene.c). The includer defines
// ACTOR_SLOTS (actors drawn at once, + 2), ACTOR_UNITS (slots that may wait a frame for a new
// pose), ACTOR_KEY(logical) (its frame key: bank frame, bit 15 mirrored), and a W pointer
// whose struct holds ACTOR_FIELDS; actors.inc holds the functions.
#define ACTOR_FIELDS \
    u8 actor_budget, actor_misses, actor_next; \
    u16 actor_key[ACTOR_SLOTS], ac_now;  /* each slot's pose; frames drawn so far */ \
    u8 actor_ent[ACTOR_SLOTS];           /* its acache entry */
static const u8 *abank[2];              // the common actor frames (ffca, ffcb: common_actors.py)
static u16 abank_n0;                    // frames in ffca
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
