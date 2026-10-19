// Every extracted word must appear once across fitted continuation pages.
#include "academy.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    unsigned message,pages=0;
    if(access("ffac.bin",R_OK))assert(!chdir("data"));
    sw_init(0);
    for(message=0;message<AC_NMESSAGES;message++){
        academy.message=(u8)message;academy.offset=0;
        for(;;){
            char actual[300],expected[300];unsigned a=0,b=0,r,c;
            const char *source;layout();source=body();
            for(r=0;r<rows;r++)for(c=0;lines[r][c];c++)if(lines[r][c]!=' ')actual[a++]=lines[r][c];
            for(c=0;c<next;c++)if(source[c]!=' ')expected[b++]=source[c];
            assert(a==b&&!memcmp(actual,expected,a));assert(next);pages++;
            if(!source[next])break;
            academy.offset+=next;assert(pages<100);
        }
    }
    printf("Academy: all words preserved across %u fitted pages / %u source messages\n",pages,message);
    return 0;
}
