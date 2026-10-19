#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "../../../runtime/core/rt.h"
#include "../../../runtime/platform-sw/rt_sw.h"
extern u8 chapter_completed(void);
int main(void)
{
    unsigned i;u8 go=1;
    if(access("ffac.bin",R_OK))assert(!chdir("data"));
    sw_init(0);
    for(i=0;i<2000&&go;i++){go=sw_step(i%2?0:K_ENTER);}
    assert(!go&&chapter_completed());
    sw_init(0);assert(!sw_step(K_ESC)&&!chapter_completed());
    printf("Academy: original dialogue and continuation, cancellation passed (%u frames)\n",i);
    return 0;
}
