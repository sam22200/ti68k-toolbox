#ifndef FFT_MANAGEMENT_H
#define FFT_MANAGEMENT_H
#include "../../../runtime/core/rt.h"
enum { UI_FORMATION, UI_CONTEXT, UI_ITEM, UI_EQUIP, UI_ABILITY, UI_LEARN,
       UI_JOBS, UI_STATUS, UI_DIALOGUE, UI_SET, UI_CONFIG, UI_LOCATION, UI_SCREEN_N };
enum { LAYOUT_CLASSIC, LAYOUT_ROSTER, LAYOUT_CARD };
typedef struct { char name[13], job[9]; u8 v[11]; const char *eq[5], *slots[5]; u8 icons[5]; } MenuUnit;
typedef struct {
    u16 tick;
    u8 screen, layout, unit, row, compact, reveal, repeat, notice, dialog_at, dialog_light;
    u8 small_actors, /* 0: normal, 1:4x3, 2:5x3 */ job_confirm, job[5], title_at, title_clock, stress, roster_demo;
} MenuState;
extern MenuState menu;
#endif
