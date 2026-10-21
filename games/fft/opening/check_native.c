// Run the real frame loop, then compare endian-independent tactical state on PC/TI.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main auto_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "opening.c"
#ifndef __m68k__
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif

static void summary(void)
{
#if !defined(__m68k__) || !defined(FFT_PROFILE_ONLY)
    const Unit *u=st.unit; u16 h=0; u8 i;
#define HASH(v) h=(u16)((h<<1)|(h>>15))^(u16)(v)
    for(i=0;i<NUNIT;i++,u++) {
        HASH(u->x); HASH(u->z); HASH(u->face); HASH(u->hp); HASH(u->mp); HASH(u->ct);
        HASH(u->status); HASH(u->dc); HASH(u->exp); HASH(u->jp); HASH(u->gexp); HASH(u->gjp);
        HASH(st.pending[i].state);HASH(st.pending[i].ticks);HASH(st.pending[i].tile);HASH(st.pending[i].ability);
    }
    for(i=0;i<NBAG;i++) { HASH(st.bag[i]); }
#undef HASH
#ifdef __m68k__
#define VALUE(v) BENCH_VALUE(v)
    VALUE(0x0B0E0001UL); VALUE(sizeof(State)); VALUE(sizeof(Work));
#else
#define VALUE(v) printf("STATE,%08lX\n",(unsigned long)(v))
#endif
    VALUE(h); VALUE(rng32); VALUE(st.auto_units);
    VALUE(((u32)st.open_stage<<24)|((u32)st.open_beat<<16)|st.open_reveal);
    VALUE(((u32)st.mode<<24)|((u32)st.ai<<16)|((u16)st.act<<8)|st.end);
    VALUE(((u32)st.moved<<24)|((u32)st.acted<<16)|((u16)st.rounds<<8)|st.act_t);
    VALUE(((u32)st.open_resolving<<24)|((u32)st.open_fade<<16)|((u16)st.open_pan<<8)|st.open_handoff);
#undef VALUE
#endif
}

#ifndef __m68k__
int main(int argc,char **argv)
{
    SwScript script; unsigned frame,frames;
    if(argc!=5) return 2;
    frames=(unsigned)atoi(argv[2]); if(sw_load_script(&script,argv[3])) return 3;
    sw_init((u16)atoi(argv[1]));
    for(frame=0;frame<frames;frame++) if(!sw_step(sw_script_keys(&script,rt_frame))) break;
    printf("executed %u\n",rt_frame);
    printf("checksum %04X\n",sw_checksum()); summary();
    if(strcmp(argv[4],"-")) return sw_write_png(argv[4],1);
    return 0;
}
#else
void _main(void) { auto_runtime_main(); summary(); }
#endif
