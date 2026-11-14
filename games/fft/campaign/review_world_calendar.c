// Review-only map date overlay. Mar21 is a picture example, not a migration date.
#include <assert.h>
#include <stdio.h>
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"
static void shot(const char *directory,const char *name)
{
    char path[4096];snprintf(path,sizeof path,"%s/%s.png",directory,name);
    assert(!sw_write_png(path,1));printf("%s %04X\n",name,sw_checksum());
}
int main(int argc,char **argv)
{
    FILE *file;if(argc!=3)return 2;
    sw_init(0);assert(bank&&mgb&&wmcache);
    file=fopen(argv[2],"rb");assert(file);assert(read_save_fixture(file,&save));fclose(file);
    save.loc=7;save.flags[0]|=2;game_scenario(0);game_render();shot(argv[1],"map-before");
    panel(4,3,42,11);black(8,6,"Mar 21");shot(argv[1],"map-date");
    release_map();return 0;
}
