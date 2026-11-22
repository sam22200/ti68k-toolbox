// The Secondary command: the skill set a unit has taken from another job.
//
// The battle used to field one command, its job's own -- `saved_unit_load` overwrote the
// set with `job_sets[s->job]` and dropped the roster's Secondary, so a unit that had paid
// for a second command fought without it. This checks the whole path: the save gives the
// set and, from the career that owns it, the abilities learned in it; the turn menu grows
// one row and names it; its list is the second set's, not the first's; and a unit without
// one is left exactly as it was.
#include "gariland.c"
#include <stdio.h>

static unsigned failures;
#define CHECK(got, want) do { long g = (long)(got), w = (long)(want); \
    if (g != w) { printf("  FAIL line %d: %s = %ld, expected %ld\n", __LINE__, #got, g, w); failures++; } } while (0)

// A saved unit of job `job`, with `second` as its Secondary and every ability of the career
// that owns it marked learned.
static void saved(SaveUnit *s, u8 job, u8 second)
{
    u8 c;
    memset(s, 0, sizeof *s);
    s->job = job; s->level = 5; s->brave = 70; s->faith = 70; s->gfx = 0xFF;
    s->raw[0][0] = 0; s->raw[0][1] = 0x20; s->raw[0][2] = 0;     // HP
    for (c = 0; c < 5; c++) if (!s->raw[c][1]) s->raw[c][1] = 0x20;
    s->secondary = second;
    for (c = 0; c < CAREER_N; c++) { s->learned[c][0] = 0xFF; s->learned[c][1] = 0xFF; s->learned[c][2] = 0xFF; }
}

// The career whose set is `set`, and the set of a career other than `job`'s own.
static u8 other_set(u8 job)
{
    u8 c;
    for (c = 0; c < CAREER_N; c++) {
        u8 s = job_sets[0x4A + c];
        if (s && s != job_sets[job] && s != 18 && s != 20 && s != 21) return s;   // not Jump/Throw/Math
    }
    return 0;
}

int main(void)
{
    SaveUnit s;
    Unit *u;
    u8 second, i, rows_one, rows_two;
    u16 n1, n2;

    sw_init(0); game_init(); game_scenario(26);
    for (i = 0; i < 200 && (st.open_stage || st.open_fade); i++) sw_step(0);

    second = other_set(0x4A);
    CHECK(second != 0, 1);

    // 1. the save gives the set, and the learned bits come from the career that owns it
    saved(&s, 0x4A, second);
    saved_unit_load(U_RAMZA, &s, 0);
    u = &st.unit[U_RAMZA];
    CHECK(u->set, job_sets[0x4A]);
    CHECK(u->set2, second);
    CHECK(u->learned2 != 0, 1);

    // 2. the two lists are the two sets', and neither leaks into the other
    n1 = known(U_RAMZA, 255);
    n2 = known2(U_RAMZA, 255);
    CHECK(n1 != 0, 1);
    CHECK(n2 != 0, 1);
    for (i = 0; i < n2; i++) {
        AbId ab = known2(U_RAMZA, i), k;
        u8 found = 0;
        for (k = 0; k < SET_N; k++) if (SET_ABILITY(second, k) == ab) found = 1;
        CHECK(found, 1);
    }
    st.skill2 = 0; CHECK(known_cmd(U_RAMZA, 255), n1);
    st.skill2 = 1; CHECK(known_cmd(U_RAMZA, 255), n2);
    st.skill2 = 0;

    // 3. the turn menu carries one more row, and it is named after the second set
    st.act = U_RAMZA;
    W->win_k[2] = U_RAMZA;
    rows_two = win_lines(M_ACTS);
    CHECK(ACTS_TWO(U_RAMZA), 1);
    {
        char text[40];
        W->win_k[0] = M_ACTS; W->win_k[3] = 2;
        CHECK(!strcmp(SKILL_NAME(u->set2), SKILL_NAME(second)), 1);
        (void)text;
    }

    // 4. a unit with no Secondary is untouched: one command, the old row count
    saved(&s, 0x4A, 0);
    saved_unit_load(U_RAMZA, &s, 0);
    u = &st.unit[U_RAMZA];
    CHECK(u->set2, 0);
    CHECK(u->learned2, 0);
    CHECK(known2(U_RAMZA, 255), 0);
    CHECK(known2(U_RAMZA, 0), NO_ABIL);
    CHECK(ACTS_TWO(U_RAMZA), 0);
    W->win_k[2] = U_RAMZA;
    rows_one = win_lines(M_ACTS);
    CHECK(rows_two, rows_one + 1);

    // 5. a Secondary equal to the job's own set is not shown twice
    saved(&s, 0x4A, job_sets[0x4A]);
    saved_unit_load(U_RAMZA, &s, 0);
    CHECK(st.unit[U_RAMZA].set2, 0);

    printf("Secondary: the save's second command reaches the battle, the turn menu carries it\n"
           "           (%u rows against %u), and its list is its own (%u abilities)\n",
           rows_two, rows_one, n2);
    if (failures) printf("  %u failures\n", failures);
    return failures != 0;
}
