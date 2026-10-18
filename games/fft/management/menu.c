// Portable management-menu prototypes with session-local Squire/Chemist previews. Pixel fonts, original actors/portraits;
// layout/navigation adapted to 160x100. No battle or campaign state is modified.
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "../art.h"
#include "../units.h"
#include "../interface.h"
#include "../x/management/data/fixture.h"
#ifdef RT_CYCLES
#include "../../../tools/m68kbench/bench.h"
static u32 frame_start;
#endif

MenuState menu;
static const u8 *font, *actors, *ui, *icons, *visuals;
static u8 *cache;
static u8 dirty;
static u16 cached_key;
static u8 cached_at;
typedef struct { u8 start[6],end[6],next,chars,rows,x,y,w,h; } DialogPage;
static DialogPage page;
static u8 cached_reveal;
#define DIALOG_ROWS 6
static const char dialogue_text[] = "We should prepare before the next battle, Ramza. Check everyone's equipment and abilities before we leave.";
static const char *const commands[] = { "Item", "Ability", "Change Job", "Dismiss", "Order Unit" };
static const char *const equipment[] = { "R.Hand", "L.Hand", "Head", "Body", "Accessory" };
static const char *const ability_menu[] = { "Set", "Remove", "Learn" };
static const char *const item_menu[] = { "Equip", "Best Fit", "Remove", "Inventory" };
static const char *const learn[] = { "Accumulate", "Dash", "Throw Stone", "Heal" };
static const char *const item_learn[] = { "Potion", "Antidote", "Eye Drop", "Echo Grass", "Phoenix Down" };
static const char *const jobs[] = { "Squire", "Chemist", "Knight", "Archer" };
static const char *const slot_name[] = { "Action", "Second", "React", "Support", "Move" };
static const char *const titles[] = { "Formation", "Formation", "Item", "Equip", "Ability", "Learn",
                                     "Change Job", "Status", "Gariland", "Set Ability", "Configuration", "Location" };
static const u8 idle[8] = { 2,1,0,1,2,3,4,3 };
void game_cleanup(void);

// Dense visual fixtures repeat the five source records; never index beyond the party.
static u8 source_unit(u8 unit) { while(unit>=5)unit-=5;return unit; }
static const MenuUnit *unit_data(u8 unit) {
    unit=source_unit(unit);return menu.job[unit]?&alternate[unit]:&party[unit];
}
static u8 grid_columns(void) { return menu.small_actors==2?5:4; }
static u8 grid_width(void) { return menu.small_actors==2?32:40; }
static u8 roster_count(void) { return menu.roster_demo?(u8)(3*grid_columns()):5; }
static s16 grid_x(u8 i) { return (i%grid_columns())*grid_width(); }
static s16 grid_y(u8 i) { return 15+(i/grid_columns())*22; }
static u8 current_job(void) { return unit_data(menu.unit)->v[0]>=4; }
static const char *dialog_text(void) { return menu.stress==2?"Let us go, Ramza.":menu.stress?dialogue_stress:dialogue_text; }

#ifndef __m68k__
static u16 be16(const u8 *p) { return (u16)((u16)p[0]<<8 | p[1]); }
static u32 be32(const u8 *p) { return (u32)be16(p)<<16 | be16(p+2); }
#endif

// Font is eight columns wide, one ink plane and one mask. No endian conversion, scaling,
// per-pixel division or unpacked glyph cache on either target.
static s16 text_color(s16 x,s16 y,const char *s,u8 small,u8 color,s16 limit)
{
    static const u8 zero[8]={0};
    u8 h=small?5:8;
    const u8 *base=font+198+(small?1520:0);
    while(*s) {
        u8 c=(u8)*s++,adv;
        const u8 *g;
        if(c<32||c>126)c='?';
        c-=32;adv=font[8+(small?95:0)+c];
        if(x+adv>limit)break;
        g=base+(u16)c*(small?10:16);
        {RtSprite sp={8,h,color&1?g:zero,color&2?g:zero,g+h};draw_sprite(x,y,&sp);}
        x+=adv;
    }
    return x;
}

static s16 text(s16 x,s16 y,const char *s,u8 small,u8 inverse,s16 limit)
{return text_color(x,y,s,small,inverse?C_WHITE:C_BLACK,limit);}

static u8 row_pitch(void) { return menu.compact?8:10; }
static u8 visible_rows(void) { return menu.compact?5:4; }
static u8 dialog_pitch(void) { return 7; }
static u8 dialog_y(void) { return page.y+4; }

