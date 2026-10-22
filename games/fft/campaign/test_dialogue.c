// Every word of every scene beat appears once across its fitted continuation pages
// (the dialogue shared by the scene player and the battle-map beats: ../beats.inc).
#include "scene.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
static unsigned pages, beats;
static void check_set(u16 set_number)
{
    u8 i;
    sw_init(set_number);
    for (st.step = 0; st.step < RD16(set, 5); st.step++) {
        if (step_at(st.step)[0] != STEP_STORY) continue;
        step_start();
        for (i = 0; i < OPENING_BEATS; i++) {
            st.open_beat = i; st.open_offset = 0;
            if (opening_beat()[2]) continue;           // a silent hold
            for (;;) {
                char actual[300], expected[300]; unsigned a = 0, b = 0, r, c;
                const char *source = opening_text();
                opening_layout();
                for (r = 0; r < W->dialog_n; r++) for (c = 0; W->dialog[r][c]; c++) if (W->dialog[r][c] != ' ') actual[a++] = W->dialog[r][c];
                for (c = 0; c < W->dialog_next; c++) if (source[c] != ' ') expected[b++] = source[c];
                assert(a == b && !memcmp(actual, expected, a)); assert(W->dialog_next); pages++;
                if (!source[W->dialog_next]) break;
                st.open_offset += W->dialog_next; assert(pages < 500);
            }
            beats++;
        }
    }
}
int main(void)
{
    assert(!chdir("../opening/data")); check_set(0);    // Orbonne's chapel
    assert(!chdir("../../campaign/data")); check_set(1);   // the Academy
    printf("Scene dialogue: all words preserved across %u fitted pages / %u beats\n", pages, beats);
    return 0;
}
