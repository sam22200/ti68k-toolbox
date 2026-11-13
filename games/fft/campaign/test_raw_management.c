// Current-raw records keep their statistics through management/job/save changes.
#include "manage.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    Save back;SaveUnit *u;Stats seed,current;u8 k;u32 raw=508530UL;
    sw_init(0);assert(bank&&common);
    memset(&save,0,sizeof save);memcpy(save.magic,SAVE_MAGIC,4);save.n=1;
    u=&save.unit[0];u->type=2;u->job=1;u->level=10;
    for(k=0;k<5;k++){u->raw[k][0]=(u8)(raw>>16);u->raw[k][1]=(u8)(raw>>8);u->raw[k][2]=(u8)raw;}
    seed=stats(u);save_raw_set(&save,0,1);current=stats(u);
    assert(seed.hp>current.hp&&current.hp==(raw*job_rec(RAMZA_JOB)[J_MULT]/100>>14));
    change_job(u,3);assert(save_raw_current(&save,0)&&u->job==0x4d);
    assert(stats(u).hp==(raw*job_rec(3)[J_MULT]/100>>14));
    assert(rt_save(SAVE_NAME,&save,sizeof save)&&save_load(&back));
    assert(!memcmp(&save,&back,sizeof save)&&save_raw_current(&back,0));
    puts("Current raw stats: management, job changes and save keep the same raw values");
    return 0;
}
