// PC-only strategy preview. No TI binary, emulator, timing or hardware calls.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.h"
#include "../../../runtime/platform-sw/rt_sw.h"
int main(int argc, char **argv)
{
    unsigned view, mode, n;
    char file[1024];
    if (argc != 4) return 2;
    view = (unsigned)atoi(argv[1]); mode = (unsigned)atoi(argv[2]);
    if (view > 3 || mode > 2) return 2;
    sw_init(62);
    if (view >= 2) { st.pdist = 80; st.ppitch = 6; }
    if (view & 1) st.pyaw ^= 32;
    st.psolid = (u8)mode;
    for (n = 0; n < 32; n++) {
        if (!sw_step(0)) return 3;
        snprintf(file, sizeof file, "%s/%03u.png", argv[3], n);
        if (sw_write_png(file, 1)) return 4;
        printf("%u %04X\n", n, sw_checksum());
    }
    return 0;
}
