// PC-only proposals: use the actual shared renderers; do not change shipping screens/saves.
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"
#include <assert.h>
#include <stdio.h>

static const char *output;
static u8 bar_face[348], bar_page[2 * PAGE_PLANE];
static char article[4096];
static const char *const topics[] = { "Death Corps", "King Omdolia", "Fifty Year War" };
static u8 rumor_topic;
enum { BAR_READER = 0 };  // User-selected A: left list and wide text on grey.

static void read_bytes(const char *name, void *dst, unsigned size)
{
    char path[512]; FILE *f;
    snprintf(path, sizeof path, "%s/%s", output, name);
    f = fopen(path, "rb"); assert(f);
    assert(fread(dst, 1, size, f) == size); assert(fgetc(f) == EOF); fclose(f);
}

static void shot(const char *name)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.png", output, name);
    assert(!sw_write_png(path, 1));
}

static void read_article(const char *name)
{
    char path[512]; FILE *f; size_t size;
    snprintf(path, sizeof path, "%s/%s", output, name); f = fopen(path, "rb"); assert(f);
    size = fread(article, 1, sizeof article - 1, f); assert(feof(f)); fclose(f); article[size] = 0;
}

static void bar_background(void)
{
    u8 *saved = page;
    page = bar_page; page_blit(); page = saved;
}

static void bar_header(const char *first, const char *second)
{
    // Feed the existing town dialogue renderer a borrowed review-only portrait.
    // Three aliases share one record; no second dialogue renderer or room bank.
    u8 dialogue[12 + 348] = { 'F','S','K','4',3,0, 0,12, 0,12, 0,12 };
    const u8 *saved = scene;
    memcpy(dialogue + 12, bar_face, sizeof bar_face);
    scene = dialogue; says("Bar owner", first, second); scene = saved;
}

static ChoiceLayout list_box(u8 rows, u8 wide)
{
    ChoiceLayout p = { 24, 36, 0, 0, 7, CHOICE_TAB, 3, 1, 0, 6, 12, 9, 156, 0, 0 };
    u8 i, w = 0;
    for (i = 0; i < 3; i++) {
        u8 n = map_menu_width(topics[i]); if (n > w) w = n;
    }
    p.width = (u8)(w + 12); p.height = (u8)(13 + 9 * rows);
    if (wide) p.x = (s16)((160 - p.width) / 2);
    p.hand_x = CHOICE_HAND_X(p.text_x); p.hand_y = CHOICE_HAND_Y(p.row_y);
    return p;
}

static void bar_menu(void)
{
    static const char *const rows[] = { "Rumor", "Leave bar" };
    ChoiceLayout p = list_box(2, 0);
    p.width = (u8)(map_menu_width("Leave bar") + 12);
    bar_background(); bar_header("Welcome. Come in", "and relax.");
    choice_frame(&p, "Menu");
    choice_row(&p, 0, rows[0], C_BLACK); choice_row(&p, 1, rows[1], C_BLACK);
    choice_cursor_animated(&p, 0, rt_frame); war_funds(gil());
}

static void rumor_list(u8 variant)
{
    ChoiceLayout p = list_box(3, variant);
    u8 i;
    bar_background(); bar_header("What would you", "like to hear?");
    choice_frame(&p, "Rumor");
    for (i = 0; i < 3; i++) choice_row(&p, i, topics[i], C_BLACK);
    choice_cursor_animated(&p, 0, rt_frame);
    war_funds(gil()); shot(variant ? "b-list" : "a-list");
}