// Actual font advances, explicit source newlines, no reserved empty text rows.
// Speaker portrait sits above the box so it does not force empty space below.
static void dialogue_layout(void)
{
    const char *str=dialog_text();u8 row,at=menu.dialog_at,max_width=0;
    page.chars=0;page.rows=0;
    for(row=0;row<DIALOG_ROWS&&str[at];row++) {
        u8 end,last=255,width=0,start,k;
        while(str[at]==' ')at++;
        start=at;
        while(str[at]&&str[at]!='\n') {
            u8 w=font[103+(u8)str[at]-32];
            if((u16)width+w>148)break;
            width+=w;if(str[at]==' ')last=at;at++;
        }
        end=at;
        if(str[at]&&str[at]!='\n'&&last!=255&&last>start)end=last;
        page.start[row]=start;page.end[row]=end;
        width=0;for(k=start;k<end;k++)width+=font[103+(u8)str[k]-32];
        if(width>max_width)max_width=width;
        page.chars+=(u8)(end-start);page.rows++;at=end;
        if(str[at]=='\n')at++;
        while(str[at]==' ')at++;
    }
    page.next=str[at]?at:0;
    page.w=max_width+8;if(page.w<58)page.w=58;
    page.h=6+page.rows*7;page.x=(160-page.w)>>1;page.y=92-page.h;
}

static s16 number(s16 x,s16 y,u16 n,u8 small)
{
    char b[6];u8 k=5;b[k]=0;
    do{b[--k]=(char)('0'+n%10);n/=10;}while(n);
    return text(x,y,b+k,small,0,158);
}

static void panel(s16 x,s16 y,s16 w,s16 h)
{
    draw_rect(x,y,w,h,C_BLACK);
    draw_rect(x+1,y+1,w-2,h-2,C_WHITE);
    draw_rect(x+2,y+2,w-4,1,C_LGRAY);
}

static void dialogue_window(s16 x,s16 y,s16 w,s16 h)
{
    u8 row,band=(u8)(2+(h>>3));
    draw_rect(x,y,w,h,C_BLACK);draw_rect(x+1,y+1,w-2,h-2,C_WHITE);
    draw_rect(x+2,y+2,w-4,h-4,C_BLACK);
    draw_rect(x+2,y+2,w-4,band-2,C_DGRAY);
    // Same dark-grey -> dark/black checker -> black ramp as FFA's CT-style UI.
    // This is cached on a window change, not generated in every animation frame.
    for(row=band;row<band+2&&row<h-2;row++) {
        s16 xx;u8 pat=(y+row)&1?0xaa:0x55;
        u8 *l=(u8*)rt_light+(y+row)*RT_PBYTES;
        for(xx=x+2;xx<x+w-2;xx++) {
            u8 m=(u8)(0x80>>(xx&7));
            if(pat&m)l[xx>>3]|=m;else l[xx>>3]&=(u8)~m;
        }
    }
    draw_rect(x+1,y+h-2,w-2,1,C_LGRAY);
    if(menu.dialog_light) {
        // Invert only the cached window bytes, including feathered top and bevel.
        // Portraits/scenery are drawn afterwards in their original palette.
        u8 left=(u8)x>>3,right=(u8)(x+w-1)>>3;
        u8 first=(u8)(0xffu>>(x&7)),last=(u8)(0xffu<<(7-((x+w-1)&7)));
        for(row=0;row<h;row++) {
            u8 *l=(u8*)rt_light+(y+row)*RT_PBYTES+left;
            u8 *d=(u8*)rt_dark+(y+row)*RT_PBYTES+left;
            u8 col;
            for(col=left;col<=right;col++) {
                u8 mask=0xff;
                if(col==left)mask&=first;
                if(col==right)mask&=last;
                *l++^=mask;*d++^=mask;
            }
        }
    }
}

static void equipment_icon(s16 x,s16 y,u8 id)
{
    const u8 *p=icons+8+(u16)id*24;
    RtSprite sp={8,8,p,p+8,p+16};draw_sprite(x,y,&sp);
}

static void heading(const char *s)
{
    draw_rect(0,0,160,13,C_BLACK);text(5,3,s,menu.compact,1,153);
}

static void foot(const char *s)
{
    draw_rect(0,94,160,6,C_WHITE);draw_rect(0,93,160,1,C_BLACK);
    draw_text(2,95,s,0,C_BLACK);
}

static void portrait(s16 x,s16 y,u8 gfx)
{
    const u8 *p=ui+UI_PORTRAITS+(u16)gfx*348;
#ifdef __m68k__
    RtSprite sp={32,29,p,p+116,p+232};
#else
    u32 r[3][29];u8 k,j;
    RtSprite sp={32,29,r[0],r[1],r[2]};
    for(k=0;k<3;k++)for(j=0;j<29;j++)r[k][j]=be32(p+(u16)k*116+(u16)j*4);
#endif
    draw_sprite(x,y,&sp);
}

static void actor_gfx(s16 x,s16 y,u8 gfx)
{
    u8 frame=idle[(menu.tick>>3)&7];
    const u8 *p=actors+(u16)gfx*(UNIT_FRAMES*2*UNIT_FB)+(u16)frame*UNIT_FB;
#ifdef __m68k__
    RtSprite sp={16,UNIT_SH,p,p+52,p+104};
#else
    u16 r[3][UNIT_SH];u8 k,j;
    RtSprite sp={16,UNIT_SH,r[0],r[1],r[2]};
    for(k=0;k<3;k++)for(j=0;j<UNIT_SH;j++)r[k][j]=be16(p+(u16)k*52+(u16)j*2);
#endif
    draw_sprite(x,y,&sp);
}

