// Cached menu compositor; RESULTS owns the native entry.
// Cached one part per frame: raised title/frame, then each line. Glove sways into the box.
#ifdef __m68k__
#include <alloc.h>
#include <vat.h>
#include <stdio.h>
// GCC4TI's strputchar is an external library callback, unsuitable for a PIC
// image. Keep the same ROM formatter with a local C callback, no relocation.
static void __ATTR_TIOS_CALLBACK__ heap_log_putchar(char c,void **out){*(*(char **)out)++=c;}
// GCC4TI's whole-program pass otherwise inlines this variadic function and
// leaves va_start pointing at make_window's frame instead of its arguments.
static void tilog(const char *fmt,...) __attribute__((noinline));
#define strputchar heap_log_putchar
#include "../../lib/tilog.h"
#undef strputchar
#endif
static u8 win_lines(u8 m)
{
    u8 n = 0;
    if (m == M_MENU) return MN_N;
    if (m == M_AUTO) return 3;
    if (m == M_QUIT) return 3;
    if (m == M_ORDER) { while (n < ORDER_N && st.turns[n] != 0xFF) n++; return n; }
    if (m == M_ACTS) return (u8)(2 + ACTS_TWO(W->win_k[2]) + st.unit[W->win_k[2]].defend
#ifdef FFT_CHAPTER
        +cj_support(common,st.unit[W->win_k[2]].job,st.support[W->win_k[2]],480)
#endif
    );
    if (m == M_ACT || m == M_PICK) return 1;
    while (n < SKILL_ROWS && known_cmd(W->win_k[2], (u8)(n + (W->win_k[1] & 0x7F))) != NO_ABIL) n++; // M_SKILLS
    return n;
}


#include "window_label.inc"
static void make_window(void)
{

    u16 *l = W->win[0], *d = W->win[1];
    const Unit *u = &st.unit[W->win_k[2]];
    u8 r, i, k, m = W->win_k[0], c = C_BLACK;
    char s[28];
next_part:
    k=W->win_n++;
    if (!k) {
#ifdef __m68k__
        // RAM-only opt-in, consumed once. No allocation, ABI field or ordinary
        // per-frame heap scan; the production program is exactly what is measured.
        if(st.dbg&DBG_HEAP){st.dbg&=(u8)~DBG_HEAP;tilog("battle heap free %lu max %lu",HeapAvail(),HeapMax());}
#endif
        u8 h = win_lines(m) * 6 + (m == M_MENU || m == M_AUTO || m == M_ACTS || m == M_SKILLS || m == M_QUIT ? 15 : 11), w = 4;
        const char *title = m == M_MENU ? "Menu" : m == M_ACTS || m == M_ACT ? "Act"
            : m == M_AUTO ? "Auto-battle" : m == M_SKILLS ? SKILL_NAME(CMD_SET(u)) : m == M_ORDER ? "Turn order" : m == M_PICK ? "Found" : "Quit?";
#ifdef FFT_CHAPTER
        if(m==M_PICK&&W->trap_message)title="Trap";
#endif
        u8 pixels=64,offset=0;
#ifndef FFT_CHOICE_LEGACY_WIDTH
        if(m==M_MENU||m==M_ACTS) {
            u8 widest=0;
            for(r=0;r<win_lines(m);r++) {
                u8 width;W->win_n=(u8)(r+2);window_label(s);width=ui_width(s);if(width>widest)widest=width;
            }
            W->win_n=1;pixels=(u8)(widest+12);if(pixels>64)pixels=64;
            offset=(u8)(64-pixels);
        }
#endif
        W->win_w = pixels; W->win_h = h; W->win_m = (u8)(win_lines(m) + 1);
        // Keep the existing strided cache and incremental one-row-per-part schedule.
        for(r=0;r<WIN_ROWS;r++)for(i=0;i<w;i++) {
            WR16((u8 *)l,r*(RT_PBYTES/2)+i,0);
            WR16((u8 *)d,r*(RT_PBYTES/2)+i,0);
        }
        {
            ChoiceLayout menu={0,0,64,0,3,CHOICE_TAB,3,1,0,6,10,6,160,-16,7};
            void *sl=rt_light,*sd=rt_dark;menu.height=h;menu.x=offset;menu.width=pixels;
            rt_light=l;rt_dark=d;choice_frame(&menu,title);rt_light=sl;rt_dark=sd;
        }
        if (m == M_SKILLS) {
            // Cached with the page: triangles occupy its unused top/bottom margin.
            void *sl = rt_light, *sd = rt_dark;
            rt_light = l; rt_dark = d;
            for (i = 0; i < 2; i++) if (i ? known_cmd(W->win_k[2], (u8)((W->win_k[1] & 0x7F) + SKILL_ROWS)) != NO_ABIL : (W->win_k[1] & 0x7F) != 0) {
                u8 y = i ? h - 5 : 5;
                for (r = 0; r < 3; r++) {
                    u8 width = i ? 5 - 2 * r : 1 + 2 * r;
                    draw_rect(57 - (width >> 1), y + r, width, 1, C_BLACK);
                }
            }
            rt_light = sl; rt_dark = sd;
        }
        goto part_done;
    }
    c = window_label(s);
    k--;
    {
        ChoiceLayout menu={0,0,64,0,3,CHOICE_TAB,3,1,0,6,10,6,160,-16,7};
        void *sl=rt_light,*sd=rt_dark;
        if(m==M_MENU||m==M_ACTS){menu.x=(s16)(64-W->win_w);menu.width=W->win_w;}
        if(!(m==M_MENU||m==M_AUTO||m==M_ACTS||m==M_SKILLS||m==M_QUIT))menu.text_x=3;
        rt_light=l;rt_dark=d;choice_row(&menu,k,s,c);rt_light=sl;rt_dark=sd;
    }
part_done:
    // The high sub bit identifies Math's staged skill menu; complete it now.
    if(m==M_SKILLS&&(st.sub&128)&&W->win_n<W->win_m){W->left-=HUD_K;goto next_part;}

}

