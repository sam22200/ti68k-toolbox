/* Private pixel-identical pose-preparation study. No shipping actor path changes.
 * Uses the existing verified ZX0/ExtGraph routines; no new assembly or I/O.
 * Build with -DSTUDY_FIXTURES_FILE='"/absolute/private/fixtures.h"'. */
#ifdef __m68k__
#define USE_TI89
#define OPTIMIZE_ROM_CALLS
#include <tigcclib.h>
#include <extgraph.h>
#include "../../../lib/unpack68k.h"
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#include <string.h>
#endif

typedef unsigned char StudyByte;
typedef unsigned short StudyWord;
typedef struct { const StudyByte *packed, *raw; } StudyPayload;
typedef struct {
    StudyWord identity, frame;
    StudyPayload payload[6]; /* whole; head/body; head/torso/feet */
} StudyFixture;

#ifndef STUDY_FIXTURES_FILE
#error Supply private fixtures from actor_storage_study.py
#endif
#include STUDY_FIXTURES_FILE

/* The assembled208-byte temporary can reuse the existing actor mirror buffer.
 * This straightforward prototype also needs144 transient bytes for a part.
 * Three parts alone would need80, but both variants share the larger probe. */
static StudyWord assembled[104], output[104], fragment[72];
static StudyWord failures, signature;

static void unpack(const StudyPayload *p, StudyWord *dest, StudyWord bytes)
{
#ifdef __m68k__
    (void)bytes;
    zx0_asm(p->packed, (StudyByte *)dest);
#else
    memcpy(dest, p->raw, bytes);
#endif
}

static void insert_part(StudyWord *dest, const StudyPayload *p,
                        StudyWord first, StudyWord rows)
{
    const StudyWord *src;
    StudyWord plane;
    unpack(p, fragment, (StudyWord)(rows << 3));
    src = fragment;
    dest += first;
    for (plane = 0; plane < 4; plane++) {
        StudyWord n = rows, *row = dest;
        while (n--) *row++ = *src++;
        dest += 26;
    }
}

static StudyWord reverse_word(StudyWord v)
{
    v = (StudyWord)(((v >> 1) & 0x5555U) | ((v & 0x5555U) << 1));
    v = (StudyWord)(((v >> 2) & 0x3333U) | ((v & 0x3333U) << 2));
    v = (StudyWord)(((v >> 4) & 0x0f0fU) | ((v & 0x0f0fU) << 4));
    return (StudyWord)((v >> 8) | (v << 8));
}

static void mirror(const StudyWord *src, StudyWord *dest)
{
#ifdef __m68k__
    SpriteX8_MIRROR_H_R(104, (const unsigned char *)src, 2, (unsigned char *)dest);
#else
    /* Source payloads are big endian on both hosts. */
    const StudyByte *s = (const StudyByte *)src;
    StudyByte *d = (StudyByte *)dest;
    StudyWord n = 104;
    while (n--) {
        StudyWord v = reverse_word((StudyWord)((StudyWord)s[0] << 8 | s[1]));
        d[0] = (StudyByte)(v >> 8); d[1] = (StudyByte)v;
        s += 2; d += 2;
    }
#endif
}

static void prepare(const StudyFixture *p, StudyWord mode, StudyWord flipped)
{
    StudyWord *dest = flipped ? assembled : output;
    if (!mode) unpack(p->payload, dest, 208);
    else if (mode == 1) {
        insert_part(dest, p->payload + 1, 0, 8);
        insert_part(dest, p->payload + 2, 8, 18);
    } else {
        insert_part(dest, p->payload + 3, 0, 8);
        insert_part(dest, p->payload + 4, 8, 10);
        insert_part(dest, p->payload + 5, 18, 8);
    }
    if (flipped) mirror(assembled, output);
}

static void verify(const StudyFixture *p, StudyWord flipped)
{
    const StudyByte *expected = p->payload[0].raw;
    const StudyByte *actual = (const StudyByte *)output;
    StudyWord n = 104;
    while (n--) {
        StudyWord word = (StudyWord)((StudyWord)expected[0] << 8 | expected[1]);
        if (flipped) word = reverse_word(word);
        if (actual[0] != (StudyByte)(word >> 8) || actual[1] != (StudyByte)word)
            failures++;
        signature = (StudyWord)((signature << 1) | (signature >> 15));
        signature ^= word;
        expected += 2; actual += 2;
    }
}

static void run(void)
{
    StudyWord index, flipped, mode;
    failures = signature = 0;
#ifdef __m68k__
    BENCH_NAME(1, "Whole actor pose");
    BENCH_NAME(2, "Two actor parts");
    BENCH_NAME(3, "Three actor parts");
#endif
    for (index = 0; index < STUDY_FIXTURES; index++) {
        const StudyFixture *p = study_fixtures + index;
        for (flipped = 0; flipped < 2; flipped++) {
            for (mode = 0; mode < 3; mode++) {
#ifdef __m68k__
                BENCH_BEGIN(mode + 1);
#endif
                prepare(p, mode, flipped);
#ifdef __m68k__
                BENCH_END(mode + 1);
#endif
                verify(p, flipped);
            }
        }
    }
}

#ifdef __m68k__
void _main(void)
{
    run();
    BENCH_VALUE(STUDY_FIXTURES);
    BENCH_VALUE(failures);
    BENCH_VALUE(signature);
    BENCH_STOP();
}
#else
int main(void)
{
    run();
    printf("{\"fixtures\":%u,\"cases\":%u,\"failures\":%u,\"signature\":%u}\n",
           STUDY_FIXTURES, STUDY_FIXTURES*6U, failures, signature);
    return failures ? 1 : 0;
}
#endif
