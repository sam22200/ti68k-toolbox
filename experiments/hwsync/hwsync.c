// Forum findings checked: LCD frame-sync bit 7 of port 0x70001D (HW2+), and the fine timer
// (0x600015 bits 5-4 = 00: 0x600017 counts at OSC2/32). Counts both over 256 auto-int-1 ticks
// (1 s on HW2+), then restores the ports.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned short ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    volatile unsigned char *sync = (volatile unsigned char *)0x70001D;
    volatile unsigned char *rate = (volatile unsigned char *)0x600015, *cnt = (volatile unsigned char *)0x600017;
    unsigned char r0 = *rate, c0, last, prev;
    unsigned short flips = 0, s;
    unsigned long fine = 0;

    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    s = ticks; while (ticks == s); s = ticks;
    prev = *sync & 0x80;
    while ((unsigned short)(ticks - s) < 256) {
        unsigned char b = *sync & 0x80;
        if (b != prev) { flips++; prev = b; }
    }
    c0 = PRG_getStart();
    *rate = r0 & ~0x30;                    // prescaler OSC2/32
    s = ticks; while (ticks == s); s = ticks;
    last = *cnt;
    while ((unsigned short)(ticks - s) < 256) {
        unsigned char v = *cnt;
        fine += v >= last ? v - last : (256 - last) + (v - c0);   // wraps reload the start value
        last = v;
    }
    *rate = r0;
    PRG_setStart(c0);
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    ClrScr();
    printf_xy(0, 0, "HW%d  port 600015=%02X", (short)HW_VERSION, r0);
    printf_xy(0, 10, "sync flips/s: %u", flips);
    printf_xy(0, 20, "fine ticks/s: %lu", fine);
    GKeyFlush();
    ngetchx();
}
