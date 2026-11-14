// Actual scene input/render/session path; authored QA, no campaign extraction.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef off
#undef _main
#define _main story_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
static u8 io_fault;
static size_t fault_write(const void *p,size_t size,size_t count,FILE *f)
{return io_fault==1?fwrite(p,size,17,f):fwrite(p,size,count,f);}
static int fault_close(FILE *f){int rc=fclose(f);return io_fault==2?EOF:rc;}
static int fault_rename(const char *a,const char *b){return io_fault==3?-1:rename(a,b);}
#define SC_FILE_WRITE fault_write
#define SC_FILE_CLOSE fault_close
#define SC_FILE_RENAME fault_rename
#endif
#include "scene.c"
#include "x/story-choices/scene_fixture.h"
#ifdef __m68k__
static u8 portable_cases=1;
#else
static u8 portable_cases;
#endif
static Save baseline,back;static u16 checks,failures;static u32 digest=17;
static void check(u32 a,u32 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("story-scene line%u: %lu expected%lu\n",line,(unsigned long)a,(unsigned long)b);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static void flush(void)
{
#ifdef __m68k__
    if(sv_data){save_write();sv_data=RT_NULL;}scene_save_dirty=0;
#endif
}
static void seed(u8 n)
{
    u8 i;memset(&baseline,0,sizeof baseline);memcpy(baseline.magic,SAVE_MAGIC,4);
    baseline.n=n;baseline.loc=7;baseline.flow=4;
    for(i=0;i<n;i++){baseline.unit[i].job=0x4a;baseline.unit[i].gfx=2;baseline.unit[i].level=10;baseline.unit[i].name=i;}
    baseline.unit[0].job=1;baseline.unit[0].gfx=0;baseline.unit[0].type=2;
    baseline.guest_id[0][1]=4;baseline.guest[0]=baseline.unit[1];baseline.guest[0].job=4;
    baseline.guest[0].gfx=1;baseline.guest[0].level=17;baseline.guest[0].exp=82;baseline.guest[0].eq[2]=0xd1;
    save_raw_set(&baseline,16,1);sc_flag_set(&baseline,3,1);sc_flag_set(&baseline,70,1);
    CHECK(rt_save(SAVE_NAME,&baseline,sizeof baseline),1);flush();scene_save_loaded=scene_save_dirty=0;
}
static u8 frame(u32 buttons)
{
    u8 run;rt_prev=rt_keys;rt_keys=buttons;
#ifdef __m68k__
    BENCH_BEGIN(31);
#endif
    run=game_update();game_render();
#ifdef __m68k__
    BENCH_END(31);
#endif
    rt_frame++;return run;
}
static void press(u32 buttons){CHECK(frame(buttons),1);CHECK(frame(0),1);}
static void question(void)
{
    u8 i;game_scenario(3);CHECK(set!=RT_NULL,1);if(!set)return;
    CHECK(st.step,1);CHECK(st.open_stage,OPEN_CHAPEL);
    press(K_ESC);CHECK(skip_ask,1);press(K_DOWN);CHECK(skip_ask,2);
    CHECK(frame(K_ENTER),1);CHECK(st.step,2);CHECK(st.open_stage,OPEN_CHOICE);
    CHECK(frame(K_ENTER),1);CHECK(st.choice_row,0);CHECK(sc_flag(&scene_save,40),0); // held launch confirmation
    CHECK(frame(0),1);for(i=0;i<12;i++)CHECK(frame(0),1);
    CHECK(W->dialog_face!=255,1);CHECK(W->dialog_drawn,W->dialog_total);
}
static u32 pixels(void)
{
    u16 row,col;u8 plane;u32 h=17;
    rt_frame=0;game_render();
    for(plane=0;plane<2;plane++)for(row=0;row<100;row++)for(col=0;col<20;col++)
        h=(h<<1|h>>31)^((const u8 *)(plane?rt_dark:rt_light))[row*RT_PBYTES+col];
    return h;
}
static void resume(void)
{
    CHECK(save_load(&baseline),1);game_scenario(3);press(K_ESC);press(K_DOWN);
    CHECK(frame(K_ENTER),0);CHECK(chapter_completed(),1);flush();
    CHECK(save_load(&back),1);CHECK(memcmp(&baseline,&back,sizeof baseline),0);
}
static void recruit(void)
{
    seed(15);question();press(K_DOWN);CHECK(st.choice_row,1);
    CHECK(frame(K_ENTER),1);CHECK(st.step,5);CHECK(sc_flag(&scene_save,42),1);
#ifdef __m68k__
    CHECK(sv_data==&scene_save,1);CHECK(save_load(&back),1);CHECK(back.n==15,1);
#else
    CHECK(!scene_save_dirty,1);CHECK(save_load(&back),1);CHECK(back.n==16,1);
#endif
    game_scenario(3); // same-process reentry must see the accepted queued Save
    CHECK(sc_flag(&scene_save,42),1);CHECK(scene_save.n,16);
    flush();CHECK(save_load(&back),1);CHECK(back.n,16);CHECK(back.unit[15].exp,82);
    CHECK(save_raw_current(&back,15),1);CHECK(save_guest_find(&back,4),255);
    CHECK(sc_flag(&back,40)&&sc_flag(&back,42)&&sc_flag(&back,71)&&!sc_flag(&back,70),1);
}
static void trials(void)
{
    seed(16);question();
    {u32 h=pixels();CHECK(h,APPROVED_CHOICE_PLANES);
#ifndef __m68k__
     printf("choice-planes %08lX\n",(unsigned long)h);
#endif
    }
    press(K_DOWN);CHECK(st.choice_row,1);CHECK(frame(K_ENTER),1);
    CHECK(st.choice_error,SC_PARTY_FULL);CHECK(st.open_stage,OPEN_CHOICE);CHECK(sc_flag(&scene_save,40),0);
    CHECK(save_load(&back),1);CHECK(memcmp(&baseline,&back,sizeof baseline),0);
    {u32 h=pixels();
#ifndef __m68k__
     printf("full-planes %08lX\n",(unsigned long)h);
#endif
     CHECK(h,APPROVED_FULL_PLANES);}
    press(K_ESC);CHECK(st.choice_row,0);CHECK(st.choice_error,0);CHECK(st.open_stage,OPEN_CHOICE);
    CHECK(frame(K_ENTER),1);CHECK(st.step,3);flush();CHECK(save_load(&back),1);
    CHECK(back.n,16);CHECK(sc_flag(&back,41),1);CHECK(save_guest_find(&back,4),255);
#ifndef __m68k__
    // Real temporary-file replacement failure at each phase must preserve all
    // original bytes; retry once the fault is removed commits the whole choice.
    if(!portable_cases){u8 fault;for(fault=1;fault<=3;fault++){
        seed(15);question();press(K_DOWN);io_fault=fault;
        CHECK(frame(K_ENTER),1);CHECK(st.choice_error,SC_IO);CHECK(st.step,2);
        CHECK(save_load(&back),1);CHECK(memcmp(&baseline,&back,sizeof baseline),0);
        CHECK(memcmp(&baseline,&scene_save,sizeof baseline),0);
        io_fault=0;CHECK(frame(0),1);CHECK(frame(K_ENTER),1);CHECK(st.step,5);
        CHECK(save_load(&back),1);CHECK(back.n,16);CHECK(sc_flag(&back,42),1);
    }}
#endif
    seed(15);sc_flag_set(&baseline,3,0);CHECK(rt_save(SAVE_NAME,&baseline,sizeof baseline),1);flush();
    scene_save_loaded=0;game_scenario(3);CHECK(chapter_completed(),1);CHECK(save_load(&back),1);CHECK(sc_flag(&back,40),0);
    scene_save_loaded=0;game_scenario(4);CHECK(set==RT_NULL,1); // backward branch rejected
    recruit();resume(); // leave one resolved session for actual Title Save/Continue children
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    sw_init(3);if(argc>1&&!strcmp(argv[1],"resume"))resume();
    else{portable_cases=argc>1&&!strcmp(argv[1],"portable");trials();}
    printf("story-scene %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);release();return failures!=0;
}
#else
void _main(void){story_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
