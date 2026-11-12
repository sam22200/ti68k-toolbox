// The AI and the Chapter I techniques: the actual planner on a flat grid picks each
// technique, then the production order executes it (campaign/MONSTER_STATUS_STRIKES.md).
#define AREA_UI_CHECK
#include "check_area_ai.c"
// The actor (slot 0, the acting side) at (2,4) has already moved; an enemy stands at (2,4-gap).
static void technique_fixture(AbId ab,u8 gap)
{
    u8 i,k;Unit seed,*u;
    CHECK(chapter_load(1),1);reset();seed=st.unit[U_RAMZA];
    flat(5,5,0,0);
    for(i=0;i<NUNIT;i++){
        st.unit[i]=seed;st.unit[i].hp=0;st.unit[i].x=st.unit[i].z=255;st.unit[i].dc=DC_GONE;
        st.unit[i].sp=5;st.unit[i].pa=5;st.unit[i].ma=40;st.unit[i].faith=70;st.unit[i].brave=70;
        st.unit[i].zodiac=0;st.unit[i].move=3;st.unit[i].learned=0;st.unit[i].defend=0;
        st.unit[i].mp=st.unit[i].max_mp=100;memset(st.unit[i].eq,255,sizeof st.unit[i].eq);
        memset(&st.unit[i].statuses,0,sizeof st.unit[i].statuses);st.unit[i].status=0;
        memset(st.affinity[i],0,5);st.reaction[i]=st.support[i]=0;
    }
    put_unit(0,2,4,0,TEAM_PLAYER);put_unit(1,2,(u8)(4-gap),0,TEAM_ENEMY);
    u=&st.unit[0];
    for(u->set=0x1D;u->set<0xE0;u->set++){
        for(k=0;k<SET_N;k++)if(SET_ABILITY(u->set,k)==ab)break;
        if(k<SET_N)break;
    }
    CHECK(u->set<0xE0,1);u->learned=(LearnBits)1<<k;if(ab>=0x100)u->job=94;else u->eq[EQ_WEAPON]=19;
    st.act=0;st.moved=1;st.acted=0;st.ai=AI_THINK;st.banner=st.turn=0;
    st.mode=M_BROWSE;W->ai_k=0;
}
static u8 at(u8 x,u8 z){return TILE_X[st.ai_tgt]==x&&TILE_Z[st.ai_tgt]==z;}
// The planned action through the production order (the AI's own aim step).
static void execute(void)
{
    CHECK(cursor_place(st.ai_tgt),1);W->hud_ok=1;st.ai=AI_AIM;st.ai_t=AI_PREVIEW+AI_NAME;
    ai_step();CHECK(st.mode,M_ACT);CHECK(st.acted,1);
}
static void monster_choices(void)
{
    static const AbId single[]={0x111,0x11B,0x132,0x130,0x10F,0x135,0x11E,0x12D};   // one adjacent enemy
    u8 i;
    for(i=0;i<sizeof single/sizeof single[0];i++){
        technique_fixture(single[i],1);
        if(single[i]==0x10F)st.unit[0].hp=20;             // Goblin Punch: the caster's missing HP
        if(single[i]==0x135)st.unit[0].pa=0;              // Beaking's PA - 2 against a weak Attack
        plan(0,0);CHECK(st.ai_ab,single[i]);CHECK(at(2,3),1);
        execute();CHECK(W->cast.mask,2);
    }
    // Mind Blast (radius 1): a centre covering the enemy, not the caster.
    technique_fixture(0x120,1);plan(0,0);CHECK(st.ai_ab,0x120);
    execute();CHECK(W->cast.mask,2);
    // Odd Soundwave (around the caster): an enemy with Protect and Haste to strip.
    technique_fixture(0x11F,1);st.unit[1].statuses.inflicted[3]=0x28;unit_status_refresh(&st.unit[1]);
    plan(0,0);CHECK(st.ai_ab,0x11F);CHECK(at(2,4),1);
    execute();CHECK(W->cast.mask,3);impact();CHECK(st.unit[1].statuses.inflicted[3],0);
    // Turn Punch (all adjacent panels): two enemies beat one Attack.
    technique_fixture(0x110,1);st.unit[0].pa=30;put_unit(2,1,4,0,TEAM_ENEMY);
    plan(0,0);CHECK(st.ai_ab,0x110);execute();CHECK(W->cast.mask,6);
    // Look of Fright: only a Brave drop into Chicken has an immediate value.
    technique_fixture(0x12E,3);plan(0,0);CHECK(st.ai_ab!=0x12E,1);
    technique_fixture(0x12E,3);st.unit[1].brave=15;plan(0,0);CHECK(st.ai_ab,0x12E);CHECK(at(2,1),1);
    execute();CHECK(W->cast.mask,2);
    // Self Destruct is not offered yet.
    technique_fixture(0x115,1);st.unit[0].hp=20;plan(0,0);CHECK(st.ai_ab!=0x115,1);
}
// Direct targeting from the planner: a clear line is used; an ally on the line makes the
// technique useless (it would take it); an enemy on the line takes it at execution.
static void direct_choices(void)
{
    technique_fixture(0x11E,2);plan(0,0);CHECK(st.ai_ab,0x11E);CHECK(at(2,2),1);
    execute();CHECK(W->cast.mask,2);
    technique_fixture(0x11E,2);put_unit(3,2,3,0,TEAM_PLAYER);plan(0,0);CHECK(st.ai_ab,AI_NONE);
    technique_fixture(0x11E,2);put_unit(3,2,3,0,TEAM_ENEMY);plan(0,0);CHECK(st.ai_ab,0x11E);
    execute();CHECK(W->cast.mask,8);
    technique_fixture(0x12D,3);plan(0,0);CHECK(st.ai_ab,0x12D);CHECK(at(2,1),1);
    execute();CHECK(W->cast.mask,2);
    technique_fixture(0x12D,3);put_unit(3,2,2,0,TEAM_PLAYER);plan(0,0);CHECK(st.ai_ab,AI_NONE);
}
// Holy Sword: each skill is chosen over the plain Attack and damages the enemy.
static void sword_choices(void)
{
    AbId ab;
    for(ab=0x9B;ab<=0x9F;ab++){
        u16 hp;technique_fixture(ab,1);plan(0,0);CHECK(st.ai_ab,ab);
        execute();hp=st.unit[1].hp;
        // Area skills own a batch; the single-panel ones (Split Punch, Crush Punch) the scalar strike.
        if(W->cast.phase==CAST_BATCH_READY)CHECK(W->cast.mask,2);else{CHECK(st.tgt,1);CHECK(st.hit,1);}
        impact();CHECK(st.unit[1].hp<hp,1);
    }
}
static void technique_trials(void)
{
    f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
    monster_choices();direct_choices();sword_choices();CHECK(chapter_load(1),1);reset();free(f);
}
#ifndef __m68k__
int main(void){sw_init(61);technique_trials();printf("ai_techniques %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();technique_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
