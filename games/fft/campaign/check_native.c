#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main chapter_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#ifdef CHECK_GAR
#include "gariland.c"
#else
#include "academy.c"
#endif
#ifndef __m68k__
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
static u32 state_hash(void)
{
    u32 h=17;
#define HASH(v) h=(h<<1|h>>31)^(u32)(v)
#ifdef CHECK_GAR
    u8 i,k;
    for(i=0;i<NUNIT;i++){
        const Unit*u=&st.unit[i];
        HASH(u->x);HASH(u->z);HASH(u->face);HASH(u->hp);HASH(u->max_hp);HASH(u->mp);
        HASH(u->ct);HASH(u->sp);HASH(u->pa);HASH(u->zodiac);HASH(u->learned);HASH(u->jp);
        HASH(u->exp);HASH(u->dc);HASH(u->gexp);HASH(u->gjp);
        for(k=0;k<5;k++){HASH(u->eq[k]);HASH(u->raw[k]);}
    }
    HASH(st.act);HASH(st.mode);HASH(st.ai);HASH(st.abil);HASH(st.end);HASH(st.rot);HASH(rng32);
#else
    HASH(academy.message);HASH(academy.offset);HASH(academy.reveal);
    HASH(academy.tick);HASH(academy.white);HASH(academy.done);
#endif
#undef HASH
    return h;
}
#ifndef __m68k__
int main(int argc,char**argv)
{
    SwScript script;unsigned f,limit;if(argc!=5)return 2;
    limit=(unsigned)atoi(argv[2]);if(sw_load_script(&script,argv[3]))return 3;
    sw_init((u16)atoi(argv[1]));
    for(f=0;f<limit;f++)if(!sw_step(sw_script_keys(&script,rt_frame)))break;
    printf("executed %u checksum %04X state %08lX\n",rt_frame,sw_checksum(),(unsigned long)state_hash());
    return sw_write_png(argv[4],1);
}
#else
void _main(void){chapter_runtime_main();BENCH_VALUE(0xACAD0001UL);BENCH_VALUE(state_hash());}
#endif
