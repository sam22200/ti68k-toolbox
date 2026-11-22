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

// Three tabs, as the management sheet has them: Eqp., Abil., Status. Left and right change
// tab, up and down walk the page's rows, ENTER opens the detail of whatever is under the
// glove. st.status_col stays as the detail panel's own question -- an item or an ability.
static void status_keys(void)
{
    u8 count;
    if (st.status_info) {
        if (input_pressed(K_A | K_ENTER | K_ESC | K_B)) st.status_info = 0;
        return;
    }
    if (input_pressed(K_LEFT)) { st.page = (u8)(st.page ? st.page - 1 : 2); st.status_row = 0; }
    if (input_pressed(K_RIGHT)) { st.page = (u8)(st.page + 1 > 2 ? 0 : st.page + 1); st.status_row = 0; }
    st.status_col = (u8)(st.page == 1);
    count = st.page == 0 ? 5 : st.page == 1 ? status_ability_count() : 0;
    if (count) {
        if (st.status_row >= count) st.status_row = (u8)(count - 1);
        if (input_pressed(K_UP)) st.status_row = st.status_row ? (u8)(st.status_row - 1) : (u8)(count - 1);
        if (input_pressed(K_DOWN)) st.status_row = (u8)(st.status_row + 1 < count ? st.status_row + 1 : 0);
        if (input_pressed(K_A | K_ENTER)) st.status_info = 1;
    } else if (input_pressed(K_A | K_ENTER)) st.mode = st.back;
    if (input_pressed(K_ESC | K_B)) st.mode = st.back;
#ifdef FFT_CHAPTER
    if(st.mode!=M_STATUS){W->ai_k=0;if(W->edge_phase){W->edge_phase=0;W->ej=255;}W->pan_ok[0]=W->pan_ok[1]=0;W->pan_k[0][0]=W->pan_k[1][0]=0;}
#endif
}
