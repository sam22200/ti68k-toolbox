// The world map (fftmap, campaign/ENGINE.md core milestone): the reviewed source texture
// and original lettering, with native markers and roads, the locations the
// story has shown, Ramza travelling FFT's routes between them. fftmap() returns what the
// campaign does next (fftstory runs it, then the map again): 0 quit, 1 the management menus,
// 100+ a battle program's door (fftbat). The bank ffwm (tools/world_assets.py) holds the map's
// compressed colour planes cached in eight-row strips, the43 locations,48 routes and story
// list (a location shown by a story flag, entered as its battle until that battle is won);
// ffmg gives the font; ffwm also holds Ramza's eight idle_sprites poses. The save gives the flags and
// the location, written back when the map closes.
// Keys: arrows move a free cursor, with nearby locations snapped on release. ENTER travels
// shown routes: FFT's costs) or enters the one Ramza stands on, ALPHA / DIAMOND the menus, ESC
// quits.
// A town (ffsh, tools/shop_assets.py) opens its services instead: the Shop buys and sells
// FFT's items at FFT's prices, by the stock tier a story flag has opened, and the Soldier
// Office hires a Squire or a Chemist from the source's own out-of-battle generator. A field
// location rolls one of its random formations and enters it like any other battle door.
#include "../../../runtime/core/rt.h"
#include "../art.h"
#include "save.h"
#include <stdlib.h>
#include <string.h>
#include "../base.inc"
#include "breeding.h"
#include "../identity_pool.h"
#ifndef __m68k__
#include "story_choice_file.h"
#endif

enum { LOC_N = 43, ROUTE_N = 48, NOTE_NONE = 0, NOTE_NOROAD, NOTE_LATER, NOTE_UNAVAILABLE, NOTE_SAVED,
       NOTE_NOGIL, NOTE_STOCK, NOTE_PARTY };
enum { SC_MAP = 0, SC_TOWN, SC_STORE, SC_SHOP, SC_FIT, SC_OFFICE, SC_HIRE, SC_FUR };
enum { FIT_ROWS = 4 };               // the party rows the fitting bench shows at once
enum { ROOM_STORE = 0, ROOM_OFFICE, ROOM_FUR, ROOM_N };
// The Fur Shop opens in the source's Chapter III; no battle of this build sets its flag.
enum { FUR_FLAG = 255 };
enum { SHOP_ROWS = 6, OFFICE_ROWS = 4, ENC_MAX = 3, OFFICE_REC = 17 };
// The source's six shop tabs, by the item's category byte: [first, last] of each.
enum { SHOP_TABS = 6 };
static const u8 tab_first[SHOP_TABS] = { 1, 19, 20, 23, 26, 32 };
static const u8 tab_last[SHOP_TABS]  = { 18, 19, 22, 25, 31, 34 };
typedef struct {
    u8 at, sel;                          // Ramza's location, the chosen one
    u8 moving, leg, legs, step;          // travelling: leg of path[], its point
    u8 path[16];                         // route ids, bit 7: run end to start
    s16 x, y;                            // Ramza on the map (texture pixels)
    s16 handx, handy;                    // free hand hotspot; never commits a journey
    u8 menu, row;
    u8 screen, trow, ttop, buy;          // a town's services: its row, the list's first row
    u8 cat, detail;                      // the shop tab (F1/F5) and whether [MODE] is showing
    u8 card, cand;                       // SC_HIRE: the status panel, and which row made him
    u8 note, changed, tick;
    u16 result;                          // what fftmap returns
} Wm;
Wm wm;
Save save;
static const u8 *bank, *mgb,*breed_rules;
static const u8 *shop,*sh_price,*sh_tier,*sh_kind,*sh_cats,*sh_lone,*sh_shop,*sh_tierflag;
static const u8 *sh_place,*sh_office,*sh_jobs,*sh_enc;
static const u8 *scene;                  // ffsk: the source's rooms and the speaker's portrait
static const u8 *identity;               // ffcp: the generic names a recruit is given
static const u8 *help;                   // ffid: FFT's own item descriptions, read in place
#define PAGE_PLANE (100 * RT_PBYTES)
static u8 *page;
static u8 p_screen = 255, p_cat, p_buy, p_trow, p_ttop, p_n;
static u32 p_gil;

static RtTilemap map;
static u8 *wmcache,*idle_sprites;
static u8 strip_tag[16];
static u16 stride,strip_bytes;
static u8 strip_decor,decor_ready,decor_at,visible[LOC_N];

#ifdef __m68k__
static void __attribute__((__stkparm__)) release_map(void)
#else
static void release_map(void)
#endif
{
    if(wmcache)free(wmcache);
    wmcache=idle_sprites=RT_NULL;
}
void rt_clamp_cam(const RtTilemap *m, s16 *cx, s16 *cy);   // (rt_core.c: draw_tilemap's camera)

static u16 b16(const u8 *p) { return (u16)((u16)p[0] << 8 | p[1]); }
static s16 s16v(const u8 *p) { return (s16)b16(p); }
static const u8 *part(u8 k) { return bank + b16(bank + 4 + 2 * k); }
static const u8 *loc(u8 i) { return part(2) + 8 * i; }
static s16 loc_x(u8 i) { return s16v(loc(i)); }
static s16 loc_y(u8 i) { return s16v(loc(i) + 2); }
static const char *loc_name(u8 i) { return (const char *)bank + b16(loc(i) + 6); }
static const u8 *route(u8 r) { return part(3) + 6 * r; }
static const u8 *menu_font(void) { return mgb + b16(mgb + 4 + 2 * 7); }
#include "mgtext.inc"

static u8 map_menu_width(const char *s)
{
    const u8 *advance=menu_font()+103;u8 width=0;
    while(*s)width+=advance[(u8)*s++-32];
    return width;
}
#define CHOICE_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define CHOICE_WIDTH(s) map_menu_width(s)
#include "../choice_menu.inc"

#define DLG_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#include "../dialogue_box.inc"
#define WF_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define WF_WIDTH(s) map_menu_width(s)
#include "../war_funds.inc"

static u8 flag(u8 k) { return (u8)(save.flags[k >> 3] >> (k & 7) & 1); }

static const u8 *scene_part(u8 k) { return scene + b16(scene + 6 + 2 * k); }
static const u8 *scene_face(u8 room) { return scene_part(room); }
static const u8 *scene_room(u8 room) { return scene_part((u8)(ROOM_N + room)); }
// Which room a screen shows, and who speaks in it.
static u8 town_room(void)
{ return (u8)(wm.screen == SC_OFFICE || wm.screen == SC_HIRE ? ROOM_OFFICE :
               wm.screen == SC_FUR ? ROOM_FUR : ROOM_STORE); }
static const char *town_speaker(void)
{
    return wm.screen == SC_OFFICE || wm.screen == SC_HIRE ? "Guildmaster" :
           wm.screen == SC_FUR ? "Fur Merchant" : "Shopkeeper";
}
static u8 fur_open(void)
{ return (u8)(FUR_FLAG != 255 && flag(FUR_FLAG)); }

// the story list's entry of location i (RT_NULL: none): location, flag + 1 that shows it,
// battle door (u16, 0 none), flag + 1 its victory sets
static const u8 *story(u8 i)
{
    const u8 *s = part(4) + 1;
    u8 n = part(4)[0];
    for (; n; n--, s += 6) if (s[0] == i) return s;
    return RT_NULL;
}
u8 shown(u8 i)
{
    const u8 *s = story(i);
    return i == wm.at || (s && s[1] && flag((u8)(s[1] - 1)));
}

// the cheapest chain of shown routes from wm.at to `to` (Dijkstra on FFT's route costs, 43
// nodes): into wm.path, 0 when none
u8 find_path(u8 to)
{
    u16 dist[LOC_N];
    u8 from[LOC_N], via[LOC_N], done[LOC_N], i, r, n = 0;
    memset(done, 0, sizeof done);
    for (i = 0; i < LOC_N; i++) dist[i] = 0xFFFF;
    dist[wm.at] = 0;
    for (;;) {
        u8 u = 0xFF;
        for (i = 0; i < LOC_N; i++) if (!done[i] && dist[i] != 0xFFFF && (u == 0xFF || dist[i] < dist[u])) u = i;
        if (u == 0xFF || u == to) break;
        done[u] = 1;
        for (r = 0; r < ROUTE_N; r++) {
            const u8 *rt = route(r);
            u8 a = rt[0], b = rt[1], v;
            if (a != u && b != u) continue;
            v = a == u ? b : a;
            if (!shown(v) || dist[u] + rt[2] >= dist[v]) continue;
            dist[v] = (u16)(dist[u] + rt[2]); from[v] = u; via[v] = (u8)(r | (a == u ? 0 : 0x80));
        }
    }
    if (dist[to] == 0xFFFF || to == wm.at) return 0;
    for (i = to; i != wm.at; i = from[i]) n++;
    if (n > sizeof wm.path) return 0;
    wm.legs = n;
    for (i = to; i != wm.at; i = from[i]) wm.path[--n] = via[i];
    return 1;
}

