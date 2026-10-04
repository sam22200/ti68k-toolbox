// Bubble Ghost trace tests: each key script (logic frames) runs on the C port from the ROM's
// own memory at the door (build/door_NN.bin, tools/door.py), and the RAM is printed per logic
// frame in gbtrace.py's format (named variables, then Fletcher-16 hashes of whole regions),
// compared line by line with the ROM under PyBoy (traces/<name>.txt, `make traces`).
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "vars.h"
#include "flow.h"
#include "play.h"
#include "render.h"
#include "screens.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

typedef struct { const char *label; u16 a; u8 size; } Var;
static const Var VARS[] = {
    { "hall_id", 0xC0AC, 1 }, { "lives", 0xC0AF, 1 }, { "travel_dir", 0xC0B0, 1 },
    { "dpad_dir", 0xC0F1, 1 }, { "frame_count", 0xC0F2, 1 }, { "hall_end", 0xC0F3, 1 },
    { "score", 0xC0FB, 2 }, { "hiscore", 0xC0FD, 2 }, { "bonus", 0xC129, 1 },
    { "keys_held", 0xFF8B, 1 }, { "keys_pressed", 0xFF8C, 1 },
    { "ghost_state", 0xC15F, 1 }, { "ghost_timer", 0xC160, 1 }, { "ghost_x", 0xC161, 1 },
    { "ghost_y", 0xC162, 1 }, { "ghost_anim", 0xC163, 1 }, { "ghost_dir", 0xC164, 1 },
    { "ghost_near", 0xC167, 1 }, { "bubble_state", 0xC168, 1 }, { "bubble_timer", 0xC169, 1 },
    { "bubble_x", 0xC16A, 1 }, { "bubble_y", 0xC16B, 1 }, { "bubble_speed", 0xC16F, 1 },
    { "bubble_fx", 0xC170, 2 }, { "bubble_fy", 0xC172, 2 },
    { "LCDC", 0xFF40, 1 }, { "SCY", 0xFF42, 1 }, { "BGP", 0xFF47, 1 }, { "WY", 0xFF4A, 1 },
    { "WX", 0xFF4B, 1 },
};
// whole regions (C0F0, the VBlank counter, depends on the loading time: left out; C0E7, the
// VBlank job flags, too: the GB's logic sometimes overruns a VBlank (fuzz_06 frame 475: the
// interrupt runs mid-frame, clears it and sets FF91), a CPU-time effect no port reproduces; DE60-DFFF and
// the stack: the sound driver (DE60-DFFF) and the CPU stack (FFD0-FFFE))
typedef struct { const char *label; u16 lo, hi; } Region;
static const Region REGIONS[] = {
    { "OAM shadow", 0xC000, 0xC09F }, { "vars C0A0-C0E6", 0xC0A0, 0xC0E6 },
    { "vars C0E8-C0EF", 0xC0E8, 0xC0EF },
    { "vars C0F1-C2FE (hazards, objects)", 0xC0F1, 0xC2FE }, { "collision mask", 0xC2FF, 0xCA7F },
    { "hall pack CB00-D7FF", 0xCB00, 0xD7FF }, { "WRAM D800-DE5F", 0xD800, 0xDE5F },
    { "HRAM FF8B-FF90", 0xFF8B, 0xFF90 }, { "HRAM FF92-FFCF", 0xFF92, 0xFFCF },
};
#define NVARS (int)(sizeof(VARS) / sizeof(*VARS))
#define NREG (int)(sizeof(REGIONS) / sizeof(*REGIONS))

static u16 fletcher(u16 lo, u16 hi)
{
    u16 a = 0, b = 0;
    for (;;) { a = (u16)((a + rd(lo)) % 255); b = (u16)((b + a) % 255); if (lo++ == hi) break; }
    return (u16)(b << 8 | a);
}

static void print_specs(void)
{
    int k;
    for (k = 0; k < NVARS; k++) printf("%04x%s,", VARS[k].a, VARS[k].size == 2 ? ":2" : "");
    for (k = 0; k < NREG; k++) printf("%04x-%04x:h%s", REGIONS[k].lo, REGIONS[k].hi, k + 1 < NREG ? "," : "\n");
}

static const char *label(int n)
{
    if (n == 0) return "frame";
    if (n <= NVARS) return VARS[n - 1].label;
    n -= NVARS + 1;
    return n < NREG ? REGIONS[n].label : "?";
}

static void trace_line(char *s, u16 fr)
{
    int k;
    s += sprintf(s, "%u", fr);
    for (k = 0; k < NVARS; k++) {
        const Var *v = &VARS[k];
        if (v->size == 1) s += sprintf(s, " %04x=%02x", v->a, rd(v->a));
        else s += sprintf(s, " %04x=%04x", v->a, rd(v->a) | rd((u16)(v->a + 1)) << 8);
    }
    for (k = 0; k < NREG; k++) s += sprintf(s, " %04x-%04x=%04x", REGIONS[k].lo, REGIONS[k].hi, fletcher(REGIONS[k].lo, REGIONS[k].hi));
}

