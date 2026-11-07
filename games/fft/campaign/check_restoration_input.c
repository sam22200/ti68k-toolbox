// Old direct policy against native SCREENS dispatch; no emulator or hardware input.
#define RESTORATION_RUNTIME_CHECK
#include "check_restoration.c"
#define move_cursor reference_move_cursor
#define face_of reference_face_of
#define to_menu reference_to_menu
#define auto_control reference_auto_control
#define menu_keys reference_menu_keys
#define acts_keys reference_acts_keys
#define ai_shown reference_ai_shown
#include "input_reference.inc"
#undef move_cursor
#undef face_of
#undef to_menu
#undef auto_control
#undef menu_keys
#undef acts_keys
#undef ai_shown
static u32 input_work(void)
{
    u8 j;u32 h=W->frz_h;
    for(j=0;j<3;j++)h=(h<<1|h>>31)^W->win_k[j];
    h=(h<<1|h>>31)^W->status_i;h=(h<<1|h>>31)^W->hold;
    h=(h<<1|h>>31)^W->frz;h=(h<<1|h>>31)^W->frz_n;
    return (h<<1|h>>31)^W->ai_k;
}
static void input_start(u8 n)
{
    prepare_spell(0x13);st.mode=M_MENU;st.menu=MN_MOVE;st.sub=st.skill=0;
    cursor_place(UNIT_CELL(&st.unit[st.act]));rt_keys=K_A;rt_prev=0;
    W->hold=7;W->frz=1;W->frz_n=3;W->frz_h=0x12345678UL;W->ai_k=4;
    W->status_i=2;W->win_k[0]=M_MENU;W->win_k[1]=W->win_k[2]=0;
    if(n<5)st.menu=n;
    else if(n==5||n==6){st.moved=1;st.acted=n==6;st.move_cell=UNIT_CELL(&st.unit[st.act]);rt_keys=K_ESC;}
    else if(n==7)rt_keys=K_DOWN;
    else if(n<12){st.mode=M_ACTS;st.sub=n==8?0:n==9?1:2;st.unit[st.act].defend=1;if(n==11)rt_keys=K_ESC;}
    else if(n<16){st.mode=M_SKILLS;if(n==13)st.unit[st.act].mp=0;if(n==14)rt_keys=K_ESC;
        if(n==15){st.unit[st.act].status_flags|=STATUS_NO_ACT;st.mode=M_ACTS;}}
    else if(n<19){st.mode=M_BROWSE;st.ai=AI_THINK;st.auto_units=0x1fff;}
    else {st.ai=AI_MOVE;st.ai_o=second;st.ai_t=0;W->hud_ok=1;}
}
static void run_input(u8 n,u8 old)
{
    u8 j;
    if(n<8){if(old)reference_menu_keys();else menu_keys();}
    else if(n<16){if(old)reference_acts_keys();else acts_keys();}
    else if(n<19){if(old)reference_auto_control(n-16);else auto_control(n-16);}
    else for(j=0;j<18;j++){u8 v=old?reference_ai_shown():ai_shown();digest=(digest<<1|digest>>31)^v;}
}
static void input_trials(void)
{
    State *saved=malloc(sizeof(State));u8 n,rot,dir;u32 expected;
    static const s8 du[4]={1,-1,0,0},dv[4]={0,0,1,-1};
    CHECK(saved!=RT_NULL,1);if(!saved)return;
    for(n=0;n<20;n++){
        input_start(n);run_input(n,1);memcpy(saved,&st,sizeof st);expected=input_work();
        input_start(n);run_input(n,0);CHECK(memcmp(saved,&st,sizeof st),0);CHECK(input_work()==expected,1);
    }
    for(rot=0;rot<4;rot++)for(dir=0;dir<4;dir++){
        input_start(0);st.rot=rot;reference_move_cursor(du[dir],dv[dir]);memcpy(saved,&st,sizeof st);
        n=reference_face_of(du[dir],dv[dir]);input_start(0);st.rot=rot;move_cursor(du[dir],dv[dir]);
        CHECK(memcmp(saved,&st,sizeof st),0);CHECK(face_of(du[dir],dv[dir]),n);
    }
    free(saved);
}
#ifndef __m68k__
int main(void){sw_init(61);input_trials();printf("restoration_input %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();input_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