static const u8 *point(u8 leg, u8 k)     // point k of a path leg, in its running order
{
    const u8 *rt = route(wm.path[leg] & 0x7F);
    if (wm.path[leg] & 0x80) k = (u8)(rt[3] - 1 - k);
    return bank + b16(rt + 4) + 4 * k;
}

static void arrive(void)
{
    const u8 *rt = route(wm.path[wm.legs - 1] & 0x7F);
    wm.at = wm.path[wm.legs - 1] & 0x80 ? rt[0] : rt[1];
    wm.moving = 0; wm.x = loc_x(wm.at); wm.y = loc_y(wm.at);
    save.loc = (u8)(wm.at + 1); wm.changed = 1;
}

// Same bounded xorshift family as battle rand15, with an independent durable
// world stream. Idle frames, menu input and child reentry consume no draws.
static u16 world_draw(void *context)
{
    u8 *p=((Save *)context)->world_rng;
    u32 n=(u32)b16(p)<<16|b16(p+2);
    n^=n<<13;n^=n>>17;n^=n<<5;
    p[0]=(u8)(n>>24);p[1]=(u8)(n>>16);p[2]=(u8)(n>>8);p[3]=(u8)n;
    return (u16)(n>>17);
}

static void travel_step(void)            // a pixel a frame along the route's points
{
    const u8 *p = point(wm.leg, (u8)(wm.step + 1));
    s16 tx = s16v(p), ty = s16v(p + 2);
    wm.x += tx > wm.x ? 1 : tx < wm.x ? -1 : 0;
    wm.y += ty > wm.y ? 1 : ty < wm.y ? -1 : 0;
    if (wm.x != tx || wm.y != ty) return;
    if (++wm.step + 1 < route(wm.path[wm.leg] & 0x7F)[3]) return;
    {
        u8 result=world_party_day(&save.world,&save,breed_rules,world_draw,&save);
        const u8 *rt=route(wm.path[wm.leg]&0x7f);
        if(result==WB_INVALID){wm.moving=0;wm.x=loc_x(wm.at);wm.y=loc_y(wm.at);wm.note=NOTE_UNAVAILABLE;return;}
        wm.at=wm.path[wm.leg]&0x80?rt[0]:rt[1];save.loc=(u8)(wm.at+1);wm.changed=1;
    }
    wm.step = 0;
    if (++wm.leg == wm.legs) arrive();
}

// ------------------------------------------------- a town's services, a field's encounters
// The bank ffsh (FSH2) holds FFT's own prices, stock tiers and item categories, the shop type
// of each location, the two Soldier Office generator rows and the random formations of each
// field. Lists are walked over the item ids rather than materialized: 255 cheap tests cost far
// less than the32-byte buffer a cached list would add to Wm and to every state file.
static u16 wdraw(void) { return (u16)(world_draw(&save) & 32767U); }
static u32 gil(void)
{ return (u32)save.money[0] << 24 | (u32)save.money[1] << 16 | (u32)save.money[2] << 8 | save.money[3]; }
static void gil_set(u32 v)
{
    if (v > 99999999UL) v = 99999999UL;
    save.money[0] = (u8)(v >> 24); save.money[1] = (u8)(v >> 16);
    save.money[2] = (u8)(v >> 8); save.money[3] = (u8)v;
}
#define MG_ITEM_SIZE 14
// ffmg's item record: u16 name, flags, type, reserved, HP MP PA MA SP Move Jump, icon, WP
#define MI_FLAGS 2
#define MI_TYPE 3
#define MI_HP 5
#define MI_ICON 12
#define MI_WP 13
static const u8 *item_rec(u8 id) { return mgb + b16(mgb + 4 + 2 * 1) + (u16)id * MG_ITEM_SIZE; }
static const char *item_name(u8 id)
{ u16 at = b16(item_rec(id)); return at ? (const char *)mgb + at : ""; }
// ffid (tools/item_help.py): FFT's own description, read in place from the archive.
static const char *item_help(u8 id)
{
    u16 at;
    if (!help) return "";
    at = b16(help + 6 + 2 * (u16)id);
    return at ? (const char *)help + at : "";
}
static void item_icon(s16 x, s16 y, u8 id)
{
    const u8 *p = mgb + b16(mgb + 4 + 2 * 4) + (u16)item_rec(id)[MI_ICON] * 24;
    RtSprite sp = { 8, 8, p, p + 8, p + 16 };
    draw_sprite(x, y, &sp);
}
// How many of the party can wear it: the job's equipment categories, as management rules it.
#define MG_JOB_SIZE 32
#define MJ_CATS 16
static u8 item_fits(const SaveUnit *u, u8 id)
{
    // Ramza's own Squire is not one of the twenty generic careers; management reads it as the
    // generic Squire's record, and so does this.
    u8 career = (u8)(u->job >= 0x4A && u->job <= 0x5D ? u->job - 0x4A : 0);
    const u8 *j = mgb + b16(mgb + 4) + (u16)career * MG_JOB_SIZE;
    u8 type = item_rec(id)[MI_TYPE];
    if (type >= 32) return 0;
    return (u8)((j[MJ_CATS + (type >> 3)] >> (7 - (type & 7))) & 1);
}
static u8 item_wearers(u8 id)
{ u8 i, n = 0; for (i = 0; i < save.n; i++) if (item_fits(&save.unit[i], id)) n++; return n; }
#define ID_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define ID_NUM(x,y,n) number(x,y,n)
#define ID_ICON(x,y,id) item_icon(x,y,id)
#define ID_ADV(c) (menu_font()[103 + (u8)(c) - 32])
#include "../item_detail.inc"
static void item_detail(u8 id)
{
    const u8 *r = item_rec(id);
    ItemDetail d;
    u8 i;
    d.id = id; d.kind = sh_kind[id]; d.flags = r[MI_FLAGS]; d.name = item_name(id);
    d.help = item_help(id);
    for (i = 0; i < 7; i++) d.stat[i] = (s8)r[MI_HP + i];
    d.power = r[MI_WP];
    item_detail_draw(&d);
}
static u16 item_price(u8 id) { return b16(sh_price + 2 * (u16)id); }
static u8 shop_type(void) { return shop && wm.at < LOC_N ? sh_shop[wm.at] : 0; }
// A stock tier opens with its story flag (0: from the start, 255: no battle of this build
// reaches it), exactly as the source widens a shop's stock as the chapter advances.
static u8 tier_open(u8 tier)
{
    u8 f;
    if (!tier || tier >= 16) return 0;
    f = sh_tierflag[tier];
    return f == 255 ? 0 : (u8)(!f || flag((u8)(f - 1)));
}
static u8 on_sale(u8 id)
{
    u8 t = shop_type(), k;
    if (!t || !id) return 0;
    k = sh_kind[id];
    if (!(sh_cats[5 * (u16)t + (k >> 3)] & (u8)(1u << (k & 7))) && id != sh_lone[t]) return 0;
    return tier_open(sh_tier[id]);
}
static u8 in_tab(u8 id)
{ u8 k = sh_kind[id]; return (u8)(k >= tab_first[wm.cat] && k <= tab_last[wm.cat]); }
static u8 shop_has(u8 id)
{
    if (!in_tab(id)) return 0;
    return wm.buy ? on_sale(id) : (u8)(save.inv[id] && item_price(id));
}
// The list as a 256-bit set rather than a rescan: walking the ids cost 1.3M cycles a frame on
// the 68000, four times the budget. One pass per trade or page switch, then a byte-skipping
// walk; the set is a static, so no state file grows by it.
static u8 shop_bits[32], shop_n;
static void shop_refresh(void)
{
    u16 i;
    shop_n = 0; memset(shop_bits, 0, sizeof shop_bits);
    for (i = 1; i < 256; i++)
        if (shop_has((u8)i)) { shop_bits[i >> 3] |= (u8)(0x80 >> (i & 7)); shop_n++; }
}
static u8 shop_nth(u8 n)
{
    u8 i, k;
    for (i = 0; i < 32; i++) {
        u8 b = shop_bits[i];
        if (!b) continue;
        for (k = 0; k < 8; k++) if ((b & (u8)(0x80 >> k)) && !n--) return (u8)(8 * (u16)i + k);
    }
    return 0;
}

