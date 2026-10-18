// PC oracle for every TI capture frame. No SDL, no hardware, no timing claims.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.h"
#include "../../../runtime/platform-sw/rt_sw.h"
static SwScript script;
int main(int argc, char **argv)
{
    unsigned frame, frames;
    if (argc != 4 || sw_load_script(&script, argv[2])) return 2;
    frames = (unsigned)atoi(argv[3]);
    sw_init((u16)atoi(argv[1]));
    for (frame = 0; frame < frames; frame++) {
        if (!sw_step(sw_script_keys(&script, (u16)frame))) return 3;
        printf("%u %04X\n", frame, sw_checksum());
    }
    return 0;
}
