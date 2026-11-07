// Half-height interval model for strike/lunge collision;255 denotes no actor.
#ifndef FFT_MELEE_RULES_H
#define FFT_MELEE_RULES_H
typedef struct {u8 feet,top,id,pad;} MeleeSpan;
static u8 core_melee_overlap(const MeleeSpan *a,const MeleeSpan *t,u8 extra)
{return a->id!=255&&t->id!=255&&t->feet<(u16)a->top+extra&&(u16)t->top+extra>=a->feet;}
static u8 core_melee_lunge(const MeleeSpan *a,const MeleeSpan *t,u8 distance,const u8 *floor,const MeleeSpan *middle)
{
    u8 low,high,order0=0,order1=1,mid;const MeleeSpan *v;
    if(a->id==255)return 255;
    if(distance==1)return core_melee_overlap(a,t,2)?t->id:255;
    if(distance!=2)return 255;
    if(t->id!=255&&t->feet<(u16)a->top+2&&t->top>=a->feet){
        low=(s16)a->feet-6<t->feet?t->feet:a->feet-6;
        high=t->top<(u16)a->top+2?t->top:a->top+2;
        if(floor[0]>floor[1]){order0=1;order1=0;}
        if(floor[order0]!=255){
            u8 top=middle[order0].id==255?floor[order0]:middle[order0].top;
            if(low<top)low=top;if(high<top)low=255;
        }
        if(floor[order1]!=255&&middle[order1].id!=255&&
           low>=floor[order1]&&middle[order1].top>=high)low=255;
        if(high>low)return t->id;
    }
    mid=((u16)a->feet+a->top)>>1;
    for(low=0;low<2;low++){v=&middle[low];if(v->id!=255&&v->feet<mid&&mid<v->top)return v->id;}
    return 255;
}
#endif
