// What a data plan may actually use on THIS calculator: the AMS archive survey.
// The 2.7 MB quoted by TI is a capacity, not free space; EM_survey (flash.h) reports
// the real figures. Available for new archived variables = free + freedByGC.
// Values also go to the tilog ring, so `ti-mem log` reads them without a screenshot.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "../../lib/tilog.h"

void _main(void)
{
    unsigned long inuse = 0, gc = 0, avail = 0, unused = 0, bad = 0, all = 0;

    EM_survey(&inuse, &gc, &avail, &unused, &bad, &all);
    ClrScr();
    FontSetSys(F_6x8);
    printf_xy(0,  0, "archive in use : %lu", inuse);
    printf_xy(0,  9, "free           : %lu", avail);
    printf_xy(0, 18, "freed by GC    : %lu", gc);
    printf_xy(0, 27, "AVAILABLE      : %lu", avail + gc);
    printf_xy(0, 36, "unused sectors : %lu", unused);
    printf_xy(0, 45, "bad sectors    : %lu", bad);
    printf_xy(0, 54, "not TIOS       : %lu", all);
    printf_xy(0, 63, "RAM free       : %lu", (unsigned long)HeapAvail());
    tilog("flash inuse=%lu free=%lu gc=%lu avail=%lu", inuse, avail, gc, avail + gc);
    tilog("flash unused=%lu bad=%lu nottios=%lu ram=%lu",
          unused, bad, all, (unsigned long)HeapAvail());
    ngetchx();
}
