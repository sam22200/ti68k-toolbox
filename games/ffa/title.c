// Title, new game choices and the intro (ffa: dialog, growth stat, name, LIONHEART's words,
// "20 years after..."). Pages fade in and out; 2nd turns them.
#include "ffa.h"

enum { TS_TITLE, TS_STAT, TS_NAME, TS_INTRO };
#define NPAGE 5
static const char *const intro[NPAGE][6] = {
    { "In a poor Milunian", "family...", 0 },
    { "LIONHEART:", "My poor baby, you", "can't even walk yet,", "and all the matters", "are falling on you...", 0 },
    { "Mary, my dear wife, is", "caught by the Bramanian", "soldiers, and I must", "return to this cursed", "War...", 0 },
    { "I'll give the priest", "to look after you,", "wishing I will come", "back alive from this", "War...", 0 },
    { "20 years after...", 0 },
};
static const char *const stat_name[6] = { "Strength", "Magic", "Vitality", "Spirit", "Speed", "Luck" };
static const u8 stat_of[6] = { S_STR, S_MAG, S_DEF, S_MDEF, S_SPD, S_LUCK };
static const char letters[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
#define NLETTER 53

static Save loaded;
static u8 can_continue;

void title_open(void)
{
    u8 k;
    can_continue = game_saved(&loaded);
    st.mode = M_TITLE;
    st.tstep = TS_TITLE;
    st.tcur = 0;
    st.fade = 3;                             // fades in
    st.tfade = 1;
    for (k = 0; k < 9; k++) st.name[k] = "Arthur"[k < 7 ? k : 6];
}

static u8 letter_index(char c)
{
    u8 i;
    for (i = 0; i < NLETTER; i++) if (letters[i] == c) return i;
    return 0;
}

void title_update(void)
{
    u8 go = input_pressed(K_A | K_ENTER) != 0;   // K_ENTER is bit 8: != 0 before the u8
    if (st.tfade == 1) { if (!(rt_frame & 1) && st.fade) st.fade--; if (!st.fade) st.tfade = 0; return; }
    if (st.tfade == 2) {                     // fading out, then the next step
        if (!(rt_frame & 1) && st.fade < 3) { st.fade++; return; }
        st.tfade = 1;
        if (st.tstep == TS_INTRO + NPAGE - 1) {   // into the bedroom
            u8 sp = st.hero.speci;
            char nm[9];
            u8 k;
            for (k = 0; k < 9; k++) nm[k] = st.name[k];
            game_scenario(10);
            st.hero.st[S_STR] -= STAT / 4;   // the default choice, replaced by the player's
            st.hero.speci = sp;
            st.hero.st[sp] += STAT / 4;
            for (k = 0; k < 9; k++) st.name[k] = nm[k];
            st.fade = 3;
            st.mode = M_FADE_IN;
            st.fade_t = 0;
            return;
        }
        st.tstep++;
        st.tcur = 0;
        return;
    }
    switch (st.tstep) {
    case TS_TITLE:
        if (input_pressed(K_UP | K_DOWN) && can_continue) st.tcur ^= 1;
        if (go && st.tcur) {                 // Continue: the saved state, walking
            st = loaded.g;
            story_room();
            st.fade = 3; st.fade_t = 0;
            st.mode = M_FADE_IN;
            return;
        }
        if (go) st.tfade = 2;                // New Game
        break;
    case TS_STAT:
        if (input_pressed(K_UP)) st.tcur = st.tcur ? st.tcur - 1 : 5;
        if (input_pressed(K_DOWN)) st.tcur = st.tcur < 5 ? st.tcur + 1 : 0;
        if (go) { st.hero.speci = stat_of[st.tcur]; st.tfade = 2; }
        break;
    case TS_NAME: {                          // 8 letters, up/down change, left/right move
        u8 i = letter_index(st.name[st.tcur]);
        if (input_pressed(K_UP)) st.name[st.tcur] = letters[i + 1 < NLETTER ? i + 1 : 0];
        if (input_pressed(K_DOWN)) st.name[st.tcur] = letters[i ? i - 1 : NLETTER - 1];
        if (input_pressed(K_RIGHT) && st.tcur < 7) {
            st.tcur++;
            if (!st.name[st.tcur]) { st.name[st.tcur] = ' '; st.name[st.tcur + 1] = 0; }
        }
        if (input_pressed(K_LEFT) && st.tcur) st.tcur--;
        if (go) {                            // trim the spaces; an empty name stays Arthur
            s8 e = 0;
            while (e < 8 && st.name[e]) e++;          // the end of the string, then its spaces
            for (e--; e >= 0 && st.name[e] == ' '; e--) st.name[e] = 0;
            if (e < 0) { u8 k; for (k = 0; k < 7; k++) st.name[k] = "Arthur"[k]; }
            st.tfade = 2;
        }
        break;
    }
    default:
        if (go) st.tfade = 2;
        break;
    }
}

static void frame(s16 x, s16 y, s16 w, s16 h)
{
    draw_rect(x, y, w, h, C_BLACK);
    draw_rect(x + 1, y + 1, w - 2, h - 2, C_DGRAY);
    draw_rect(x + 2, y + 2, w - 4, h - 4, C_WHITE);
}

void title_render(void)
{
    u8 i;
    draw_clear();
    if (st.tstep == TS_TITLE) {
        draw_rect(0, 0, 160, 100, C_BLACK);
        frame(14, 10, 132, 40);
        draw_text(41, 16, "FINAL FANTASY", F_MEDIUM, C_BLACK);
        draw_rect(30, 27, 100, 1, C_DGRAY);
        draw_text(47, 32, "ALTERNATIVE", F_MEDIUM, C_BLACK);
        draw_rect(52, 60, 56, 22, C_WHITE);
        draw_text(59, 63, "New Game", F_MEDIUM, C_BLACK);
        draw_text(59, 72, "Continue", F_MEDIUM, can_continue ? C_BLACK : C_LGRAY);
        draw_rect(55, 64 + st.tcur * 9, 2, 5, C_BLACK);
        draw_rect(0, 88, 160, 9, C_DGRAY);
        draw_rect(0, 89, 160, 7, C_LGRAY);
        draw_text(28, 90, "remake of David COZ's game", F_SMALL, C_BLACK);
    } else if (st.tstep == TS_STAT) {
        frame(10, 4, 140, 92);
        draw_text(16, 8, "Choose a capacity that", F_MEDIUM, C_BLACK);
        draw_text(16, 17, "will increase more.", F_MEDIUM, C_BLACK);
        for (i = 0; i < 6; i++) draw_text(40, 32 + i * 10, stat_name[i], F_MEDIUM, C_BLACK);
        draw_rect(32, 33 + st.tcur * 10, 3, 6, C_BLACK);
    } else if (st.tstep == TS_NAME) {
        frame(10, 20, 140, 60);
        draw_text(22, 26, "Enter your name,", F_MEDIUM, C_BLACK);
        draw_text(22, 35, "8 letters max.", F_MEDIUM, C_BLACK);
        for (i = 0; i < 8; i++) {
            char c[2];
            c[0] = st.name[i] ? st.name[i] : ' '; c[1] = 0;
            if (i == st.tcur) draw_rect(30 + i * 12, 50, 10, 12, C_LGRAY);
            draw_rect(30 + i * 12, 63, 10, 1, C_BLACK);
            draw_text(32 + i * 12, 52, c, F_MEDIUM, C_BLACK);
            if (!st.name[i]) break;
        }
    } else {
        const char *const *p = intro[st.tstep - TS_INTRO];
        u8 n = 0;
        while (p[n]) n++;
        for (i = 0; i < n; i++) draw_text(12, 50 - n * 5 + i * 10, p[i], F_MEDIUM, C_BLACK);
    }
    if (st.fade) fade_planes(st.fade);
}
