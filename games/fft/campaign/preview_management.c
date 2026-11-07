// Isolated layout candidates for QA.md. No shipping management behavior is changed.
#include "manage.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"

static void shot(const char *folder,const char *name)
{
    char path[512];snprintf(path,sizeof path,"%s/%s.png",folder,name);sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    const u8 *original;u8 i;FILE *f;
    if(argc!=3)return 2;sw_init(0);if(!bank)return 3;
    f=fopen(argv[2],"rb");if(!f)return 4;
    if(!read_save_fixture(f,&save))return 5;fclose(f);
    for(i=3;i<10;i++){save.unit[i]=save.unit[i%3];save.unit[i].type=(i&1);save.unit[i].job=0x4A+(i%4);}
    save.n=10;mg.unit=0;mg.screen=S_FORM;
    original=bank;bank=rt_file("ffneutral",RT_NULL);if(!bank)return 6;
    page();shot(argv[1],"formation-after");
    mg.screen=S_STATUS;page();shot(argv[1],"status-after");
    // One other unit has unlocked Monk: demonstrate the source's gray rule.
    set16(save.unit[1].tjp[0],200);set16(save.unit[1].tjp[2],200);open_jobs();
    mg.job_pos=2;mg.row=mg.jobs[mg.job_pos];
    for(i=0;i<=8;i++) {char n[32];mg.spin=(s8)(8-i);page();snprintf(n,sizeof n,"jobs-after-%u",i);shot(argv[1],n);}
    bank=original;return 0;
}
