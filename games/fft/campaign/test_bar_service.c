// Real map input -> town -> Bar -> source rumor -> pages/back -> session/reentry.
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
static const char *output;
static void tap(u32 key){assert(sw_step(0));assert(sw_step(key));}
static void shot(const char *name)
{char path[512];snprintf(path,sizeof path,"%s/%s.png",output,name);assert(!sw_write_png(path,1));}
static void enter_bar(void)
{
    tap(K_ENTER);assert(wm.screen==SC_TOWN);
    tap(K_DOWN);tap(K_DOWN);tap(K_DOWN);tap(K_ENTER);assert(wm.screen==SC_BAR);
}
static void malformed(void)
{
    u16 n=b16(bar_text+4),i;u8 *copy=malloc(n);assert(copy);
    assert(!bar_text_valid(RT_NULL,0));
    for(i=0;i<8;i++)assert(!bar_text_valid(bar_text,i));
    memcpy(copy,bar_text,n);copy[6]=65;assert(!bar_text_valid(copy,n));
    memcpy(copy,bar_text,n);copy[22]=copy[8];assert(!bar_text_valid(copy,n));
    memcpy(copy,bar_text,n);copy[14]=255;copy[15]=255;assert(!bar_text_valid(copy,n));
    memcpy(copy,bar_text,n);copy[n-1]=255;assert(!bar_text_valid(copy,n));
    memcpy(copy,bar_text,n);copy[32]=255;copy[33]=255;assert(!bar_text_valid(copy,n));
    assert(!bar_art_valid(bar_art,6359));
    free(copy);
}
int main(int argc,char **argv)
{
    Save before,expected;u8 i;char line[64];u16 consumed=0;const char *article;
    assert(argc==2);output=argv[1];sw_init(0);
    assert(bank&&shop&&bar_art&&bar_text&&save.n);before=save;
    malformed();
    assert(sizeof(Save)==4024);assert(rumor_count()==3);
    assert(!strcmp(rumor_string(0,6),"The Death Corps"));
    // Source town masks: native locations retain the source's0..42 identities.
    for(i=0;i<LOC_N;i++){wm.at=i;assert(rumor_count()==(i==2||i==6||i==9?3:0));}
    wm.at=6;save.flags[0]&=(u8)~2U;assert(!rumor_count());save=before;
    enter_bar();rt_frame=0;game_render();shot("home");
    assert(!memcmp(&save,&before,sizeof save)&&!wm.changed);
    tap(K_ENTER);assert(wm.screen==SC_RUMORS);shot("list");
    tap(K_ENTER);assert(wm.screen==SC_RUMOR&&wm.rumor==0);shot("short");
    assert(!memcmp(&save,&before,sizeof save)&&!wm.changed);
    // Empty/terminal input is bounded; ENTER closes only after the final page.
    tap(K_LEFT);assert(!wm.rpage);
    while(wm.rpage<wm.rlast)tap(K_ENTER);
    tap(K_RIGHT);assert(wm.screen==SC_RUMOR&&wm.rpage==wm.rlast);
    tap(K_ENTER);assert(wm.screen==SC_RUMORS&&!wm.trow);
    tap(K_DOWN);tap(K_DOWN);tap(K_ENTER);
    assert(wm.rumor==2&&wm.rlast==9);article=rumor_string(2,8);
    tap(K_RIGHT);assert(wm.rpage==1);
    assert(sw_step(K_RIGHT)&&wm.rpage==1); // held input cannot skip a second page
    tap(K_LEFT);assert(!wm.rpage);
    for(i=0;i<=wm.rlast;i++) {
        char actual[64];const u8 *record=rumor_record(2);
        u16 offset=b16(bar_text+b16(record+12)+2*(u16)i);
        u8 row,col;assert(wm.rpage==i);
        if(!i)shot("long-first");if(i==wm.rlast)shot("long-last");
        for(row=0;row<7;row++) {
            if(!item_detail_line(article,142,(u8)(i*7+row),line))break;
            assert(item_detail_line(article+offset,142,row,actual));assert(!strcmp(actual,line));
            for(col=0;line[col];col++)if(line[col]!=' ') {
                while(article[consumed]==' ')consumed++;
                assert(line[col]==article[consumed++]);
            }
        }
        if(i<wm.rlast)tap(K_RIGHT);
    }
    while(article[consumed]==' ')consumed++;assert(!article[consumed]);
    tap(K_LEFT);assert(wm.rpage==8);tap(K_RIGHT);assert(wm.rpage==9);
    rt_frame=0;game_render();shot("blink-0");rt_frame=16;game_render();shot("blink-16");
    rt_frame=32;game_render();shot("blink-32");
    tap(K_ESC);assert(wm.screen==SC_RUMORS&&wm.trow==2);tap(K_B);assert(wm.screen==SC_BAR);
    tap(K_ESC);assert(wm.screen==SC_TOWN);tap(K_ESC);assert(wm.screen==SC_MAP);
    assert(!memcmp(&save,&before,sizeof save)&&!wm.changed);
    // Only the King's source profile effects write durable state; money/date/RNG/party stay.
    enter_bar();tap(K_ENTER);tap(K_DOWN);tap(K_ENTER);
    expected=before;assert(save_profile_set(&expected,8,1));assert(save_profile_set(&expected,9,1));
    assert(wm.changed&&!memcmp(&save,&expected,sizeof save));
    tap(K_B);tap(K_B);tap(K_B);tap(K_B);assert(wm.screen==SC_MAP);
    assert(sw_step(0));assert(!sw_step(K_ESC));
    sw_init(0);assert(!memcmp(&save,&expected,sizeof save));
    enter_bar();tap(K_ENTER);tap(K_DOWN);tap(K_ENTER);
    assert(!wm.changed&&!memcmp(&save,&expected,sizeof save));
    tap(K_B);tap(K_B);tap(K_B);tap(K_B);assert(wm.screen==SC_MAP);
    // An empty list cannot select a nonexistent record, and cancel always unwinds one level.
    save.flags[0]&=(u8)~2U;wm.screen=SC_BAR;wm.trow=0;p_screen=255;
    tap(K_ENTER);assert(wm.screen==SC_RUMORS&&!rumor_count());shot("empty");
    tap(K_ENTER);assert(wm.screen==SC_RUMORS);tap(K_B);assert(wm.screen==SC_BAR);
    tap(K_DOWN);tap(K_ENTER);assert(wm.screen==SC_TOWN);
    release_map();
    printf("Bar service:3 source rumors,10 long pages,%u prose bytes; passive Save exact; profiles8/9 once, reload exact.\n",consumed);
    return 0;
}
