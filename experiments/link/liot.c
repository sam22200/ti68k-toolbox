// Does the LIO_RecvData timeout still work when auto-int 5 is replaced? (no cable connected)
// -DKILL5: int 5 -> DUMMY_HANDLER, as games that install their own clock do.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static INT_HANDLER old1;
static volatile unsigned short ticks;
DEFINE_INT_HANDLER(tick1) { ticks++; ExecuteHandler(old1); }   // 256 Hz, chained to AMS

void _main(void)
{
    INT_HANDLER old5 = GetIntVec(AUTO_INT_5);
    unsigned char b;
    unsigned short r, t;
    old1 = GetIntVec(AUTO_INT_1);
    clrscr();
    printf("waiting 20 ticks (1 s)...\n");
    SetIntVec(AUTO_INT_1, tick1);
#ifdef KILL5
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
#endif
    ticks = 0;
    r = LIO_RecvData(&b, 1, 20);
    t = ticks;
    SetIntVec(AUTO_INT_5, old5);
    SetIntVec(AUTO_INT_1, old1);
    printf("ret %u after %u/256 s", r, t);
    ngetchx();
}
