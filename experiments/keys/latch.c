// Keyboard latch in auto-int 1 (TI-Chess style) vs plain _rowread polling, with a slow main loop
// (one poll per second). Counts ENTER taps seen by each method; ESC quits.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned char trow[7], clear_req;
static unsigned char orow[7], r;
static volatile unsigned short ticks;

DEFINE_INT_HANDLER(kbd)
{
    unsigned char n;
    ticks++;
    if (clear_req) { memset((void *)trow, 0, 7); clear_req = 0; }
    n = _rowread(~(1 << r));
    trow[r] |= ~orow[r] & n;                // latch new presses
    orow[r] = n;
    if (++r == 7) r = 0;
}

void _main(void)
{
    INT_HANDLER old1 = GetIntVec(AUTO_INT_1);
    unsigned short latched = 0, polled = 0, frame;
    ClrScr();
    printf_xy(0, 0, "1 poll/s. ENTER taps, ESC quits");
    for (r = 0; r < 7; r++) orow[r] = _rowread(~(1 << r));  // keys already held are not "new"
    r = 0;
    SetIntVec(AUTO_INT_1, kbd);
    for (;;) {
        frame = ticks;
        while ((unsigned short)(ticks - frame) < 256) ;       // a very slow "frame" (1 s)
        if (_rowread(~(1 << 1)) & 1) polled++;               // plain polling: key held right now?
        if (trow[1] & 1) latched++;                          // latch: pressed since last frame?
        if (trow[6] & 1) break;
        clear_req = 1;
        printf_xy(0, 20, "latched %u  polled %u  ", latched, polled);
    }
    SetIntVec(AUTO_INT_1, old1);
    while (_rowread(0)) ;
    GKeyFlush();
}
