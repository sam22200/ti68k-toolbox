// Celeste trace tests: each key script runs on the C port (sw_step, no window) and its state is
// printed per frame in p8trace.py's format (trace.lua's __st()), then compared line by line with
// the reference made by the original cart under z8lua (traces/<name>.txt, `make traces`).
// The first divergent frame and field is printed. Unit tests follow for what traces miss.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "celeste.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static const char *COMMON[] = { "frames", "seconds", "freeze", "shake", "deaths", "will_restart",
    "delay_restart", "sfx_timer", "has_dashed", "got_fruit[1]", "#objects", "types", "sum_xy",
    "sfx", "kind" };
static const char *PFIELDS[] = { "x", "y", "spd.x", "spd.y", "rem.x", "rem.y", "p_jump", "p_dash",
    "grace", "jbuffer", "djump", "dash_time", "dash_effect_time", "dash_target.x", "dash_target.y",
    "dash_accel.x", "dash_accel.y", "flip.x", "spr", "spr_off", "was_on_ground" };
static const char *SFIELDS[] = { "x", "y", "spd.x", "spd.y", "rem.x", "rem.y", "state", "delay",
    "spr", "target.x", "target.y" };

static char *put(char *s, fix v) { *s++ = ' '; p8_hex(s, v); return s + strlen(s); }
static char *putb(char *s, u8 b) { return s + sprintf(s, b ? " true" : " false"); }

static void trace_line(char *s, u16 f)
{
    static const char code[] = "?SPsTWFL??";
    char types[MAXOBJ + 1];
    fix sum = 0;
    Obj *p = RT_NULL;
    u8 k, n = 0;
    for (k = 0; k < S.nobj; k++) {     // smoke is cosmetic: not traced (trace.lua)
        Obj *o = &S.pool[S.order[k]];
        if (o->type != T_SMOKE) { types[n++] = code[o->type]; sum += FIX(o->x) + OBJ_Y(o); }
        if (!p && (o->type == T_PLAYER || o->type == T_SPAWN)) p = o;
    }
    types[n] = 0;
    s += sprintf(s, "%u", f);
    s = put(s, FIX(S.frames)); s = put(s, FIX(S.seconds)); s = put(s, FIX(S.freeze));
    s = put(s, FIX(S.shake)); s = put(s, FIX(S.deaths)); s = putb(s, S.will_restart);
    s = put(s, FIX(S.delay_restart)); s = put(s, FIX(S.sfx_timer)); s = putb(s, S.has_dashed);
    s = putb(s, S.got_fruit & 1); s = put(s, FIX(n));
    s += sprintf(s, " %s", types);
    s = put(s, sum);
    s += sprintf(s, " {");
    for (k = 0; k < S.nsfx; k++) {
        if (k) *s++ = ',';
        p8_hex(s, FIX(S.sfx_ev[k])); s += strlen(s);
    }
    s += sprintf(s, "}");
    if (!p) { sprintf(s, " none"); return; }
    s += sprintf(s, p->type == T_PLAYER ? " P" : " S");
    s = put(s, FIX(p->x)); s = put(s, OBJ_Y(p)); s = put(s, p->spdx); s = put(s, p->spdy);
    s = put(s, p->remx); s = put(s, p->remy);
    if (p->type == T_PLAYER) {
        s = putb(s, p->p_jump); s = putb(s, p->p_dash); s = put(s, FIX(p->grace));
        s = put(s, FIX(p->jbuffer)); s = put(s, FIX(p->djump)); s = put(s, FIX(p->dash_time));
        s = put(s, p->dash_effect_time); s = put(s, p->dtx); s = put(s, p->dty);
        s = put(s, p->dax); s = put(s, p->day); s = putb(s, p->flipx); s = put(s, p->spr);
        s = put(s, p->spr_off); s = putb(s, p->was_on_ground);
    } else {
        s = put(s, FIX(p->state)); s = put(s, FIX(p->delay)); s = put(s, p->spr);
        s = put(s, FIX(p->x)); s = put(s, FIX(p->ty));       // target = the spawn tile
    }
}