static void first_diff(const char *ref, const char *got, u16 f)
{
    static char a[4096], b[4096];
    char *sa, *sb, *ta, *tb;
    int n = 0;
    strcpy(a, ref); strcpy(b, got);
    ta = strtok_r(a, " ", &sa); tb = strtok_r(b, " ", &sb);
    while (ta || tb) {
        if (!ta || !tb || strcmp(ta, tb)) {
            printf("  first difference: frame %u, field %d (%s): ROM %s, port %s\n", f, n,
                   label(n), ta ? ta : "(none)", tb ? tb : "(none)");
            return;
        }
        ta = strtok_r(NULL, " ", &sa); tb = strtok_r(NULL, " ", &sb); n++;
    }
}

static u8 joy_of(u32 k)                // runtime keys -> GB joypad, as gbtrace --map
{
    u8 j = 0;
    if (k & K_A) j |= J_A;
    if (k & K_B) j |= J_B;
    if (k & K_C) j |= J_START;
    if (k & K_D) j |= J_SELECT;
    if (k & K_RIGHT) j |= J_RIGHT;
    if (k & K_LEFT) j |= J_LEFT;
    if (k & K_UP) j |= J_UP;
    if (k & K_DOWN) j |= J_DOWN;
    return j;
}

// the ROM's memory at the door of hall n (0 = a new game): WRAM, VRAM, HRAM, I/O, OAM
static int load_door(u16 n)
{
    char path[64];
    static u8 buf[0x2000 + 0x2000 + 0x80 + 0x50 + 0xA0];
    FILE *f;
    snprintf(path, sizeof(path), "build/door_%02u.bin", n);
    if (!(f = fopen(path, "rb")) || fread(buf, 1, sizeof(buf), f) != sizeof(buf)) {
        printf("FAIL no %s (make doors)\n", path);
        if (f) fclose(f);
        return 1;
    }
    fclose(f);
    gb_reset();
    memcpy(gb_wram, buf, 0x2000);
    memcpy(gb_vram, buf + 0x2000, 0x2000);
    memcpy(gb_hram, buf + 0x4000, 0x80);
    memcpy(gb_io, buf + 0x4080, 0x50);
    memcpy(gb_oam, buf + 0x40D0, 0xA0);
    vram_dirty(0);
    set_main(door_loop);
    return 0;
}

// coverage of the mechanics, read from memory after each logic frame
enum { C_BLOW, C_DIAG, C_DECAY, C_REST, C_POP, C_DEATH, C_RETRY, C_EXIT, C_NEXT, C_BONUS,
       C_HAZARD, C_N };
static u8 cov[C_N];
static u8 prev[0x200];                 // C000-C1FF at the previous sample
static void cover(void)
{
#define P(a) prev[(a) - 0xC000]
    if (b_state == 1 && P(0xC168) != 1) cov[C_BLOW] = 1;
    if (b_state == 1 && !(b_dir & 1)) cov[C_DIAG] = 1;
    if (b_speed < P(0xC16F) && b_state == 1) cov[C_DECAY] = 1;
    if (P(0xC168) == 1 && b_state == 0) cov[C_REST] = 1;
    if (b_state == 2 && P(0xC168) != 2) cov[C_POP] = 1;
    if (hall_end == 2) cov[C_DEATH] = 1;
    if (P(0xC0F3) == 2 && lives < P(0xC0AF)) cov[C_RETRY] = 1;
    if (hall_end == 1) cov[C_EXIT] = 1;
    if (hall_id != P(0xC0AC)) cov[C_NEXT] = 1;
    if (W16(SCORE) > (u16)(P(0xC0FB) | P(0xC0FC) << 8)) cov[C_BONUS] = 1;
    if (W8(HAZARDS) && W8(HAZARDS) != 0xFF) cov[C_HAZARD] = 1;
#undef P
}

static int shot_fr = -1;               // --shot SCRIPT SCEN FRAME OUT.png: the screen there
static const char *shot_path;
#define POWER_ON 99                    // scripts.txt scenario 99: power-on; 100 + n: hall n's door;
                                       // both sampled at every key read (04A4), screens included
static char got[4096];
static u16 got_fr;
static u8 sampled, read_joy;
static void sample(void)
{
    if (sampled) return;               // one sample per call of trace_test's loop
    if (got_fr == shot_fr) {           // the picture of this frame: OAM and BG of the one before
        rt_light = sw_planes[0];
        rt_dark = sw_planes[1];
        game_render();
        sw_write_png(shot_path, 1);
    }
    cover();
    memcpy(prev, gb_wram, sizeof(prev));
    trace_line(got, got_fr);
    sampled = 1;
    gb_joy = read_joy;                 // the read that follows sees this sample's keys
}

static int dump_fr = -1;               // --dump SCRIPT FRAME LO HI: the port's bytes there

static u16 dump_lo, dump_hi;

