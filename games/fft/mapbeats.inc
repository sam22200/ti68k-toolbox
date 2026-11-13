// The beats played on the battle map (campaign/ENGINE.md M2): the chapter's script, its
// descriptor's MAP_SCRIPT (tools/chapter_desc.py), plays sequences of the beats bank
// (beats.inc, opening_story) before the battle and after a victory. A sequence: the stage
// it shows as (st.open_stage, never OPEN_PLAY or OPEN_DONE), the beats' event, MS_* flags,
// the camera (rot, camx, camy). Its last beat leads to the next sequence of the same side
// of the battle (faded or cut), else to the battle, or after it to the end (OPEN_DONE).
// The includer provides opening_battle (the battle starting) and opening_after (the
// chapter's own state put back after it).
#if !MOD_CORE
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define hand_gfx CONTOUR_REF_hand_gfx
#endif
#define CHOICE_TEXT(x,y,s,c,l) ui_text(x,y,s,c,l)
#define CHOICE_WIDTH(s) ui_width(s)
#include "choice_menu.inc"
#endif
#include "scene_skip.h"
enum { MS_AFTER = 1, MS_FADE = 2, MS_PLACED = 4, MS_CAMERA = 8 };
static const u8 *map_script_data(void)
{
    return flow_bank&&CD(LOCATION)==7?qpart(flow_bank,Q_SCRIPT):cdesc+CD(MAP_SCRIPT);
}
#define MAP_SCRIPT map_script_data()
#define HAS_MAP_SCRIPT (CD(MAP_SCRIPT)||(flow_bank&&CD(LOCATION)==7))

static const u8 *map_seq(u8 stage)              // the sequence shown as stage (RT_NULL: none)
{
    const u8 *s = MAP_SCRIPT + 2;
    u8 n = MAP_SCRIPT[1];
    if (!HAS_MAP_SCRIPT) return RT_NULL;
    for (; n; n--, s += 8) if (s[0] == stage) return s;
    return RT_NULL;
}

static const u8 *map_first(u8 after)            // the first sequence before / after the battle
{
    const u8 *s = MAP_SCRIPT + 2;
    u8 n = MAP_SCRIPT[1];
    if (!HAS_MAP_SCRIPT) return RT_NULL;
    for (; n; n--, s += 8) if ((s[2] & MS_AFTER) == after) return s;
    return RT_NULL;
}

static void map_stage(u8 stage)
{
    const u8 *s = map_seq(stage);
    st.open_stage = stage; st.open_clock = 0; st.open_seen = 0; st.quit = 0;
    memset(W->actor_key, 255, sizeof W->actor_key);
    W->dialog_key = 0xFFFF; W->bg_ok = W->hold = W->frz = 0;
    if (!s) { if (stage == OPEN_PLAY) st.open_stage=OPEN_BEGIN; return; }
    opening_find(s[1]);
    if (s[2] & MS_AFTER) { st.mode = M_BROWSE; st.banner = st.end = 0; st.morph_mask = 0; }
    if (s[2] & MS_CAMERA) { st.rot = s[3]; st.camx = (s16)RD16(s, 2); st.camy = (s16)RD16(s, 3); scene_rot = vbuf_rot = 255; }
    if(s[2]&MS_AFTER)opening_after();
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

static void map_start_step(u8 after)                 // the first sequence before the battle, or after
{                                               // it (test doors)
    if (after) map_stage(map_first(MS_AFTER)[0]); else map_begin();
}

static void map_skip(void)                      // to the battle, or after it to the end
{
    u8 stage=st.open_fade&&st.open_fade<9?st.open_next:st.open_stage;
    const u8 *s = map_seq(stage);
    map_transition(stage==OPEN_DONE||(s && s[2]&MS_AFTER)?OPEN_DONE:OPEN_PLAY);
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

static u8 map_step(void)
{
    const u8 *s;
    if(st.open_stage==OPEN_BEGIN)return 1; // resident resumes after this image returns
    if (!W || !opening_story) return !input_pressed(K_ESC);
    // A completed script hands over to the next program (the scene player's chapter card).
    if (st.open_stage == OPEN_DONE) {
        if(flow_bank&&CD(LOCATION)==7&&st.reward_ready) {
            st.open_stage=st.open_fade=0;st.story_done=1;st.mode=M_END;st.end=END_VICTORY;
            st.reward_page=st.open_clock=0;W->bg_ok=0;return 1;
        }
        st.open_handoff = 1; return 0;
    }
    if(!st.open_stage&&!st.open_fade){ // settle entry before accepting ESC, including fade-out
        st.quit=0;s=map_first(MS_AFTER);
        if(s)map_enter(s);else map_transition(OPEN_DONE);
    }
    // The ordinary quit selector is idle during map dialogue. Reuse its byte
    // for the scene prompt, without adding battle/save/overlay state.
    {u8 skip=scene_skip_input(&st.quit);if(skip){W->frz_n++;if(skip==2)map_skip();return 1;}}
    st.tick++; if (st.open_clock < 250) st.open_clock++;
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
        if(flow_bank&&s&&(s[2]&MS_AFTER)) {
            u16 represented=0,hidden_roles=0;u8 j;
            if(s[2]&MS_PLACED)for(j=0;j<b[1];j++) {
                const u8 *role=b+4+8*j;u8 k,dead=0;
                for(k=0;k<NUNIT;k++)if(st.unit[k].team!=TEAM_ENEMY&&st.unit[k].gfx==(role[0]&15)) {
                    if(!st.unit[k].hp)dead=1;
                    else if(!(represented&(1U<<k))){represented|=(u16)(1U<<k);break;}
                }
                if(k==NUNIT&&dead)hidden_roles|=(u16)(1U<<(role[0]>>4));
            }
            // Keep unrepresented allies at their actual ending tiles, including
            // KO poses. Scripted living roles retain the existing choreography.
            for(i=0;i<NUNIT;i++)if(!(st.scene_hide&(1U<<i))&&!(represented&(1U<<i))&&st.unit[i].x<MAP_W) {
                Feet f=unit_feet(st.rot,i);u8 dir=unit_direction(st.rot,i);u16 old=W->feet_k[i];
                W->feet_k[i]=0;opening_tactical_cover=1;
                opening_actor(st.unit[i].gfx,i,f.x,f.y,(f.depth<<2)|dir,
                    (u16)(st.unit[i].gfx*44+(st.unit[i].hp?2:P_KO)+(st.unit[i].hp?(dir&1?5:0):(dir&1))+(dir&2?22:0)));
                opening_tactical_cover=0;W->feet_k[i]=old;
            }
            if(s[2]&MS_PLACED)for(i=0;i<b[1];i++,a+=8)if(!(hidden_roles&(1U<<(a[0]>>4)))) {
                u8 slot=a[0]>>4;
                opening_actor(a[0]&15,slot,W->feet[slot].x,W->feet[slot].y,a[1],RD16(a+6,0));
            }
        }
        else if (s && s[2] & MS_PLACED) {
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
    if(st.quit)scene_skip_draw(st.quit,W->frz_n);
}
