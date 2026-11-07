// Original magical evasion profiles through pure queries and actual COMMAND/ACTION.
#define ELEMENTS_CHECK
#include "check_magic.c"

static u8 columns[2048];
static void prepare_evasion(const u16 *p)
{
    Unit *a,*t;u8 sid;
    holy_prepare();a=&st.unit[U_RAMZA];t=&st.unit[10];
    memset(&a->statuses,0,sizeof a->statuses);memset(&t->statuses,0,sizeof t->statuses);
    memset(st.affinity[U_RAMZA],0,5);memset(st.affinity[10],0,5);
    st.support[U_RAMZA]=st.support[10]=st.reaction[10]=0;
    a->mp=999;memset(t->eq,255,5);t->eq[EQ_ACC]=p[1];t->eq[EQ_WEAPON]=p[2];t->eq[EQ_SHIELD]=p[3];
    if(p[4]){sid=p[4]-1;t->statuses.innate[sid>>3]|=0x80>>(sid&7);if(sid==SID_DEAD)t->hp=0;}
    if(p[5])t->statuses.innate[0]|=2;
    if(p[6])a->statuses.innate[2]|=0x10;
    if(p[7])st.reaction[10]=0x1c1;
    memset(columns,0,sizeof columns);W->surface=columns;
    columns[((u16)(W->grid_row[t->z]+t->x)<<3)+4*node_layer(UNIT_CELL(t))+1]=p[8]<<5;
    unit_status_refresh(a);unit_status_refresh(t);rand_seed(p[9]);
}
static void source_profiles(void)
{
    u16 n,count,size,p[17];const u8 *source=rt_file("fme00",&size);u8 k,present,valid;u32 before,after,hash;char text[40];
    CHECK(source!=RT_NULL,1);if(!source)return;
    count=RD16(source,2);CHECK(count,238);
    for(n=0;n<count;n++){
        for(k=0;k<17;k++)p[k]=RD16(source+6+34*n,k);prepare_evasion(p);before=rng32;hash=state_hash();
        ZB(51);CHECK(magic_hit(U_RAMZA,10,p[0],0),p[10]);ZE(51);
        CHECK(state_hash()==hash,1);CHECK(rng32==before,1);
        for(k=0;k<p[12];k++)rand15();after=rng32;rng32=before;
        ZB(52);CHECK(magic_hit(U_RAMZA,10,p[0],1),p[11]);ZE(52);CHECK(rng32==after,1);
        present=magic_target(10);CHECK(present==0,p[16]);
        valid=p[4]!=SID_DEAD+1&&p[4]!=SID_PETRIFY+1&&p[4]!=32&&!p[16];
        CHECK(present==1,valid);
        prepare_evasion(p);CHECK(skill_hit(U_RAMZA,10,p[0]),valid?p[10]:0);
        CHECK(rng32==before,1);forecast_text(U_RAMZA,10,p[0],text);CHECK(rng32==before,1);
        if(!valid)CHECK(text[0],'M');
        if(!p[16]){
            for(k=0;k<p[15];k++)rand15();after=rng32;rng32=before;
            st.open_resolving=1;ZB(53);skill(U_RAMZA,10,p[0]);ZE(53);
            CHECK(st.hit,p[13]);CHECK(st.dmg,p[14]);CHECK(st.unit[U_RAMZA].mp,999-skill_cost(U_RAMZA,p[0]));
            CHECK(rng32==after,1);CHECK(st.push|st.crit,0);
            impact();CHECK(st.unit[10].hp,p[4]==SID_DEAD+1?0:999-p[14]);
            CHECK(st.action.pending,0);
        }
    }
}
static void due_validity(void)
{
    static const u8 ids[6]={SID_DEAD,SID_PETRIFY,31,SID_CRYSTAL,SID_JUMP,15};
    u8 i,k;u32 seed,after;u16 p[10]={0x10,255,255,255,0,0,0,0,0,9};
    for(i=0;i<6;i++){
        prepare_evasion(p);seed=rng32;CHECK(cast_order(U_RAMZA,10,0x10,1),2);
        st.pending[U_RAMZA].ticks=0;
        st.unit[10].statuses.innate[ids[i]>>3]|=0x80>>(ids[i]&7);if(i==0)st.unit[10].hp=0;
        unit_status_refresh(&st.unit[10]);
        for(k=0;k<(i<3?5:0);k++)rand15();after=rng32;rng32=seed;
        CHECK(charge_resolve(),i<3);CHECK(st.unit[U_RAMZA].mp,999-skill_cost(U_RAMZA,0x10));
        CHECK(rng32==after,1);CHECK(st.pending[U_RAMZA].state,0);
        if(i<3){CHECK(st.hit,0);CHECK(st.dmg,0);impact();CHECK(st.unit[10].hp,i==0?0:999);}
    }
    // Nullification runs after evasion but before the generic proc draw.
    prepare_evasion(p);st.affinity[10][1]=128;seed=rng32;
    for(k=0;k<4;k++)rand15();after=rng32;rng32=seed;
    cast_order(U_RAMZA,10,0x10,1);st.pending[U_RAMZA].ticks=0;charge_resolve();
    CHECK(st.hit,0);CHECK(st.dmg,0);CHECK(rng32==after,1);impact();CHECK(st.unit[10].hp,999);
}
static void ai_and_facings(void)
{
    u16 p[10]={0xf,232,141,138,0,0,0,0,0,9},steps;u8 i,k,f,af;
    prepare_evasion(p);
    for(af=0;af<4;af++)for(f=0;f<4;f++){
        st.unit[U_RAMZA].face=af;st.unit[10].face=f;
        CHECK(skill_hit(U_RAMZA,10,0x10),54); // accessory25%, right20%, left10%
    }
    for(i=0;i<3;i++){
        p[4]=i==0?SID_PETRIFY+1:i==1?32:SID_CRYSTAL+1;prepare_evasion(p);
        st.unit[U_RAMZA].learned=0xffff;st.unit[U_RAMZA].defend=0;st.acted=0;st.moved=1;W->ai_k=0;
        steps=0;while(!ai_think()&&++steps<1024){}CHECK(steps<1024,1);
        for(k=0;k<W->ai_n;k++)CHECK(W->ai_rank[k].ab==0xf&&W->ai_rank[k].pri>0,0);
    }
}
static void menu_cache(void)
{
    u16 p[10]={0xf,255,255,255,0,0,0,0,0,9};u8 k;char out[40];
    prepare_evasion(p);W->win_k[0]=M_MENU;W->win_k[1]=0;W->win_k[2]=U_RAMZA;W->win_n=1;
#ifdef FFT_MODULES
    mod_call(MOD_ACTION,255,0);
#endif
    ZB(54);make_window();ZE(54);CHECK(W->win_n,2);
    for(k=0;k<MN_N;k++){
        W->win_n=k+2;
#ifdef FFT_MODULES
        mod_call(MOD_RESULTS,255,0);mod_context(MOD_RESULTS)->window_out=out;
        CHECK(mod_call(MOD_RESULTS,1,0),C_BLACK);
#else
        CHECK(window_label(out),C_BLACK);
#endif
        CHECK(memcmp(out,k==0?"Move":k==1?"Act":k==2?"Wait":k==3?"Status":"Auto-battle",k==1?4:k==3?7:k==4?12:5),0);
    }
}
static void trials_evasion(void){
#ifndef MAGIC_EVASION_INTEGRATION_ONLY
    source_profiles();
#endif
    due_validity();ai_and_facings();menu_cache();
}
#ifndef __m68k__
int main(void){sw_init(61);trials_evasion();printf("magic-evasion %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();trials_evasion();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
