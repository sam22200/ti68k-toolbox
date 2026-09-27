// Table-driven rotation demo: a square rotates with 8-bit angles and a signed-char sine table
// (no float, no division in the loop); double buffering with ExtGraph. Shows the frame rate.
// Also self-tests a table-based 8-bit atan2. ESC quits.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"
#include "tables.h"          // generated: tools/bin/ti-table sin atan > tables.h

#define COS(a) sin_tab[(unsigned char)((a) + 64)]   // one table for both
#define SIN(a) sin_tab[(unsigned char)(a)]

static volatile unsigned short ticks;               // 256 Hz (auto-int 1 replaced)
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

// 8-bit angle of the vector (x, y) (0 = +x, 64 = +y, 128 = -x, 192 = -y), |x|,|y| < 1024.
static unsigned char atan2_8(short x, short y)
{
    unsigned short ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    unsigned char a;
    if (ax == 0 && ay == 0) return 0;
    if (ax >= ay) a = atan_tab[(unsigned short)(ay << 6) / ax];       // one divu.w per call
    else          a = 64 - atan_tab[(unsigned short)(ax << 6) / ay];
    if (x < 0) a = 128 - a;
    if (y < 0) a = -a;                                                  // wraps to 256 - a
    return a;
}

void _main(void)
{
    void *buf = malloc(LCD_SIZE);
    INT_HANDLER old_int1 = GetIntVec(AUTO_INT_1);
    unsigned char angle = 0, k;
    unsigned short frames = 0, t0, fps = 0;
    static const short tx[] = {1, 0, -1, 0, 1, -5, 3}, ty[] = {0, 1, 0, -1, 1, -5, -9};
    char msg[40];

    if (!buf) return;
    msg[0] = 0;
    // atan2 self-test, printed once and then kept on the status line area
    ClrScr();
    FontSetSys(F_4x6);
    for (k = 0; k < 7; k++) {
        sprintf(msg, "atan2(%d,%d)=%u", tx[k], ty[k], atan2_8(tx[k], ty[k]));
        DrawStr(0, k * 7, msg, A_NORMAL);
    }
    DrawStr(0, 60, "any key: rotation demo, ESC quits", A_NORMAL);
    ngetchx();

    SetIntVec(AUTO_INT_1, tick_handler);
    PortSet(buf, 239, 127);                                 // AMS text drawing now goes to buf
    t0 = ticks;
    while (!_keytest(RR_ESC)) {
        short i, x[4], y[4];
        FastClearScreen_R(buf);
        for (i = 0; i < 4; i++) {                          // corners at angle + i*64, radius 40
            unsigned char a = angle + (i << 6);
            x[i] = 80 + ((40 * COS(a)) >> 7);                // 16-bit muls, shift instead of /128
            y[i] = 50 + ((40 * SIN(a)) >> 7);
        }
        for (i = 0; i < 4; i++)
            FastLine_Draw_R(buf, x[i], y[i], x[(i + 1) & 3], y[(i + 1) & 3]);
        if (++frames == 64) {                              // fps = 64 frames * 256 / elapsed ticks
            fps = (unsigned short)((64UL * 256) / (unsigned short)(ticks - t0));
            frames = 0; t0 = ticks;
            sprintf(msg, "%u fps", fps);                   // format only when it changes
        }
        DrawStr(0, 0, msg, A_REPLACE);                     // ROM call into buf: fine for a demo
        FastCopyScreen_R(buf, LCD_MEM);
        angle += 2;
    }
    PortRestore();
    SetIntVec(AUTO_INT_1, old_int1);
    free(buf);
    GKeyFlush();
}
