// PC backend: SDL2 window on top of the software platform. Same frame rate as the calculator
// (256 / RT_FRAME_TICKS Hz), keys mapped to the TI-89 ones (rt.h).
// Options (all optional):
//   --scenario N   start in game_scenario(N) (the injection door)
//   --load F       load a raw state file after the scenario (made with --save or F2)
//   --keys F       input script ("<frame> <keys...>" lines), replaces the keyboard
//   --frames N     quit after N frames      --headless  no window, run as fast as possible
//   --shot F       write a PNG at the end   --save F    write the state at the end
//   --scale N      window zoom (default 4)  --seed N    PRNG seed before game_init
// In the window: F2 save state (<prog>.state), F3 load it, F12 screenshot (shot-NNN.png),
// Tab held = 8x speed, P pause, O one frame while paused.
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../platform-sw/rt_sw.h"

static u32 read_keyboard(void)
{
    const Uint8 *k = SDL_GetKeyboardState(RT_NULL);
    u32 keys = 0;
    int d;
    if (k[SDL_SCANCODE_UP]) keys |= K_UP;
    if (k[SDL_SCANCODE_LEFT]) keys |= K_LEFT;
    if (k[SDL_SCANCODE_DOWN]) keys |= K_DOWN;
    if (k[SDL_SCANCODE_RIGHT]) keys |= K_RIGHT;
    if (k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL] || k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_Z] || k[SDL_SCANCODE_W]) keys |= K_A;
    if (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT] || k[SDL_SCANCODE_X]) keys |= K_B;
    if (k[SDL_SCANCODE_C]) keys |= K_C;
    if (k[SDL_SCANCODE_V]) keys |= K_D;
    if (k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER]) keys |= K_ENTER;
    if (k[SDL_SCANCODE_ESCAPE]) keys |= K_ESC;
    for (d = 1; d <= 9; d++)             // keypad, or the number row (AZERTY: same scancodes)
        if (k[SDL_SCANCODE_KP_1 + d - 1] || k[SDL_SCANCODE_1 + d - 1]) keys |= K_DIGIT(d);
    return keys;
}

int main(int argc, char **argv)
{
    const char *load = 0, *keysf = 0, *shot = 0, *save = 0;
    long frames = -1;
    int headless = 0, scale = 4, k, shots = 0, paused = 0, step1 = 0;
    u16 scenario = 0;
    char statef[256];
    SwScript script;
    SDL_Window *win = 0;
    SDL_Renderer *ren = 0;
    SDL_Texture *tex = 0;
    Uint64 next = 0;

    for (k = 1; k < argc; k++) {
        const char *a = argv[k], *v = k + 1 < argc ? argv[k + 1] : "";
        if (!strcmp(a, "--scenario")) scenario = atoi(v), k++;
        else if (!strcmp(a, "--load")) load = v, k++;
        else if (!strcmp(a, "--keys")) keysf = v, k++;
        else if (!strcmp(a, "--frames")) frames = atol(v), k++;
        else if (!strcmp(a, "--shot")) shot = v, k++;
        else if (!strcmp(a, "--save")) save = v, k++;
        else if (!strcmp(a, "--scale")) scale = atoi(v), k++;
        else if (!strcmp(a, "--seed")) rt_seed = atoi(v), k++;
        else if (!strcmp(a, "--headless")) headless = 1;
        else { fprintf(stderr, "unknown option %s\n", a); return 2; }
    }
    snprintf(statef, sizeof(statef), "%s.state", argv[0]);
    if (keysf && sw_load_script(&script, keysf)) { fprintf(stderr, "cannot read %s\n", keysf); return 2; }

    sw_init(scenario);
    if (load && sw_load_state(load)) { fprintf(stderr, "cannot load %s\n", load); return 2; }

    if (!headless) {
        if (SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
        win = SDL_CreateWindow(argv[0], SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, RT_W * scale, RT_H * scale, 0);
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RT_W, RT_H);
        if (!win || !ren || !tex) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
        next = SDL_GetTicks64();
    }

    for (;;) {
        u32 keys = 0;
        int n = 1, j;
        if (frames >= 0 && rt_frame >= frames) break;
        if (!headless) {
            static const Uint32 pal[4] = { 0xFFD6DEC6, 0xFF9CA38E, 0xFF5A604F, 0xFF1C1F18 };
            SDL_Event e;
            Uint32 *pix;
            int pitch, x, y, quit = 0;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) quit = 1;
                if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                    SDL_Keycode c = e.key.keysym.sym;
                    if (c == SDLK_F2) printf(sw_save_state(statef) ? "save failed\n" : "saved %s\n", statef);
                    if (c == SDLK_F3) printf(sw_load_state(statef) ? "load failed\n" : "loaded %s\n", statef);
                    if (c == SDLK_F12) { char f[32]; snprintf(f, sizeof(f), "shot-%03d.png", shots++); sw_write_png(f, 4); printf("%s\n", f); }
                    if (c == SDLK_p) paused = !paused;
                    if (c == SDLK_o) step1 = 1;
                }
            }
            if (quit) break;
            keys = keysf ? sw_script_keys(&script, rt_frame) : read_keyboard();
            if (SDL_GetKeyboardState(RT_NULL)[SDL_SCANCODE_TAB]) n = 8;
            if (paused) n = step1, step1 = 0;
            for (j = 0; j < n; j++) {
                if (!sw_step(keys)) goto done;
                if (keysf) keys = sw_script_keys(&script, rt_frame);
            }
            SDL_LockTexture(tex, RT_NULL, (void **)&pix, &pitch);
            for (y = 0; y < RT_H; y++)
                for (x = 0; x < RT_W; x++) pix[y * (pitch / 4) + x] = pal[sw_level(x, y)];
            SDL_UnlockTexture(tex);
            SDL_RenderCopy(ren, tex, RT_NULL, RT_NULL);
            SDL_RenderPresent(ren);
            next += RT_FRAME_LEN(rt_frame - 1) * 1000 / RT_HZ;  // 31 ms at 8 ticks (32.3 fps)
            {
                Uint64 now = SDL_GetTicks64();
                if (next > now) SDL_Delay((Uint32)(next - now));
                else if (now - next > 200) next = now;          // too slow: no catch-up
            }
        } else {
            keys = keysf ? sw_script_keys(&script, rt_frame) : 0;
            if (!sw_step(keys)) break;
        }
    }
done:
    if (shot) sw_write_png(shot, 4);
    if (save && sw_save_state(save)) fprintf(stderr, "cannot save %s\n", save);
    printf("frames %u checksum %04X\n", rt_frame, sw_checksum());
    if (!headless) SDL_Quit();
    return 0;
}
