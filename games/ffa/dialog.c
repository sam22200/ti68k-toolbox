// Dialogue box: name substitution, word wrap (24 columns of the 6x8 font, 3 lines a page),
// typewriter (2 characters a frame), pages, and a Yes/No choice.
#include "ffa.h"
#include "texts.h"

#define COLS 24
#define LINES 3
static char buf[200];                        // expanded text of the open dialogue
static u8 ls[16], le[16];                    // line starts and ends (offsets in buf)
static u8 nline, cached = 0xFF;
static u16 cached_num;

static char *put_str(char *d, const char *s) { while (*s) *d++ = *s++; return d; }

static char *put_u16(char *d, u16 v)
{
    char t[6];
    u8 k = 0;
    do t[k++] = '0' + v % 10; while ((v /= 10));
    while (k) *d++ = t[--k];
    return d;
}

static void layout(void)                     // expand st.dlg_text into buf and wrap it
{
    const char *s = texts[st.dlg_text].s;
    char *d = buf;
    u8 k, cut, start = 0;
    for (; *s; s++) {
        if (*s == '\1') d = put_str(d, st.name);
        else if (*s == '\2') d = put_u16(d, st.num);
        else *d++ = *s;
    }
    *d = 0;
    nline = 0;
    while (buf[start] && nline < 16) {
        for (k = start, cut = 0xFF; buf[k] && buf[k] != '\n' && k - start < COLS; k++)
            if (buf[k] == ' ') cut = k;
        if (buf[k] && buf[k] != '\n' && buf[k] != ' ' && cut != 0xFF) k = cut;   // at the last space
        ls[nline] = start; le[nline] = k; nline++;
        start = k;
        if (buf[start] == '\n' || buf[start] == ' ') start++;
    }
    cached = st.dlg_text;
    cached_num = st.num;
}

static void ensure(void) { if (cached != st.dlg_text || cached_num != st.num) layout(); }

static u16 page_chars(void)                  // characters on the current page
{
    u8 i, n = 0;
    for (i = st.dlg_page * LINES; i < nline && i < st.dlg_page * LINES + LINES; i++) n += le[i] - ls[i];
    return n;
}

// The page's text is drawn once into a small plane (only the characters newly shown by the
// typewriter) at its screen x, then copied byte by byte into both hidden planes every frame
// (x 8..151 is byte-aligned: no shift). 72 characters of draw_text cost ~125k a frame; 5 opaque
// 32x31 sprites still ~75k (TI bench, scenario 53); the copy is a few k.
#define SROWS 31                              // the box's white inside: y0 + 2 .. y0 + 32
static u8 scratch[RT_PBYTES * SROWS];         // plane format: draw_text can write into it
static u16 sdrawn = 0xFFFF;
static u8 spage, stext;

static void strip_update(void)
{
    u16 n = 0, i;
    u8 l, a, c, k;
    void *ol = rt_light, *od = rt_dark;
    if (sdrawn == 0xFFFF || spage != st.dlg_page || stext != st.dlg_text || st.dlg_shown < sdrawn) {
        for (i = 0; i < sizeof(scratch); i++) scratch[i] = 0;
        sdrawn = 0; spage = st.dlg_page; stext = st.dlg_text;
    }
    if (st.dlg_shown == sdrawn) return;
    rt_light = rt_dark = scratch;
    for (l = st.dlg_page * LINES; l < nline && l < st.dlg_page * LINES + LINES; l++)
        for (a = ls[l], c = 0; a < le[l]; a++, c++, n++)
            if (n >= sdrawn && n < st.dlg_shown) {
                char t[2];
                t[0] = buf[a]; t[1] = 0;
                draw_text(9 + c * 6, 3 + (l - st.dlg_page * LINES) * 10, t, F_MEDIUM, C_LGRAY);
            }
    rt_light = ol; rt_dark = od;
    sdrawn = st.dlg_shown;
    (void)k;
}

void dialog_open(u8 text, u8 ask)
{
    st.dlg_text = text;
    st.dlg_on = 1;
    st.dlg_ask = ask;
    st.dlg_page = 0;
    st.dlg_shown = 0;
    st.dlg_cur = 0;
    ensure();
}