static void actor(s16 x,s16 y,u8 unit) { actor_gfx(x,y,unit_data(unit)->v[0]); }

static void visual_actor(s16 x,s16 y,const u8 *p,u8 small)
{
    u8 h=small?16:26;u16 plane=(u16)h*2;
#ifdef __m68k__
    RtSprite sp={16,h,p,p+plane,p+2*plane};
#else
    u16 rows[3][26];u8 k,r;RtSprite sp={16,h,rows[0],rows[1],rows[2]};
    for(k=0;k<3;k++)for(r=0;r<h;r++)rows[k][r]=be16(p+k*plane+2*r);
#endif
    draw_sprite(x,y,&sp);
}
static void small_actor(s16 x,s16 y,u8 u)
{
    u8 gfx=unit_data(u)->v[0];
    // Delita has no player-party slot: the compact bank omits his unused variant.
    if(gfx)gfx--;
    const u8 *p=visuals+MV_SMALL+(u16)gfx*480+(u16)idle[(menu.tick>>3)&7]*96;
    visual_actor(x,y,p,1);
}
static void job_actor(s16 x,s16 y,u8 job)
{
    u8 female=party[source_unit(menu.unit)].v[0]==3||party[source_unit(menu.unit)].v[0]==5;
    if(job<2) {
        u8 gfx=job?(female?5:4):(female?3:2);
        if(job==0&&source_unit(menu.unit)==0)gfx=UG_RAMZA;
        actor_gfx(x,y,gfx);
    } else {
        u8 gfx=(job-2)*2+female;
        visual_actor(x,y,visuals+MV_JOB+(u16)gfx*780+(u16)idle[(menu.tick>>3)&7]*156,0);
    }
}
static void formation_actors(void)
{
    u8 i;
    if(menu.layout==LAYOUT_CLASSIC)for(i=0;i<roster_count();i++) {
        if(menu.small_actors)small_actor(grid_x(i)+(grid_width()>>1)-5,grid_y(i),i);
        else actor(17+(i&3)*39,18+(i>>2)*32,i);
    }
    else if(menu.layout==LAYOUT_CARD)for(i=0;i<5;i++) {
        if(menu.small_actors)small_actor(13+i*31,70,i);else actor(11+i*31,63,i);
    }
}

static void hand(s16 x,s16 y)
{
    static const u8 sway[8]={0,1,2,3,3,2,1,0};
    RtSprite sp={32,HAND_H,hand_gfx[0],hand_gfx[1],hand_gfx[2]};
    draw_sprite(x+sway[(menu.tick>>2)&7],y,&sp);
}

static void unit_header(void)
{
    const MenuUnit *u=unit_data(menu.unit);
    portrait(5,15,source_unit(menu.unit)==0?UG_RAMZA:u->v[0]);
    text(28,15,u->name,menu.compact,0,126);text(28,26,u->job,1,0,126);
    draw_text(78,27,"Mv",0,C_BLACK);number(90,27,u->v[9],1);
    draw_text(105,27,"Jp",0,C_BLACK);number(117,27,u->v[10],1);
    draw_text(28,39,"HP",0,C_BLACK);number(42,39,u->v[1],1);
    draw_text(72,39,"MP",0,C_BLACK);number(86,39,u->v[2],1);
    draw_text(119,39,"Lv",0,C_BLACK);number(133,39,u->v[3],1);
    draw_rect(2,46,156,1,C_LGRAY);
}

static u8 count(void)
{
    switch(menu.screen) {
    case UI_CONTEXT:case UI_EQUIP:case UI_SET:return 5;
    case UI_LEARN:return unit_data(menu.unit)->v[0]>=4?5:4;
    case UI_ITEM:case UI_JOBS:return 4;
    case UI_ABILITY:return 3;
    case UI_CONFIG:return 4;
    default:return 1;
    }
}

static u8 first_row(void) { u8 n=visible_rows();return menu.row>=n?menu.row-n+1:0; }

