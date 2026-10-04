// Bubble Ghost: the GB memory, the VBlank interrupt, the waits, and the runtime hooks. One
// gb_vblank() = one GB VBlank: the interrupt's jobs (033A), then the main thread resumes until
// its next wait. The runtime runs two per 30.1 fps frame (decision 6).
#include "vars.h"
#include "play.h"
#include "loader.h"
#include "screens.h"
#include "render.h"
#include "hazards.h"
#include "flow.h"
#if defined(STATE_HASH) || defined(RT_CYCLES)
#include "../../tools/m68kbench/bench.h"
#endif

const u8 *gb_rom;
u8 *gb_wram, *gb_vram;
u8 gb_hram[0x80], gb_io[0x50];
u8 gb_ret;
u8 gb_joy;                             // the joypad this VBlank (FF8B layout)
u8 gb_oam[0xA0];                       // OAM as the last DMA left it (what the screen shows)

u8 rd_slow(u16 a)
{
    if (a < 0x8000) return gb_rom[a];
    if (a < 0xA000) return gb_vram[a - 0x8000];
    if (a >= 0xC000 && a < 0xE000) return gb_wram[a - 0xC000];
    if (a >= 0xE000 && a < 0xFE00) return gb_wram[a - 0xE000];  // echo RAM
    if (a >= 0xFE00 && a < 0xFEA0) return gb_oam[a - 0xFE00];    // the OAM the screen shows
    if (a >= 0xFF80) return gb_hram[a - 0xFF80];
    if (a >= 0xFF00 && a < 0xFF50) return gb_io[a - 0xFF00];
    return 0xFF;
}

void wr_slow(u16 a, u8 v)
{
    if (a >= 0x8000 && a < 0xA000) {
        if (gb_vram[a - 0x8000] != v) { gb_vram[a - 0x8000] = v; vram_dirty(a); }
    } else if (a >= 0xC000 && a < 0xE000) gb_wram[a - 0xC000] = v;
    else if (a >= 0xE000 && a < 0xFE00) gb_wram[a - 0xE000] = v;
    else if (a >= 0xFF80) gb_hram[a - 0xFF80] = v;
    else if (a >= 0xFF00 && a < 0xFF50) {
        gb_io[a - 0xFF00] = v;
    }
}

void fill(u16 dst, u8 v, u16 n) { while (n--) wr(dst++, v); }
void copy(u16 dst, u16 src, u16 n) { while (n--) wr(dst++, rd(src++)); }

// ---------------------------------------------------------------- input and waits
void (*read_hook)(void);               // tests: every key read (04A4), the power-on traces' sample

void read_keys(void)                   // 04A4
{
    if (read_hook) read_hook();
    keys_pressed = (u8)((keys_held ^ gb_joy) & gb_joy);
    keys_held = gb_joy;
    dpad_dir = R8(0x0494 + (gb_joy >> 4));
}

u8 wait_vbl(void)                      // 0390
{
    static Pt pt;
    PT_BEGIN(pt);
    vbl_count = 0;
    PT_WAIT1(pt);
    PT_END(pt);
}

static Pt pt_frames;
static u8 frames_left;
u8 wait_frames(u8 n)                   // 0387
{
    PT_BEGIN(pt_frames);
    frames_left = n;
    do PT_CALL(pt_frames, wait_vbl()); while (--frames_left);
    PT_END(pt_frames);
}

static Pt pt_start;
static u8 start_left;
u8 wait_start(u8 n)                    // 04F6
{
    PT_BEGIN(pt_start);
    start_left = n;
    for (;;) {
        read_keys();
        if (keys_pressed & J_START) { gb_ret = 1; PT_EXIT(pt_start); }
        PT_CALL(pt_start, wait_vbl());
        if (!--start_left) break;
    }
    gb_ret = 0;
    PT_END(pt_start);
}

void r_0379(u16 de)                    // 0379
{
    vbl_flags |= 4;
    w16(0xC0E8, de);
}

void oam_dma(void)                     // FF80 (also 0523: called directly)
{
    u32 *d = (u32 *)gb_oam;
    const u32 *s = (const u32 *)gb_wram;
    u8 k;
    for (k = 0; k < 0xA0 / 4; k++) *d++ = *s++;
}

void r_033a(void)                      // 033A the VBlank interrupt (the sound driver: not ported)
{
    u8 f = vbl_flags;
    vbl_flags = 0;
    if (f) {
        if (f & 4) {
            u16 cb = W16(0xC0E8);
            if (cb == 0xFF80) oam_dma();
            else if (cb == 0x1B01) r_1b01();
            else if (cb == 0x2130) r_2130();
            else if (cb == 0x213A) r_213a();
            else if (cb == 0x2F0B) r_2f0b();
            else screens_vbl_callback(cb);                     // 1D7C, 220C
        } else {
            if (f & 2) r_1af8();
            r_2e1d();
            oam_dma();
        }
    }
    vblank_done = 1;
    vbl_count++;
}

// ---------------------------------------------------------------- the main thread
static u8 (*gb_main)(void);            // r_main (0150, screens.c) or door_loop
u8 door_end;                           // door_loop: 0 playing, 1 game over, 2 won, 3 reset

void set_main(u8 (*f)(void)) { gb_main = f; }

// a door: the game from play_hall on (01ED, screens.c), as the ROM continues after the pokes
static Pt pt_door;
u8 door_loop(void)
{
    PT_BEGIN(pt_door);
    door_end = 0;
    PT_CALL(pt_door, r_01ed());
    door_end = 3;
    for (;;) PT_WAIT1(pt_door);
    PT_END(pt_door);
}