// The fitting bench. The source's *Best fit* puts the best the store carries on a unit and
// *Try it on* shows what it would make of them; the bench does both at once. The statistics
// come from the campaign's one derivation (unit_stats.inc), the same management shows.
static const u8 *fit_job(const SaveUnit *u)
{
    u8 career = (u8)(u->job >= 0x4A && u->job <= 0x5D ? u->job - 0x4A : 0);
    return mgb + b16(mgb + 4) + (u16)career * MG_JOB_SIZE;
}
static u8 fit_current(const SaveUnit *u)
{
    u8 k;
    for (k = 0; k < save.n; k++) if (u == &save.unit[k]) return save_raw_current(&save, k);
    return 0;
}
// The world map carries no ffcj, so a movement ability's bonus is not added here. The bench
// shows HP, MP, PA, MA and Speed only, which that bonus never touches.
#define UST_JOB(u) fit_job(u)
#define UST_ITEM(id) item_rec(id)
#define UST_CURRENT(u) fit_current(u)
#define UST_MOVE(u,m,j) ((void)(u),(void)(m),(void)(j))
#include "../unit_stats.inc"
// Where it goes, by the slot flags management rules on: head, body, accessory, weapon, shield.
static u8 fit_slot(u8 id)
{
    static const u8 flag_of[5] = { 0x20, 0x10, 0x08, 0x80, 0x40 };
    u8 f = item_rec(id)[MI_FLAGS], i;
    for (i = 0; i < 5; i++) if (f & flag_of[i]) return i;
    return 255;
}
// What a unit is worth to the bench: the figures a player reads, weighted so a point of PA is
// not traded for a point of HP. A weapon's power is counted too, being why one is worn.
static u16 fit_score(const Stats *s, u8 power)
{
    return (u16)(s->hp + s->mp + 10U * (s->pa + s->ma + s->sp) + 8U * power);
}
// The best stock item unit `u` could wear: the one that makes the most of it, measured with
// the campaign's own derivation rather than a rule of thumb. 0 when the store has none.
static u8 best_fit(const SaveUnit *u)
{
    SaveUnit worn;
    Stats now;
    u16 i, score;
    u8 best = 0, slot;
    worn = *u;
    now = unit_stats(u);
    score = fit_score(&now, 0);
    {
        u8 k;
        for (k = 0; k < 5; k++) if (u->eq[k]) {
            u8 p = item_rec(u->eq[k])[MI_WP];
            if (p) score = (u16)(score + 8U * p);
        }
    }
    for (i = 1; i < 256; i++) {
        u8 id = (u8)i;
        Stats then;
        u16 s2;
        if (!on_sale(id) || !item_fits(u, id)) continue;
        slot = fit_slot(id);
        if (slot >= 5) continue;                    // not a thing one wears
        worn = *u; worn.eq[slot] = id;
        then = unit_stats(&worn);
        s2 = fit_score(&then, item_rec(id)[MI_WP]);
        {
            u8 k;
            for (k = 0; k < 5; k++) if (k != slot && worn.eq[k]) {
                u8 p = item_rec(worn.eq[k])[MI_WP];
                if (p) s2 = (u16)(s2 + 8U * p);
            }
        }
        if (s2 > score) { score = s2; best = id; }
    }
    return best;
}
static void fit_buy(void)
{
    SaveUnit *u = &save.unit[wm.trow];
    u8 id = best_fit(u), slot;
    u16 price;
    if (!id) { wm.note = NOTE_LATER; return; }
    slot = fit_slot(id);
    if (slot == 255) { wm.note = NOTE_LATER; return; }
    price = item_price(id);
    if (gil() < price) { wm.note = NOTE_NOGIL; return; }
    gil_set(gil() - price);
    if (u->eq[slot]) save.inv[u->eq[slot]]++;      // what comes off goes back to the bag
    u->eq[slot] = id;
    wm.changed = 1;
}

// FFT sells back at half the shop price; the row leaves the list with its last copy.
static void shop_trade(void)
{
    u8 id = shop_nth(wm.trow);
    u16 price;
    if (!id) return;
    price = item_price(id);
    if (wm.buy) {
        if (save.inv[id] >= 99) { wm.note = NOTE_STOCK; return; }
        if (gil() < price) { wm.note = NOTE_NOGIL; return; }
        gil_set(gil() - price); save.inv[id]++;
    } else {
        gil_set(gil() + (price >> 1)); save.inv[id]--;
        if (!shop_has(id) && wm.trow) wm.trow--;
    }
    shop_refresh();
    wm.changed = 1;
}

// FFT's Soldier Office: a Squire or a Chemist, man or woman, from the source out-of-battle
// generator (the same seeds, variances and draw order breeding.h runs for monsters), equipped
// as the source equips a recruit. Price: the guide's, we have not found it on the disc.
static u8 office_sex(u8 row) { return (u8)(row & 1); }
static u8 office_job(u8 row) { (void)row; return sh_jobs[0]; }   // the source's menu offers only a soldier
static const u8 *office_rec(u8 row) { return sh_office + OFFICE_REC * (u16)office_sex(row); }
static u16 office_price(u8 row) { return b16(office_rec(row) + 15); }
// The source generates the fighter first and shows him; Hire is a second, separate answer.
// The candidate lives in the party's own free slot, which nothing reads until save.n grows.
static void office_make(u8 row)
{
    const u8 *g = office_rec(row);
    SaveUnit *u;
    u8 i, name, used, sex = office_sex(row);
    if (save.n >= PARTY_MAX) { wm.note = NOTE_PARTY; return; }
    u = &save.unit[save.n];
    memset(u, 0, sizeof *u);
    u->job = office_job(row); u->gfx = 255; u->type = sex; u->level = 1;
    u->brave = (u8)((muls16((s16)wdraw(), 31) >> 15) + 40);
    u->faith = (u8)((muls16((s16)wdraw(), 31) >> 15) + 40);
    for (i = 0; i < 5; i++) {
        u32 raw = ((u32)g[i] << 14) + (u32)(muls16((s16)wdraw(), g[5 + i]) >> 1);
        u->raw[i][0] = (u8)(raw >> 16); u->raw[i][1] = (u8)(raw >> 8); u->raw[i][2] = (u8)raw;
    }
    do {
        name = (u8)(muls16((s16)wdraw(), 255) >> 15); used = 0;
        for (i = 0; i < save.n; i++)
            if (save.unit[i].type == sex && save.unit[i].name == name) { used = 1; break; }
    } while (used);
    u->name = name;
    u->zodiac = wb_birth_sign(breed_rules, (u16)(wdraw() % 365 + 1));
    memcpy(u->eq, g + 10, 5);
    save_raw_set(&save, save.n, 0);
    world_party_clear_slot(&save.world, save.n);
    wm.screen = SC_HIRE; wm.trow = 0; wm.card = 0;
}
// Hire: the fighter in the free slot joins, and only now does he cost anything.
static void office_take(u8 row)
{
    if (save.n >= PARTY_MAX) { wm.note = NOTE_PARTY; return; }
    if (gil() < office_price(row)) { wm.note = NOTE_NOGIL; return; }
    gil_set(gil() - office_price(row));
    save.n++; wm.changed = 1;
    wm.screen = SC_OFFICE; wm.trow = 0;
}
static const char *unit_label(const SaveUnit *u)
{
    if (u->type == 2) return "Ramza";
    return identity ? identity_name(identity, (u16)u->name + (u->type == 1 ? 255 : 0)) : "";
}
static const char *candidate_name(void)
{
    const SaveUnit *u = &save.unit[save.n];
    if (!identity) return "";
    return unit_label(u);
}

// A field location: one of its three authored formations, 0 while none is ripped.
static u16 encounter(void)
{
    if (!shop || sh_place[wm.at] < 2) return 0;
    return b16(sh_enc + 2 * (ENC_MAX * (u16)wm.at + wdraw() % ENC_MAX));
}