// name of the n-th space-separated field (0 = the frame number)
static const char *field_name(const char *line, int n)
{
    const char *kind = "";
    int i = 0;
    const char *t = line;
    if (n == 0) return "frame";
    for (; *t && i < 15; t++) if (*t == ' ') { i++; if (i == 15) kind = t + 1; }
    if (n <= 15) return COMMON[n - 1];
    if (*kind == 'P' && n - 16 < (int)(sizeof(PFIELDS) / sizeof(*PFIELDS))) return PFIELDS[n - 16];
    if (*kind == 'S' && n - 16 < (int)(sizeof(SFIELDS) / sizeof(*SFIELDS))) return SFIELDS[n - 16];
    return "?";
}

static void first_diff(const char *ref, const char *got, u16 f)
{
    char a[1024], b[1024], *sa, *sb, *ta, *tb;
    int n = 0;
    strcpy(a, ref); strcpy(b, got);
    ta = strtok_r(a, " ", &sa); tb = strtok_r(b, " ", &sb);
    while (ta || tb) {
        if (!ta || !tb || strcmp(ta, tb)) {
            printf("  first difference: frame %u, field %d (%s): cart %s, port %s\n", f, n,
                   field_name(ref, n), ta ? ta : "(none)", tb ? tb : "(none)");
            return;
        }
        ta = strtok_r(NULL, " ", &sa); tb = strtok_r(NULL, " ", &sb); n++;
    }
}

// coverage of the mechanics, read from the traced state (see the gate in main)
enum { C_JUMP, C_WALLJUMP, C_DASH8, C_DASH0 = C_DASH8 + 8, C_DASHNONE, C_DEATH, C_RESPAWN,
       C_FAKEWALL, C_FRUIT, C_REFILL, C_CLAMP, C_EXIT, C_N };
static u8 cov[C_N];
static u16 cov_frames;
static void cover(void)
{
    static const s8 dir[3][3] = { { 5, 4, 3 }, { 6, -1, 2 }, { 7, 0, 1 } };   // [dy+1][dx+1]
    u8 k;
    Obj *p = RT_NULL;
    for (k = 0; k < S.nobj; k++)
        if (S.pool[S.order[k]].type == T_PLAYER) p = &S.pool[S.order[k]];
    for (k = 0; k < S.nsfx; k++)
        switch (S.sfx_ev[k]) {
        case 1: cov[C_JUMP] = 1; break;
        case 2: cov[C_WALLJUMP] = 1; break;
        case 9: cov[C_DASHNONE] = 1; break;
        case 0: cov[C_DEATH] = 1; break;
        case 16: cov[C_FAKEWALL] = 1; break;
        case 13: cov[C_FRUIT] = 1; break;
        case 54: cov[C_REFILL] = 1; break;
        case 3:
            if (p) {
                s8 d = dir[(p->spdy > 0) - (p->spdy < 0) + 1][(p->spdx > 0) - (p->spdx < 0) + 1];
                if (p->dtx == 0 && p->dty == 0) d = -1;
                if (input_held(K_LEFT | K_RIGHT | K_UP | K_DOWN) == 0) cov[C_DASH0] = 1;
                else if (d >= 0) cov[C_DASH8 + d] = 1;
            }
            break;
        case 4: if (S.deaths) cov[C_RESPAWN] = 1; break;
        }
    if (p && (p->x == -1 || p->x == 121)) cov[C_CLAMP] = 1;
    if (S.mode == M_END) cov[C_EXIT] = 1;
}

