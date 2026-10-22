// The beats played on the battle map (campaign/ENGINE.md M2): the chapter's script, its
// descriptor's MAP_SCRIPT (tools/chapter_desc.py), plays sequences of the beats bank
// (beats.inc, opening_story) before the battle and after a victory. A sequence: the stage
// it shows as (st.open_stage, never OPEN_PLAY or OPEN_DONE), the beats' event, MS_* flags,
// the camera (rot, camx, camy). Its last beat leads to the next sequence of the same side
// of the battle (faded or cut), else to the battle, or after it to the end (OPEN_DONE).
// The includer provides opening_battle (the battle starting) and opening_after (the
// chapter's own state put back after it).
enum { MS_AFTER = 1, MS_FADE = 2, MS_PLACED = 4, MS_CAMERA = 8 };
#define MAP_SCRIPT (cdesc + CD(MAP_SCRIPT))

static const u8 *map_seq(u8 stage)              // the sequence shown as stage (RT_NULL: none)
{
    const u8 *s = MAP_SCRIPT + 2;
    u8 n = MAP_SCRIPT[1];
    if (!CD(MAP_SCRIPT)) return RT_NULL;
    for (; n; n--, s += 8) if (s[0] == stage) return s;
    return RT_NULL;
}

static const u8 *map_first(u8 after)            // the first sequence before / after the battle
{
    const u8 *s = MAP_SCRIPT + 2;
    u8 n = MAP_SCRIPT[1];
    if (!CD(MAP_SCRIPT)) return RT_NULL;
    for (; n; n--, s += 8) if ((s[2] & MS_AFTER) == after) return s;
    return RT_NULL;
}

static void map_stage(u8 stage)
{
    const u8 *s = map_seq(stage);
    st.open_stage = stage; st.open_clock = 0; st.open_seen = 0;
    memset(W->actor_key, 255, sizeof W->actor_key);
    W->dialog_key = 0xFFFF; W->bg_ok = W->hold = W->frz = 0;
    if (!s) { if (stage == OPEN_PLAY) opening_battle(0); return; }
    opening_find(s[1]);
    if (s[2] & MS_AFTER) { st.mode = M_BROWSE; st.banner = st.end = 0; st.morph_mask = 0; opening_after(); }
    if (s[2] & MS_CAMERA) { st.rot = s[3]; st.camx = (s16)RD16(s, 2); st.camy = (s16)RD16(s, 3); scene_rot = vbuf_rot = 255; }
}

static void map_transition(u8 stage)            // faded out, then in on that stage
{
    st.open_next = stage; st.open_fade = 1;
}

static void map_enter(const u8 *s)              // a sequence, faded in if it says so
{
    if (s[2] & MS_FADE) map_transition(s[0]);
    else map_stage(s[0]);
}

static void map_begin(void)                     // the first sequence (the program starts in the
{                                               // middle of the previous one's fade); none: the
    const u8 *s = map_first(0);                 // battle
    if (!s) { map_stage(OPEN_PLAY); return; }
    map_stage(s[0]);
    if (s[2] & MS_FADE) st.open_fade = 9;
}

static void map_start(u8 after)                 // the first sequence before the battle, or after
{                                               // it (test doors)
    if (after) map_stage(map_first(MS_AFTER)[0]); else map_begin();
}

static void map_skip(void)                      // to the battle, or after it to the end
{
    const u8 *s = map_seq(st.open_stage);
    map_transition(s && s[2] & MS_AFTER ? OPEN_DONE : OPEN_PLAY);
}

static void map_advance(void)
{
    u8 event = opening_beat()[0];
    const u8 *s;
    if (st.open_beat + 1 < OPENING_BEATS) {
        st.open_beat++;
        if (opening_beat()[0] == event) { opening_dialog_reset(); return; }
    }
    s = map_seq(st.open_stage);
    if (s && s + 8 < MAP_SCRIPT + 2 + 8 * MAP_SCRIPT[1] && (s[10] & MS_AFTER) == (s[2] & MS_AFTER)) map_enter(s + 8);
    else map_skip();
}