static int frames_done;
static int trace_test(const char *name, u16 scenario, u16 frames)
{
    char path[256];
    static char ref[4096];
    SwScript sc;
    FILE *f;
    u16 fr;
    int bad = 0;
    snprintf(path, sizeof(path), "keys/%s.txt", name);
    if (sw_load_script(&sc, path)) { printf("FAIL %s: no %s\n", name, path); return 1; }
    snprintf(path, sizeof(path), "traces/%s.txt", name);
    if (!(f = fopen(path, "r"))) { printf("FAIL %s: no %s (make traces)\n", name, path); return 1; }
    if (scenario == POWER_ON) {                                // from power-on, sampled at
        gb_reset();                                            // every key read (04A4)
        set_main(r_0150);
    } else if (load_door(scenario >= 100 ? scenario - 100 : scenario)) { fclose(f); return 1; }
    memcpy(prev, gb_wram, sizeof(prev));
    for (fr = 0; fr < frames; fr++) {
        u8 joy = joy_of(sw_script_keys(&sc, fr));
        u16 v = 0;
        got_fr = fr;
        sampled = 0;
        if (scenario >= POWER_ON) { read_hook = sample; read_joy = joy; }
        else logic_hook = sample;
        while (!sampled && ++v < 3000 && !door_end) gb_vblank(joy);
        logic_hook = read_hook = RT_NULL;
        if (!fgets(ref, sizeof(ref), f)) { printf("FAIL %s: reference ends at frame %u\n", name, fr); bad = 1; break; }
        ref[strcspn(ref, "\n")] = 0;
        if (!sampled) {                                        // no more logic frames
            if (!strncmp(ref, "# stalled", 9)) { printf("     %s: frame %u, the game left play (end %u), as the ROM\n", name, fr, door_end); break; }
            printf("FAIL %s: no logic frame at %u (end %u), the ROM goes on\n", name, fr, door_end);
            bad = 1;
            break;
        }
        if (fr == dump_fr) {
            u16 a;
            for (a = dump_lo; a <= dump_hi; a++) printf("%04x=%02x%c", a, rd(a), (a - dump_lo) % 16 == 15 ? '\n' : ' ');
            printf("\n");
        }
        if (strcmp(ref, got)) {
            printf("FAIL %s: frame %u differs\n", name, fr);
            first_diff(ref, got, fr);
            bad = 1;
            break;
        }
    }
    fclose(f);
    if (!bad) {
        printf("ok   %-10s scenario %2u, %5u logic frames identical to the ROM (hall %u, lives %u, score %u0)\n",
               name, scenario, fr, hall_id - 1, lives, W16(SCORE));
        frames_done += fr;
    }
    return bad;
}

int main(int argc, char **argv)
{
    char name[64], line[128];
    unsigned scen, frames, nscripts = 0;
    FILE *list;
    if (argc == 2 && !strcmp(argv[1], "--vars")) { print_specs(); return 0; }
    game_init();
    if (argc == 5 && !strcmp(argv[1], "--hash")) {   // --hash script scenario frames: the
        SwScript sc;                                  // runtime's frames (game_update, two
        u16 fr, n = (u16)atoi(argv[4]);               // VBlanks each), ti-cycles' value format
        char path[256];
        snprintf(path, sizeof(path), "keys/%s.txt", argv[2]);
        if (sw_load_script(&sc, path)) return 1;
        game_scenario((u16)atoi(argv[3]));
        for (fr = 0; fr < n; fr++) {
            rt_prev = rt_keys;
            rt_keys = sw_script_keys(&sc, fr);
            game_update();
            printf("(0x%x)\n", (unsigned)state_hash());
        }
        return 0;
    }
    if (argc == 6 && !strcmp(argv[1], "--shot")) {
        shot_fr = atoi(argv[4]);
        shot_path = argv[5];
        trace_test(argv[2], (u16)atoi(argv[3]), (u16)(shot_fr + 1));
        return 0;
    }
    if (argc == 7 && !strcmp(argv[1], "--dump")) {
        dump_fr = atoi(argv[4]);
        dump_lo = (u16)strtol(argv[5], 0, 16);
        dump_hi = (u16)strtol(argv[6], 0, 16);
        return trace_test(argv[2], (u16)atoi(argv[3]), (u16)(dump_fr + 1));
    }
    if (!gb_rom) { printf("FAIL no bgrom.bin (make)\n"); return 1; }
    if (!(list = fopen("scripts.txt", "r"))) { printf("FAIL no scripts.txt\n"); return 1; }
    while (fgets(line, sizeof(line), list))
        if (line[0] != '#' && sscanf(line, "%63s %u %u", name, &scen, &frames) == 3) {
            fails += trace_test(name, (u16)scen, (u16)frames);
            nscripts++;
        }
    fclose(list);
    printf("trace tests: %u scripts, %d logic frames compared (variables and whole RAM regions)\n", nscripts, frames_done);
    {
        static const char *what[] = { "a blow", "a diagonal blow", "the bubble slowing down",
            "the bubble stopping", "a pop", "a death", "a retry with one life less", "an exit",
            "the next hall", "the hall bonus", "a hazard" };
        unsigned c;
        for (c = 0; c < C_N; c++)
            if (!cov[c]) { printf("FAIL coverage: no script shows %s\n", what[c]); fails++; }
    }
    if (fails) { printf("%d FAILED\n", fails); return 1; }
    printf("all tests passed\n");
    return 0;
}
