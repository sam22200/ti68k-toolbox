// Tiny AMS supervisor: each packed chapter returns before the next is loaded.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#define SAVE_OFFSETS_ONLY
#include "save.h"
static short chapter(const char *command)
{
    HANDLE h;short result;
    push_parse_text(command);h=HS_popEStack();
    NG_execute(h,0);HeapFree(h);
    result=GetArgType(top_estack)==POSINT_TAG ? GetIntArg(top_estack):0;
    return result;
}
// The campaign save (save.h): story flag k set (a chapter's victory), 0 without a save
static short won(short k)
{
    SYM_ENTRY *e=SymFindPtr(SYMSTR(SAVE_NAME),0);
    const unsigned char *p;
    if(!e)return 0;
    p=(const unsigned char *)HeapDeref(e->handle)+2;
    return !memcmp(p,SAVE_MAGIC,4)&&p[SAVE_FLAGS+(k>>3)]>>(k&7)&1;
}
void _main(void)
{
    // The scene player (fftscn) shows what lies between battles: Orbonne's title and chapel
    // (ffs0), the chapter card and the Academy (ffs1).
    // The battle program (fftbat) plays every chapter: 100 + an Orbonne door, 200 + Gariland's.
    // The management menus (fftmng) edit the party after a victory.
    static const char *const steps[]={"fftscn(0)","fftbat(100)","fftscn(1)","fftbat(200)","fftmng()"};
    ESI args;short arg=0,k=0;
    InitArgPtr(args);if(GetArgType(args)==POSINT_TAG)arg=GetIntArg(args);
    if(arg==99){if(chapter("fftbat(105)")&&chapter("fftscn(1)"))chapter("fftbat(261)");}
    else{
        // fftstory(1): a new game, the save deleted; else the story resumes after the last
        // victory saved (Orbonne's: the card; Gariland's: the management menus)
        if(arg==1){SYM_ENTRY *e=SymFindPtr(SYMSTR(SAVE_NAME),0);
            if(e){if(e->flags.bits.archived)EM_moveSymFromExtMem(SYMSTR(SAVE_NAME),HS_NULL);SymDel(SYMSTR(SAVE_NAME));}}
        else if(won(1))k=4;
        else if(won(0))k=2;
        while(k<5&&chapter(steps[k]))k++;
    }
    push_shortint(0);
}