static void formation(void)
{
    const MenuUnit *u=unit_data(menu.unit);u8 i;
    if(menu.layout==LAYOUT_CLASSIC) {
        if(menu.small_actors) {
            u8 slots=3*grid_columns(),w=grid_width();
            for(i=0;i<slots;i++) {
                s16 x=grid_x(i),y=grid_y(i);
                if(i==menu.unit)draw_rect(x+1,y,w-2,21,C_LGRAY);
                draw_rect(x+1,y+21,w-2,1,i==menu.unit?C_BLACK:C_LGRAY);
                if(i<roster_count())text(x+2,y+16,unit_data(i)->name,1,0,x+w-2);
            }
        } else for(i=0;i<8;i++) {
            s16 x=3+(i&3)*39,y=16+(i>>2)*32;
            if(i==menu.unit) {draw_rect(x,y,37,31,C_LGRAY);draw_rect(x,y+30,37,1,C_BLACK);}
            else draw_rect(x+4,y+30,29,1,C_LGRAY);
            if(i<5) {draw_text(x+1,y+1,"Lv",0,C_BLACK);number(x+10,y+1,unit_data(i)->v[3],1);}
        }
        draw_rect(0,81,160,1,C_BLACK);
        text(4,81,u->name,menu.small_actors?1:menu.compact,0,79);
        text(84,81,u->job,menu.small_actors?1:menu.compact,0,158);
        if(menu.small_actors) {
            text(4,88,"Lv",1,0,14);number(16,88,u->v[3],1);
            text(35,88,"HP",1,0,47);number(49,88,u->v[1],1);
            text(82,88,"MP",1,0,94);number(96,88,u->v[2],1);
        }
    } else if(menu.layout==LAYOUT_ROSTER) {
        panel(2,16,38,71);portrait(10,20,u->v[0]);
        draw_text(6,55,"HP",0,C_BLACK);number(19,55,u->v[1],1);
        draw_text(6,68,"MP",0,C_BLACK);number(19,68,u->v[2],1);
        for(i=0;i<5;i++) {
            s16 y=16+i*14;
            if(i==menu.unit)draw_rect(45,y,112,14,C_LGRAY);
            text(65,y+1,unit_data(i)->name,menu.compact,0,157);
            if(menu.compact) {
                draw_text(65,y+8,unit_data(i)->job,F_SMALL,C_BLACK);
                number(139,y+1,unit_data(i)->v[1],1);
            }
            draw_rect(64,y+13,91,1,C_LGRAY);
        }
        text(45,85,u->job,1,0,157);
    } else {
        unit_header();
        draw_text(5,51,"Brave",0,C_BLACK);number(29,51,u->v[4],1);
        draw_text(51,51,"Faith",0,C_BLACK);number(75,51,u->v[5],1);
        draw_text(100,51,"Move",0,C_BLACK);number(121,51,u->v[9],1);
        for(i=0;i<5;i++) {
            s16 x=5+i*31;
            if(i==menu.unit)draw_rect(x,61,28,31,C_LGRAY);
            draw_rect(x+4,89,20,1,C_BLACK);
        }
    }
    foot("ENTER:status AL:menu CAT:config");
}

static void body_rows(const char *const *labels,u8 n)
{
    u8 i,first=first_row();
    for(i=first;i<n&&i<first+visible_rows();i++)text(31,48+(i-first)*row_pitch(),labels[i],menu.compact,0,155);
}

static void status(void)
{
    const MenuUnit *u=unit_data(menu.unit);
    const char *const names[]={"Move","Jump","Speed","PA","MA","Brave","Faith"};
    static const u8 at[]={9,10,8,6,7,4,5};u8 i;
    for(i=0;i<7;i++) {
        s16 x=i<4?6:85,y=49+(i<4?i:i-4)*row_pitch();
        text(x,y,names[i],1,0,x+52);number(x+54,y,u->v[at[i]],1);
    }
    text(6,85,u->eq[0],1,0,155);
    foot("F1/F5:unit  ENTER:equipment");
}

static void dialogue(void)
{
    const char *str=dialog_text(),*speaker=menu.stress==1?"Help":"Delita";u8 i,width=8;
    const char *c=speaker;
    while(*c)width+=font[103+(u8)*c++-32];
    dialogue_layout();
    draw_rect(0,13,160,79,C_LGRAY);
    draw_rect(7,37,146,1,C_WHITE);draw_rect(19,24,1,28,C_WHITE);
    dialogue_window(page.x,page.y,page.w,page.h);
    dialogue_window(page.x+27,page.y-12,width,12);
    text(page.x+31,page.y-9,speaker,1,!menu.dialog_light,page.x+27+width-4);
    portrait(page.x+3,page.y-29,menu.stress==1?0:1);
    for(i=0;i<page.rows;i++) {
        char line[80];u8 at=page.start[i],k=0,complete=menu.reveal>=page.chars;
        while(at<page.end[i]&&k<79)line[k++]=str[at++];
        line[k]=0;
        text_color(page.x+4,dialog_y()+i*dialog_pitch(),line,1,
                   menu.dialog_light?(complete?C_BLACK:C_LGRAY):(complete?C_WHITE:C_DGRAY),page.x+page.w-4);
    }
    foot("ENTER:next F5:103-char test");
}

// Recolour the frontier at the exact proportional pen position, in cached planes.
static void dialogue_range(u8 first,u8 last,u8 color)
{
    const char *str=dialog_text();u8 row,n=0;
    for(row=0;row<page.rows;row++) {
        u8 len=page.end[row]-page.start[row],a,b,k=0,j;u16 x=page.x+4;char s[80];
        if(first>=n+len){n+=len;continue;}if(last<=n)break;
        a=first>n?(u8)(first-n):0;b=last<n+len?(u8)(last-n):len;
        for(j=0;j<a;j++)x+=font[103+(u8)str[page.start[row]+j]-32];
        while(a+k<b){s[k]=str[page.start[row]+a+k];k++;}s[k]=0;
        text_color(x,dialog_y()+row*dialog_pitch(),s,1,color,page.x+page.w-4);n+=len;
    }
}

