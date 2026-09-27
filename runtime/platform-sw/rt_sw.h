// Software platform (PC): renders into ExtGraph-format planes in memory, bit-exact with the TI
// build on the visible 160x100 (checked by runtime/tests/xcheck). Used by platform-sdl (window)
// and directly by unit tests (no window, no SDL).
#ifndef RT_SW_H
#define RT_SW_H
#include "../core/rt.h"

extern u8 sw_planes[2][RT_PSIZE];      // light, dark (mono: light only)

void sw_init(u16 scenario);            // planes, game_init, game_scenario
u8 sw_step(u32 keys);                  // one frame with these keys held: update + render.
                                       // Returns game_update's result (0 = quit)
u8 sw_level(s16 x, s16 y);             // grey level 0..3 of a visible pixel (mono: 0 or 3)
u16 sw_checksum(void);                 // Fletcher-16 of the visible part of the planes
int sw_write_png(const char *path, int scale);
int sw_save_state(const char *path);   // raw rt_state bytes
int sw_load_state(const char *path);
u32 sw_parse_keys(const char *s);      // "UP A 5" -> K_UP | K_A | K_DIGIT(5)

// Input script: lines "<frame> <keys...>", the keys stay held from that frame to the next line.
typedef struct { u16 n, pos; u16 frame[256]; u32 keys[256]; } SwScript;
int sw_load_script(SwScript *s, const char *path);
u32 sw_script_keys(SwScript *s, u16 frame);

#endif
