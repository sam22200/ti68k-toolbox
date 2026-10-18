// Presentation preferences: no tactical state or time advances while this menu is open.
#if !defined(FFT_FIXED)
#error FFT_CONFIG requires fixed inspection.
#endif
#ifdef FFT_TURN_TEXTURE
#error Textured rotations were retired from the configuration build to save archive space.
#endif

static u8 config_update(void)
{
    if (!W) return 0;
    if (st.config) {
        if (input_pressed(K_CATALOG | K_ESC | K_B)) {
            st.config = 0; W->hold = W->frz = 0;
            return 1;                       // consume close, including CATALOG's legacy APPS
        }
        if (input_pressed(K_UP | K_DOWN)) st.config_row ^= 1;
        if (input_pressed(K_A | K_ENTER | K_LEFT | K_RIGHT)) {
            if (!st.config_row) {
                st.pzoom ^= 1;
                if (st.perspective) {
                    st.pdist = st.pzoom ? 80 : 40; st.ppitch = st.pzoom ? 6 : 1;
                }
            } else st.pwire ^= 1;
            pvalid = 0;                     // defer expensive preparation until the menu closes
        }
        return 1;
    }
    if (input_pressed(K_CATALOG)) {
        if (!st.ai && !st.banner && !st.morph_mask && !st.turn && st.want == 0xFF && !st.ptrans
#ifdef FFT_CINEMA
            && !st.cphase
#endif
            && st.mode != M_ACT && st.mode != M_WALK && st.mode != M_END) {
            st.config = 1; st.config_row = 0;
            return 1;
        }
        rt_keys &= ~K_APPS;                 // a blocked CATALOG must not open the turn list
    }
    return 0;
}

static void config_draw(void)
{
    static const u8 rows[2] = {38, 59};
    const char *labels[2]; u8 values[2], i;
    labels[0] = st.pzoom ? "Wide view" : "Close view";
    labels[1] = st.pwire ? "Filled wireframe" : "Textured";
    values[0] = !st.pzoom; values[1] = st.pwire;
    draw_clear();
    draw_text(4,1,"CONFIGURATION",F_MEDIUM,C_BLACK); draw_rect(3,11,154,1,C_BLACK);
    draw_text(5,18,"PERSPECTIVE VIEW",F_SMALL,C_BLACK);
    for (i = 0; i < 2; i++) {
        u8 y = rows[i];
        if (st.config_row == i) {
            draw_rect(3,y-3,154,12,C_BLACK); draw_rect(4,y-2,152,10,C_LGRAY);
        }
        draw_rect(7,y-1,7,7,C_BLACK); draw_rect(8,y,5,5,C_WHITE);
        if (values[i]) draw_text(8,y,"x",F_SMALL,C_BLACK);
        draw_text(21,y,labels[i],F_SMALL,C_BLACK);
    }
    draw_text(4,93,"ENTER:toggle  ESC:back",F_SMALL,C_BLACK);
}
