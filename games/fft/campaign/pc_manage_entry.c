// Match the native management result, including durable-service reentry.
#define main management_pc_main
#include "../../../runtime/platform-sdl/rt_sdl.c"
#undef main
extern unsigned short manage_result(void);
int main(int argc,char **argv){int r=management_pc_main(argc,argv);return r?r:(int)manage_result();}
