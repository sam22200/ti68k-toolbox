#ifndef FFT_STATUS_AI_H
#define FFT_STATUS_AI_H
#include "cleanse_rules.h"
// Cancellation utility uses the frozen subset, not every status in its set.
// Keep the existing immediate heuristic; future-turn/control simulation is separate.
static s16 cleanse_ai_gain(u8 t,AbId ab,u16 packed,u8 same)
{
    const u8 *r=cj_section(common,CJ_STATUS)+6*abilities[ab].y;
    u8 remove[5],b,m; s16 score=0;(void)t;
    core_cleanse_unpack(r+1,packed,remove);
    for(b=0;b<5;b++)for(m=128;m;m>>=1)if(remove[b]&m){
        u8 helpful=(b==2&&(m&0x70))||(b==3&&(m&0x78))||(b==4&&(m&0x82));
        u8 weight=b==1&&(m&0x80)?96:b==1&&(m&32)?64:24;
        score+=same!=helpful?weight:-weight;
    }
    return score;
}
// Conservative immediate utility, not the original future-turn simulation.
static s16 status_ai_gain(u8 t,u16 value,u8 same)
{
    const Unit *u=&st.unit[t];u8 b=(u8)(value>>8),m=(u8)value,bit,k;
    s16 score=0;u8 current=core_status_current(&u->statuses,b);
    for(bit=128,k=0;bit;bit>>=1,k++)if(m&bit){
        u8 helpful=0,weight=24;
        if(b==2&&bit==2&&(current&bit)){
            if(!(u->statuses.innate[2]&bit))score+=same?48:-48;
            continue; // Frog on Frog removes the mutable transformation.
        }
        if(current&bit){
            if(b<3||(u->statuses.innate[b]&bit)||u->status_ct[((b-3)<<3)+k]>3)continue;
            weight=8;
        }
        if(b==2)helpful=(bit&0x70)!=0;   // Reraise/Float/Transparent
        if(b==3){helpful=(bit&0x78)!=0;if(bit&6)weight=48;}
        if(b==0)weight=bit==16?32:24;
        if(b==1){weight=bit==128?96:bit==32?64:32;}
        if(b==4){
            helpful=bit==128||bit==2;
            if(bit&0xc0){
                u8 n,spell=0;AbId ab;
                for(n=0;(ab=known(t,n))!=NO_ABIL;n++)if(abilities[ab].flags&AF_CAST){spell=1;break;}
                if(!spell)continue;
            }else if(bit&0x14)weight=48;
        }
        score+=same==helpful?weight:-weight;
    }
    return score;
}
#endif