u8 gb_vblank(u8 joy)
{
    gb_joy = joy;
    gb_logic = 0;
#ifdef RT_CYCLES
    { static u8 named; if (!named) { BENCH_NAME(20, "update: VBlank jobs"); BENCH_NAME(21, "update: main thread"); named = 1; } }
    BENCH_BEGIN(20);
    if (H8(0xFFFF) & 1) r_033a(); else vbl_count++;
    BENCH_END(20);
    BENCH_BEGIN(21);
    if (gb_main) gb_main();
    BENCH_END(21);
#else
    // IE: the VBlank interrupt (033A: jobs, OAM DMA), or, while 055E has the LCD off, only the
    // timer one (039B: the counter the waits poll, and the sound driver)
    if (H8(0xFFFF) & 1) r_033a(); else vbl_count++;
    if (gb_main) gb_main();
#endif
    return gb_logic;
}

void gb_reset(void)
{
    u16 k;
    for (k = 0; k < 0x2000; k++) gb_wram[k] = gb_vram[k] = 0;
    for (k = 0; k < 0x80; k++) gb_hram[k] = 0;
    for (k = 0; k < 0x50; k++) gb_io[k] = 0;
    for (k = 0; k < 0xA0; k++) gb_oam[k] = 0;
    pt_frames = pt_start = pt_door = 0;
    door_end = 0;
    pt_reset_play();
    loader_reset();
    screens_reset();
    after_1f4b = RT_NULL;
    render_reset();
}

// ---------------------------------------------------------------- runtime hooks
void game_init(void)
{
    static u8 ok;
    if (!gb_wram) gb_wram = malloc(0x2000);
    if (!gb_vram) gb_vram = malloc(0x2000);
    gb_rom = rt_file("bgrom", RT_NULL);
    ok = gb_rom && gb_wram && gb_vram;
    if (ok) { gb_reset(); set_main(r_0150); }
    rt_state = RT_NULL;
    rt_state_size = 0;
}

// the injection door without a ROM dump: 0 = power-on (r_0150); n = hall n through the
// game's own start (01A3..01EA, as after the title) with the door's pokes
static u16 scen_hall;
static Pt pt_scen;
static u8 scenario_main(void)
{
    PT_BEGIN(pt_scen);
    r_1f4b();
    r_050b();
    r_03e2();
    W8(0xC0AD) = W8(0xC0AE) = 0xFF;
    hall_id = 2;
    travel_dir = 0;
    w16(SCORE, 0);
    W8(0xC0B2) = 2;
    r_1e74();
    r_055e();
    fill(SECRET_DONE, 0, 0x24);
    W8(VISITED) = W8(VISITED + 1) = 1;
    fill(VISITED + 2, 0, 0x23);
    lives = 5;
    music_req = 2;
    intro_flag = 1;
    if (scen_hall) {
        hall_id = (u8)(scen_hall + 1);
        travel_dir = scen_hall == 1 ? 0 : (scen_hall == 5 || scen_hall == 6 || scen_hall == 17
                     || scen_hall == 18 || scen_hall == 29 || scen_hall == 30) ? 7 : 3;
        intro_flag = 0;
    }
    PT_CALL(pt_scen, door_loop());
    PT_END(pt_scen);
}

// decision 14: the high score and the five-name table (C0FD..C126) kept in the file "bghsav":
// loaded after the power-on reset (1F4B), saved when the program quits
#define SAVE_LO 0xC0FD
#define SAVE_N 0x2A
static void load_scores(void) { rt_load("bghsav", gb_wram + (SAVE_LO - 0xC000), SAVE_N); }

void game_scenario(u16 n)
{
    if (!gb_rom || !gb_wram) return;
    gb_reset();
    pt_scen = 0;
    scen_hall = n;
    after_1f4b = load_scores;
    set_main(n ? scenario_main : r_0150);
}

// Hash of the game state (WRAM C000-DE5F without C0E7 and C0F0, HRAM FF8B-FFCF without FF91;
// a rotate-xor: no division on the 68000): the TI binary prints it per frame under ti-cycles
// (-DSTATE_HASH, `make tihash`), the PC test (--hash) too: the 68000 build checked against the
// PC build, itself checked against the ROM
u16 state_hash(void)
{
    u16 h = 0, k;
    for (k = 0; k < 0x1E60; k++)
        if (k != 0xE7 && k != 0xF0) h = (u16)(((h << 1) | (h >> 15)) ^ gb_wram[k]);
    for (k = 0x0B; k < 0x50; k++)
        if (k != 0x11) h = (u16)(((h << 1) | (h >> 15)) ^ gb_hram[k]);
    return h;
}

u8 game_update(void)
{
    u8 joy = 0, k;
    if (!gb_rom || !gb_wram) return !input_pressed(K_ESC);
    if (input_pressed(K_ESC)) {
        static u8 saved[SAVE_N];       // valid until the TI writes it at the exit
        for (k = 0; k < SAVE_N; k++) saved[k] = gb_wram[SAVE_LO - 0xC000 + k];
        rt_save("bghsav", saved, SAVE_N);
        return 0;
    }
    if (input_held(K_A | K_B)) joy |= J_A;
    if (input_held(K_ENTER)) joy |= J_START;
    if (input_held(K_RIGHT)) joy |= J_RIGHT;
    if (input_held(K_LEFT)) joy |= J_LEFT;
    if (input_held(K_UP)) joy |= J_UP;
    if (input_held(K_DOWN)) joy |= J_DOWN;
    for (k = 0; k < 2; k++) gb_vblank(joy);
#ifdef STATE_HASH
    BENCH_VALUE(state_hash());
#endif
    return 1;
}
