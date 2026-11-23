// Real native writer under the VAT runner, never a live calculator save.
#define USE_TI89
#define MIN_AMS 200
#define RT_CYCLES
#define RT_SAVE_ATOMIC
#include <tigcclib.h>
#undef _main
#define _main atomic_unused_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
static u8 bytes[4684],back[4684];
static u16 checks,failures;
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
static void check(u8 ok){checks++;if(!ok)failures++;}
void _main(void)
{
    char sym[12];SYM_ENTRY *entry;u16 i;
    for(i=0;i<sizeof bytes;i++)bytes[i]=(u8)(i*13);memcpy(bytes,"FSV9",4);
    check(rt_save("svcheck",bytes,sizeof bytes));check(!rt_save_committed());save_write();
    check(rt_save_committed());check(rt_load("svcheck",back,sizeof back));check(!memcmp(back,bytes,sizeof bytes));
    entry=SymFindPtr(symstr(sym,"svcheck"),0);check(entry&&entry->flags.bits.archived&&entry->flags.bits.locked);
    bytes[4427]=255;check(rt_save("svcheck",bytes,sizeof bytes));save_write();
    check(rt_save_committed());check(rt_load("svcheck",back,sizeof back));check(!memcmp(back,bytes,sizeof bytes));
    entry=SymFindPtr(symstr(sym,"svcheck"),0);check(entry!=0);
    if(entry){EM_moveSymFromExtMem(symstr(sym,"svcheck"),HS_NULL);
        entry=SymFindPtr(symstr(sym,"svcheck"),0);entry->flags.bits.locked=1;bytes[4427]=14;
        rt_save("svcheck",bytes,sizeof bytes);save_write();check(!rt_save_committed());
        check(rt_load("svcheck",back,sizeof back));check(back[4427]==255);}
    BENCH_VALUE(checks);BENCH_VALUE(failures);BENCH_VALUE(sizeof bytes);
}
