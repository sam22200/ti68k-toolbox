// How big a single heap block AMS will give, and whether code runs past 64 KB
// inside it.
//
// Why: FFT's chapter executables are packed with TTPack, whose input ceiling is
// 64,936 bytes, and the battle program is at it. Replacing TTPack with our own
// ZX0 self-extractor only helps if ONE contiguous block can hold a bigger
// unpacked image, and if the 68000 reaches code above the 64 KB mark inside it.
// experiments/heapcode already showed that a heap block executes at all, with
// HW3Patch on a Titanium; this measures the size instead.
//
// Values go to the tilog ring, so `ti-mem log` reads them without a screenshot.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "../../lib/tilog.h"

// moveq #42,d0 ; rts - the same two instructions experiments/heapcode used.
static const unsigned char STUB[4] = { 0x70, 0x2A, 0x4E, 0x75 };

static void *try_block(unsigned long n)
{
    void *p = HeapAllocPtr(n);
    return p;                      // caller frees; a probe must leave no trace
}

// Writes the stub at `off` inside the block and calls it; 42 means it ran.
static short run_at(unsigned char *blk, unsigned long delta)
{
    short (*fn)(void);
    memcpy(blk + delta, STUB, sizeof STUB);
    fn = (short (*)(void))(blk + delta);
    return fn();
}

void _main(void)
{
    static const unsigned long ladder[] = {
        32768UL, 60000UL, 65000UL, 65520UL, 65535UL, 65536UL, 66000UL,
        70000UL, 80000UL, 100000UL, 120000UL, 160000UL, 0UL };
    unsigned long avail = (unsigned long)HeapAvail();
    unsigned long best = 0, lo, hi, mid;
    unsigned char *p;
    short i, line = 0;

    ClrScr();
    FontSetSys(F_6x8);
    printf_xy(0, line, "heap free: %lu", avail); line += 9;
    tilog("heapbig avail=%lu", avail);

    for (i = 0; ladder[i]; i++) {
        void *q = try_block(ladder[i]);
        tilog("probe %lu -> %s", ladder[i], q ? "ok" : "refused");
        if (q) { best = ladder[i]; HeapFreePtr(q); }
        else break;
    }
    printf_xy(0, line, "ladder best: %lu", best); line += 9;

    // The exact ceiling, between the last success and the first refusal.
    lo = best;
    hi = ladder[i] ? ladder[i] : best;
    while (hi - lo > 1) {
        void *q;
        mid = lo + (hi - lo) / 2;
        q = try_block(mid);
        if (q) { lo = mid; HeapFreePtr(q); }
        else hi = mid;
    }
    printf_xy(0, line, "largest block: %lu", lo); line += 9;
    tilog("heapbig largest_block=%lu", lo);

    // Does code run from it, and does it still run past the 64 KB mark?
    p = (unsigned char *)HeapAllocPtr(lo);
    if (!p) {
        printf_xy(0, line, "could not reallocate"); line += 9;
        tilog("heapbig realloc failed");
    } else {
        short at0 = run_at(p, 0);
        printf_xy(0, line, "call at +0      : %d", at0); line += 9;
        tilog("heapbig call_at_0=%d addr=%lu", at0, (unsigned long)p);
        if (lo > 65536UL + 16UL) {
            short hiq = run_at(p, 65536UL);
            printf_xy(0, line, "call at +65536  : %d", hiq); line += 9;
            tilog("heapbig call_at_65536=%d", hiq);
        } else {
            printf_xy(0, line, "no room past 64K"); line += 9;
            tilog("heapbig no room past 64K");
        }
        HeapFreePtr(p);
    }
    printf_xy(0, line, "HW_VERSION %u", (unsigned)HW_VERSION);
    tilog("heapbig hw=%u done", (unsigned)HW_VERSION);
    ngetchx();
}