static int trace_test(const char *name, u16 scenario, u16 frames)
{
    char path[256], ref[1024], got[1024];
    SwScript sc;
    FILE *f;
    u16 fr;
    int bad = 0;
    snprintf(path, sizeof(path), "keys/%s.txt", name);
    if (sw_load_script(&sc, path)) { printf("FAIL %s: no %s\n", name, path); return 1; }
    snprintf(path, sizeof(path), "traces/%s.txt", name);
    if (!(f = fopen(path, "r"))) { printf("FAIL %s: no %s (make traces)\n", name, path); return 1; }
    sw_init(scenario);
    for (fr = 0; fr < frames; fr++) {
        sw_step(sw_script_keys(&sc, fr));
        cover();
        if (S.mode != M_PLAY) break;   // exit at the top: the cart goes on to room 1
        trace_line(got, fr);
        if (!fgets(ref, sizeof(ref), f)) { printf("FAIL %s: reference ends at frame %u\n", name, fr); bad = 1; break; }
        ref[strcspn(ref, "\n")] = 0;
        if (strcmp(ref, got)) {
            printf("FAIL %s: frame %u differs\n  cart %s\n  port %s\n", name, fr, ref, got);
            first_diff(ref, got, fr);
            bad = 1;
            break;
        }
    }
    fclose(f);
    cov_frames = fr;
    if (S.overflow || S.unsupported) { printf("FAIL %s: overflow %d unsupported %d\n", name, S.overflow, S.unsupported); bad = 1; }
    if (!bad) printf("ok   %-9s scenario %u, %4u frames identical to the cart (%d deaths%s)\n",
                     name, scenario, fr, S.deaths, S.mode == M_END ? ", exit" : "");
    return bad;
}

// all()/foreach() semantics, against p8shim.lua's all() worked by hand
static u8 seen[16], nseen;
static Obj *victim;
static void visit_del_self(Obj *o) { seen[nseen++] = (u8)(o - S.pool); if (nseen == 2) destroy_object(o); }
static void visit_del_later(Obj *o) { seen[nseen++] = (u8)(o - S.pool); if (nseen == 1) destroy_object(victim); }

