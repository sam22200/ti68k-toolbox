#ifndef FFT_DIALOGUE_BOX_INC
#define FFT_DIALOGUE_BOX_INC
// The campaign's one dialogue look, shared by the opening/scene player (beats.inc) and by the
// world map's town screens (campaign/worldmap.c): a detached portrait in its own frame, a
// detached name box above the text box, and a black box with white text whose top three rows
// fade in through an ordered gradient. `inverted` swaps black and white, as the opening's
// CATALOG toggle does. Geometry is the reviewed one; do not retune it here.
// The includer defines DLG_TEXT(x, y, s, color, limit) with its own font.
#define DLG_FACE_W 20                   // its outlined art is 19 pixels wide; 22 with frame and gap

static void dlg_face(const u8 *portrait, s16 x, s16 y, u8 inverted)   // y: the frame's top
{
#ifdef __m68k__
    RtSprite sp = { 32, 29, portrait, portrait + 116, portrait + 232 };
#else
    u32 rows[3][29];
    u8 k, r;
    RtSprite sp = { 32, 29, rows[0], rows[1], rows[2] };
    for (k = 0; k < 3; k++) for (r = 0; r < 29; r++) rows[k][r] = RD32(portrait + k * 116, r * 2);
#endif
    draw_rect(x, y, DLG_FACE_W + 1, 31, inverted ? C_BLACK : C_WHITE);
    draw_rect(x + 1, y + 1, DLG_FACE_W - 1, 30, inverted ? C_WHITE : C_BLACK);
    draw_sprite(x, y + 2, &sp);
}

static void dlg_frame(s16 x, s16 y, u8 w, u8 h, u8 inverted)
{
    u8 row;
    draw_rect(x, y, w, h, inverted ? C_BLACK : C_WHITE);
    draw_rect(x + 1, y + 1, w - 2, h - 2, inverted ? C_WHITE : C_BLACK);
    // Word masks apply the ordered gradient without a per-pixel loop.
    for (row = 2; row < 5; row++) {
        u16 pat = row == 2 ? 0xAAAA : row == 3 ? 0x8888 : 0;
        u8 first = (u8)((x + 2) >> 4), last = (u8)((x + w - 3) >> 4), word;
        u16 left = (u16)(0xFFFFU >> ((x + 2) & 15)), right = (u16)(0xFFFFU << (15 - ((x + w - 3) & 15)));
        u8 *l = (u8 *)rt_light + (y + row) * RT_PBYTES;
        for (word = first; word <= last; word++) {
            u16 mask = pat & (word == first ? left : 0xFFFF) & (word == last ? right : 0xFFFF);
            RMW16(l + 2 * word, mask, inverted ? mask : 0);
        }
    }
}

static void dlg_name(s16 x, s16 y, u8 w, const char *name, u8 inverted)   // y: the name box's top
{
    draw_rect(x, y, w, 11, inverted ? C_BLACK : C_WHITE);
    draw_rect(x + 1, y + 1, w - 2, 10, inverted ? C_WHITE : C_BLACK);
    DLG_TEXT(x + 4, y + 3, name, inverted ? C_BLACK : C_WHITE, 156);
}

// Campaign's existing3x2 continuation cue, shared with other paged text owners.
// The caller owns readiness/page navigation and redraws the background before each blink.
static void dlg_continue(s16 x, s16 y, u16 clock, u8 inverted)
{
    if (clock & 16) draw_rect(x, y, 3, 2, inverted ? C_BLACK : C_WHITE);
}
#ifdef DLG_WIDTH
// A town speaker's two-line greeting, shared with service/dispatch screens.
static void dlg_town_says(const u8 *portrait,const char *name,const char *first,const char *second)
{
    s16 x=DLG_FACE_W+4;
    u8 w=(u8)(DLG_WIDTH(first)+10),k=(u8)(DLG_WIDTH(second)+10);
    if(k>w)w=k;
    dlg_name(x,1,(u8)(DLG_WIDTH(name)+8),name,0);
    dlg_frame(x,13,w,20,0);
    DLG_TEXT(x+4,17,first,C_WHITE,156);DLG_TEXT(x+4,24,second,C_WHITE,156);
    dlg_face(portrait,1,1,0);
}
#endif
#endif
