// Private current-renderer review doors; no user save writes or emulator input.
#if defined(REVIEW_DEPLOY)
#include "deploy.c"
#elif defined(REVIEW_MAP)
#include "worldmap.c"
#elif defined(REVIEW_FILES)
#include "title.c"
#else
#include "manage.c"
#endif
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static const char *output;
static void shot(const char *group,u16 frame)
{
    char path[512];game_render();snprintf(path,sizeof path,"%s/%s-%03u.png",output,group,frame);sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    u16 frame;if(argc!=2)return 2;output=argv[1];sw_init(0);
#if defined(REVIEW_DEPLOY)
    if(!flowbank||!bank)return 3;
    for(frame=0;frame<48;frame++) {
        u32 keys=frame==16?K_F5:frame==24?K_F1:frame==32?K_LEFT:frame==36?K_A:0;
        if(!sw_step(keys))return 4;shot("deployment",frame);
    }
    dp.confirm=1;shot("confirm",0);
#elif defined(REVIEW_MAP)
    if(!bank||!mgb)return 3;
    wm.handx=160;wm.handy=120;wm.sel=255;shot("seam",0);
    game_scenario(0);wm.handx=loc_x(wm.at)+18;wm.handy=loc_y(wm.at);
    wm.sel=magnet();shot("magnet",0);
#elif defined(REVIEW_FILES)
    game_scenario(3);if(!title_save_ready)return 3;
    for(frame=0;frame<32;frame++){if(!sw_step(frame==8?K_DOWN:0))return 4;shot("save-files",frame);}
    title_cleanup();
#else
    if(!bank||save.n<2)return 3;
    for(frame=0;frame<32;frame++) {
        if(!sw_step(frame==8?K_RIGHT:frame==20?K_DOWN:0))return 4;shot("formation",frame);
    }
    mg.unit=1;set16(unit(1)->tjp[0],200);set16(unit(1)->tjp[2],200);
    set16(unit(0)->tjp[0],200);mg.unit=0;open_jobs();
    for(frame=0;frame<mg.job_n;frame++)if(mg.jobs[frame]==2){mg.job_pos=(u8)frame;mg.row=2;break;}
    dirty=1;shot("white-jobs",0);
    rt_prev=0;rt_keys=K_ENTER;if(!game_update()||!mg.confirm)return 5;rt_frame++;dirty=1;
    for(frame=0;frame<16;frame++){if(!sw_step(0))return 6;shot("job-confirm",frame);}
    if(!sw_step(K_ENTER)||!mg.job_anim)return 7;
    for(frame=0;frame<JOB_CHANGE_FRAMES+6;frame++) {
        u8 animation=mg.job_anim;
        shot("job-change",frame);
        mg.job_anim=0;dirty=1;shot("job-static",frame);mg.job_anim=animation;dirty=1;
        if(!sw_step(0))return 8;
    }
    set16(unit(0)->jp[career(unit(0))],250);game_scenario(4);shot("learn-jp",0);
#endif
    return 0;
}
