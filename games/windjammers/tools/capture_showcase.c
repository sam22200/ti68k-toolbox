/* Export real software-runtime LCD frames, input and mechanic states. PC only. */
#include <stdio.h>
#include <stdlib.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#include "../windjam.h"
static SwScript script;
int main(int argc, char **argv)
{
    unsigned frame, x, y, count, scenario;
    unsigned char pixels[16000];
    FILE *out, *states;
    if (argc != 6) return 1;
    scenario = (unsigned)atoi(argv[1]); count = (unsigned)atoi(argv[3]);
    if (!count || count > 1000 || sw_load_script(&script, argv[2])) return 2;
    out = fopen(argv[4], "wb"); if (!out) return 3;
    states = fopen(argv[5], "w"); if (!states) { fclose(out); return 4; }
    sw_init(scenario);
    fprintf(states, "frame,keys,mode,charging,charged,throw_catch,dash,z,points1,points2,checksum,effect_kind,effect_count\n");
    for (frame = 0; frame < count; frame++) {
        u32 keys = sw_script_keys(&script, frame);
        if (!sw_step(keys)) return 5;
        for (y = 0; y < 100; y++)
            for (x = 0; x < 160; x++) pixels[y * 160 + x] = sw_level(x, y);
        if (fwrite(pixels, 1, sizeof pixels, out) != sizeof pixels) return 6;
        fprintf(states, "%u,%u,%u,%u,%u,%u,%u,%ld,%u,%u,%u,%u,%u\n", frame, (unsigned)keys,
            st.disc.mode, st.player[st.human_port].charging, st.player[st.human_port].charged,
            st.player[st.human_port].throw_catch, st.player[st.human_port].dash_age,
            (long)st.disc.z, st.points[0], st.points[1], sw_checksum(),st.effect_kind,st.effect_count);
    }
    fclose(out); fclose(states);
    return 0;
}
