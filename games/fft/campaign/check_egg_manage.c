#include <assert.h>
#include <stdio.h>
static int fail_store;
static int egg_rename(const char *from,const char *to){return fail_store?-1:rename(from,to);}
#define SC_FILE_RENAME egg_rename
#include "manage.c"
#include "../../../runtime/platform-sw/rt_sw.h"
static void key(u32 k){assert(sw_step(k));assert(sw_step(0));}
static void shot(const char *path,u16 expected)
{assert(sw_checksum()==expected);assert(!sw_write_png(path,1));}
int main(int argc,char **argv)
{
    Save before,baseline,persisted;u8 i;
    if(argc!=2)return 2;sw_init(0);assert(bank&&common&&egg_bank);
    baseline=save;
    if(!strcmp(argv[1],"review")){
        save.n=10;
        for(i=7;i<10;i++){
            memset(&save.unit[i],0,sizeof save.unit[i]);save.unit[i].type=3;save.unit[i].job=94;save.unit[i].level=1;save.unit[i].gfx=255;
            save.world.egg[i]=i==7?0x32:i==8?0x31:0x55;wb_color_set(&save.world,i,(u8)(i-7));
        }
        mg.unit=7;mg.slide=0;dirty=1;game_render();shot("egg.png",0x9c07);
        key(K_C);shot("menu.png",0xaa1b);before=save;
        key(K_ENTER);shot("confirm.png",0xe3df);assert(mg.confirm==1);
        key(K_ENTER);assert(!mg.confirm&&memcmp(&save,&before,sizeof save)==0); // default No
        key(K_ENTER);key(K_B);assert(!mg.confirm&&memcmp(&save,&before,sizeof save)==0);
        key(K_ENTER);key(K_DOWN);key(K_ENTER);assert(save.n==9&&mg.changed);
        assert(save.world.egg[7]==0x31&&world_egg_color(&save.world,7)==1);
        before=save;
        for(i=9;i<16;i++)save.unit[i]=save.unit[1];save.n=16;
        save.unit[15].type=3;save.unit[15].job=94;save.world.egg[15]=0x32;
        mg.unit=15;mg.top=2;dirty=1;game_render();key(K_ENTER);assert(mg.screen==S_FORM&&mg.top==2);
        mg.unit=14;dirty=1;key(K_ENTER);assert(mg.screen==S_STATUS);key(K_F5);assert(mg.unit==0);
        key(K_B);save=before;dirty=1;
        assert(save_world_valid(&save));fail_store=1;assert(leave()&&cache&&mg.changed);
        assert(save_load(&persisted)&&memcmp(&persisted,&baseline,sizeof baseline)==0);
        fail_store=0;assert(!leave());puts("manage reviewed pixels/default No/cancel/Yes/compaction/failed-save retry pass");
    }else{
        assert(save_egg(&save,8));before=save;mg.unit=8;dirty=1;game_render();
        key(K_ENTER);assert(mg.screen==S_FORM&&memcmp(&save,&before,sizeof save)==0);
        key(K_C);key(K_ENTER);key(K_ENTER);assert(memcmp(&save,&before,sizeof save)==0);
        key(K_B);assert(mg.screen==S_FORM);assert(!leave());
        puts("manage cold child: egg stays out of fighter Status; no default dismissal or time/RNG changes");
    }
    return 0;
}
