// Cold scalar prediction preparation; area UI supplies its cached batch line.
#include "forecast_box.inc"
static void make_forecast(u8 a,u8 t,AbId ab)
{
    char s[28];forecast_text(a,t,ab,s);forecast_panel_text(s);
}
