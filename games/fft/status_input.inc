// Status input shared by the PC engine and the calculator screens overlay.
static u8 status_ability_count(void)
{
    u8 n = 0;
    n=(u8)known(st.sel,255);
    n += st.unit[st.sel].defend;
    return n ? n : 1;
}

static AbId status_ability(void)
{
    AbId ab = known(st.sel, st.status_row);
    return ab == NO_ABIL && st.unit[st.sel].defend ? ABIL_DEFEND : ab;
}

static void status_keys(void)
{
    if (st.status_info) {
        if (input_pressed(K_A | K_ENTER | K_ESC | K_B)) st.status_info = 0;
    } else if (!st.page) {
        if (input_pressed(K_LEFT | K_RIGHT)) { st.page = 1; st.status_col = st.status_row = 0; }
        if (input_pressed(K_A | K_ENTER | K_ESC | K_B)) st.mode = st.back;
    } else {
        u8 count;
        if (input_pressed(K_LEFT)) {
            if (st.status_col) st.status_col = 0;
            else { st.page = 0; return; }
        }
        if (input_pressed(K_RIGHT)) {
            if (!st.status_col) st.status_col = 1;
            else { st.page = 0; return; }
        }
        count = st.status_col ? status_ability_count() : 5;
        if (st.status_row >= count) st.status_row = count - 1;
        if (input_pressed(K_UP)) st.status_row = st.status_row ? st.status_row - 1 : count - 1;
        if (input_pressed(K_DOWN)) st.status_row = st.status_row + 1 < count ? st.status_row + 1 : 0;
        if (input_pressed(K_A | K_ENTER)) st.status_info = 1;
        if (input_pressed(K_ESC | K_B)) st.mode = st.back;
    }
#ifdef FFT_CHAPTER
    if(st.mode!=M_STATUS){W->ai_k=0;if(W->edge_phase){W->edge_phase=0;W->ej=255;}W->pan_ok[0]=W->pan_ok[1]=0;W->pan_k[0][0]=W->pan_k[1][0]=0;}
#endif
}
