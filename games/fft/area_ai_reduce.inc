// Source accounting averages counted target chances; it does not weight HP value
// by chance. Caller supplies current per-victim gains and usefulness classification.
static u8 area_ai_reduce(const AreaAi *q,u16 mask,u8 units,AiAct *out,u16 *counted)
{
    u8 t,count=0;u16 bit,hits=0;s16 score=0;
    *counted=0;
    for(t=0,bit=1;t<units;t++,bit<<=1)if((mask&bit)&&q->hit[t]){
        score+=q->gain[t];hits+=q->hit[t];count++;*counted|=bit;
    }
    if(!count||!(*counted&q->useful))return 0;
    out->pri=score;out->hit=hits/count;return 1;
}
