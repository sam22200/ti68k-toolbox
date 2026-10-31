// Prior cache-box rasterization, immutable regression reference.
// Shared forecast-cache box, given an already formatted pure line.
#ifndef FFT_FORECAST_BOX_INC
#define FFT_FORECAST_BOX_INC
static void forecast_panel_text(const char *s)
{
                                         // a white box framed black, black text,
        void *sl = rt_light, *sd = rt_dark;    // centered; its corners left out
        u8 w = (u8)(ui_width(s) + 5), x = (u8)((RT_W - w) >> 1);
        memset(W->forecast, 0, sizeof W->forecast);
        rt_light = rt_dark = W->forecast[2];
        draw_rect(x, 0, w, FC_ROWS, C_BLACK);
        draw_rect(x, 0, 1, 1, C_WHITE); draw_rect(x + w - 1, 0, 1, 1, C_WHITE);
        draw_rect(x, FC_ROWS - 1, 1, 1, C_WHITE); draw_rect(x + w - 1, FC_ROWS - 1, 1, 1, C_WHITE);
        rt_light = W->forecast[0]; rt_dark = W->forecast[1];
        draw_rect(x + 1, 0, w - 2, FC_ROWS, C_BLACK);
        draw_rect(x, 1, w, FC_ROWS - 2, C_BLACK);
        draw_rect(x + 1, 1, w - 2, FC_ROWS - 2, C_WHITE);
        rt_light = sl; rt_dark = sd;
        text_into(W->forecast[0], W->forecast[1], x + 2, 2, s, C_BLACK);
}
#endif
