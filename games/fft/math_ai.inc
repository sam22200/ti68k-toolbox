// One global footprint per numeric criterion, no origin/centre quadratic scan.
// Native policy keeps the caster on its current panel; source AI ranking is separate.
static u8 math_ai_step(void)
{
    AreaAi *q=&W->area_ai;u8 budget=6,a=st.act,home=UNIT_CELL(&st.unit[a]);
#if !defined(FFT_MODULE)
    if(W->ai_k==4){memset(q,0,sizeof *q);q->next=1;W->ai_k=7;}
#endif
    while(q->next<60&&budget){
        u8 t;
        if(!q->x){
            if(!math_spell(q->next)||!can_use(a,q->next)){q->next++;continue;}
            W->ab=q->next;q->x=1; // every victim overwrites its usefulness bit before reduction
        }
        if(q->x<=NUNIT){
            t=q->x-1;area_ai_victim(t);
            if(q->gain[t]>0)q->useful|=(u16)1<<t;else q->useful&=(u16)~((u16)1<<t);
            q->x++;budget--;continue;
        }
        for(t=0;t<16;t++){
            AiAct candidate;u16 counted,mask;
            // Numeric masks are spell-independent while this frozen search owns scratch.
#if !defined(FFT_MODULE)
            if(!q->layer)q->math_mask[t]=math_filters(a,t)?(u16)math_targets(a,t):0;
#endif
            mask=q->math_mask[t];if(!mask)continue;
            if(area_ai_reduce(q,mask,NUNIT,&candidate,&counted)){
                candidate.pri+=W->ai_gain[home];candidate.ab=q->next;candidate.o=candidate.t=home;
                candidate.f=128|(t<<1);ai_insert(&candidate);
            }
        }
        q->layer++;q->next++;q->x=0;budget--;
    }
    return q->next>=60;
}
