// Presentation preferences: no tactical state or time advances while this menu is open.
#if defined(FFT_CONFIG) && !defined(FFT_FIXED)
#error FFT_CONFIG requires fixed inspection.
#endif
#ifdef FFT_TURN_TEXTURE
#error Textured rotations were retired from the configuration build to save archive space.
#endif

enum {
#ifdef FFT_REACH_CONTOUR
    CONFIG_MOVE,
#endif
#ifdef FFT_CONFIG
    CONFIG_ZOOM, CONFIG_MESH,
#endif
    CONFIG_ROWS
};

static u8 config_update(void)
{
    if (!W) return 0;
    if (st.config) {
        if (input_pressed(K_CATALOG | K_ESC | K_B)) {
            st.config = 0; W->hold = W->frz = 0;
            return 1;                       // consume close, including CATALOG's legacy APPS
        }
#ifdef FFT_CONFIG
        if (input_pressed(K_DOWN)) {
            if(++st.config_row==CONFIG_ROWS)st.config_row=0;
        }else if(input_pressed(K_UP))st.config_row=st.config_row ? st.config_row-1 : CONFIG_ROWS-1;
#endif
        if (input_pressed(K_A | K_ENTER | K_LEFT | K_RIGHT)) {
#ifdef FFT_REACH_CONTOUR
            if(st.config_row==CONFIG_MOVE){
                if(contour_gfx){st.reach_contour^=1;hl_seq++;}
            }
#endif
#ifdef FFT_CONFIG
            if(st.config_row==CONFIG_ZOOM){
                st.pzoom ^= 1;
                if (st.perspective) {
                    st.pdist = st.pzoom ? 80 : 40; st.ppitch = st.pzoom ? 6 : 1;
                }
                pvalid=0;
            }else if(st.config_row==CONFIG_MESH){st.pwire^=1;pvalid=0;}
#endif
        }
        return 1;
    }
    if (input_pressed(K_CATALOG)) {
        if (!st.ai && !st.banner && !st.morph_mask && !st.turn && st.want == 0xFF
#ifdef FFT_CONFIG
            && !st.ptrans
#endif
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
#ifndef FFT_CONFIG
    // The chapter only has one setting. Keep its menu small enough for the
    // original 64K variable ceiling, including the native profiling build.
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define chapter_ui CONTOUR_REF_chapter_ui
#else
    static const char chapter_ui[4][24]={"SETTINGS","Move: full tiles","Move: contour","ENTER:toggle  ESC:back"};
#endif
    draw_clear();
    draw_text(4,1,chapter_ui[0],F_MEDIUM,C_BLACK);
    draw_text(5,30,st.reach_contour ? "[x]" : "[ ]",F_SMALL,C_BLACK);
    draw_text(21,30,chapter_ui[st.reach_contour ? 2 : 1],F_SMALL,C_BLACK);
#else
    const char *labels[CONFIG_ROWS]; u8 values[CONFIG_ROWS],i,y=38;
#ifdef FFT_REACH_CONTOUR
    labels[CONFIG_MOVE]=!contour_gfx ? "No contours" :
        st.reach_contour ? "Move: contour" : "Move: full tiles";
    values[CONFIG_MOVE]=st.reach_contour;
#endif
#ifdef FFT_CONFIG
    labels[CONFIG_ZOOM]=st.pzoom ? "Wide view" : "Close view";
    labels[CONFIG_MESH]=st.pwire ? "Filled wireframe" : "Textured";
    values[CONFIG_ZOOM]=!st.pzoom;values[CONFIG_MESH]=st.pwire;
#endif
    draw_clear();
    draw_text(4,1,"CONFIGURATION",F_MEDIUM,C_BLACK); draw_rect(3,11,154,1,C_BLACK);
#ifdef FFT_REACH_CONTOUR
    draw_text(5,18,"NORMAL VIEW",F_SMALL,C_BLACK);
    y=30;
#else
    draw_text(5,18,"PERSPECTIVE VIEW",F_SMALL,C_BLACK);
#endif
    for(i=0;i<CONFIG_ROWS;i++){
#if defined(FFT_CONFIG) && defined(FFT_REACH_CONTOUR)
        if(i==CONFIG_ZOOM){draw_text(5,47,"PERSPECTIVE VIEW",F_SMALL,C_BLACK);y=59;}
#endif
        if(st.config_row==i){
            draw_rect(3,y-3,154,12,C_BLACK); draw_rect(4,y-2,152,10,C_LGRAY);
        }
        draw_rect(7,y-1,7,7,C_BLACK); draw_rect(8,y,5,5,C_WHITE);
        if(values[i])draw_text(8,y,"x",F_SMALL,C_BLACK);
        draw_text(21,y,labels[i],F_SMALL,C_BLACK);
#ifdef FFT_REACH_CONTOUR
        y+=18;
#else
        y+=21;
#endif
    }
#endif
#ifndef FFT_CONFIG
    draw_text(4,93,chapter_ui[3],F_SMALL,C_BLACK);
#else
    draw_text(4,93,"ENTER:toggle  ESC:back",F_SMALL,C_BLACK);
#endif
}
