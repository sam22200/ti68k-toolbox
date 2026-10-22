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
    // The scene player (fftscn) shows what lies between battles: Orbonne's title and chapel
    // (ffs0), the chapter card and the Academy (ffs1).
    // The battle program (fftbat) plays every chapter: 100 + an Orbonne door, 200 + Gariland's.
    if(test){if(chapter("fftbat(105)")&&chapter("fftscn(1)"))chapter("fftbat(261)");}
    else if(chapter("fftscn(0)")&&chapter("fftbat(100)")&&chapter("fftscn(1)"))chapter("fftbat(200)");
    push_shortint(0);
}
