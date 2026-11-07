// Shared-bank bounds, fallback, fixed/proportional layout and real sprite planes.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main font_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "../../../lib/bitmap_font.h"
#include <stdlib.h>
#include <string.h>
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#endif
static u8 state;
static u16 failures,checks;
static u32 digest=17;
void game_init(void){rt_state=&state;rt_state_size=1;}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 1;}
void game_render(void){draw_clear();}
static void check(u8 value){checks++;if(!value)failures++;}
static void hash_screen(void)
{
    u8 plane;u16 row,col;
    for(plane=0;plane<2;plane++)for(row=0;row<100;row++)for(col=0;col<20;col++)
        digest=(digest<<1|digest>>31)^((const u8 *)(plane?rt_dark:rt_light))[row*RT_PBYTES+col];
}
static void trials(void)
{
    static const char *const words[]={"THIS BATTLE IS","Military Academy's","FINAL FANTASY TACTICS",
                                     "CONGRATULATIONS!","WAR TROPHIES","68k"};
    static const u16 sweeps[]={0,34,70,100,180,255};
    BfFont font;BfMetrics a,b;u16 size,n;u8 face,k,*copy;
    const u8 *bank=rt_file("ffserif",&size);
    check(bank!=RT_NULL);if(!bank)return;
    for(n=0;n<((u16)bank[6]<<8|bank[7]);n++)check(!bf_open(&font,bank,n,0));
    check(!bf_open(&font,RT_NULL,size,0));check(!bf_open(&font,bank,size,6));
    for(face=0;face<6;face++) {
        check(bf_open(&font,bank,size,face));
        check(bf_measure(&font,words[face],0,&a));check(a.right-a.left<=160);
        check(bf_measure(&font,"",0,&a)&&!a.advance);
        check(bf_measure(&font,"\001",0,&a)&&bf_measure(&font,"?",0,&b)&&!memcmp(&a,&b,sizeof a));
        check(!bf_measure(&font,"abc",33,&a));
        for(k=0;k<6;k++) {
            BfLayout layout={0,20,160,sweeps[k],0};
            draw_rect(0,0,160,100,C_BLACK);bf_draw(&font,words[face],&layout);hash_screen();
        }
    }
    check(bf_open(&font,bank,size,2));
    check(bf_measure(&font,"2500 GIL",8,&a)&&a.advance==64);
    {BfLayout layout={0,30,160,255,8};draw_rect(0,0,160,100,C_BLACK);bf_draw(&font,"2500 GIL",&layout);hash_screen();}
    {char long_text[130];memset(long_text,' ',129);long_text[129]=0;check(!bf_measure(&font,long_text,0,&a));}
    copy=malloc(size);check(copy!=RT_NULL);if(!copy)return;
    memcpy(copy,bank,size);copy[8]=copy[9]=255;check(!bf_open(&font,copy,size,0));
    memcpy(copy,bank,size);check(bf_open(&font,copy,size,2));
    n=(u16)(font.glyphs-copy)+2*('A'-32);copy[n]=copy[n+1]=255;
    check(!bf_measure(&font,"A",0,&a));
    free(copy);
}
#ifndef __m68k__
int main(void)
{
    sw_init(0);trials();printf("font %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    return failures!=0;
}
#else
void _main(void){font_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
