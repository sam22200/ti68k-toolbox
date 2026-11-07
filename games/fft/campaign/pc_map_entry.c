// Map result codes are the same as the native supervisor's (currently0/1/200).
#define main map_pc_main
#include "../../../runtime/platform-sdl/rt_sdl.c"
#undef main
extern unsigned short map_result(void);
int main(int argc,char **argv){int result=map_pc_main(argc,argv);return result?result:(int)map_result();}