int main(int argc, char **argv)
{
    char name[64], line[128];
    unsigned scen, frames, nscripts = 0;
    int traced = 0;
    FILE *list;
    if (argc == 5 && !strcmp(argv[1], "--hash")) {   // --hash script scenario frames: per-frame
        SwScript sc;                                  // state hash, ti-cycles' value format
        u16 fr, n = (u16)atoi(argv[4]);
        char path[256];
        snprintf(path, sizeof(path), "keys/%s.txt", argv[2]);
        if (sw_load_script(&sc, path)) return 1;
        sw_init((u16)atoi(argv[3]));
        for (fr = 0; fr < n; fr++) {
            sw_step(sw_script_keys(&sc, fr));
            printf("(0x%x)\n", (unsigned)state_hash());
        }
        return 0;
    }
    if (!(list = fopen("scripts.txt", "r"))) { printf("FAIL no scripts.txt\n"); return 1; }
    while (fgets(line, sizeof(line), list))
        if (line[0] != '#' && sscanf(line, "%63s %u %u", name, &scen, &frames) == 3) {
            int b = trace_test(name, (u16)scen, (u16)frames);
            fails += b;
            nscripts++;
            if (!b) traced += cov_frames;
        }
    fclose(list);
    printf("trace tests: %u scripts, %d frames compared field by field\n", nscripts, traced);

    // coverage gate: every mechanic of the slice happened in some script (else the traces prove
    // nothing about it)
    {
        static const char *what[] = { "jump", "wall jump", "dash right", "dash up-right", "dash up",
            "dash up-left", "dash left", "dash down-left", "dash down", "dash down-right",
            "dash without direction", "dash with none left", "death", "respawn", "fake wall broken",
            "fruit", "dash refill on landing", "screen edge clamp", "exit at the top" };
        unsigned c;
        for (c = 0; c < sizeof(what) / sizeof(*what); c++)
            if (!cov[c]) { printf("FAIL coverage: no script shows \"%s\"\n", what[c]); fails++; }
    }

    // unit: foreach deleting the current element visits every element once (spawn -> player);
    // deleting a later one visits the current element again (Lua: a, a, b)
    {
        Obj *a, *b, *c;
        u8 ia, ib, ic;
        sw_init(0);
        while (S.nobj) destroy_object(&S.pool[S.order[0]]);
        a = init_object(T_TITLE, 0, 0); b = init_object(T_TITLE, 0, 0); c = init_object(T_TITLE, 0, 0);
        ia = (u8)(a - S.pool); ib = (u8)(b - S.pool); ic = (u8)(c - S.pool);
        nseen = 0;
        for_all(visit_del_self);
        CHECK(nseen == 3 && seen[0] == ia && seen[1] == ib && seen[2] == ic && S.nobj == 2);
        sw_init(0);
        while (S.nobj) destroy_object(&S.pool[S.order[0]]);
        a = init_object(T_TITLE, 0, 0); b = init_object(T_TITLE, 0, 0); victim = init_object(T_TITLE, 0, 0);
        ia = (u8)(a - S.pool); ib = (u8)(b - S.pool);
        nseen = 0;
        for_all(visit_del_later);
        CHECK(nseen == 3 && seen[0] == ia && seen[1] == ia && seen[2] == ib);
    }
    // unit: pool overflow is reported, never written past
    {
        u16 k;
        u8 busy = 0;
        sw_init(1);
        for (k = 0; k < 2 * MAXOBJ; k++) init_object(T_SMOKE, 0, 0);
        for (k = 0; k < MAXOBJ; k++) busy += S.used[k] != 0;
        CHECK(S.overflow == 1 && busy == MAXOBJ && init_object(T_SMOKE, 0, 0) == RT_NULL);
    }
    // unit: injection door 1 = a player standing on the spawn tile, list [wall, title, player]
    {
        Obj *p;
        sw_init(1);
        p = &S.pool[S.order[2]];
        CHECK(S.nobj == 3 && p->type == T_PLAYER && p->x == 8 && p->y == 96);
        CHECK(S.pool[S.order[0]].type == T_FAKEWALL && S.pool[S.order[1]].type == T_TITLE);
    }
    // unit: a fruit got in room 0 keeps the fake wall away when the room reloads (if_not_fruit)
    {
        SwScript sc;
        u16 fr;
        u8 k, walls = 0;
        sw_load_script(&sc, "keys/fakewall.txt");
        sw_init(1);
        for (fr = 0; fr < 160; fr++) sw_step(sw_script_keys(&sc, fr));
        CHECK(S.got_fruit == 1);
        S.will_restart = 1; S.delay_restart = 1;
        sw_step(0);
        for (k = 0; k < S.nobj; k++) walls += S.pool[S.order[k]].type == T_FAKEWALL;
        CHECK(walls == 0 && S.pool[S.order[0]].type == T_SPAWN);
    }
    // unit: the exit at the top opens the "end of demo" screen; [ENTER] plays room 0 again from
    // the start (deaths, time and berries reset), [ESC] still quits
    {
        SwScript sc;
        u16 fr;
        sw_load_script(&sc, "keys/exit.txt");
        sw_init(0);
        for (fr = 0; fr < 100 && S.mode == M_PLAY; fr++) sw_step(sw_script_keys(&sc, fr));
        CHECK(S.mode == M_END && fr == 94);
        sw_step(0); sw_step(0);
        CHECK(S.mode == M_END);
        sw_step(K_ENTER);
        CHECK(S.mode == M_PLAY && S.deaths == 0 && S.got_fruit == 0 && S.nobj == 3 &&
              S.pool[S.order[1]].type == T_SPAWN);
        sw_init(0);
        for (fr = 0; fr < 100 && S.mode == M_PLAY; fr++) sw_step(sw_script_keys(&sc, fr));
        CHECK(sw_step(K_ESC) == 0);
    }
    // unit: ESC quits
    sw_init(0);
    CHECK(sw_step(0) == 1 && sw_step(K_ESC) == 0);

    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