static void configuration(void)
{
    text(8,21,"Display",0,0,156);
    panel(6,33,148,17);text(33,38,menu.small_actors==2?"[x] Small: 5 x 3":menu.small_actors?"[x] Small: 4 x 3":"[ ] Small characters",1,0,155);
    text(33,54,"Dialogue preview",1,0,154);text(33,69,"Location introduction",1,0,154);
    text(33,84,menu.dialog_light?"[x] White dialogue":"[ ] White dialogue",1,0,154);
    foot("ENTER:change/open SHIFT:back");
}

static void job_window(void)
{
    u8 i;
    // Four readable cards replace the PS1 wheel's overlapping perspective sprites.
    for(i=0;i<4;i++) {
        if(i==menu.row)draw_rect(i*40+1,48,38,36,C_LGRAY);
        text(i*40+3,77,jobs[i],1,0,i*40+39);
    }
    text(7,86,menu.row==current_job()?"Current job":menu.row<2?"Available - job Lv 1":"Requires Squire Lv 2",1,0,156);
    foot("LEFT/RIGHT:job ENTER:change");
    if(menu.job_confirm) {
        panel(5,58,150,31);text(12,63,"Change to",1,0,65);text(69,63,jobs[menu.row],1,0,150);
        text(12,78,"ENTER:yes SHIFT:cancel",1,0,150);
    }
}

// Original MAPTITLE artwork, no offset shadow. A 16-pixel fade frontier
// sweeps left-to-right; only byte masks are composed, never a full-screen image.
static void location(void)
{
    u8 col,y;const u8 *src=visuals+MV_TITLE+(u16)menu.title_at*756;
    u16 sweep=menu.title_clock<112?(u16)menu.title_clock*2:224;
    u8 closing=menu.title_clock>=176;
    if(closing)sweep=(menu.title_clock-176)*2;
    draw_rect(0,36,160,27,C_BLACK);
    for(col=0;col<18;col++) {
        const u8 *p=src+(u16)col*42;u8 l[14],d[14],m[14];
        s16 edge=(s16)sweep-(s16)col*8;
        u8 bright=0,dim=0,mid=0,x;
        for(x=0;x<8;x++) {
            s16 delta=edge-x;u8 b=0x80>>x;
            if(closing)delta=-delta+16;
            if(delta>=16)bright|=b;else if(delta>=8)mid|=b;else if(delta>=0)dim|=b;
        }
        for(y=0;y<14;y++) {
            u8 ink=(u8)~p[28+y],visible=bright|mid|dim;
            m[y]=(u8)~(ink&visible);
            // White core and light/dark-grey advancing edge, preserving source antialias.
            l[y]=(p[y]&bright)|(ink&mid);
            d[y]=(p[14+y]&bright)|(ink&dim);
        }
        {RtSprite sp={8,14,l,d,m};draw_sprite(8+col*8,42,&sp);}
    }
    foot("F1/F5:place ENTER:replay");
}

static void notice_panel(void)
{
    if(!menu.notice)return;
    panel(5,60,150,31);
    if(menu.screen==UI_LEARN) {
        u8 chemist=unit_data(menu.unit)->v[0]>=4;
        text(12,63,chemist?item_learn[menu.row]:learn[menu.row],menu.compact,0,150);
        number(110,64,chemist?item_costs[menu.row]:learn_costs[menu.row],1);
        text(12,78,"Learning preview only",1,0,151);
    } else {
        text(12,63,"Read-only preview",menu.compact,0,150);
        text(12,78,"ENTER or SHIFT to close",1,0,151);
    }
}

static void base(void)
{
    u8 i,first=first_row();const MenuUnit *u=unit_data(menu.unit);
    draw_clear();heading(menu.roster_demo?"Formation test":titles[menu.screen]);
    if(menu.screen==UI_FORMATION||menu.screen==UI_CONTEXT) {
        formation();
        if(menu.screen==UI_CONTEXT) {
            // Composite actors before the opaque menu, never over its text.
            formation_actors();
            panel(56,15,101,18+5*row_pitch());text(65,18,"Menu",1,0,153);
            for(i=0;i<5;i++)text(81,29+i*row_pitch(),commands[i],menu.compact,0,153);
        }
        notice_panel();
        return;
    }
    if(menu.screen==UI_DIALOGUE){dialogue();return;}
    if(menu.screen==UI_CONFIG){configuration();return;}
    if(menu.screen==UI_LOCATION){formation();formation_actors();return;}
    unit_header();foot("ENTER:open  SHIFT:back F1/F5:unit");
    switch(menu.screen) {
    case UI_STATUS:status();break;
    case UI_ITEM:body_rows(item_menu,4);break;
    case UI_ABILITY:body_rows(ability_menu,3);break;
    case UI_EQUIP:
        for(i=first;i<5&&i<first+visible_rows();i++) {
            s16 y=48+(i-first)*row_pitch();
            text(31,y,equipment[i],1,0,69);
            equipment_icon(70,y-1,u->icons[i]);
            text(80,y,u->eq[i],menu.compact,0,155);
        }
        foot("Item preview only   SHIFT:back");break;
    case UI_SET:
        for(i=first;i<5&&i<first+visible_rows();i++) {
            s16 y=48+(i-first)*row_pitch();
            text(31,y,slot_name[i],1,0,76);
            text(80,y,u->slots[i],menu.compact,0,155);
        }
        foot("Loadout preview     SHIFT:back");break;
    case UI_LEARN:
        // Illustrative balance; costs are extracted from the original ability table.
        draw_rect(100,26,58,11,C_WHITE);text(102,27,"200 JP",1,0,157);
        for(i=first;i<count()&&i<first+visible_rows();i++) {
            s16 y=48+(i-first)*row_pitch();u8 chemist=u->v[0]>=4;
            text(31,y,chemist?item_learn[i]:learn[i],menu.compact,0,122);
            number(133,y,chemist?item_costs[i]:learn_costs[i],1);
        }
        foot("ENTER:description   SHIFT:back");break;
    case UI_JOBS:
        job_window();break;
    default:break;
    }
    notice_panel();
}

