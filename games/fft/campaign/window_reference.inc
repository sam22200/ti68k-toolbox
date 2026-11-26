// Cached one part per frame: raised title / framed box, then each line. Hand sits outside.
static u8 win_lines(u8 m)
{
    u8 n = 0;
    if (m == M_MENU) return MN_N;
    if (m == M_AUTO) return 3;
    if (m == M_QUIT) return 3;
    if (m == M_ORDER) { while (n < ORDER_N && st.turns[n] != 0xFF) n++; return n; }
    if (m == M_ACTS) return (u8)(2 + st.unit[W->win_k[2]].defend);
    if (m == M_ACT || m == M_PICK) return 1;
    while (n < SKILL_ROWS && known(W->win_k[2], n + W->win_k[1]) != NO_ABIL) n++; // M_SKILLS
    return n;
}


#ifdef FFT_MODULES
static u8 window_label(char *s)
{
    ModCtx *c;mod_call(MOD_RESULTS,255,0);c=mod_context(MOD_RESULTS);c->window_out=s;
    return (u8)mod_call(MOD_RESULTS,1,0);
}
#else
#include "../window_label.inc"
#endif

static void make_window(void)
{

    u16 *l = W->win[0], *d = W->win[1];
    const Unit *u = &st.unit[W->win_k[2]];
    u8 r, i, k = W->win_n++, m = W->win_k[0], c = C_BLACK;
    char s[28];
    if (!k) {
        u8 h = win_lines(m) * 6 + (m == M_MENU || m == M_AUTO || m == M_ACTS || m == M_SKILLS || m == M_QUIT ? 15 : 11), w = 4;
        const char *title = m == M_MENU ? "Menu" : m == M_ACTS || m == M_ACT ? "Act"
            : m == M_AUTO ? "Auto-battle" : m == M_SKILLS ? SKILL_NAME(u->set) : m == M_ORDER ? "Turn order" : m == M_PICK ? "Found" : "Quit?";
        W->win_w = w; W->win_h = h; W->win_m = (u8)(win_lines(m) + 1);
        // Keep the existing strided cache and incremental one-row-per-part schedule.
        for(r=0;r<WIN_ROWS;r++)for(i=0;i<w;i++) {
            WR16((u8 *)l,r*(RT_PBYTES/2)+i,0);
            WR16((u8 *)d,r*(RT_PBYTES/2)+i,0);
        }
        {
            ChoiceLayout menu={0,0,64,0,3,CHOICE_TAB,3,1,0,6,10,6,160,-16,7};
            void *sl=rt_light,*sd=rt_dark;menu.height=h;
            rt_light=l;rt_dark=d;choice_frame(&menu,title);rt_light=sl;rt_dark=sd;
        }
        if (m == M_SKILLS || m == M_ORDER) {
            // Cached with the page: triangles occupy its unused top/bottom margin. The
            // turn list has six rows and no bottom margin, so its lower mark sits in the
            // last row's right edge, which no name reaches.
            void *sl = rt_light, *sd = rt_dark;
            rt_light = l; rt_dark = d;
            for (i = 0; i < 2; i++) if (m == M_ORDER
                    ? (i ? (u8)((u8)W->win_k[1] + ORDER_N) < (u8)(W->win_k[1] >> 8) : (u8)W->win_k[1] != 0)
                    : (i ? known(W->win_k[2], W->win_k[1] + SKILL_ROWS) != NO_ABIL : W->win_k[1] != 0)) {
                u8 y = i ? (u8)(h - (m == M_ORDER ? 4 : 5)) : 5;
                for (r = 0; r < 3; r++) {
                    u8 width = i ? 5 - 2 * r : 1 + 2 * r;
                    draw_rect(57 - (width >> 1), y + r, width, 1, C_BLACK);
                }
            }
            rt_light = sl; rt_dark = sd;
        }
        return;
    }
    c = window_label(s);
    k--;
    {
        ChoiceLayout menu={0,0,64,0,3,CHOICE_TAB,3,1,0,6,10,6,160,-16,7};
        void *sl=rt_light,*sd=rt_dark;
        if(!(m==M_MENU||m==M_AUTO||m==M_ACTS||m==M_SKILLS||m==M_QUIT))menu.text_x=3;
        rt_light=l;rt_dark=d;choice_row(&menu,k,s,c);rt_light=sl;rt_dark=sd;
    }
    if (m == M_ORDER && st.unit[st.turns[k]].team == TEAM_ENEMY) {
        // The source colours an enemy's row red; four greys cannot, so the dark-grey
        // name also carries the diamond the map already puts above an enemy's head.
        void *sl = rt_light, *sd = rt_dark;
        rt_light = l; rt_dark = d;
        for (i = 0; i < 5; i++) {
            u8 edge = i < 3 ? i : (u8)(4 - i);
            draw_rect((s16)(53 - edge), (s16)(11 + 6 * k + i), (u8)(1 + 2 * edge), 1, C_BLACK);
        }
        rt_light = sl; rt_dark = sd;
    }

}
