// Shared identity records against independent pre-factorization byte fixtures.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main identity_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "../../../runtime/core/rt.h"
#include "../identity_pool.h"
static u16 failures,checks;static u32 hash=17;
void game_init(void){}void game_scenario(u16 n){(void)n;}u8 game_update(void){return 0;}void game_render(void){}
static void check(u8 got){checks++;hash=(hash<<1|hash>>31)^got;if(!got)failures++;}
static void setword(u8 *p,u16 at,u16 value){p[at]=value>>8;p[at+1]=value;}
static u8 same_text(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void trials(void)
{
    u16 size,reference_size,i,faces,meta,at,offset;const u8 *pool=rt_file("ffcp",&size),*expected=rt_file("fidref",&reference_size),*names;u8 *bad;
#ifdef __m68k__
    size-=6;reference_size-=6;
#endif
    check(pool!=0&&expected!=0);if(!pool||!expected)return;
    check(identity_valid(pool,size));check(!identity_valid(0,size));check(!identity_valid(pool,17));check(!identity_valid(pool,size-1));
    faces=identity_word(pool,2);meta=faces-10;offset=4;
    for(i=0;i<61;i++){u8 id=i<6?identity_section(pool,0)[i]:i<14?identity_section(pool,1)[i-6]:i<55?identity_section(pool,2)[i-14]:i-55;check(!memcmp(identity_portrait(pool,id),expected+offset+348U*i,348));}
    names=expected+offset+61U*348;
    for(i=0;i<510;i++){check(same_text(identity_name(pool,i),(const char *)names));while(*names++);}
    bad=malloc(size);check(bad!=0);if(!bad)return;
    for(i=0;i<12;i++){
        memcpy(bad,pool,size);at=identity_word(pool+meta,0);
        switch(i){
        case 0:bad[3]='1';break;
        case 1:setword(bad,4,17);break;
        case 2:setword(bad,4,size+1);break;
        case 3:setword(bad,meta+8,255);break;
        case 4:setword(bad,meta,faces);break;
        case 5:bad[at]=255;break;
        case 6:setword(bad,meta+6,identity_word(pool+meta,3)|1);break;
        case 7:setword(bad,meta+6,size);break;
        case 8:setword(bad,identity_word(pool+meta,3),1019);break;
        case 9:setword(bad,identity_word(pool+meta,3),size);break;
        case 10:bad[7]=255;break;
        case 11:memset(bad+8,'x',at-8);break;
        }
        check(!identity_valid(bad,size));
    }
    free(bad);
}
#ifndef __m68k__
int main(void){trials();printf("identity %08lX failures %u checks %u\n",(unsigned long)hash,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(hash);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
