// Pure multi-target predictions through actual cold images and an owned live cast.
#define FORECAST_CHECK
#include "check_batch.c"
static CastBatch prediction;
static void request(AbId ab,u8 ref,u8 follow)
{
    memset(&prediction,0xA5,sizeof prediction);prediction.actor=0;
    prediction.ability=ab;prediction.ref=ref;prediction.follow=follow;
    prediction.phase=CAST_BATCH_EMPTY;fixture_monster(0,ab);   // before any world hash
}
static void forecast_profiles(void)
{
    u16 bytes,n,p[189],mask;u8 i;u32 hash,seed;CastBatch before,live;
    const u8 *src=rt_file("fbf00",&bytes);CHECK(src!=RT_NULL,1);if(!src)return;
    CHECK(RD16(src,2),4);CHECK(sizeof prediction,60);
    for(n=0;n<4;n++){
        read_profile(src,n,p);prepare_profile(p);request(0x100+n,first,0);
        hash=world_hash();seed=rng32;live=W->cast;
#ifdef FFT_MODULES
        mod_call(MOD_PARTY,255,0);
#endif
        ZB(58);CHECK(cast_forecast(&prediction),1);ZE(58);
        CHECK(prediction.phase,CAST_BATCH_FORECAST);mask=0;
        for(i=0;i<NUNIT;i++){
            const u16 *r=p+7+14*i;
            if(r[10])mask|=(u16)1<<i;
            // The power-255 profile's wrapped amount (above 999) is not representable: bit 7 of
            // the power marks formula 0x4E (no Faith); retail powers stay below 128 (max 96).
            CHECK(prediction.victim[i].hit,r[11]);if(r[12]<=999)CHECK(prediction.victim[i].amount,r[12]);
            CHECK(prediction.victim[i].effect,r[13]);
        }
        CHECK(prediction.mask,mask);CHECK(world_hash()==hash,1);CHECK(rng32==seed,1);
        CHECK(memcmp(&live,&W->cast,sizeof live)==0,1);before=prediction;
        ZB(59);CHECK(cast_forecast(&prediction),1);ZE(59);
        CHECK(memcmp(&before,&prediction,sizeof before)==0,1);
        CHECK(world_hash()==hash,1);CHECK(rng32==seed,1);
        CHECK(cast_batch(CAST_BATCH_PREPARE,&prediction),0);
        CHECK(cast_batch(CAST_BATCH_COMMIT,&prediction),0);
        CHECK(world_hash()==hash,1);CHECK(rng32==seed,1);
        CHECK(memcmp(&before,&prediction,sizeof before)==0,1);
        // Effect previews remain available when payment/command eligibility changes.
        st.unit[0].mp=0;hash=world_hash();CHECK(cast_forecast(&prediction),1);
        CHECK(memcmp(&before,&prediction,sizeof before)==0,1);CHECK(world_hash()==hash,1);
    }
}
static void rejected_forecasts(void)
{
    u16 size,p[189];u8 i;const u8 *surface,*src=rt_file("fbf00",&size);
    u32 hash,seed;CastBatch before;
    read_profile(src,1,p);prepare_profile(p);
    for(i=0;i<9;i++){
        request(0x101,first,0);
        if(i==0)prediction.actor=13;
        if(i==1)prediction.ability=512;
        if(i==2)prediction.ability=0xf; // disabled retail record
        if(i==3)prediction.ability=0x106; // unsupported normalized drain
        if(i==4)prediction.ability=0x107; // unsupported raw performance
        if(i==5)prediction.follow=2;
        if(i==6){prediction.follow=1;prediction.ref=13;}
        if(i==7)prediction.phase=CAST_BATCH_READY;
        if(i==8)prediction.phase=CAST_BATCH_APPLIED;
        fixture_monster(prediction.actor,prediction.ability);
        before=prediction;hash=world_hash();seed=rng32;
        CHECK(cast_forecast(&prediction),0);CHECK(world_hash()==hash,1);CHECK(rng32==seed,1);
        CHECK(memcmp(&before,&prediction,sizeof before)==0,1);
    }
    request(0x101,first,0);before=prediction;surface=W->surface;W->surface=RT_NULL;
    hash=world_hash();CHECK(cast_forecast(&prediction),0);CHECK(world_hash()==hash,1);
    CHECK(memcmp(&before,&prediction,sizeof before)==0,1);W->surface=surface;
    CHECK(cast_forecast(RT_NULL),0);
}
static void live_and_bindings(void)
{
    u16 size,p[189];u8 i;u32 hash,seed;CastBatch live,before;
    const u8 *src=rt_file("fbf00",&size);
    read_profile(src,1,p);prepare_profile(p);
    memset(&W->cast,0,sizeof W->cast);W->cast.ability=0x101;W->cast.ref=first;
    CHECK(cast_batch(CAST_BATCH_PREPARE,&W->cast),1);CHECK(st.unit[0].mp,0);
    CHECK(W->cast.phase,CAST_BATCH_READY);live=W->cast;hash=world_hash();seed=rng32;
    request(0x101,first,0);CHECK(cast_forecast(&prediction),1);
    CHECK(world_hash()==hash,1);CHECK(rng32==seed,1);CHECK(memcmp(&W->cast,&live,sizeof live)==0,1);
    CHECK(cast_forecast(&W->cast),0);CHECK(memcmp(&W->cast,&live,sizeof live)==0,1);
    CHECK(cast_batch(CAST_BATCH_COMMIT,&W->cast),1);CHECK(W->cast.phase,CAST_BATCH_APPLIED);
    for(i=0;st.action.phase!=CTX_IDLE&&i<32;i++)end_anim();CHECK(i<32,1);
    // Followed references use current positions; a fixed tile remains fixed.
    read_profile(src,1,p);prepare_profile(p);
    for(i=1;i<NUNIT;i++){st.unit[i].hp=100;memset(&st.unit[i].statuses,0,sizeof st.unit[i].statuses);}
    unit_place(&st.unit[1],second);unit_place(&st.unit[2],third);
    request(0x104,1,1);CHECK(cast_forecast(&prediction),1);before=prediction;
    CHECK(prediction.mask,(u16)cast_targets(0,1,0x104,1));
    unit_place(&st.unit[1],third);CHECK(cast_forecast(&prediction),1);
    CHECK(prediction.mask,(u16)cast_targets(0,1,0x104,1));CHECK(prediction.mask!=before.mask,1);
    request(0x104,second,0);CHECK(cast_forecast(&prediction),1);
    CHECK(prediction.mask,(u16)cast_targets(0,second,0x104,0));
    // Empty footprints are valid; every old victim entry must be erased.
    for(i=1;i<NUNIT;i++){st.unit[i].x=255;st.unit[i].dc=DC_GONE;}
    request(0x105,first,0);CHECK(cast_forecast(&prediction),1);CHECK(prediction.mask,0);
    for(i=0;i<UNIT_CAPACITY;i++){CHECK(prediction.victim[i].hit,0);CHECK(prediction.victim[i].amount,0);CHECK(prediction.victim[i].effect,AB_NULL);}
}
static void last_node(void)
{
    u16 size,n,len,i,j,count,area;const u8 *src=rt_file("fat00",&size),*p;u8 found=0,rot;
    CHECK(src!=RT_NULL,1);if(!src)return;p=src+6;n=RD16(src,2);
    f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
    for(i=0;i<n;i++,p+=len+2){
        len=RD16(p,0);
        area=(u16)p[2]*p[3];count=0;
        for(j=0;j<2*area;j++)if(!p[16+3*j+2])count++;
        if(count!=256)continue;
        bind_profile(p+2,0);found=1;break;
    }
    CHECK(found,1);
    if(found){
        CHECK(MAP_N,256);CHECK(UNIT_CELL(&st.unit[12]),255);
        request(0x101,255,0);CHECK(cast_forecast(&prediction),1);
        CHECK(prediction.mask,(u16)cast_targets(0,255,0x101,0));
        request(0x101,12,1);CHECK(cast_forecast(&prediction),1);
        CHECK(prediction.mask,(u16)cast_targets(0,12,0x101,1));
        for(rot=0;rot<4;rot++){st.rot=rot;CHECK(cast_forecast(&prediction),1);CHECK(prediction.mask,(u16)cast_targets(0,12,0x101,1));}
    }
    // Scenario reset intentionally keeps a loaded chapter; restore our synthetic
    // topology explicitly before returning to its real unit definitions.
    CHECK(chapter_load(1),1);reset();free(f);
}
static void forecast_trials(void){forecast_profiles();rejected_forecasts();live_and_bindings();last_node();}
#ifndef __m68k__
int main(void){sw_init(61);forecast_trials();printf("forecast %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();forecast_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