void dialog_update(void)                     // one frame while st.dlg_on
{
    u16 n;
    ensure();
    n = page_chars();
    if (st.dlg_shown < n) {
        st.dlg_shown += 2;
        if (input_pressed(K_A | K_ENTER)) st.dlg_shown = n;
        if (st.dlg_shown > n) st.dlg_shown = n;
        return;
    }
    if ((st.dlg_page + 1) * LINES >= nline && st.dlg_ask) {   // last page: Yes / No
        if (input_pressed(K_UP | K_DOWN)) st.dlg_cur ^= 1;
        if (input_pressed(K_A | K_ENTER)) { st.ans = !st.dlg_cur; st.dlg_on = 0; }
        return;
    }
    if (!input_pressed(K_A | K_ENTER)) return;
    if ((st.dlg_page + 1) * LINES < nline) { st.dlg_page++; st.dlg_shown = 0; }
    else st.dlg_on = 0;
}

void dialog_render(u8 top)
{
    s16 y0 = top ? 2 : 63, x, y;
    u8 sp = texts[st.dlg_text].speaker;
    ensure();
    // frame as thin rectangles: the copied text strip (x 8..151) fills the inside
    draw_rect(3, y0, 154, 1, C_BLACK); draw_rect(3, y0 + 34, 154, 1, C_BLACK);
    draw_rect(3, y0 + 1, 1, 33, C_BLACK); draw_rect(156, y0 + 1, 1, 33, C_BLACK);
    draw_rect(4, y0 + 1, 152, 1, C_DGRAY); draw_rect(4, y0 + 33, 152, 1, C_DGRAY);
    draw_rect(4, y0 + 2, 1, 31, C_DGRAY); draw_rect(155, y0 + 2, 1, 31, C_DGRAY);
    draw_rect(5, y0 + 2, 3, 31, C_WHITE); draw_rect(152, y0 + 2, 3, 31, C_WHITE);
    if (sp) {                                // name tag on the box's top edge
        const char *nm = speaker_name[sp][0] == '\1' ? st.name : speaker_name[sp];
        u8 w = 0;
        while (nm[w]) w++;
        y = top ? y0 + 35 : y0 - 8;
        draw_rect(8, y, w * 6 + 6, 10, C_BLACK);
        draw_rect(9, y + 1, w * 6 + 4, 8, C_LGRAY);
        draw_text(11, y + 1, nm, F_MEDIUM, C_BLACK);
    }
    strip_update();
    {                                         // copy x 8..151 (bytes 1..18) of each row
        const u8 *src = scratch + 1;
        u8 *l = (u8 *)rt_light + (y0 + 2) * RT_PBYTES + 1, *d = rt_dark ? (u8 *)rt_dark + (y0 + 2) * RT_PBYTES + 1 : 0;
        u8 r, b;
        for (r = 0; r < SROWS; r++, src += RT_PBYTES, l += RT_PBYTES) {
            for (b = 0; b < 18; b++) l[b] = src[b];
            if (d) { for (b = 0; b < 18; b++) d[b] = src[b]; d += RT_PBYTES; }
        }
    }
    if (st.dlg_shown >= page_chars()) {
        if ((st.dlg_page + 1) * LINES >= nline && st.dlg_ask) {   // Yes / No at the right
            draw_rect(118, y0 + 5, 34, 25, C_BLACK);
            draw_rect(119, y0 + 6, 32, 23, C_WHITE);
            draw_text(130, y0 + 8, "Yes", F_MEDIUM, C_BLACK);
            draw_text(130, y0 + 19, "No", F_MEDIUM, C_BLACK);
            y = y0 + 9 + st.dlg_cur * 11;
            draw_rect(122, y, 2, 6, C_BLACK); draw_rect(124, y + 1, 1, 4, C_BLACK); draw_rect(125, y + 2, 1, 2, C_BLACK);
        } else if (rt_frame & 8) {                                   // blinking "next" triangle
            {
                x = 147; y = y0 + 27;
                draw_rect(x, y, 5, 1, C_BLACK); draw_rect(x + 1, y + 1, 3, 1, C_BLACK); draw_rect(x + 2, y + 2, 1, 1, C_BLACK);
            }
        }
    }
}