void game_init(void)
{
    u16 size;
    // Static pointers survive calculator launches; clear before allocation.
    cache=RT_NULL;font=actors=ui=icons=visuals=RT_NULL;
    memset(&menu,0,sizeof(menu));menu.compact=1;
    font=rt_file("fftmfont",&size);
    // Native rt_file size also includes the six-byte AMS OTH trailer.
    if(!font||size<2668||memcmp(font,"FFMF\x03\x08\x05\x5f",8))font=RT_NULL;
    icons=rt_file("fftmicon",&size);
    if(!icons||size<152||memcmp(icons,"FFMI\x01\x08\x08\x06",8))icons=RT_NULL;
    visuals=rt_file("fftmvis",&size);if(!visuals||size<MV_TITLE+1512||memcmp(visuals,"FFMV\x01\x10\x10\x00",8))visuals=RT_NULL;
    actors=rt_file("fftu",&size);if(!actors||size<6UL*UNIT_FRAMES*2*UNIT_FB)actors=RT_NULL;
    ui=rt_file("ffti",&size);if(!ui||size<UI_PORTRAITS+6*348)ui=RT_NULL;
    cache=(u8*)malloc(2*RT_PSIZE);dirty=1;cached_key=0;cached_at=0;cached_reveal=0;
    rt_state=&menu;rt_state_size=sizeof(menu);
#ifdef RT_CYCLES
    BENCH_VALUE(0x4D454E55UL);BENCH_VALUE(sizeof(menu));
#endif
}

void game_scenario(u16 n)
{
    menu.small_actors=menu.job_confirm=menu.stress=menu.title_clock=menu.roster_demo=0;
    memset(menu.job,0,sizeof(menu.job));
    if(n>=40) {
        menu.screen=n==40?UI_CONFIG:n==41?UI_LOCATION:n==42||n==48||n==49?UI_FORMATION:n==43||n==47?UI_DIALOGUE:UI_JOBS;
        menu.layout=0;menu.row=n==44||n==45?1:n==46?2:0;menu.unit=0;
        menu.small_actors=n==49?2:n==42||n==48;menu.roster_demo=n==48||n==49;menu.stress=n==47?2:n==43;menu.job_confirm=n==45;
        menu.reveal=255;menu.dialog_at=0;menu.dialog_light=0;menu.title_at=0;dirty=1;
        if(menu.screen==UI_DIALOGUE&&font)dialogue_layout();
        return;
    }
    menu.dialog_light=n>=30;if(n>=30)n-=30;
    menu.layout=(u8)(n/10);if(menu.layout>2)menu.layout=0;
    menu.screen=(u8)(n%10);if(menu.screen>=UI_SCREEN_N)menu.screen=0;
    menu.unit=0;menu.row=0;menu.reveal=255;menu.dialog_at=0;dirty=1;
    if(menu.screen==UI_DIALOGUE&&font)dialogue_layout();
}

