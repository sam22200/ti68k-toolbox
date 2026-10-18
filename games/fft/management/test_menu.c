#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "menu.h"
#include "../../../runtime/platform-sw/rt_sw.h"
void game_cleanup(void);
static void press(u32 k){assert(sw_step(k));assert(sw_step(0));}
static unsigned pixels(s16 x,s16 y,s16 w,s16 h,u8 level)
{
    unsigned n=0;s16 a,b;
    for(b=y;b<y+h;b++)for(a=x;a<x+w;a++)n+=sw_level(a,b)==level;
    return n;
}
int main(void)
{
    sw_init(0);assert(menu.layout==0&&menu.unit==0&&menu.compact);
    press(K_D);assert(menu.screen==UI_CONTEXT);
    press(K_UP);press(K_ENTER);assert(menu.notice&&menu.screen==UI_CONTEXT);
    press(K_B);assert(!menu.notice);press(K_DOWN);assert(menu.row==0);
    press(K_ENTER);assert(menu.screen==UI_ITEM);
    press(K_ENTER);assert(menu.screen==UI_EQUIP);
    assert(pixels(80,80,75,6,C_BLACK)>10); // accessory text is already visible
    assert(!pixels(80,47,75,42,C_DGRAY)&&!pixels(80,47,75,42,C_LGRAY)); // shadow-free names
    assert(pixels(70,47,8,8,C_BLACK)>0); // original item icon beside the weapon
    assert(!pixels(5,3,75,7,C_LGRAY)&&!pixels(5,3,75,7,C_DGRAY)); // shadow-free headings
    press(K_UP);assert(menu.row==4); // accessory scroll and wrap
    press(K_ENTER);assert(menu.notice);
    press(K_B);assert(!menu.notice&&menu.screen==UI_EQUIP);
    press(K_B);assert(menu.screen==UI_ITEM);
    press(K_B);assert(menu.screen==UI_CONTEXT);
    press(K_DOWN);press(K_ENTER);assert(menu.screen==UI_ABILITY);
    press(K_DOWN);press(K_DOWN);press(K_ENTER);assert(menu.screen==UI_LEARN);
    press(K_F5);press(K_F5);assert(menu.unit==2);
    press(K_UP);assert(menu.row==4); // fifth Chemist ability, with scrolling
    press(K_F5);assert(menu.row==4);
    press(K_F5);assert(menu.unit==4&&menu.row==0); // shorter Squire list
    press(K_F5);assert(menu.unit==0);
    press(K_B);press(K_B);press(K_B);assert(menu.screen==UI_FORMATION);
    press(K_F1);assert(menu.unit==4);press(K_F5);assert(menu.unit==0);
    press(K_APPS);assert(menu.layout==1);press(K_APPS);assert(menu.layout==2);
    press(K_CATALOG);assert(menu.screen==UI_CONFIG);
    press(K_ENTER);assert(menu.small_actors);press(K_B);assert(menu.screen==UI_FORMATION);
    press(K_APPS);assert(menu.layout==0);
    press(K_DOWN);assert(menu.unit==4);press(K_UP);assert(menu.unit==0);
    press(K_CATALOG);press(K_DOWN);press(K_ENTER);
    assert(menu.screen==UI_DIALOGUE&&menu.reveal<3);
    {u8 n=menu.reveal;unsigned i;for(i=0;i<5;i++)assert(sw_step(0));assert(menu.reveal==n+5);}
    assert(pixels(0,60,160,32,C_DGRAY)>40); // unrevealed small text + gradient
    press(K_ENTER); // finish the complete passage on one fitted page
    assert(pixels(0,60,160,32,C_WHITE)>100);
    press(K_F5);assert(menu.stress&&menu.dialog_at==0&&menu.reveal<3);
    press(K_ENTER); // original longest-page capacity fixture
    assert(menu.screen==UI_DIALOGUE&&menu.dialog_at==0&&menu.reveal>=103);
    press(K_C);assert(menu.dialog_light);press(K_C);assert(!menu.dialog_light);
    press(K_ENTER);assert(menu.screen==UI_FORMATION); // 103 source chars fit without pagination
    game_scenario(48);assert(sw_step(0));assert(menu.roster_demo&&menu.small_actors==1);
    press(K_DOWN);assert(menu.unit==4);press(K_DOWN);assert(menu.unit==8);
    assert(pixels(15,59,10,16,C_BLACK)>4); // an actual sprite in the third row
    press(K_RIGHT);press(K_RIGHT);press(K_RIGHT);assert(menu.unit==11);
    press(K_ENTER);assert(menu.screen==UI_STATUS); // high roster indices map safely to fixtures
    press(K_B);assert(menu.screen==UI_FORMATION);
    press(K_CATALOG);press(K_ENTER);assert(menu.small_actors==2);press(K_B);
    press(K_RIGHT);press(K_RIGHT);press(K_RIGHT);assert(menu.unit==14);
    press(K_DOWN);assert(menu.unit==4);press(K_UP);assert(menu.unit==14);
    press(K_CATALOG);press(K_ENTER);assert(!menu.small_actors&&!menu.roster_demo&&menu.unit==4);press(K_B);
    game_scenario(49);assert(sw_step(0));assert(menu.small_actors==2);
    press(K_DOWN);assert(menu.unit==5);press(K_DOWN);assert(menu.unit==10);
    assert(pixels(11,59,10,16,C_BLACK)>4);
    press(K_F1);assert(menu.unit==9);press(K_F5);assert(menu.unit==10);
    press(K_APPS);assert(!menu.roster_demo&&menu.unit==0); // other layouts keep their five-unit fixture
    game_scenario(47);assert(sw_step(0));
    assert(!pixels(77,69,20,1,C_WHITE)); // one interior pixel of top padding in Delita badge
    assert(pixels(77,70,20,5,C_WHITE)>8);
    assert(sw_level(112,72)==C_LGRAY); // scenery resumes immediately after the fitted badge
    assert(!pixels(77,75,20,1,C_WHITE)); // equal one-row interior top/bottom padding
    assert(sw_level(20,85)==C_LGRAY); // a short line leaves scenery beside the fitted box
    press(K_ENTER);assert(menu.screen==UI_FORMATION);
    game_scenario(6);assert(sw_step(0));assert(menu.screen==UI_JOBS&&!menu.job[0]);
    press(K_RIGHT);assert(menu.row==1);press(K_ENTER);assert(menu.job_confirm&&!menu.job[0]);
    press(K_B);assert(!menu.job_confirm&&!menu.job[0]); // cancel never changes the job
    press(K_ENTER);press(K_ENTER);assert(menu.job[0]&&!menu.job_confirm);
    press(K_RIGHT);press(K_ENTER);assert(menu.notice&&menu.job[0]); // locked Knight
    press(K_B);press(K_LEFT);press(K_LEFT);press(K_ENTER);press(K_ENTER);
    assert(!menu.job[0]); // return to the original Squire without changing the fixture
    press(K_B);press(K_B);assert(menu.screen==UI_FORMATION);
    press(K_CATALOG);press(K_DOWN);press(K_DOWN);press(K_ENTER);
    assert(menu.screen==UI_LOCATION);
    {unsigned i;u32 start=sw_checksum();for(i=0;i<64;i++)assert(sw_step(0));assert(sw_checksum()!=start);}
    press(K_F5);assert(menu.title_at==1&&menu.title_clock<3);
    press(K_ENTER);assert(menu.title_clock<3);press(K_B);assert(menu.screen==UI_FORMATION);
    // A true palette inverse, including the name badge, not a flat light replacement.
    game_scenario(8);assert(sw_step(0));
    {
        u8 body[22][100],badge[10][16];unsigned x,y;
        for(y=0;y<22;y++)for(x=0;x<100;x++)body[y][x]=sw_level(40+x,68+y);
        for(y=0;y<10;y++)for(x=0;x<16;x++)badge[y][x]=sw_level(40+x,54+y);
        press(K_C);assert(menu.dialog_light);
        for(y=0;y<22;y++)for(x=0;x<100;x++)assert(sw_level(40+x,68+y)==3-body[y][x]);
        for(y=0;y<10;y++)for(x=0;x<16;x++)assert(sw_level(40+x,54+y)==3-badge[y][x]);
        press(K_C);assert(!menu.dialog_light);
        for(y=0;y<22;y++)for(x=0;x<100;x++)assert(sw_level(40+x,68+y)==body[y][x]);
    }
    press(K_CATALOG);press(K_UP);assert(menu.screen==UI_CONFIG&&menu.row==3);
    press(K_ENTER);assert(menu.dialog_light&&menu.row==3);press(K_B);
    press(K_CATALOG);press(K_DOWN);press(K_ENTER);
    assert(menu.screen==UI_DIALOGUE&&menu.dialog_light); // retained across screens
    press(K_B);press(K_CATALOG);press(K_UP);press(K_ENTER);assert(!menu.dialog_light);
    press(K_B);
    // Input-free menu animation must not mutate the selected unit or mode.
    {u8 u=menu.unit,s=menu.screen;unsigned i;for(i=0;i<130;i++)assert(sw_step(0));assert(menu.unit==u&&menu.screen==s);}
    assert(!sw_step(K_ESC));
    game_cleanup();puts("Management navigation, nesting, wrap, preview/back, dialogue and cleanup passed");return 0;
}
