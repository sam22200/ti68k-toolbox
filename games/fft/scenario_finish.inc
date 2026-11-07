// Cold scenario presentation reset, after the resident clock/turn selection.
static void scenario_finish(u8 n)
{
    if (n >= 61 && n <= 68) {                 // direct Ramza; the three shoulder views
        st.ai = AI_OFF; st.mode = M_MENU; st.act = st.sel = U_RAMZA;
        cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));
        st.want = st.prot = 0xFF; st.turn = 0;
#ifdef FFT_EXPLORATION
        if (n >= 62 && n <= 64) perspective_enter((u8)(n - 61));
#ifdef FFT_CINEMA
#ifdef FFT_CAMERA_MOTION
        st.cenabled = 1;
#endif
        if (n >= 65) {
            // Native combat fixtures: original enemies, Ramza already in attack range.
            st.unit[U_RAMZA].x = n == 66 ? 4 : 5;
            st.unit[U_RAMZA].z = n == 66 ? 6 : 5;
            if (n == 66) st.unit[U_RAMZA].learned |= 2; // Throw Stone for the ranged review
            cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));
            st.menu = MN_ACT;
        }
#endif
#endif
    }
    W->status_i = 0; W->pan_k[0][0] = W->pan_k[1][0] = 0; W->pan_ok[0] = W->pan_ok[1] = 0; W->fc_key = 0; W->win_k[0] = 0;
}
