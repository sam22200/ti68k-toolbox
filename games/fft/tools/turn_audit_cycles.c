#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main rt_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "turn_audit.inc"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
int main(int argc,char **argv)
{
    unsigned c; sw_init(65);
    if(argc>2 && !strcmp(argv[2],"--source")) {
        turn_setup(0);
        draw_rect(0,0,160,8,C_WHITE); draw_text(2,0,"F5 / RIGHT",F_SMALL,C_BLACK);
        return sw_write_png(argv[1],1) ? 4:0;
    }
    for(c=0;c<8;c++) {
        u16 seen[TURN_FRAMES]; u8 n=0,i;
        turn_setup((u8)c);
        do {
            char path[1024];
            if(!turn_next()) return 2;
            if(st.turn) {
                u16 hash=sw_checksum();
                for(i=0;i<n;i++) if(seen[i]==hash) return 5;
                seen[n++]=hash;
            }
            if(argc>1) {
                snprintf(path,sizeof path,"%s/%u-%u.png",argv[1],c,turn_frame);
                if(sw_write_png(path,1)) return 4;
            }
            printf("%u,%u,%04X\n",c,turn_frame,sw_checksum());
        } while(st.turn && turn_frame<16);
        if(!turn_verify() || n!=TURN_FRAMES) return 3;
    }
    return 0;
}
#else
void _main(void)
{
    void *planes=malloc(2*RT_PSIZE); u8 c;
    rt_keys=rt_prev=0; rt_seed=1; rt_frame=0; rt_state=0; rt_state_size=0;
    if(!planes || !get_fonts()) return;
    rt_dark=planes; rt_light=(u8 *)planes+RT_PSIZE; BENCH_DARK_FIRST(1);
    game_init(); BENCH_VALUE(sizeof *W);
    for(c=0;c<8;c++) {
        turn_setup(c); BENCH_VALUE(0x71000000UL|c);
        do {
            u32 started=BENCH_CYCLES;
            if(!turn_next()) break;
            BENCH_VALUE(0x72000000UL|turn_frame); BENCH_VALUE(BENCH_CYCLES-started);
            BENCH_SHOT(rt_dark);
        } while(st.turn && turn_frame<16);
        BENCH_VALUE(0x73000000UL|turn_verify());
    }
    free(planes);
}
#endif
