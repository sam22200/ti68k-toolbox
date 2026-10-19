// Tiny AMS supervisor: each packed chapter returns before the next is loaded.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
static short chapter(const char *command)
{
    HANDLE h;short result;
    push_parse_text(command);h=HS_popEStack();
    NG_execute(h,0);HeapFree(h);
    result=GetArgType(top_estack)==POSINT_TAG ? GetIntArg(top_estack):0;
    return result;
}
void _main(void)
{
    ESI args;short test=0;
    InitArgPtr(args);if(GetArgType(args)==POSINT_TAG)test=GetIntArg(args)==99;
    if(test){if(chapter("fftopen(5)")&&chapter("fftacad(14)"))chapter("fftgar(61)");}
    else if(chapter("fftopen()") && chapter("fftacad()")) chapter("fftgar()");
    push_shortint(0);
}
