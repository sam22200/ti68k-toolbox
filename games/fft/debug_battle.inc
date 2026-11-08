// Hidden debug aids (SCREENS on the TI; direct in builds without modules). st.dbg holds them
// for the current battle; tools/fftmem.py set writes the same byte from the PC (live RAM).
// In Settings (MODE/CATALOG), [alpha] opens the DEBUG page: God mode, Fast AI, Skip this fight
// ("Skip this fight?" No/Yes). Skipping takes every enemy out: the next turn start finds the
// victory, then the rewards, aftermath and session save run as in play.

static void debug_skip(void)
{
    u8 i; Unit *u = st.unit;
    for (i = 0; i < NUNIT; i++, u++) if (u->team == TEAM_ENEMY) u->hp = 0;
    st.config = 0; st.ai = AI_OFF; st.mode = M_CLOCK; W->hold = W->frz = 0;
}

static u8 debug_config(void)
{
    if (st.config == 3) {                        // "Skip this fight?": No unless Yes is picked
        if (input_pressed(K_UP | K_DOWN)) st.config_row ^= 1;
        if (input_pressed(K_A | K_ENTER) && st.config_row) debug_skip();
        else if (input_pressed(K_A | K_ENTER | K_ESC | K_B)) { st.config = 2; st.config_row = 2; }
        return 1;
    }
    if (input_pressed(K_ESC | K_B | K_MODE | K_CATALOG)) { st.config = 0; W->hold = W->frz = 0; return 1; }
    if (input_pressed(K_DOWN)) st.config_row = st.config_row < 2 ? st.config_row + 1 : 0;
    else if (input_pressed(K_UP)) st.config_row = st.config_row ? st.config_row - 1 : 2;
    if (st.config_row < 2) { if (input_pressed(K_A | K_ENTER | K_LEFT | K_RIGHT)) st.dbg ^= (u8)(1 << st.config_row); }
    else if (input_pressed(K_A | K_ENTER)) { st.config = 3; st.config_row = 0; }
    return 1;
}

static void debug_draw(void)
{
    u8 row, y;                                   // (no pointer table: module images have no relocations)
    draw_rect(0, 0, RT_W, RT_H, C_WHITE);
    draw_text(4, 1, "DEBUG", F_MEDIUM, C_BLACK);
    for (row = 0; row < 3; row++) {
        y = 28 + 18 * row;
        if (row == (st.config == 3 ? 2 : st.config_row)) { draw_rect(3, y - 3, 154, 12, C_BLACK); draw_rect(4, y - 2, 152, 10, C_LGRAY); }
        draw_text(8, y, row == 0 ? "God mode" : row == 1 ? "Fast AI" : "Skip this fight", F_SMALL, C_BLACK);
        if (row < 2) draw_text(120, y, st.dbg & (1 << row) ? "On" : "Off", F_SMALL, C_BLACK);
    }
    if (st.config == 3) {                        // the question, its hand on No or Yes
        draw_rect(30, 34, 100, 34, C_BLACK); draw_rect(31, 35, 98, 32, C_WHITE);
        draw_text(36, 38, "Skip this fight?", F_SMALL, C_BLACK);
        draw_text(50, 48, "No", F_SMALL, C_BLACK); draw_text(50, 57, "Yes", F_SMALL, C_BLACK);
        draw_text(42, 48 + 9 * st.config_row, ">", F_SMALL, C_BLACK);
    }
    draw_text(4, 93, "ENTER:toggle  ESC:back", F_SMALL, C_BLACK);
}

// God mode, just before a blow or an area lands (resident hook at ACT_HIT): the allied side's
// (player and guests: Agrias, Delita...) damaging blows kill enemies; enemy actions and any
// damage (friendly fire included) on the allied side miss. The ordinary commit then applies
// them (KO, rewards, reactions unchanged). Covers single blows and area batches (W->cast);
// deferred weapon panel/line victims are not rewritten.
static void debug_hit(void)
{
    u8 i, actor = W->cast.phase == CAST_BATCH_READY ? W->cast.actor : st.atk;
    u8 mine = st.unit[actor].team != TEAM_ENEMY;
    if (W->cast.phase == CAST_BATCH_READY) {
        for (i = 0; i < NUNIT; i++) if (W->cast.mask & (1U << i)) {
            CastVictim *v = &W->cast.victim[i];
            if (st.unit[i].team != TEAM_ENEMY && (!mine || v->effect == AB_DAMAGE)) v->hit = 0;
            else if (st.unit[i].team == TEAM_ENEMY && mine && v->effect == AB_DAMAGE) { v->hit = 1; v->amount = 999; }
        }
        return;
    }
    if (st.unit[st.tgt].team != TEAM_ENEMY && (!mine || (st.effect_kind & EFFECT_KIND_MASK) == AB_DAMAGE)) st.hit = 0;
    else if (st.unit[st.tgt].team == TEAM_ENEMY && mine && (st.effect_kind & EFFECT_KIND_MASK) == AB_DAMAGE) { st.hit = 1; st.dmg = 999; }
}
#ifndef FFT_MODULES
void debug_hit_direct(void){debug_hit();}   // builds without modules call it from the resident
#endif
