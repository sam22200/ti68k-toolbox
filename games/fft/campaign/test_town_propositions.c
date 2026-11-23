// Actual management input, atomic session writes and continuation across processes.
#include <stdio.h>
static int fail_replace;
static int service_rename(const char *a,const char *b){return fail_replace?-1:rename(a,b);}
#define SC_FILE_RENAME service_rename
#include "manage.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
static const char *shots;
static u8 go=1;
static void tap(u32 key){if(!go)return;go=sw_step(key);if(go)go=sw_step(0);}
static void shot(const char *name)
{char p[512];game_render();snprintf(p,sizeof p,"%s/%s.png",shots,name);assert(!sw_write_png(p,1));}
static void list30(void)
{
    u8 n=0;tap(K_ENTER);assert(ps.screen==PS_LIST);
    while(ps_nth(PS_LIST,ps.row)!=30&&n++<96)tap(K_DOWN);
    assert(ps_nth(PS_LIST,ps.row)==30);tap(K_ENTER);assert(ps.screen==PS_TEXT);
}
int main(int argc,char **argv)
{
    Save initial,back;assert(argc==3);shots=argv[2];sw_init(100);
    assert(sizeof(Save)==4684&&ps.valid&&service_open(&save,ps.gates,0));initial=save;
    if(!strcmp(argv[1],"dispatch")) {
        u8 i;assert(!save.missions.dispatches.n&&save_generic(&save,1)&&!save_generic(&save,0));
        assert(!memcmp(save.magic,"FSV9",4));for(i=0;i<255;i++)assert(!save.poach[i]);
        shot("propositions-home");list30();shot("proposition-description");
        tap(K_ENTER);assert(ps.screen==PS_WORKERS&&ps.n==0);
        tap(K_ENTER);tap(K_DOWN);tap(K_ENTER);assert(ps.n==2);
        shot("proposition-workers");tap(K_F5);assert(ps.screen==PS_DAYS);tap(K_RIGHT);assert(ps.days==9);
        tap(K_ENTER);assert(ps.screen==PS_CONFIRM&&ps.row==1);shot("proposition-confirm");
        tap(K_ENTER);assert(ps.screen==PS_WORKERS&&!save.missions.dispatches.n&&ps_money()==prop_long(initial.money));
        tap(K_F5);tap(K_ENTER);tap(K_UP);tap(K_ENTER);assert(!go&&manage_result()==2);
        assert(save_load(&back)&&back.missions.dispatches.n==1&&prop_long(back.money)==prop_long(initial.money)-600);
        assert(back.missions.dispatches.job[0].days==9&&prop_away(&back.missions.dispatches,1)&&prop_away(&back.missions.dispatches,2));
        assert(!memcmp(back.unit,initial.unit,sizeof back.unit)&&!memcmp(back.inv,initial.inv,256));
    } else if(!strcmp(argv[1],"collect")) {
        PropositionState frozen;Save frozen_save;
        assert(save.missions.dispatches.n==1&&prop_report_ready(&save.missions.dispatches,0,6));
        assert(!(service_workers(&save)&6));assert(!roster_remove(&save,1));assert(!memcmp(&save,&initial,sizeof save));
        tap(K_DOWN);tap(K_DOWN);tap(K_ENTER);assert(ps.screen==PS_REPORTS);tap(K_ENTER);
        assert(ps.screen==PS_REPORT&&save_load(&back)&&back.missions.report.ready);
        shot(save.missions.dispatches.job[0].flags&PROP_RECALLED?"proposition-report-recall":"proposition-report");frozen=save.missions;frozen_save=save;
        fail_replace=1;tap(K_ENTER);assert(go&&ps.note==PROP_STORAGE);
        assert(!memcmp(&save.missions,&frozen,sizeof frozen)&&!memcmp(&save,&frozen_save,sizeof save));
        assert(save_load(&back)&&!memcmp(&back,&frozen_save,sizeof back));
        fail_replace=0;tap(K_ENTER);assert(!ps.note);tap(K_ENTER);assert(!go&&manage_result()==2);
        assert(save_load(&back)&&!back.missions.dispatches.n&&!back.missions.report.ready);
        assert(back.unit[1].exp==initial.unit[1].exp&&back.unit[1].level==initial.unit[1].level);
        assert(!memcmp(back.unit[1].raw,initial.unit[1].raw,sizeof back.unit[1].raw));
        assert(prop_long(back.money)>=prop_long(initial.money));
        frozen_save=back;assert(prop_collect(&back.missions,ps.rules,&back,30,6)==PROP_UNAVAILABLE);
        assert(!memcmp(&back,&frozen_save,sizeof back));
    } else if(!strcmp(argv[1],"recall")) {
        assert(save.missions.dispatches.n==1);tap(K_DOWN);tap(K_ENTER);assert(ps.screen==PS_ACTIVE);
        tap(K_ENTER);assert(ps.screen==PS_JOB);tap(K_ENTER);assert(ps.screen==PS_RECALL&&ps.row==1);
        shot("proposition-recall");tap(K_UP);tap(K_ENTER);assert(!go&&manage_result()==2);
        assert(save_load(&back)&&back.missions.dispatches.job[0].flags&PROP_RECALLED);
    } else if(!strcmp(argv[1],"resume")) {
        assert(!save.missions.report.ready);shot("propositions-after-report");
        assert(save.missions.dispatches.n==0);
    } else assert(0);
    puts("Playable Propositions: real input/session/continuation passed");return 0;
}
