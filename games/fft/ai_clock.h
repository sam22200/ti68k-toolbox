// Pure next-turn estimate using the real clock's finite and innate status policy.
#ifndef FFT_AI_CLOCK_H
#define FFT_AI_CLOCK_H
static u16 ai_status_span(const Unit *u,u8 sid)
{
    if(u->statuses.innate[sid>>3]&(128>>(sid&7)))return 65535U;
    // The source timer deliberately wraps zero to255 on its next decrement.
    return u->status_ct[sid-SID_POISON]?u->status_ct[sid-SID_POISON]:256;
}
static u16 ai_ticks(const Unit *u)
{
    u16 tick=0,ct=u->ct,haste=0,slow=0,delay=0,span,steps;u8 rate,gain,frozen=0;
    if(!u->hp||!unit_on_field(u)||unit_airborne(u)||
       (core_status_current(&u->statuses,0)&64)||(core_status_current(&u->statuses,1)&129))return 65535U;
    rate=core_status_current(&u->statuses,3);
    if(rate&2){delay=ai_status_span(u,SID_STOP);frozen=1;}
    if(core_status_current(&u->statuses,4)&16){span=ai_status_span(u,SID_SLEEP);if(span>delay)delay=span;frozen=1;}
    if(delay==65535U)return delay;
    // Status expiry precedes gain on the wake tick. An already-ready frozen
    // actor still waits for that tick, even with Speed0.
    if(ct>=100)return frozen?delay:0;
    if(!u->sp)return 65535U;
    tick=delay?delay-1:0;
    if(rate&8){haste=ai_status_span(u,SID_HASTE);if(haste!=65535U)haste=haste>tick+1?haste-tick-1:0;}
    if(rate&4){slow=ai_status_span(u,SID_SLOW);if(slow!=65535U)slow=slow>tick+1?slow-tick-1:0;}
    while(ct<100){
        gain=u->sp;span=65535U;
        if(slow){gain>>=1;span=slow;}
        else if(haste){gain+=gain>>1;span=haste;}
        if(!gain){if(span==65535U)return span;steps=span;}
        else{steps=(100-ct+gain-1)/gain;if(steps>span)steps=span;}
        tick+=steps;ct+=steps*gain;
        if(slow!=65535U)slow=slow>steps?slow-steps:0;
        if(haste!=65535U)haste=haste>steps?haste-steps:0;
    }
    return tick;
}
#endif
