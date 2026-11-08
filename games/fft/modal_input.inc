// Cold input for three overlays (SCREENS op13 on the TI; direct in builds without modules):
// M_ORDER, the turn order ([APPS]): UP/DOWN select an entry, the cursor (and the camera,
// which follows it) goes to that unit; any other key closes it and puts the cursor back.
// st.status_row/status_col/status_info hold the selection, the saved cursor and "saved":
// Status resets them when it opens, and the turn order cannot be opened from Status.
// M_QUIT, "Quit?": No unless Yes is picked. M_AUTO: Manual / This unit / All allies.
// Returns 1 when "Quit?" is answered Yes (the resident leaves the battle).
static u8 modal_keys(void)
{
    if (st.mode == M_ORDER) {
        if (input_pressed(K_UP | K_DOWN)) {
            u8 r = st.status_row;
            if (!st.status_info) { st.status_col = (u8)CURSOR_CELL; st.status_info = 1; }
            r = input_pressed(K_UP) ? (r ? r - 1 : ORDER_N - 1) : (r + 1 < ORDER_N ? r + 1 : 0);
            st.status_row = r;
            cursor_place(UNIT_CELL(&st.unit[st.turns[r]]));
        } else if (input_pressed(K_A | K_B | K_ENTER | K_ESC | K_APPS | K_LEFT | K_RIGHT)) {
            if (st.status_info) cursor_place(st.status_col);
            st.status_row = st.status_info = 0; st.mode = st.back;
        }
    } else if (st.mode == M_QUIT) {
        if (input_pressed(K_UP | K_DOWN)) st.quit ^= 1;
        if (input_pressed(K_A | K_ENTER)) { if (!st.quit) return 1; st.mode = st.back==M_CLOCK?M_CLOCK:M_BROWSE; }
        else if (input_pressed(K_ESC | K_B)) st.mode = st.back==M_CLOCK?M_CLOCK:M_BROWSE;
    } else {
        if (input_pressed(K_UP)) st.sub=st.sub ? st.sub-1:2;
        if (input_pressed(K_DOWN)) st.sub=st.sub<2 ? st.sub+1:0;
        if (input_pressed(K_A | K_ENTER)) {
            auto_control(st.sub);
            if (!st.sub) to_menu();
        } else if (input_pressed(K_ESC | K_B)) to_menu();
    }
    return 0;
}
