// Manual hardware-only check: run from HOME with the complete archived group.
// Uses a private fixture save. No production code or drawing changes.
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>

static short run(const char *command)
{
    HANDLE h;
    short result;
    push_parse_text(command);
    h=HS_popEStack();
    NG_execute(h,0);
    HeapFree(h);
    result=GetArgType(top_estack)==POSINT_TAG?GetIntArg(top_estack):0;
    return result;
}

void _main(void)
{
    static const char *const commands[]={
        "fftdep(200)","fftbat(261)","fftmap()","fftmng()",
        "fftmap()","fftmng()","fftbat(261)"
    };
    unsigned long available[8];
    short result[7],i;
    available[0]=HeapAvail();
    for(i=0;i<7;i++){
        result[i]=run(commands[i]);
        available[i+1]=HeapAvail();
    }
    ClrScr();FontSetSys(F_4x6);
    printf_xy(0,0,"Titanium package heap check");
    printf_xy(0,10,"before %lu",available[0]);
    for(i=0;i<7;i++)
        printf_xy(0,22+i*10,"%d return%d free %lu",i,result[i],available[i+1]);
    GKeyFlush();
    while(ngetchx()!=KEY_F5){}
}