// A bounded magnetic radius, measured in texture pixels. No destination on hover alone.
static u8 magnet(void)
{
    u8 i, best = 255;
    u16 bd = 401;
    for (i = 0; i < LOC_N; i++) {
        s16 ex = (s16)(loc_x(i) - wm.handx), ey = (s16)(loc_y(i) - wm.handy);
        u16 d;
        if (!shown(i) || ex < -20 || ex > 20 || ey < -20 || ey > 20) continue;
        d = (u16)(ex * ex + ey * ey);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

void game_init(void)
{
    u16 size;
#ifndef __m68k__
    release_map();
#endif
    wmcache=idle_sprites=RT_NULL;
    bank = mgb = shop = scene = help = identity = RT_NULL;   // (statics survive between runs)
    strip_decor=decor_ready=0;
    memset(&wm, 0, sizeof wm);
    bank = rt_file("ffwm", &size);
    if (bank && memcmp(bank, "FWM3", 4)) bank = RT_NULL;
    mgb = rt_file("ffmg", &size);
    if (mgb && memcmp(mgb, "FMG3", 4)) mgb = RT_NULL;
    save_load(&save);
    // A town without its services is still a map: ffsh only disables them.
    shop = rt_file("ffsh", &size);
    if (shop && !memcmp(shop, "FSH2", 4) && size > 6 && shop[4] == 255) {
        u16 types = (u16)shop[5] + 1;
        sh_price = shop + 6; sh_tier = sh_price + 512; sh_kind = sh_tier + 256;
        sh_cats = sh_kind + 256; sh_lone = sh_cats + 5 * types; sh_shop = sh_lone + types;
        sh_tierflag = sh_shop + LOC_N; sh_place = sh_tierflag + 16;
        sh_office = sh_place + LOC_N; sh_jobs = sh_office + 2 * OFFICE_REC; sh_enc = sh_jobs + 2;
        // A TI variable reports its trailing tag bytes too: the bank must fit, not match.
        if ((u16)(sh_enc + 2 * ENC_MAX * LOC_N - shop) > size) shop = RT_NULL;
    } else shop = RT_NULL;
    scene = rt_file("ffsk", &size);
    if (scene && (memcmp(scene, "FSK4", 4) || scene[4] < ROOM_N)) scene = RT_NULL;
    identity = rt_file("ffcp", &size);
    if (!identity_valid(identity, size)) identity = RT_NULL;
    help = rt_file("ffid", &size);
    if (help && (memcmp(help, "FID1", 4) || b16(help + 4) != 256)) help = RT_NULL;
    breed_rules=rt_file("ffwb",&size);
    if(!world_rules_valid(breed_rules,size)||!save_world_valid(&save))bank=RT_NULL;
    if (bank) {
        map.w = bank[14]; map.h = bank[15];
        stride=b16(part(0));strip_bytes=stride<<4;
        wmcache=malloc((u16)(strip_bytes<<4)+5*156+2*PAGE_PLANE);
        if(!wmcache){bank=RT_NULL;return;}
        idle_sprites=wmcache+(strip_bytes<<4);
        page=idle_sprites+5*156;             // the town screens' own composed page
        p_screen=255;
        zx0_unpack(part(1)+8,idle_sprites);
        memset(strip_tag,255,sizeof strip_tag);
#ifdef __m68k__
        atexit(release_map);
#endif
    }
    rt_state = &wm; rt_state_size = sizeof wm;
}

void game_scenario(u16 n)                 // the save's location (n: test doors, Gariland + n)
{
    (void)n;
    wm.at = save.loc ? (u8)(save.loc - 1) : 6;        // a save before any: Gariland
    wm.sel = wm.at; wm.moving = 0; wm.note = n==1?NOTE_SAVED:0; wm.result = 0;
    if (bank) { wm.x = loc_x(wm.at); wm.y = loc_y(wm.at); }
    wm.handx=wm.x;wm.handy=wm.y;wm.menu=wm.row=0;
    wm.screen=n==2?SC_FUR:SC_MAP;wm.trow=wm.ttop=0;wm.buy=1;   // n==2: the Fur Shop door
    decor_ready=0;
}

#include "../../../lib/tilog.h"
static u8 leave(u16 result)
{
    tilog("map leave %u at loc %u%s", result, save.loc, wm.changed ? " (session saved)" : "");
    wm.result = result;
    if (wm.changed) {
#ifdef __m68k__
        u8 ok=rt_save(SAVE_NAME,&save,sizeof save);
#else
        u8 ok=story_choice_store_file(&save);
#endif
        if(!ok){wm.note=NOTE_UNAVAILABLE;return 1;}
    }
    release_map();
    return 0;
}

// The town's own screens. They replace the map until ESC/SHIFT walks back out, so the map's
// cursor, menu and travel keep their meaning untouched.
static u8 town_update(u32 key)
{
    if ((key & (K_ESC | K_B)) && !wm.detail) {
        if (wm.card) { wm.card = 0; return 1; }
        wm.screen = (u8)(wm.screen == SC_TOWN ? SC_MAP :
                         wm.screen == SC_SHOP || wm.screen == SC_FIT ? SC_STORE :
                         wm.screen == SC_HIRE ? SC_OFFICE : SC_TOWN);
        wm.trow = wm.ttop = 0;
        return 1;
    }
    if (wm.screen == SC_TOWN) {
        wm.trow = choice_move(wm.trow, 4, key);
        if (key & (K_ENTER | K_A)) {
            if (wm.trow == 2 && !fur_open()) { wm.note = NOTE_LATER; return 1; }
            wm.screen = (u8)(wm.trow == 3 ? SC_MAP : wm.trow == 2 ? SC_FUR :
                             wm.trow ? SC_OFFICE : SC_STORE);
            wm.trow = wm.ttop = 0; wm.buy = 1;
        }
        return 1;
    }
    if (wm.screen == SC_FUR) {           // the poached hides are a system of their own
        wm.trow = choice_move(wm.trow, 3, key);
        if ((key & (K_ENTER | K_A)) && wm.trow == 2) { wm.screen = SC_TOWN; wm.trow = 0; }
        else if (key & (K_ENTER | K_A)) wm.note = NOTE_LATER;
        return 1;
    }
    if (wm.screen == SC_STORE) {       // Buy / Sell / Fitting room / Leave store
        wm.trow = choice_move(wm.trow, 4, key);
        if (key & (K_ENTER | K_A)) {
            if (wm.trow == 3) { wm.screen = SC_TOWN; wm.trow = wm.ttop = 0; return 1; }
            if (wm.trow == 2) { wm.screen = SC_FIT; wm.trow = wm.ttop = 0; return 1; }
            wm.buy = (u8)(wm.trow == 0); wm.screen = SC_SHOP; wm.trow = wm.ttop = wm.cat = 0;
            wm.detail = 0; shop_refresh();
        }
        return 1;
    }
    if (wm.screen == SC_FIT) {         // the source's Best fit, on one bench
        if (save.n) {
            wm.trow = choice_move(wm.trow, save.n, key);
            if (key & (K_ENTER | K_A)) fit_buy();
        }
        if (wm.trow < wm.ttop) wm.ttop = wm.trow;
        if (wm.trow >= wm.ttop + FIT_ROWS) wm.ttop = (u8)(wm.trow - FIT_ROWS + 1);
        return 1;
    }
    if (wm.screen == SC_OFFICE) {
        wm.trow = choice_move(wm.trow, OFFICE_ROWS, key);
        if (key & (K_ENTER | K_A)) {
            if (wm.trow == 3) { wm.screen = SC_TOWN; wm.trow = wm.ttop = 0; }
            else if (wm.trow < 2) { wm.cand = wm.trow; office_make(wm.trow); }
            else wm.note = NOTE_LATER;           // Change name: a roster picker of its own
        }
        return 1;
    }
    if (wm.screen == SC_HIRE) {                  // the source's Hire / Quit / Check status
        if (wm.card) { if (key) wm.card = 0; return 1; }
        wm.trow = choice_move(wm.trow, 3, key);
        if (key & (K_ENTER | K_A)) {
            if (!wm.trow) office_take(wm.cand);
            else if (wm.trow == 1) { wm.screen = SC_OFFICE; wm.trow = 0; }
            else wm.card = 1;
        }
        return 1;
    }
    {
        u8 n;
        if (wm.detail) { if (key & (K_MODE | K_B | K_ESC | K_ENTER | K_A)) wm.detail = 0; return 1; }
        if (key & (K_F1 | K_F5)) {      // the source's L1/R1: the next tab, wrapping
            wm.cat = (u8)(key & K_F1 ? (wm.cat ? wm.cat - 1 : SHOP_TABS - 1)
                                     : (wm.cat + 1 < SHOP_TABS ? wm.cat + 1 : 0));
            wm.trow = wm.ttop = 0; shop_refresh();
        }
        if (key & (K_LEFT | K_RIGHT)) { wm.buy ^= 1; wm.trow = wm.ttop = 0; shop_refresh(); }
        if ((key & K_MODE) && shop_n) { wm.detail = 1; return 1; }
        n = shop_n;
        if (n) {
            wm.trow = choice_move(wm.trow, n, key);
            if (key & (K_ENTER | K_A)) shop_trade();
            n = shop_n;
        }
        if (!n) wm.trow = 0;
        else if (wm.trow >= n) wm.trow = (u8)(n - 1);
        if (wm.trow < wm.ttop) wm.ttop = wm.trow;
        if (wm.trow >= wm.ttop + SHOP_ROWS) wm.ttop = (u8)(wm.trow - SHOP_ROWS + 1);
        if (wm.ttop && wm.ttop + SHOP_ROWS > n)
            wm.ttop = (u8)(n > SHOP_ROWS ? n - SHOP_ROWS : 0);
        return 1;
    }
}

u8 game_update(void)
{
    u32 key = rt_keys & ~rt_prev;
    wm.tick++;
    if (!bank || !mgb) return !(key & K_ESC);
    if (wm.moving) { travel_step(); if(wm.moving)travel_step(); return 1; }
    if (wm.note) { if (key & (K_ENTER | K_A | K_B | K_ESC)) wm.note = 0; return 1; }
    if (wm.screen) return town_update(key);
    if (key & K_ESC) {if(wm.menu){wm.menu=0;return 1;}return leave(0);}
    if (key & (K_C | K_D)) { wm.menu^=1;wm.row=0;return 1; }
    if (wm.menu) {
        if (key & K_B) { wm.menu=0;return 1; }
        wm.row=choice_move(wm.row,6,key);
        if (key & (K_ENTER|K_A)) {
            if (wm.row==1) return leave(1);
            if (!wm.row) wm.menu=0;
            else if (wm.row==4) return leave(2); // same ffttitle(3) file module, after pending save flush
            else wm.note=NOTE_UNAVAILABLE;
        }
        return 1;
    }
    if (rt_keys & (K_LEFT|K_RIGHT|K_UP|K_DOWN)) {
        if (rt_keys & K_LEFT) wm.handx-=2;
        if (rt_keys & K_RIGHT) wm.handx+=2;
        if (rt_keys & K_UP) wm.handy-=2;
        if (rt_keys & K_DOWN) wm.handy+=2;
        if(wm.handx<0)wm.handx=0;
        if(wm.handy<0)wm.handy=0;
        if(wm.handx>map.w*16-1)wm.handx=map.w*16-1;
        if(wm.handy>map.h*16-1)wm.handy=map.h*16-1;
        wm.sel=magnet();
    } else if (rt_prev & (K_LEFT|K_RIGHT|K_UP|K_DOWN)) {
        wm.sel=magnet();
        if(wm.sel!=255){wm.handx=loc_x(wm.sel);wm.handy=loc_y(wm.sel);}
    }
    if (key & (K_ENTER | K_A)) {
        if (wm.sel==255) {
            wm.handx=wm.x;wm.handy=wm.y;wm.sel=wm.at;
            return 1; // recenter only: a second explicit press is needed to enter
        }
        if (wm.sel != wm.at) {
            if (find_path(wm.sel)) { wm.moving = 1; wm.leg = wm.step = 0; }
            else wm.note = NOTE_NOROAD;
        } else {
            const u8 *s = story(wm.at);
            u16 door = s ? b16(s + 2) : 0;
            if (door && !(s[4] && flag((u8)(s[4] - 1)))) return leave(door);   // its battle
            if (shop_type()) { wm.screen = SC_TOWN; wm.trow = wm.ttop = 0; return 1; }
            door = encounter();                                  // a field: a random formation
            if (door) return leave(door);
            wm.note = NOTE_LATER;
        }
    }
    return 1;
}

// ---------------------------------------------------------------- drawing
#ifdef WM_REFERENCE
// Original immediate-mode road/marker renderer retained only as a pixel oracle.
static void pset(s16 x, s16 y, u8 c)
{
    u8 *l, *d, m;
    if ((u16)x >= 160 || (u16)y >= 100) return;
    l = (u8 *)rt_light + y * RT_PBYTES + (x >> 3); d = (u8 *)rt_dark + y * RT_PBYTES + (x >> 3);
    m = (u8)(0x80 >> (x & 7));
    if (c & 1) *l |= m; else *l &= (u8)~m;
    if (c & 2) *d |= m; else *d &= (u8)~m;
}

static void line(s16 x0, s16 y0, s16 x1, s16 y1,u8 outline) // road centre / white contour
{
    s16 dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    s16 sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (;;) {
        s16 e2;
        pset(x0,y0,outline?C_WHITE:C_BLACK);
        if(outline) {
            draw_rect(x0-2,y0-2,5,5,C_WHITE);
        }
        if (x0 == x1 && y0 == y1) return;
        e2 = (s16)(2 * e);
        if (e2 >= dy) { e += dy; x0 += sx; }
        if (e2 <= dx) { e += dx; y0 += sy; }
    }
}

static void marker(s16 x, s16 y, u8 town)     // circular white ring with a dark centre
{
    static const u8 widths[9]={3,7,7,9,9,9,7,7,3};
    u8 r;
    for(r=0;r<9;r++)draw_rect(x-(widths[r]>>1),y+r-4,widths[r],1,C_WHITE);
    for(r=0;r<9;r++) {
        pset(x-(widths[r]>>1),y+r-4,C_BLACK);pset(x+(widths[r]>>1),y+r-4,C_BLACK);
    }
    draw_rect(x-1,y-4,3,1,C_BLACK);draw_rect(x-1,y+4,3,1,C_BLACK);
    for(r=0;r<5;r++)draw_rect(x-(r==0||r==4?0:1),y+r-2,r==0||r==4?1:3,1,town?C_BLACK:C_DGRAY);
}
#endif

// Paint static roads/markers once into each decoded eight-row strip. They use
// the existing cache allocation, so ordinary scrolling only copies the planes.
static void strip_span(u8 *dst,s16 y,s16 x0,s16 x1,u8 color)
{
    u8 first,last,mask,*l,*d;
    if((u16)y>=8||x1<0||x0>=(s16)(stride<<3))return;
    if(x0<0)x0=0;
    if(x1>=(s16)(stride<<3))x1=(s16)((stride<<3)-1);
    first=(u8)((u16)x0>>3);last=(u8)((u16)x1>>3);
    mask=(u8)(255U>>((u16)x0&7));
    l=dst+(u16)y*stride+first;d=l+(stride<<3);
    for(;;l++,d++,first++) {
        u8 m=first==last?(u8)(mask&(255U<<(7-((u16)x1&7)))):mask;
        if(color&1)*l|=m;else *l&=(u8)~m;
        if(color&2)*d|=m;else *d&=(u8)~m;
        if(first==last)return;
        mask=255;
    }
}

static void strip_line(u8 *dst,s16 top,s16 x0,s16 y0,s16 x1,s16 y1,u8 outline)
{
    s16 dx,dy,sx,sy,e;
    if((y0<top-2&&y1<top-2)||(y0>top+9&&y1>top+9))return;
    dx=x1>x0?x1-x0:x0-x1;dy=y1>y0?y0-y1:y1-y0;
    sx=x0<x1?1:-1;sy=y0<y1?1:-1;e=dx+dy;
    for(;;) {
        s16 e2;
        if(outline) {
            s16 row=y0-top-2,end=row+4;
            for(;row<=end;row++)strip_span(dst,row,x0-2,x0+2,C_WHITE);
        } else strip_span(dst,y0-top,x0,x0,C_BLACK);
        if(x0==x1&&y0==y1)return;
        e2=(s16)(2*e);
        if(e2>=dy){e+=dy;x0+=sx;}
        if(e2<=dx){e+=dx;y0+=sy;}
    }
}

static void decorate_strip(u8 *dst,u16 top)
{
    static const u8 widths[9]={3,7,7,9,9,9,7,7,3};
    u8 pass,r,k,i;
    for(pass=0;pass<2;pass++)for(r=0;r<ROUTE_N;r++) {
        const u8 *rt=route(r),*p=bank+b16(rt+4);
        if(!visible[rt[0]]||!visible[rt[1]])continue;
        for(k=0;k+1<rt[3];k++,p+=4)
            strip_line(dst,(s16)top,s16v(p),s16v(p+2),s16v(p+4),s16v(p+6),!pass);
    }
    for(i=0;i<LOC_N;i++)if(visible[i]) {
        s16 x=loc_x(i),y=(s16)(loc_y(i)-top);
        if(y<-4||y>11)continue;
        for(r=0;r<9;r++)strip_span(dst,y+r-4,x-(widths[r]>>1),x+(widths[r]>>1),C_WHITE);
        for(r=0;r<9;r++) {
            s16 a=(s16)(widths[r]>>1);
            strip_span(dst,y+r-4,x-a,x-a,C_BLACK);strip_span(dst,y+r-4,x+a,x+a,C_BLACK);
        }
        strip_span(dst,y-4,x-1,x+1,C_BLACK);strip_span(dst,y+4,x-1,x+1,C_BLACK);
        for(r=0;r<5;r++) {
            s16 a=r==0||r==4?0:1;
            strip_span(dst,y+r-2,x-a,x+a,loc(i)[4]==1?C_BLACK:C_DGRAY);
        }
    }
}

static void ramza(s16 x, s16 y)              // eight source idle_sprites poses, feet on the marker
{
    static const u8 ends[8]={3,7,12,16,19,23,28,32};
    u8 phase=0,t=wm.tick&31;
    const u8 *p;
    while(phase<7&&t>=ends[phase])phase++;
    p=idle_sprites+(u16)part(1)[phase]*156;
#ifdef __m68k__
    RtSprite sp = { 16, 26, p, p + 52, p + 104 };
#else
    u16 r[3][26]; u8 i, j;
    RtSprite sp = { 16, 26, r[0], r[1], r[2] };
    for (i = 0; i < 3; i++) for (j = 0; j < 26; j++) r[i][j] = b16(p + i * 52 + 2 * j);
#endif
    draw_sprite(x - 8, y - 22, &sp);
}

static void map_background(s16 cx,s16 cy,u8 decor)
{
    // Story flags stay fixed until this module returns. Arrival may change the
    // visible set; keep the strips when the already-shown destination adds none.
    if(decor&&(!decor_ready||decor_at!=wm.at)) {
        u8 i,changed=(u8)!decor_ready;
        for(i=0;i<LOC_N;i++) {
            u8 v=shown(i);
            if(v!=visible[i])changed=1;
            visible[i]=v;
        }
        decor_at=wm.at;decor_ready=1;
        if(changed)memset(strip_tag,255,sizeof strip_tag);
    }
    if(strip_decor!=decor){memset(strip_tag,255,sizeof strip_tag);strip_decor=decor;}
    // Only the visible compressed strips are decoded; a slot survives pixel scrolling.
    {
        u16 y=0;
        u8 sh=(u8)(cx&15),rs=(u8)(16-sh);
        while(y<100) {
            u16 row=(u16)(cy+y),block=row>>3;
            u8 slot=block&15,n=(u8)(8-(row&7)),r;
            const u8 *sl,*sd;
            u8 *dl=(u8 *)rt_light+y*RT_PBYTES,*dd=(u8 *)rt_dark+y*RT_PBYTES;
            if(strip_tag[slot]!=block) {
                zx0_unpack(part(0)+b16(part(0)+4+2*block),wmcache+(u16)slot*strip_bytes);
                if(decor)decorate_strip(wmcache+(u16)slot*strip_bytes,block<<3);
                strip_tag[slot]=(u8)block;
            }
            if(y+n>100)n=(u8)(100-y);
            sl=wmcache+(u16)slot*strip_bytes+(row&7)*stride+((cx>>4)<<1);
            sd=sl+(stride<<3);
            // Same word-aligned copy/shift as battle copy_view; avoid4000
            // individually shifted byte writes on every ordinary scrolling frame.
            for(r=0;r<n;r++,sl+=stride,sd+=stride,dl+=RT_PBYTES,dd+=RT_PBYTES) {
#ifdef __m68k__
                const u16 *a=(const u16 *)sl,*b=(const u16 *)sd;
                PlaneWord *p=(PlaneWord *)dl,*q=(PlaneWord *)dd;
                if(!sh) {
                    const PlaneWord *c=(const PlaneWord *)a,*e=(const PlaneWord *)b;
                    p[0]=c[0];p[1]=c[1];p[2]=c[2];p[3]=c[3];p[4]=c[4];
                    q[0]=e[0];q[1]=e[1];q[2]=e[2];q[3]=e[3];q[4]=e[4];
                } else {
#define WM_COPY(k) p[k]=*(const PlaneWord *)(a+2*k)<<sh|a[2*k+2]>>rs; \
                   q[k]=*(const PlaneWord *)(b+2*k)<<sh|b[2*k+2]>>rs
                    WM_COPY(0);WM_COPY(1);WM_COPY(2);WM_COPY(3);WM_COPY(4);
#undef WM_COPY
                }
#else
                u8 k;
                for(k=0;k<10;k+=2) {
                    WR32(dl,k,sh?RD32(sl,k)<<sh|RD16(sl,k+2)>>rs:RD32(sl,k));
                    WR32(dd,k,sh?RD32(sd,k)<<sh|RD16(sd,k+2)>>rs:RD32(sd,k));
                }
#endif
            }
            y+=n;
        }
    }
}

// ------------------------------------------------- the town's screens
// The source's own arrangement: its room behind everything (ffsk, tools/shop_scene.py: the
// disc's MENU/BK_SHOP.TIM and BK_HONE.TIM, people included), the shared dialogue box at the
// top, the shared action menu at the left and the shared War Funds plate at the bottom right.
// Every menu obeys CHOICE_MENU.md's five rules.
#ifdef WM_ROOM_PICK                      // review door (preview_shop.c): compose any room
u8 wm_room_pick = 1;
#define TOWN_ROOM(office) wm_room_pick
#else
#define TOWN_ROOM(office) ((void)(office), town_room())
#endif
// FFT's generic careers are 0x4A..0x5D; Ramza's own Squire is not one of them, and reading
// outside the table printed rubbish on the PC and nothing on the 68000.
static const char *job_name(u8 job)
{
    if (job < 0x4A || job > 0x5D) return "Squire";
    return (const char *)mgb + b16(mgb + b16(mgb + 4) + (u16)(job - 0x4A) * MG_JOB_SIZE);
}
// The room decompresses into the map's own strip cache: it is 32 KB and idle while a town
// screen is up, so the rooms cost no second allocation. Each owner invalidates the other.
// A town screen is composed once into an off-screen page of two 100-row planes, then blitted:
// the room, the tabs, twelve icons and the two plates cost 419k cycles a frame when redrawn,
// 75k when copied. The room is stored at the runtime's own stride, so it decompresses straight
// into the page. Nothing below row 99 is ever drawn into it.
static void page_blit(void)
{
    s16 y;
    for (y = 0; y < 100; y++) {
        memcpy((u8 *)rt_light + (u16)y * RT_PBYTES, page + (u16)y * RT_PBYTES, 20);
        memcpy((u8 *)rt_dark + (u16)y * RT_PBYTES, page + PAGE_PLANE + (u16)y * RT_PBYTES, 20);
    }
}
// The map's two labels, one style: black, white text, one pixel of air over and under it, as
// wide as the text needs, one pixel off the screen's top edge. The place is centred and the
// day sits at the left; neither ever goes to the foot of the screen.
static void plate(s16 x, const char *s)
{
    s16 w = (s16)(map_menu_width(s) + 8);
    if (x < 0) x = (s16)((160 - w) / 2);
    draw_rect(x, 1, w, 7, C_BLACK);
    text((s16)(x + 4), 2, s, C_WHITE, 158);
}
static void place_plate(const char *name) { plate(-1, name); }
// The day, in its own small panel at the top left, where it has always been.
static void date_plate(void)
{
    static const char months[12][4] = { "Jan","Feb","Mar","Apr","May","Jun",
                                        "Jul","Aug","Sep","Oct","Nov","Dec" };
    char date[7];
    u8 day = save.world.day;
    memcpy(date, months[save.world.month - 1], 3); date[3] = ' ';
    if (day >= 10) { date[4] = (char)('0' + day / 10); date[5] = (char)('0' + day % 10); date[6] = 0; }
    else { date[4] = (char)('0' + day); date[5] = 0; }
    plate(4, date);
}
static void says(const char *name, const char *first, const char *second)
{
    s16 x = DLG_FACE_W + 4;
    u8 w = (u8)(map_menu_width(first) + 10), k = (u8)(map_menu_width(second) + 10);
    if (k > w) w = k;
    dlg_name(x, 1, (u8)(map_menu_width(name) + 8), name, 0);
    dlg_frame(x, 13, w, 20, 0);
    text(x + 4, 17, first, C_WHITE, 156);
    text(x + 4, 24, second, C_WHITE, 156);
    dlg_face(scene_face(town_room()), 1, 1, 0);
}
// CHOICE_MENU.md: one renderer, a width fitted to the labels, the same five pixels above the
// first row and below the last, the glove wholly on screen, the glove animated.
// The glove is drawn after the page is blitted, so the layouts live here.
static ChoiceLayout store_box = { 24, 36, 0, 0, 7, CHOICE_TAB_PLAIN, 3, 1, 0, 6, 12, 9, 158, -16, 9 };
static ChoiceLayout office_box = { 24, 36, 104, 49, 7, CHOICE_TAB_PLAIN, 3, 1, 0, 6, 12, 9, 158, -16, 9 };
static void action_menu(const char *title, const char *const *labels, u8 n, u8 grey, u8 centred)
{
    ChoiceLayout *p = &store_box;
    u8 i, w = 0;
    for (i = 0; i < n; i++) { u8 k = map_menu_width(labels[i]); if (k > w) w = k; }
    p->width = (u8)(w + 12);
    p->height = (u8)(13 + 9 * n);
    p->x = centred ? (s16)((160 - p->width) / 2) : 24;
    p->y = centred ? (s16)((100 - p->height) / 2) : 36;
    choice_frame(p, title);
    for (i = 0; i < n; i++) choice_row(p, i, labels[i], i == grey ? C_DGRAY : C_BLACK);
}
// The source's own first menu. Picking a soldier hires one; the screen that follows it in the
// original (its stats, its name) has no reference capture yet, so Change Name stays grey.
// The disc's own labels (WORLD/WORLD.BIN's world-menu strings), not the retranslation's.
static const char *const office_rows_text[OFFICE_ROWS] =
    { "Male fighter", "Female fighter", "Change name", "Leave store" };
// The candidate the guildmaster has just produced: his name, his job and what the source shows
// of him before you take him. The derived statistics belong to management's own page.
static void candidate_card(void)
{
    const SaveUnit *u = &save.unit[save.n];
    static const char *const signs[13] = { "Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
        "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces", "Serpentarius" };
    draw_rect(8, 16, 144, 62, C_BLACK);
    draw_rect(9, 17, 142, 60, C_WHITE);
    black(13, 20, candidate_name());
    black(13, 29, job_name(u->job));
    black(84, 29, u->type ? "Woman" : "Man");
    black(13, 40, "Brave"); number(48, 40, u->brave);
    black(76, 40, "Faith"); number(111, 40, u->faith);
    black(13, 49, "Level"); number(48, 49, u->level);
    black(76, 49, "Zodiac"); black(111, 49, u->zodiac < 13 ? signs[u->zodiac] : "?");
    black(13, 66, "Any key closes");
}
static void hire_rows(void)
{
    static const char *const rows[3] = { "Hire", "Quit", "Check status" };
    const SaveUnit *u = &save.unit[save.n];
    ChoiceLayout *p = &office_box;
    u8 i, w = 0;
    for (i = 0; i < 3; i++) { u8 k = map_menu_width(rows[i]); if (k > w) w = k; }
    p->width = (u8)(w + 12);
    p->height = (u8)(13 + 9 * 3);
    p->x = 24; p->y = 38;
    choice_frame(p, candidate_name());
    for (i = 0; i < 3; i++) choice_row(p, i, rows[i], C_BLACK);
    (void)u;
}
static void office_rows(void)
{
    ChoiceLayout *p = &office_box;
    u8 r, w = 0;
    for (r = 0; r < OFFICE_ROWS; r++) { u8 k = map_menu_width(office_rows_text[r]); if (k > w) w = k; }
    p->width = (u8)(w + 12);
    p->height = (u8)(13 + 9 * OFFICE_ROWS);
    choice_frame(p, "Menu");
    for (r = 0; r < OFFICE_ROWS; r++)
        choice_row(p, r, office_rows_text[r], r == 2 ? C_DGRAY : C_BLACK);
}
static u8 shop_tab_face(u8 t)
{
    u16 i;
    for (i = 1; i < 256; i++) {
        u8 k = sh_kind[i];
        if (k >= tab_first[t] && k <= tab_last[t] && sh_tier[i] && sh_tier[i] < 20) return (u8)i;
    }
    return 0;
}
// The source's tab strip: one icon a category, the chosen one boxed, F1/F5 between them.
static void shop_tabs(void)
{
    u8 t;
    draw_rect(0, 0, 160, 11, C_WHITE); draw_rect(0, 11, 160, 1, C_BLACK);
    for (t = 0; t < SHOP_TABS; t++) {
        s16 x = (s16)(4 + 13 * t);
        u8 id = shop_tab_face(t);
        if (t == wm.cat) {                   // an outline, so the icon stays readable
            draw_rect((s16)(x - 2), 0, 12, 1, C_BLACK); draw_rect((s16)(x - 2), 10, 12, 1, C_BLACK);
            draw_rect((s16)(x - 2), 0, 1, 11, C_BLACK); draw_rect((s16)(x + 9), 0, 1, 11, C_BLACK);
        }
        if (id) item_icon(x, 2, id);
        else text(x, 3, "?", C_DGRAY, 158);
    }
    text(88, 3, "F1/F5", C_BLACK, 158);
    text(120, 3, "MODE:info", C_BLACK, 158);
}
// Scroll marks in the shared style: a solid triangle at the box's right edge, only when the
// list runs past it. CHOICE_MENU.md rule 6.
static void scroll_marks(const ChoiceLayout *p, u8 n, u8 shown)
{
    s16 x = (s16)(p->x + p->width - 7), r;
    if (wm.ttop) for (r = 0; r < 3; r++) draw_rect((s16)(x + r), (s16)(p->y + 4 - r), (s16)(1 + 2 * r), 1, C_BLACK);
    if (wm.ttop + shown < n)
        for (r = 0; r < 3; r++)
            draw_rect((s16)(x + r), (s16)(p->y + p->height - 5 + r), (s16)(5 - 2 * r), 1, C_BLACK);
}
static ChoiceLayout shop_box =
    { 24, 14, 120, 0, 7, CHOICE_TAB_PLAIN, 3, 1, 0, 16, 12, 9, 158, -16, 9 };
// Everything but the swaying glove. Composed once per change into the page below, because
// redrawing the room, the tabs, twelve icons and the two plates cost 419k cycles a frame.
static void shop_rows(void)
{
    ChoiceLayout *p = &shop_box;
    u8 r, shown = shop_n - wm.ttop > SHOP_ROWS ? SHOP_ROWS : (u8)(shop_n - wm.ttop);
    if (!shop_n) shown = 1;
    p->height = (u8)(13 + 9 * shown);
    shop_tabs();
    choice_frame(p, wm.buy ? "Buy" : "Sell");
    if (!shop_n) black((s16)(p->x + p->text_x), (s16)(p->y + p->row_y), wm.buy ? "Nothing here" : "Nothing to sell");
    for (r = 0; r < shown; r++) {
        u8 id = shop_nth((u8)(wm.ttop + r));
        s16 y = (s16)(p->y + p->row_y + (u16)r * p->pitch);
        item_icon((s16)(p->x + 5), (s16)(y - 1), id);
        black((s16)(p->x + p->text_x), y, item_name(id));
        if (!wm.buy) number((s16)(p->x + 76), y, save.inv[id]);
        right((s16)(p->x + p->width - 10), y, wm.buy ? item_price(id) : (u16)(item_price(id) >> 1));
    }
    if (shop_n) scroll_marks(p, shop_n, shown);
}

// The bench itself: the party on the left, and under the cursor what the store would put on
// that unit and what it would cost.
static ChoiceLayout fit_box =
    { 20, 14, 124, 0, 7, CHOICE_TAB_PLAIN, 3, 1, 0, 6, 12, 9, 158, -16, 9 };
// "HP 51>59" when it changes, "HP 51" when it does not; returns the next column.
static s16 fit_pair(s16 x, const char *label, u16 now, u16 then)
{
    char d[9];
    const char *n;
    x = text(x, 75, label, C_DGRAY, 142);
    x += 2;
    n = wf_digits(d, now);
    x = text(x, 75, n, C_WHITE, 142);
    if (then != now) {
        x = text(x, 75, ">", C_DGRAY, 142);
        n = wf_digits(d, then);
        x = text(x, 75, n, C_WHITE, 142);
    }
    return (s16)(x + 5);
}
static void fit_rows(void)
{
    ChoiceLayout *p = &fit_box;
    u8 r, shown = save.n - wm.ttop > FIT_ROWS ? FIT_ROWS : (u8)(save.n - wm.ttop);
    u8 id = save.n ? best_fit(&save.unit[wm.trow]) : 0;
    if (!save.n) shown = 1;
    p->height = (u8)(13 + 9 * shown);
    choice_frame(p, "Best fit");
    if (!save.n) black((s16)(p->x + p->text_x), (s16)(p->y + p->row_y), "No one to fit");
    for (r = 0; r < shown; r++) {
        const SaveUnit *u = &save.unit[wm.ttop + r];
        s16 y = (s16)(p->y + p->row_y + (u16)r * p->pitch);
        black((s16)(p->x + p->text_x), y, identity ? unit_label(u) : job_name(u->job));
        black((s16)(p->x + 62), y, job_name(u->job));
    }
    if (save.n) scroll_marks(p, save.n, shown);
    draw_rect(20, 64, 124, 18, C_BLACK);
    if (!save.n) return;
    if (id) {
        item_icon(23, 66, id);
        text(34, 67, item_name(id), C_WHITE, 110);
        { char d[9]; const char *n = wf_digits(d, item_price(id));
          text((s16)(140 - 4 * (s16)strlen(n)), 67, n, C_WHITE, 142); }
    } else text(24, 67, "Nothing in stock would help", C_DGRAY, 142);
    // Try it on: the unit as it stands, and as it would stand wearing this.
    {
        SaveUnit worn = save.unit[wm.trow];
        Stats now = unit_stats(&save.unit[wm.trow]), then;
        u8 slot = id ? fit_slot(id) : 255;
        s16 x = 24;
        if (slot < 5) worn.eq[slot] = id;
        then = unit_stats(&worn);
        x = fit_pair(x, "HP", now.hp, then.hp);
        x = fit_pair(x, "MP", now.mp, then.mp);
        x = fit_pair(x, "PA", now.pa, then.pa);
        x = fit_pair(x, "MA", now.ma, then.ma);
        fit_pair(x, "Sp", now.sp, then.sp);
    }
}
static void town_compose(void)
{
    static const char *const store_rows[4] = { "Buy", "Sell", "Fitting room", "Leave store" };
    static const char *const fur_rows[3] = { "Buy", "Sell", "Leave store" };
    u8 office = (u8)(wm.screen == SC_OFFICE);
    void *l = rt_light, *d = rt_dark;
    rt_light = page; rt_dark = page + PAGE_PLANE;
    zx0_unpack(scene_room(TOWN_ROOM(office)) + 1, page);   // the room, straight into the page
    if (wm.screen == SC_SHOP) {          // a list screen keeps the room but takes its height
        shop_rows();
        {
            s16 left = war_funds(gil());
            u8 id = shop_n ? shop_nth(wm.trow) : 0;
            eqp_all((s16)(left - 3), id ? item_wearers(id) : 0, save.n);
        }
    } else if (wm.screen == SC_FIT) {
        fit_rows();
        war_funds(gil());
    } else {
        says(town_speaker(),
             wm.screen == SC_HIRE ? "This one is ready." : "Welcome.",
             office || wm.screen == SC_HIRE ? "What recruit are you after?" :
             wm.screen == SC_FUR ? "Anything from the hunt?" : "What can I do for you?");
        if (office) {
            office_rows();
            office_plates(save.n, PARTY_MAX, office_price(wm.trow < 2 ? wm.trow : 0), gil());
        } else if (wm.screen == SC_HIRE) {
            hire_rows();
            office_plates(save.n, PARTY_MAX, office_price(wm.cand), gil());
        } else if (wm.screen == SC_FUR) {
            action_menu("Menu", fur_rows, 3, 0, 0);      // Buy waits on the poaching loop
            war_funds(gil());
        } else {
            action_menu("Menu", store_rows, 4, 255, 0);
            war_funds(gil());
        }
    }
    rt_light = l; rt_dark = d;
    p_screen = wm.screen; p_cat = wm.cat; p_buy = wm.buy; p_trow = wm.trow;
    p_ttop = wm.ttop; p_n = shop_n; p_gil = gil();
}
static void town_page(void)
{
    if (p_screen != wm.screen || p_cat != wm.cat || p_buy != wm.buy || p_trow != wm.trow ||
        p_ttop != wm.ttop || p_n != shop_n || p_gil != gil())
        town_compose();
    page_blit();
    if (wm.screen == SC_SHOP) { if (shop_n) choice_cursor_animated(&shop_box, (u8)(wm.trow - wm.ttop), rt_frame); }
    else if (wm.screen == SC_FIT) { if (save.n) choice_cursor_animated(&fit_box, (u8)(wm.trow - wm.ttop), rt_frame); }
    else choice_cursor_animated(wm.screen == SC_OFFICE || wm.screen == SC_HIRE ? &office_box : &store_box,
                                wm.trow, rt_frame);
}

static void town_render(void)
{
    static const char *const town_rows[4] = { "Shop", "Soldier Office", "Fur Shop", "Leave" };
    if (wm.screen == SC_TOWN) {
        s16 cx = (s16)(wm.x - 80), cy = (s16)(wm.y - 50);
        rt_clamp_cam(&map, &cx, &cy);
        map_background(cx, cy, 1);
        ramza((s16)(wm.x - cx), (s16)(wm.y - cy));
        place_plate(loc_name(wm.at));
        action_menu("Town", town_rows, 4, (u8)(fur_open() ? 255 : 2), 1);
        choice_cursor_animated(&store_box, wm.trow, rt_frame);
        war_funds(gil());
        p_screen = 255;                 // the map wrote over nothing of the page, but the
        return;                         // next town screen must be composed again anyway
    }
    town_page();
    if (wm.screen == SC_SHOP && wm.detail && shop_n) item_detail(shop_nth(wm.trow));
    if (wm.screen == SC_HIRE && wm.card) candidate_card();
}

void game_render(void)
{
    s16 cx, cy;
    u8 i;
    if (!bank || !mgb) { draw_clear(); draw_text(4, 40, "Missing ffwm / ffmg", 0, C_BLACK); return; }
    if (wm.screen) { town_render(); if (!wm.note) return; }
    cx = (s16)((wm.moving?wm.x:wm.handx) - 80); cy = (s16)((wm.moving?wm.y:wm.handy) - 50);
    rt_clamp_cam(&map, &cx, &cy);
    map_background(cx,cy,1);
    ramza((s16)(wm.x - cx), (s16)(wm.y - cy));
    if (!wm.moving && !wm.menu) {
        s16 hx=wm.sel==255?wm.handx:loc_x(wm.sel),hy=wm.sel==255?wm.handy:loc_y(wm.sel);
        RtSprite sp = { 32, HAND_H, hand_gfx[0], hand_gfx[1], hand_gfx[2] };
        draw_sprite((s16)(hx - cx - 26), (s16)(hy - cy - 7), &sp);
    }
    place_plate(wm.sel==255?"Move cursor":loc_name(wm.sel));
    date_plate();
    war_funds(gil());               // the same plate as the town screens
    if(wm.menu) {
        static const char *const rows[]={"Move","Formation","Brave Story","Tutorial","Data","Option"};
        static const ChoiceLayout menu={88,43,72,57,3,CHOICE_TAB_PLAIN,3,1,25,6,10,7,158,-20,7};
        choice_frame(&menu,"Menu");
        for(i=0;i<6;i++)choice_row(&menu,i,rows[i],i==2||i==3||i==5?C_DGRAY:C_BLACK);
        choice_cursor_animated(&menu,wm.row,rt_frame);
    }
    if (wm.note) {
        static const char *const notes[] = { "", "No road there yet", "Not ripped yet", "Not available yet",
            "Campaign saved", "Not enough gil", "99 of those already", "The party is full" };
        panel(20, 36, 120, 22);
        black(26, 40, notes[wm.note]);
        black(26, 48, "ENTER or SHIFT");
    }
}

u16 map_result(void) { return wm.result; }