static u8 map_update(void)
{
    const u8 *s;
    if (!W || !opening_story) return !input_pressed(K_ESC);
    // A completed script hands over to the next program (the scene player's chapter card).
    if (st.open_stage == OPEN_DONE) { st.open_handoff = 1; return 0; }
    st.tick++; if (st.open_clock < 250) st.open_clock++;
    if (!st.open_stage && !st.open_fade) {      // the victory: what follows it, else the end
        s = map_first(MS_AFTER);
        if (s) map_enter(s); else map_transition(OPEN_DONE);
    }
    if (st.open_fade) {
        if (++st.open_fade == 9) { map_stage(st.open_next); st.open_fade = 9; }
        else if (st.open_fade >= 17) st.open_fade = 0;
        return !input_pressed(K_ESC);
    }
    s = map_seq(st.open_stage);
    if (s && s[2] & MS_PLACED) opening_positions();
    if (input_pressed(K_CATALOG)) { st.open_white ^= 1; W->dialog_key = 0xFFFF; }
    if (input_pressed(K_B)) { map_skip(); return 1; }
    if (opening_beat()[2]) {
        if (st.open_clock >= opening_beat()[2]) map_advance();
        return !input_pressed(K_ESC);
    }
    if (W->dialog_key != st.open_beat) opening_layout();
    if (st.open_reveal < W->dialog_total) {
        st.open_reveal += 3;
        if (st.open_reveal > W->dialog_total) st.open_reveal = W->dialog_total;
        if (input_pressed(K_A | K_ENTER)) st.open_reveal = W->dialog_total;
    } else if (input_pressed(K_A | K_ENTER)) {
        const char *t = opening_text();
        if (W->dialog_drawn != 0xFFFF && W->dialog_drawn < W->dialog_total) return 1;
        if (t[W->dialog_next]) { st.open_offset += W->dialog_next; st.open_reveal = 0; W->dialog_key = 0xFFFF; return 1; }
        map_advance();
    }
    return !input_pressed(K_ESC);
}

static void map_render(void)
{
    u8 i;
    const u8 *s;
    if (!opening_story) { draw_rect(0, 0, 160, 100, C_WHITE); ui_text(3, 40, "Missing data or memory", C_BLACK, 160); return; }
    if (st.open_stage == OPEN_DONE) draw_rect(0, 0, 160, 100, C_BLACK);   // the fade's last, black frame
    else {
        const u8 *b = opening_beat(), *a = b + 4;
        ZB(21);
        ensure_scene(st.rot); copy_view();
        s = map_seq(st.open_stage);
        if (s && s[2] & MS_PLACED) {
            for (i = 0; i < b[1]; i++, a += 8) {
                u8 slot = a[0] >> 4;
                opening_actor(a[0] & 15, slot, W->feet[slot].x, W->feet[slot].y, a[1], RD16(a + 6, 0));
            }
        } else {
            // The battle's units, still, without battle timers or markers.
            for (i = 0; i < NUNIT; i++) if (st.unit[i].hp) {
                Feet f = unit_feet(st.rot, i);
                u8 dir = unit_direction(st.rot, i);
                W->feet_k[i] = 0;
                opening_actor(st.unit[i].gfx, i, f.x, f.y, (f.depth << 2) | dir,
                    (u16)(st.unit[i].gfx * 44 + (dir & 1 ? 5 : 0) + (dir & 2 ? 22 : 0)));
            }
        }
        ZE(21);
        if (!b[2]) {
            if (W->dialog_key != st.open_beat) opening_layout();
            ZB(22); opening_dialog_draw(); ZE(22);
        }
    }
    if (st.open_fade) {
        u8 level = st.open_fade < 9 ? st.open_fade : 17 - st.open_fade;
        u8 phase = (level + 1) >> 1, r;
        u16 pat = phase >= 4 ? 0xFFFF : phase == 3 ? 0xEEEE : phase == 2 ? 0xAAAA : 0x8888;
        for (r = 0; r < 100; r++) {
            u8 *l = (u8 *)rt_light + (u16)r * RT_PBYTES, *d = (u8 *)rt_dark + (u16)r * RT_PBYTES, w;
            u16 bits = r & 1 ? (u16)((pat << 1) | (pat >> 15)) : pat;
            for (w = 0; w < 10; w++) { RMW16(l + 2 * w, bits, bits); RMW16(d + 2 * w, bits, bits); }
        }
    }
}
