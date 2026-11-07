#define main title_pc_main
#include "../../../runtime/platform-sdl/rt_sdl.c"
#undef main
extern unsigned char title_finish(void),title_verified(void);
extern void title_cleanup(void);
int main(int argc,char **argv)
{
    int result=title_pc_main(argc,argv);
    if(!result){result=title_finish();if(result)result=title_verified();}
    title_cleanup();return result;
}
