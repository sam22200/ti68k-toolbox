// Actual pilot programs, driven through their normal input/save lifecycle.
// Optional autoplay uses the real All allies menu; defeat is an explicit fixture.
#include <assert.h>
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#if PILOT_KIND==1
#include "title.c"
#elif PILOT_KIND==2
#include "scene.c"
#elif PILOT_KIND==3
#include "deploy.c"
#elif PILOT_KIND==4
#include "flow_battle.c"
#elif PILOT_KIND==5
#include "worldmap.c"
#elif PILOT_KIND==6
#include "manage.c"
#endif
static u8 running=1;
#if PILOT_KIND==4 && defined(PILOT_AUTOPLAY)
static FILE *battle_keys;
static u32 previous_keys=~0U;
static void record_keys(u32 pressed)
{
    static const struct {u32 bit;const char *name;} names[]={
        {K_UP,"UP"},{K_LEFT,"LEFT"},{K_DOWN,"DOWN"},{K_RIGHT,"RIGHT"},
        {K_A,"A"},{K_B,"B"},{K_C,"C"},{K_D,"D"},{K_ENTER,"ENTER"},{K_ESC,"ESC"}};
    unsigned i;u32 unknown=pressed;
    if(!battle_keys||pressed==previous_keys)return;
    fprintf(battle_keys,"%u",rt_frame);
    for(i=0;i<sizeof names/sizeof names[0];i++)if(pressed&names[i].bit){fprintf(battle_keys," %s",names[i].name);unknown&=~names[i].bit;}
    assert(!unknown);fputc('\n',battle_keys);previous_keys=pressed;
}
#endif
static void step(u32 pressed){assert(running);
#if PILOT_KIND==4 && defined(PILOT_AUTOPLAY)
    record_keys(pressed);
#endif
    running=sw_step(pressed);}