u8 game_update(void)
{
    u32 key=rt_keys&~rt_prev;u8 n;
#ifdef RT_CYCLES
    frame_start=BENCH_CYCLES;
#endif
    menu.tick++;
    if(menu.screen==UI_LOCATION&&menu.title_clock<255)menu.title_clock++;
    if(!font||!actors||!ui||!icons||!visuals||!cache){if(input_pressed(K_ESC)){game_cleanup();return 0;}return 1;}
    // One character each update (32/s), twice the old half-rate typewriter.
    if(menu.screen==UI_DIALOGUE&&menu.reveal<page.chars)menu.reveal++;
    // Menu repeat: a short initial pause and then one step per four ticks.
    if(rt_keys&(K_UP|K_DOWN|K_LEFT|K_RIGHT)) {
        if(rt_keys!=rt_prev)menu.repeat=0;
        else if(++menu.repeat>=12&&!(menu.repeat&3))key|=rt_keys&(K_UP|K_DOWN|K_LEFT|K_RIGHT);
    } else menu.repeat=0;
    if(!key)return 1;
    if((key&K_MODE)&&menu.screen!=UI_DIALOGUE)menu.compact^=1;
    if(key&K_CATALOG){
        menu.screen=UI_CONFIG;menu.row=0;menu.notice=menu.job_confirm=0;dirty=1;return 1;
    }
    if(menu.screen==UI_DIALOGUE&&(key&K_C)){menu.dialog_light^=1;return 1;}
    if(menu.notice){if(key&(K_ENTER|K_A|K_B|K_ESC))menu.notice=0;return 1;}
    if(menu.screen==UI_DIALOGUE&&(key&(K_F1|K_F5))) {
        menu.stress=(menu.stress+(key&K_F5?1:2))%3;menu.dialog_at=menu.reveal=0;dialogue_layout();dirty=1;return 1;
    }
    if(menu.screen==UI_LOCATION) {
        if(key&(K_F1|K_F5))menu.title_at^=1;
        if(key&(K_F1|K_F5|K_ENTER|K_A))menu.title_clock=0;
        if(key&(K_B|K_ESC))menu.screen=UI_FORMATION;
        dirty=1;return 1;
    }
    if(menu.screen==UI_JOBS&&menu.job_confirm) {
        if(key&(K_B|K_ESC)){menu.job_confirm=0;dirty=1;return 1;}
        if(key&(K_ENTER|K_A)) {
            if(menu.row!=current_job())menu.job[source_unit(menu.unit)]^=1;
            menu.job_confirm=0;dirty=1;
        }
        return 1;
    }
    if(key&(K_F1|K_F5)){
        menu.unit=(u8)((menu.unit+(key&K_F1?roster_count()-1:1))%roster_count());
        if(menu.row>=count())menu.row=0;
        if(menu.screen==UI_JOBS)menu.row=current_job();
        dirty=1;
        return 1;
    }
    if(key&(K_B|K_ESC)) {
        switch(menu.screen){
        case UI_FORMATION:if(key&K_ESC){game_cleanup();return 0;}return 1;
        case UI_EQUIP:menu.screen=UI_ITEM;break;
        case UI_SET:case UI_LEARN:menu.screen=UI_ABILITY;break;
        case UI_ITEM:case UI_ABILITY:case UI_JOBS:menu.screen=UI_CONTEXT;break;
        default:menu.screen=UI_FORMATION;break;
        }
        menu.row=0;return 1;
    }
    if(menu.screen==UI_FORMATION) {
        s16 at=menu.unit;u8 total=roster_count();
        if(key&K_LEFT)at--;
        if(key&K_RIGHT)at++;
        if(menu.layout==LAYOUT_CLASSIC&&menu.small_actors&&(key&(K_UP|K_DOWN))) {
            u8 step=grid_columns(),slots=step*3;
            // Keep the column; skip empty cells when the last row is incomplete.
            do {
                at+=key&K_UP?-(s16)step:step;
                if(at<0)at+=slots;
                if(at>=slots)at-=slots;
            } while(at>=total);
        } else {
            if(key&K_UP)at-=menu.layout==LAYOUT_CLASSIC?4:1;
            if(key&K_DOWN)at+=menu.layout==LAYOUT_CLASSIC?4:1;
        }
        while(at<0)at+=total;
        while(at>=total)at-=total;
        menu.unit=(u8)at;
        if(key&K_APPS) {
            menu.layout=(u8)((menu.layout+1)%3);
            menu.unit=source_unit(menu.unit);menu.roster_demo=0;
        }
        if(key&(K_ENTER|K_A))menu.screen=UI_STATUS;
        if(key&(K_C|K_D)){menu.screen=UI_CONTEXT;menu.row=0;}
        return 1;
    }
    if(menu.screen==UI_DIALOGUE) {
        if(key&(K_A|K_ENTER)){
            if(menu.reveal<page.chars)menu.reveal=page.chars;
            else if(page.next){menu.dialog_at=page.next;menu.reveal=0;dialogue_layout();}
            else menu.screen=UI_FORMATION;
        }
        return 1;
    }
    if(menu.screen==UI_JOBS) {
        if(key&(K_UP|K_LEFT))menu.row=(menu.row+3)&3;
        if(key&(K_DOWN|K_RIGHT))menu.row=(menu.row+1)&3;
        if(key&(K_ENTER|K_A)) {
            if(menu.row<2&&menu.row!=current_job())menu.job_confirm=1;
            else if(menu.row>=2)menu.notice=1;
        }
        dirty=1;return 1;
    }
    if(menu.screen==UI_CONFIG) {
        if(key&K_UP)menu.row=menu.row?menu.row-1:3;
        if(key&K_DOWN)menu.row=(menu.row+1)%4;
        if(key&(K_ENTER|K_A)) {
            if(!menu.row) {
                menu.small_actors=(menu.small_actors+1)%3;
                if(!menu.small_actors) {menu.roster_demo=0;menu.unit=source_unit(menu.unit);}
                else if(menu.unit>=roster_count())menu.unit=0;
            }
            else if(menu.row==1) {
                menu.screen=UI_DIALOGUE;menu.stress=menu.dialog_at=menu.reveal=0;dialogue_layout();
            } else if(menu.row==2) {menu.screen=UI_LOCATION;menu.title_at=menu.title_clock=0;}
            else menu.dialog_light^=1;
        }
        dirty=1;return 1;
    }
    n=count();
    if(key&K_UP)menu.row=(u8)(menu.row?menu.row-1:n-1);
    if(key&K_DOWN)menu.row=(u8)(menu.row+1==n?0:menu.row+1);
    if(key&(K_ENTER|K_A)) {
        switch(menu.screen) {
        case UI_CONTEXT:
            if(menu.row<3){static const u8 dest[]={UI_ITEM,UI_ABILITY,UI_JOBS};menu.screen=dest[menu.row];menu.row=menu.screen==UI_JOBS?current_job():0;}
            else menu.notice=1;
            break;
        case UI_ITEM:if(menu.row==0){menu.screen=UI_EQUIP;menu.row=0;}else menu.notice=1;break;
        case UI_ABILITY:menu.screen=menu.row==2?UI_LEARN:UI_SET;menu.row=0;break;
        case UI_STATUS:menu.screen=UI_EQUIP;menu.row=0;break;
        default:menu.notice=1;break;
        }
    }
    return 1;
}