static void rumor_page(u8 variant, u8 page_number)
{
    char line[64];
    u8 row, total = 0, rows = variant ? 5 : 7, y = variant ? 43 : 31;
    u8 first = (u8)(rows * page_number);
    u8 width = variant ? 126 : 142;
    s16 x = variant ? 28 : 9;
    while (item_detail_line(article, width, total, line)) total++;
    assert(first < total);
    bar_background();
    if (variant) {
        bar_header("The latest news", "about the Death Corps.");
        dlg_name(24, 33, (u8)(map_menu_width(topics[rumor_topic]) + 8), topics[rumor_topic], 0);
        dlg_frame(24, 45, 133, 46, 0); y = 52;
    } else {
        dlg_face(bar_face, 1, 1, 0);
        dlg_name(24, 1, (u8)(map_menu_width("Bar owner") + 8), "Bar owner", 0);
        dlg_name(24, 13, (u8)(map_menu_width(topics[rumor_topic]) + 8), topics[rumor_topic], 0);
        // Reuse the shared Help/command frame for the wider reading window.
        ChoiceLayout p = { 5, 33, 150, 61, 7, CHOICE_TAB, 3, 1, 0, 6, 12, 9, 154, 0, 0 };
        choice_frame(&p, "Rumor"); y = 46;
    }
    for (row = 0; row < rows; row++) {
        if (!item_detail_line(article, width, (u8)(first + row), line)) break;
        text(x, (s16)(y + row * 7), line, variant ? C_WHITE : C_BLACK, 153);
    }
    // The original uses graphical continuation/scroll cues, not a numeric page counter.
    // Reuse the campaign's exact ready-to-advance cue, including the final page.
    dlg_continue(variant ? 152 : 151, variant ? 87 : 91, rt_frame, !variant);
    draw_rect(4, 94, 152, 6, C_BLACK);
    text(7, 95, "L/R page     SHIFT back", C_WHITE, 155);
    shot(variant ? (page_number ? "b-read-2" : "b-read") : (page_number ? "a-read-2" : "a-read"));
}

static void approved_bar(void)
{
    char path[64], line[64]; u16 consumed = 0; u8 rows = 0, pages, page, row, col, phase;
    rt_frame = 0; bar_menu(); shot("selected-home"); rumor_list(BAR_READER);
    rumor_page(BAR_READER, 0); shot("selected-short");
    read_article("long-rumor.txt"); rumor_topic = 2;
    assert(strlen(article) >= 1000);
    while (item_detail_line(article, 142, rows, line)) rows++;
    pages = (u8)((rows + 6) / 7); assert(pages > 1);
    for (page = 0; page < pages; page++) {
        rt_frame = 16; rumor_page(BAR_READER, page);
        snprintf(path, sizeof path, "long-page-%u", page); shot(path);
        for (row = 0; row < 7; row++) {
            if (!item_detail_line(article, 142, (u8)(page * 7 + row), line)) break;
            for (col = 0; line[col]; col++) {
                if (line[col] == ' ') continue;
                while (article[consumed] == ' ') consumed++;
                assert(line[col] == article[consumed++]);
            }
        }
    }
    while (article[consumed] == ' ') consumed++;
    assert(!article[consumed]);
    for (phase = 0; phase <= 8; phase++) {
        rt_frame = (u16)(phase * 4);
        rumor_page(BAR_READER, 0); snprintf(path, sizeof path, "a-first-%u", phase); shot(path);
        rumor_page(BAR_READER, (u8)(pages - 1));
        snprintf(path, sizeof path, "a-last-%u", phase); shot(path);
    }
    printf("Bar A: %u pages, %u wrapped lines, %u source bytes preserved.\n", pages, rows, consumed);
}

static void portrait_contrasts(u8 *corrected)
{
    char path[512]; FILE *f; u8 variant, speaker;
    static const u8 screens[] = { SC_STORE, SC_OFFICE, SC_FUR };
    snprintf(path, sizeof path, "%s/portraits.bin", output);
    f = fopen(path, "rb"); assert(f);
    for (variant = 0; variant < 6; variant++) for (speaker = 0; speaker < 4; speaker++) {
        if (speaker < 3) {
            u16 at = b16(corrected + 6 + 2 * speaker);
            assert(fread(corrected + at, 1, 348, f) == 348);
            wm.screen = screens[speaker]; wm.trow = 0; scene = corrected; p_screen = 255;
            draw_clear(); town_render();
        } else {
            assert(fread(bar_face, 1, sizeof bar_face, f) == sizeof bar_face);
            bar_menu();
        }
        snprintf(path, sizeof path, "portrait-%u-%u", variant, speaker); shot(path);
    }
    assert(fgetc(f) == EOF); fclose(f);
}

