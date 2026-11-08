// Reuse the reviewed management actor/portrait/font/stat banks and primitives.
#define game_init management_init
#define game_scenario management_scenario
#define game_update management_update
#define game_render management_render
#include "manage.c"
#undef game_init
#undef game_scenario
#undef game_update
#undef game_render
#include "flow.h"
#include "deployment.inc"
static Stats deploy_stats;
static u8 stats_unit;

void game_init(void)
{
    management_init();flowbank=rt_file("ffflow",RT_NULL);
    if(!qvalid(flowbank))flowbank=RT_NULL;
    stats_unit=255;dirty=1;
    rt_state=&dp;rt_state_size=sizeof dp;
}
void game_scenario(u16 n){(void)n;if(flowbank&&bank)deploy_init();}
u8 chapter_completed(void){return dp.done;}
#include "../../../lib/tilog.h"
static u8 deploy_exit(void){if(cache)free(cache);cache=RT_NULL;return 0;}
u8 game_update(void)
{
    u32 key=rt_keys&~rt_prev;
    if(!flowbank||!bank||!save.n)return key&K_ESC?deploy_exit():1;
    if(dp.confirm) {
        if(key&(K_B|K_ESC)){dp.confirm=0;return 1;}
        dp.confirm=choice_move(dp.confirm-1,2,key)+1;
        if(key&(K_ENTER|K_A)){if(dp.confirm==1&&deploy_commit()){tilog("deploy start battle: %u units",dp.n);return deploy_exit();}dp.confirm=0;}
        return 1;
    }
    if(key&K_ESC){tilog("deploy cancelled");return deploy_exit();}
    if(key)dp.note=0;
    if(key&K_F1)dp.unit=dp.unit?dp.unit-1:save.n-1;
    if(key&K_F5)dp.unit=dp.unit+1<save.n?dp.unit+1:0;
    if(key&K_LEFT)deploy_move(0,1);
    if(key&K_RIGHT)deploy_move(0,-1);
    if(key&K_UP)deploy_move(1,0);
    if(key&K_DOWN)deploy_move(-1,0);
    if(key&(K_A|K_ENTER))deploy_place();
    if(key&K_B){u8 cell=(u8)(dp.row*5+dp.col);if(dp.placed[cell]!=255){dp.placed[cell]=255;dp.n--;}}
    if(key&(K_C|K_D)){if(deploy_ready())dp.confirm=1;else dp.note=4;}
    return 1;
}

static void deploy_tile(s16 x,s16 y,u8 valid,u8 selected)
{
    s16 row;
    // Only legal placement slabs; four-pixel sides make their thickness visible.
    if(!valid)return;
    y--;
    for(row=-4;row<=4;row++) {
        s16 half=9-2*(row<0?-row:row);
        draw_rect(x-half,y+row+4,2*half+1,1,C_WHITE);
        if(half>1) {
            draw_rect(x-half+1,y+row+4,half,1,C_DGRAY);
            draw_rect(x+1,y+row+4,half-1,1,C_BLACK);
        }
    }
    for(row=-4;row<=4;row++) {
        s16 half=9-2*(row<0?-row:row),rim=selected?2:1;
        draw_rect(x-half,y+row,2*half+1,1,C_WHITE);
        if(half>=rim)draw_rect(x-half+rim,y+row,2*(half-rim)+1,1,C_DGRAY);
    }
}

static void deploy_line(s16 x,s16 y,s16 end_x,s16 end_y,u8 color)
{
    s16 dx=abs(end_x-x),dy=-abs(end_y-y),sx=x<end_x?1:-1,sy=y<end_y?1:-1,error=dx+dy;
    for(;;) {
        s16 twice;
        draw_rect(x,y,1,1,color);if(x==end_x&&y==end_y)break;
        twice=error+error;
        if(twice>=dy){error+=dy;x+=sx;}
        if(twice<=dx){error+=dx;y+=sy;}
    }
}

static void deploy_rim(s16 x,s16 y,u8 color)
{
    deploy_line(x,y-4,x+9,y,color);deploy_line(x+9,y,x,y+4,color);
    deploy_line(x,y+4,x-9,y,color);deploy_line(x-9,y,x,y-4,color);
}

// Two-pixel contour selected in review A; keep the lower edge horizontal.
static void deploy_arrow_line(s16 x,s16 y,s16 ex,s16 ey,u8 color)
{
    deploy_line(x,y,ex,ey,color);deploy_line(x+1,y,ex+1,ey,color);
    deploy_line(x,y+1,ex,ey+1,color);deploy_line(x+1,y+1,ex+1,ey+1,color);
}

static void deploy_orientation(void)
{
    s16 i;
    // Ground-plane wedge below the grid, matching the source reference's slant.
    // Distinct from the hovering tile marker and from tactical unit facing.
    static const s16 x[3]={56,61,84},y[3]={75,87,87};
    for(i=0;i<3;i++) {
        s16 next=i==2?0:i+1;
        deploy_arrow_line(x[i]-1,y[i],x[next]-1,y[next],C_WHITE);
        deploy_arrow_line(x[i]+1,y[i],x[next]+1,y[next],C_WHITE);
        deploy_arrow_line(x[i],y[i]-1,x[next],y[next]-1,C_WHITE);
        deploy_arrow_line(x[i],y[i]+1,x[next],y[next]+1,C_WHITE);
    }
    for(i=0;i<3;i++){s16 next=i==2?0:i+1;deploy_arrow_line(x[i],y[i],x[next],y[next],C_BLACK);}
}