void game_render(void)
{
    u16 key=(u16)(menu.screen|menu.layout<<4|menu.unit<<6|menu.compact<<10|menu.notice<<11|first_row()<<12|((u16)menu.dialog_light<<15));
    u8 changed=(u8)(dirty||key!=cached_key||(menu.screen==UI_DIALOGUE&&
                    (menu.dialog_at!=cached_at||menu.reveal<cached_reveal)));
#ifdef RT_CYCLES
    u8 rebuilt=changed;
#endif
    if(!font||!actors||!ui||!icons||!visuals||!cache) {
        draw_clear();draw_text(4,40,"Missing menu assets / RAM",0,C_BLACK);return;
    }
    if(changed) {
        void *l=rt_light,*d=rt_dark;
        rt_light=cache;rt_dark=cache+RT_PSIZE;base();rt_light=l;rt_dark=d;dirty=0;cached_key=key;cached_at=menu.dialog_at;
        cached_reveal=menu.reveal>=page.chars?page.chars:0;
    }
    if(menu.screen==UI_DIALOGUE) {
        u8 shown=menu.reveal>page.chars?page.chars:menu.reveal;
        if(shown>cached_reveal||changed) {
            void *l=rt_light,*d=rt_dark;rt_light=cache;rt_dark=cache+RT_PSIZE;
            dialogue_range(cached_reveal,shown,menu.dialog_light?C_BLACK:C_WHITE);
            dialogue_range(shown,(u8)(shown+2),menu.dialog_light?C_DGRAY:C_LGRAY);
            rt_light=l;rt_dark=d;cached_reveal=shown;
        }
    }
    memcpy(rt_light,cache,RT_PSIZE);memcpy(rt_dark,cache+RT_PSIZE,RT_PSIZE);
    if(menu.screen==UI_FORMATION) {
        formation_actors();
        if(menu.screen==UI_FORMATION) {
            if(menu.layout==LAYOUT_CLASSIC) {
                if(menu.small_actors) {
                    RtSprite sp={8,7,small_hand[0],small_hand[1],small_hand[2]};
                    draw_sprite(grid_x(menu.unit)+1,grid_y(menu.unit)+6,&sp);
                }
                else hand(1+(menu.unit&3)*39,32+(menu.unit>>2)*32);
            }
            else if(menu.layout==LAYOUT_ROSTER)hand(44,17+menu.unit*14);
            else hand(1+menu.unit*31,76);
        }
    } else if(menu.screen==UI_DIALOGUE){
        actor(70,17,0);actor_gfx(119,17,UG_DELITA);
    }
    else if(menu.screen==UI_CONTEXT){ /* Actors are behind its cached window. */ }
    else if(menu.screen==UI_LOCATION)location();
    else if(menu.screen==UI_JOBS) {
        if(!menu.job_confirm&&!menu.notice) {
            u8 i;for(i=0;i<4;i++)job_actor(12+i*40,49,i);
        }
    } else if(menu.screen!=UI_CONFIG)actor(134,15,menu.unit);
    if(menu.screen==UI_CONTEXT&&!menu.notice)hand(57,25+menu.row*row_pitch());
    else if(menu.screen==UI_CONFIG)hand(6,33+menu.row*15);
    else if(menu.screen!=UI_FORMATION&&menu.screen!=UI_STATUS&&menu.screen!=UI_DIALOGUE&&menu.screen!=UI_JOBS&&menu.screen!=UI_LOCATION&&!menu.notice)
        hand(5,44+(menu.row-first_row())*row_pitch());
#ifdef RT_CYCLES
    BENCH_VALUE(((u32)rebuilt<<24)|((u32)menu.screen<<16)|menu.tick);
    BENCH_VALUE(BENCH_CYCLES-frame_start);
#endif
}

void game_cleanup(void)
{
    if(cache){free(cache);cache=RT_NULL;}
}
