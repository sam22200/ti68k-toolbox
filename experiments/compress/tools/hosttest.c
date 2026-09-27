// Host test of ../unpack.c: decodes bin/*.{rle,lz4,zx0} and compares with bin/*.raw.
// gcc -O2 -fno-strict-aliasing -o /tmp/ht tools/hosttest.c && /tmp/ht   (from experiments/compress)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../unpack.c"
static long load(const char *nm, u8 *buf)
{
    FILE *f = fopen(nm, "rb"); long n;
    if (!f) { perror(nm); exit(1); }
    n = fread(buf, 1, 70000, f); fclose(f); return n;
}
int main(void)
{
    static u8 in[70000], raw[70000], out[70000];
    const char *blob[3] = { "tiles", "sprites", "fire" }, *fmt[3] = { "rle", "lz4", "zx0" };
    int i, j, bad = 0;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            char nm[64]; long rn, cn, on;
            sprintf(nm, "bin/%s.raw", blob[i]); rn = load(nm, raw);
            sprintf(nm, "bin/%s.%s", blob[i], fmt[j]); cn = load(nm, in);
            memset(out, 0xAA, sizeof out);
            on = (j == 0 ? rle_unpack(in, cn, out) : j == 1 ? lz4_unpack(in, cn, out) : zx0_unpack(in, out)) - out;
            printf("%-8s %s %6ld -> %6ld %s\n", blob[i], fmt[j], cn, on, on == rn && !memcmp(out, raw, rn) ? "ok" : (bad++, "BAD"));
        }
    return bad;
}