static void tap(u32 pressed){step(pressed);if(running)step(0);}
static void snapshot(const char *name)
{
    FILE *a=fopen("ffsave.sav","rb"),*b;u8 bytes[sizeof(Save)];size_t n;
    if(!a)return;n=fread(bytes,1,sizeof bytes,a);fclose(a);b=fopen(name,"wb");assert(b);
    assert(fwrite(bytes,1,n,b)==n);fclose(b);
}
#ifdef PILOT_AUTOPLAY
#include "pilot_services.inc"
#endif
#if PILOT_KIND==4 && defined(PILOT_AUTOPLAY)
static FILE *battle_log;
static unsigned moves,hp_changes,turns;
static void battle_trace(u32 frame,const State *before)
{
    u8 i;
    if(before&&before->act!=st.act)turns++;
    for(i=0;i<NUNIT;i++) {
        const Unit *u=&st.unit[i],*old=before?&before->unit[i]:NULL;
        if(old){if(old->x!=u->x||old->z!=u->z)moves++;if(old->hp!=u->hp)hp_changes++;}
        if(old&&old->x==u->x&&old->z==u->z&&old->hp==u->hp&&old->mp==u->mp&&
           old->exp==u->exp&&old->jp==u->jp&&old->dc==u->dc&&before->act==st.act)continue;
        fprintf(battle_log,"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                frame,i,u->team,u->x,u->z,u->hp,u->mp,u->ct,u->level,u->exp,u->jp,u->tjp,u->dc,st.mode,st.act,rt_seed);
    }
}
static void delegate_all(void)
{
    assert(st.mode==M_MENU&&!st.ai&&st.unit[st.act].team==TEAM_PLAYER);
    while(st.menu!=MN_AUTO)tap(K_DOWN);
    tap(K_ENTER);assert(st.mode==M_AUTO);
    tap(K_DOWN);assert(st.sub==2);tap(K_ENTER);
    assert(st.auto_units&&!(st.dbg&DBG_GOD));
}
#endif
#if PILOT_KIND==3
static u8 neighbor(u8 cell,u8 direction)
{
    static const s8 dr[]={0,0,1,-1},dc[]={1,-1,0,0};
    s16 r=(s16)(cell/5)+dr[direction],c=(s16)(cell%5)+dc[direction];
    while((u16)r<5&&(u16)c<5){if(deploy_cell((u8)(r*5+c))[0])return (u8)(r*5+c);r+=dr[direction];c+=dc[direction];}
    return 255;
}
static void place_squad(void)
{
    static const u32 keys[]={K_LEFT,K_RIGHT,K_UP,K_DOWN};
    while(dp.n<deploy_rules()[2]) {
        u8 queue[25],parent[25],edge[25],head=0,tail=1,at,target=255,k,path[25],n=0;
        memset(parent,255,25);queue[0]=(u8)(dp.row*5+dp.col);parent[queue[0]]=queue[0];
        while(head<tail){at=queue[head++];if(dp.placed[at]==255){target=at;break;}
            for(k=0;k<4;k++){u8 next=neighbor(at,k);if(next!=255&&parent[next]==255){parent[next]=at;edge[next]=k;queue[tail++]=next;}}}
        assert(target!=255);for(at=target;at!=queue[0];at=parent[at])path[n++]=edge[at];
        while(n)tap(keys[path[--n]]);
        do{tap(K_F5);}while(deploy_where(dp.unit)!=255);
        tap(K_ENTER);assert(dp.placed[target]==dp.unit);
    }
    tap(K_C);assert(dp.confirm==1);tap(K_ENTER);assert(!running&&chapter_completed());
}
#endif
int main(int argc,char **argv)
{
    u16 argument;u32 frames=0;u8 result=0;const char *action;
    assert(argc==3);argument=(u16)atoi(argv[1]);action=argv[2];
#if PILOT_KIND==4 && defined(PILOT_AUTOPLAY)
    {char name[80];snprintf(name,sizeof name,"battle-%u-%s.keys",argument,action);battle_keys=fopen(name,"w");assert(battle_keys);}
#endif
    snapshot("child-entry.sav");sw_init(argument);
#if PILOT_KIND==1
    assert(title_bank&&title_mg&&title_back&&title_draft);
    if(!strcmp(action,"decline"))tap(K_B);
    else {
        if(argument==0&&!strcmp(action,"continue")){tap(K_DOWN);tap(K_ENTER);assert(title.screen==TITLE_FILES);}
        tap(K_ENTER);
        if(running&&title.screen==TITLE_OVERWRITE){assert(title.answer==1);tap(K_UP);tap(K_ENTER);}
    }
    assert(!running);result=title_finish();if(result)result=title_verified();title_cleanup();
    assert(result==(!strcmp(action,"new")?1:!strcmp(action,"continue")?2:!strcmp(action,"save")?3:0));
#elif PILOT_KIND==2
    assert(set&&W);tap(K_ESC);assert(skip_ask==1);tap(K_ENTER);assert(!skip_ask&&running);
    tap(K_ESC);tap(K_DOWN);assert(skip_ask==2);tap(K_ENTER);
    while(running&&frames++<300)step(0);
    assert(!running&&chapter_completed());result=1;release();
#elif PILOT_KIND==3
    assert(flowbank&&bank&&save.n);place_squad();result=1;
#elif PILOT_KIND==4
    assert(W&&battle_state&&flow_bank);snapshot(argument==100?"before-orbonne.sav":"before-gariland.sav");
    // Skip the on-map scene with its real default-No confirmation.
    if(st.open_stage||st.open_fade){tap(K_ESC);assert(st.quit==1);tap(K_DOWN);tap(K_ENTER);}
    while((st.open_stage||st.open_fade)&&running&&frames++<300)step(0);
    assert(running&&!st.open_stage);
#ifdef PILOT_AUTOPLAY
    {
        char name[80];snprintf(name,sizeof name,"battle-%u-%s.csv",argument,action);
        battle_log=fopen(name,"w");assert(battle_log);
        fputs("frame,unit,team,x,z,hp,mp,ct,level,exp,jp,tjp,dc,mode,active,rng\n",battle_log);
        battle_trace(frames,NULL);snprintf(name,sizeof name,"battle-%u-%s-start.state",argument,action);
        assert(!sw_save_state(name));
        printf("battle-start arg%u State%zu Work%zu Save%zu map%u nav%u\n",argument,sizeof(State),sizeof(Work),sizeof(Save),MAP_N,nav_capacity);
    }
    st.dbg=0;
#else
    st.dbg=DBG_FAST;
#endif
    if(!strcmp(action,"lose")) {
        u8 i;for(i=0;i<NUNIT;i++)if(st.unit[i].team!=TEAM_ENEMY)st.unit[i].hp=0;
        st.ai=AI_OFF;st.mode=M_CLOCK;st.banner=0;
    }
#ifndef PILOT_AUTOPLAY
    else debug_skip(); // the shipping DEBUG owner, not a fabricated victory/save
#endif
    while(running&&frames++<
#ifdef PILOT_AUTOPLAY
          200000
#else
          3000
#endif
         ) {
        u32 pressed=0;
#ifdef PILOT_AUTOPLAY
        State before=st;
        if(st.mode==M_MENU&&!st.ai&&!st.open_stage&&!st.open_fade&&st.unit[st.act].team==TEAM_PLAYER&&strcmp(action,"lose"))delegate_all();
#endif
        if(st.open_stage||st.open_fade)pressed=frames%4==0?K_B:0;
        else if(st.mode==M_END)pressed=frames%4==0?K_ENTER:0;
#ifdef PILOT_AUTOPLAY
        else if(st.ai)pressed=K_A; // ordinary held2nd acceleration, no debug flag
        step(pressed);battle_trace(frames,&before);
        if(frames%10000==0){printf("battle-progress arg%u frames%u mode%u turns%u hp-changes%u moves%u\n",argument,frames,st.mode,turns,hp_changes,moves);fflush(stdout);}
#else
        W->hud_ok=1;step(pressed);
#endif
    }
    assert(!running);
    if(!strcmp(action,"lose")){assert(st.end==END_DEFEAT&&!st.open_handoff);result=0;}
    else {assert(st.open_handoff);result=1;}
#ifdef PILOT_AUTOPLAY
    {
        char name[80];snprintf(name,sizeof name,"battle-%u-%s-end.state",argument,action);assert(!sw_save_state(name));
        fclose(battle_log);fclose(battle_keys);assert(!(st.dbg&DBG_GOD));
        if(strcmp(action,"lose"))assert(hp_changes&&moves&&turns);
        printf("battle-complete arg%u frames%u turns%u hp-changes%u moves%u end%u\n",argument,frames,turns,hp_changes,moves,st.end);
        printf("battle-final checksum%04X runtime-frames%u handoff%u dbg%u\n",sw_checksum(),rt_frame,st.open_handoff,st.dbg);
        snprintf(name,sizeof name,"battle-%u-%s.sav",argument,action);snapshot(name);
    }
#endif
    final_release();assert(!battle_state&&!rt_state&&!sbuf&&!vbuf&&!navbuf&&!acache);
#elif PILOT_KIND==5
    assert(bank&&wm.screen==SC_MAP&&(save.flags[0]&2));
#ifdef PILOT_AUTOPLAY
    if(argument==0&&!pilot_marker("pilot-managed.done")) {
        if(pilot_marker("before-gariland.sav"))pilot_town();
        tap(K_C);tap(K_DOWN);tap(K_ENTER);assert(!running&&map_result()==1);
        result=1;goto finished;
    }
#endif
    if(argument==0){tap(K_C);assert(wm.menu);tap(K_DOWN);tap(K_DOWN);tap(K_DOWN);tap(K_DOWN);tap(K_ENTER);assert(!running&&map_result()==2);}
    else {assert(wm.note==NOTE_SAVED);tap(K_ENTER);tap(K_ESC);assert(!running&&map_result()==0);}
    result=(u8)map_result();
#elif PILOT_KIND==6
    assert(bank&&save.n);pilot_formation();result=0;
#endif
finished:
    printf("pilot-child kind%u arg%u action%s result%u frames%u\n",PILOT_KIND,argument,action,result,frames);
    return result;
}
