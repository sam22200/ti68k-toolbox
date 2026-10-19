// A separate process releases all old archive pointers before the next chapter.
#define main chapter_pc_main
#include "../../../runtime/platform-sdl/rt_sdl.c"
#undef main
extern unsigned char chapter_completed(void);
int main(int argc,char **argv)
{
    int result=chapter_pc_main(argc,argv);
    return result?result:chapter_completed()?23:0;
}