int main(int argc, char **argv)
{
    u8 *corrected; const u8 *old; Save before; Wm wm_before;
    FILE *f; char path[512]; long size; u8 i;
    static const u8 screens[] = { SC_STORE, SC_OFFICE, SC_FUR };
    static const char *const names[] = { "store", "office", "fur" };
    assert(argc == 3 || (argc == 4 && (!strcmp(argv[3], "--portraits") ||
                                     !strcmp(argv[3], "--selected") ||
                                     !strcmp(argv[3], "--bar-a")))); output = argv[1];
    sw_init(0); assert(bank && mgb && shop && scene);
    f = fopen(argv[2], "rb"); assert(f); assert(read_save_fixture(f, &save)); fclose(f);
    game_scenario(0); wm.at = 6; wm.x = loc_x(6); wm.y = loc_y(6); wm.tick = 4;
    before = save; wm_before = wm; old = scene;
    snprintf(path, sizeof path, "%s/ffsk.bin", output);
    f = fopen(path, "rb"); assert(f); fseek(f, 0, SEEK_END); size = ftell(f); fclose(f);
    assert(size > 0 && size < 65536); corrected = malloc((unsigned)size); assert(corrected);
    read_bytes("ffsk.bin", corrected, (unsigned)size);
    read_bytes("bar-face.bin", bar_face, sizeof bar_face);
    read_bytes("bar-planes.bin", bar_page, sizeof bar_page);
    read_article("rumor.txt");
    if (argc == 4 && !strcmp(argv[3], "--bar-a")) {
        approved_bar(); assert(!memcmp(&save, &before, sizeof save));
        wm = wm_before; scene = old; free(corrected); release_map();
        puts("Bar A: shared grey reader; Save unchanged.");
        return 0;
    }
    if (argc == 4 && !strcmp(argv[3], "--portraits")) {
        portrait_contrasts(corrected);
        assert(!memcmp(&save, &before, sizeof save));
        wm = wm_before; scene = old; free(corrected); release_map();
        puts("Portrait contrasts: 24 shared-renderer screens; Save unchanged.");
        return 0;
    }
    for (i = 0; i < 3; i++) {
        wm.screen = screens[i]; wm.trow = 0; scene = old; p_screen = 255;
        draw_clear(); town_render(); snprintf(path, sizeof path, "%s-before", names[i]); shot(path);
        scene = corrected; p_screen = 255; draw_clear(); town_render();
        snprintf(path, sizeof path, "%s-after", names[i]); shot(path);
    }
    bar_menu(); shot("bar-home"); rumor_list(0); rumor_list(1);
    rumor_page(0, 0); rumor_page(1, 0); rumor_page(1, 1);
    for (i = 0; i < 8; i++) {
        rt_frame = (u16)(i * 4); bar_menu();
        snprintf(path, sizeof path, "bar-hand-%u", i); shot(path);
    }
    if (argc == 4) {
        // Same source rumor, first and final page, through one complete blink cycle.
        for (i = 0; i <= 8; i++) {
            rt_frame = (u16)(i * 4);
            rumor_page(1, 0); snprintf(path, sizeof path, "continue-%u", i); shot(path);
            rumor_page(1, 1); snprintf(path, sizeof path, "final-%u", i); shot(path);
        }
    }
    // Review only: no calendar, story, money, inventory, party or durable RNG edits.
    assert(!memcmp(&save, &before, sizeof save));
    wm = wm_before; scene = old; free(corrected); release_map();
    puts("Town proposals: shared renderers; Save unchanged; no shipping writes.");
    return 0;
}