static void deploy_background(void)
{
    u8 r,c; s16 diagonal;
    draw_clear();heading("Deploy: Gariland");
    if(!flowbank||!bank||!save.n){black(3,40,"Missing flow data");return;}
    draw_rect(0,38,160,1,C_WHITE);
    black(2,45,"Squad 1");black(2,53,"Limit");number(26,53,deploy_rules()[2]);
    black(2,62,"Left");
    black(2,77,"ALPHA:go");
    black(2,85,"ENTER:set");
    // Full5x5 reference grid, outline only at ground level: no inactive slabs.
    for(r=0;r<5;r++)for(c=0;c<5;c++)deploy_rim(100+((s16)r-c)*9,88-((s16)r+c)*4,C_LGRAY);
    for(diagonal=8;diagonal>=0;diagonal--)for(r=0;r<5;r++) {
        s16 col=diagonal-r;
        if(col<0||col>=5)continue;
        c=(u8)col;
        deploy_tile(100+((s16)r-c)*9,86-diagonal*4,deploy_cell((u8)(r*5+c))[0],0);
    }
    // Finish every top perimeter after adjacent sides, including both extreme tips.
    for(r=0;r<5;r++)for(c=0;c<5;c++)if(deploy_cell((u8)(r*5+c))[0]) {
        s16 x=100+((s16)r-c)*9,y=85-((s16)r+c)*4;
        deploy_rim(x-1,y,C_DGRAY);deploy_rim(x+1,y,C_DGRAY);
        deploy_rim(x,y,C_WHITE);
    }
    deploy_orientation();
    draw_rect(0,93,160,1,C_BLACK);
}

void game_render(void)
{
    u8 r,c;s16 diagonal;
    if(!flowbank||!bank||!save.n||!cache){draw_clear();draw_text(3,40,"Missing flow data / RAM",F_SMALL,C_BLACK);return;}
    if(dirty||stats_unit!=dp.unit) {
        void *light=rt_light,*dark=rt_dark;
        rt_light=cache;rt_dark=cache+RT_PSIZE;
        if(dirty)deploy_background();
        draw_rect(0,7,160,31,C_WHITE);
        deploy_stats=stats(unit(dp.unit));stats_unit=dp.unit;
        formation_details(unit(dp.unit),&deploy_stats,9);
        rt_light=light;rt_dark=dark;dirty=0;
    }
    memcpy(rt_light,cache,RT_PSIZE);memcpy(rt_dark,cache+RT_PSIZE,RT_PSIZE);
    number(26,62,deploy_rules()[2]-dp.n);
    // White floor selection, underneath the actors; no hovering tile arrow.
    {
        s16 row,x=100+((s16)dp.row-dp.col)*9,y=85-((s16)dp.row+dp.col)*4;
        for(row=-4;row<=4;row++) {
            s16 half=9-2*(row<0?-row:row);
            draw_rect(x-half,y+row,2*half+1,1,C_WHITE);
        }
        deploy_rim(x,y,C_BLACK);
    }
    // Actors are sorted back-to-front too; later tiles cannot erase their feet.
    for(diagonal=8;diagonal>=0;diagonal--)for(r=0;r<5;r++) {
        s16 col=diagonal-r;u8 cell;
        if(col<0||col>=5)continue;
        c=(u8)col;cell=(u8)(r*5+c);
        if(dp.placed[cell]!=255&&!(dp.placed[cell]==dp.unit&&(rt_frame&8))) {
            const SaveUnit *u=unit(dp.placed[cell]);actor(92+((s16)r-c)*9,63-diagonal*4,picture(u,career(u)));
        }
    }
    // The raised arrow clears the footer; keep its separator continuous.
    draw_rect(0,93,160,1,C_BLACK);
    text(2,95,dp.confirm?"ENTER:ok":"F1/F5:unit",C_BLACK,53);
    text(91,95,dp.confirm?"SHIFT:back":"SHIFT:undo",C_BLACK,159);
    if(dp.confirm) {
        static const ChoiceLayout menu={65,43,94,27,3,CHOICE_TAB,3,1,0,6,10,8,157,-15,7};
        choice_frame(&menu,"Start battle?");
        choice_row(&menu,0,"Start Battle",C_BLACK);choice_row(&menu,1,"Return",C_BLACK);
        choice_cursor_animated(&menu,dp.confirm-1,rt_frame);
    } else if(dp.note) {
        static const char *const notes[]={"","Not a legal tile","Tile occupied","Squad full","Ramza must deploy","Save failed","Monster in reserve"};
        draw_rect(12,40,136,21,C_BLACK);draw_rect(13,41,134,19,C_WHITE);
        black(18,44,notes[dp.note]);
    }
}
