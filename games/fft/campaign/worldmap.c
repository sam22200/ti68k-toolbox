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
#include "../../../runtime/core/rt.h"
#include "../art.h"
#include "save.h"
#include <stdlib.h>
#include <string.h>
#include "../base.inc"

enum { LOC_N = 43, ROUTE_N = 48, NOTE_NONE = 0, NOTE_NOROAD, NOTE_LATER, NOTE_UNAVAILABLE, NOTE_SAVED };
typedef struct {
    u8 at, sel;                          // Ramza's location, the chosen one
    u8 moving, leg, legs, step;          // travelling: leg of path[], its point
    u8 path[16];                         // route ids, bit 7: run end to start
    s16 x, y;                            // Ramza on the map (texture pixels)
    s16 handx, handy;                    // free hand hotspot; never commits a journey
    u8 menu, row;
    u8 note, changed, tick;
    u16 result;                          // what fftmap returns
} Wm;
Wm wm;
Save save;
static const u8 *bank, *mgb;
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
#define MGTEXT_TEXT_ONLY
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

static u8 flag(u8 k) { return (u8)(save.flags[k >> 3] >> (k & 7) & 1); }
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

static void travel_step(void)            // a pixel a frame along the route's points
{
    const u8 *p = point(wm.leg, (u8)(wm.step + 1));
    s16 tx = s16v(p), ty = s16v(p + 2);
    wm.x += tx > wm.x ? 1 : tx < wm.x ? -1 : 0;
    wm.y += ty > wm.y ? 1 : ty < wm.y ? -1 : 0;
    if (wm.x != tx || wm.y != ty) return;
    if (++wm.step + 1 < route(wm.path[wm.leg] & 0x7F)[3]) return;
    wm.step = 0;
    if (++wm.leg == wm.legs) arrive();
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
    bank = mgb = RT_NULL;                    // (statics survive between runs)
    strip_decor=decor_ready=0;
    memset(&wm, 0, sizeof wm);
    bank = rt_file("ffwm", &size);
    if (bank && memcmp(bank, "FWM3", 4)) bank = RT_NULL;
    mgb = rt_file("ffmg", &size);
    if (mgb && memcmp(mgb, "FMG3", 4)) mgb = RT_NULL;
    save_load(&save);
    if (bank) {
        map.w = bank[14]; map.h = bank[15];
        stride=b16(part(0));strip_bytes=stride<<4;
        wmcache=malloc((u16)(strip_bytes<<4)+5*156);
        if(!wmcache){bank=RT_NULL;return;}
        idle_sprites=wmcache+(strip_bytes<<4);
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
    decor_ready=0;
}

static u8 leave(u16 result)
{
    wm.result = result;
    if (wm.changed) rt_save(SAVE_NAME, &save, sizeof save);
    release_map();
    return 0;
}

u8 game_update(void)
{
    u32 key = rt_keys & ~rt_prev;
    wm.tick++;
    if (!bank || !mgb) return !(key & K_ESC);
    if (wm.moving) { travel_step(); if(wm.moving)travel_step(); return 1; }
    if (wm.note) { if (key & (K_ENTER | K_A | K_B | K_ESC)) wm.note = 0; return 1; }
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

void game_render(void)
{
    s16 cx, cy;
    u8 i;
    if (!bank || !mgb) { draw_clear(); draw_text(4, 40, "Missing ffwm / ffmg", 0, C_BLACK); return; }
    cx = (s16)((wm.moving?wm.x:wm.handx) - 80); cy = (s16)((wm.moving?wm.y:wm.handy) - 50);
    rt_clamp_cam(&map, &cx, &cy);
    map_background(cx,cy,1);
    ramza((s16)(wm.x - cx), (s16)(wm.y - cy));
    if (!wm.moving && !wm.menu) {
        s16 hx=wm.sel==255?wm.handx:loc_x(wm.sel),hy=wm.sel==255?wm.handy:loc_y(wm.sel);
        RtSprite sp = { 32, HAND_H, hand_gfx[0], hand_gfx[1], hand_gfx[2] };
        draw_sprite((s16)(hx - cx - 26), (s16)(hy - cy - 7), &sp);
    }
    draw_rect(0, 91, 160, 9, C_WHITE); draw_rect(0, 90, 160, 1, C_BLACK);
    black(3, 93, wm.sel==255?"Move cursor":loc_name(wm.sel));
    if(wm.menu) {
        static const char *const rows[]={"Move","Formation","Brave Story","Tutorial","Data","Option"};
        static const ChoiceLayout menu={88,43,72,57,3,CHOICE_TAB_PLAIN,3,1,25,6,10,7,158,-20,7};
        choice_frame(&menu,"Menu");
        for(i=0;i<6;i++)choice_row(&menu,i,rows[i],i==2||i==3||i==5?C_DGRAY:C_BLACK);
        choice_cursor_animated(&menu,wm.row,rt_frame);
    }
    if (wm.note) {
        static const char *const notes[] = { "", "No road there yet", "Not ripped yet", "Not available yet", "Campaign saved" };
        panel(20, 36, 120, 22);
        black(26, 40, notes[wm.note]);
        black(26, 48, "ENTER or SHIFT");
    }
}

u16 map_result(void) { return wm.result; }
