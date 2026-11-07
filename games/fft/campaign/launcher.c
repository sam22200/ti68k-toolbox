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
static const char *active_save_name(void)
{
    SYM_ENTRY *e=SymFindPtr(SYMSTR(SAVE_SELECT_NAME),0);const unsigned char *p;
    if(e){p=(const unsigned char *)HeapDeref(e->handle);
        if(*(const unsigned short *)p==SAVE_SELECT_BYTES+6&&
           !memcmp(p+2,SAVE_SELECT_MAGIC,4)&&p[6]>=1&&p[6]<=3&&!p[7])
            return save_slot_name(p[6]);}
    return SAVE_LEGACY_NAME;
}
// The campaign save (save.h): story flag k set (a chapter's victory), 0 without a save
static short won(short k)
{
    SYM_ENTRY *e=SymFindPtr(SYMSTR(SAVE_NAME),0);
    const unsigned char *p;
    if(!e)return 0;
    p=(const unsigned char *)HeapDeref(e->handle)+2;
    return (!memcmp(p,SAVE_MAGIC,4)||!memcmp(p,"FSV2",4))&&p[SAVE_FLAGS+(k>>3)]>>(k&7)&1;
}
static short flow_phase(void)
{
    SYM_ENTRY *e=SymFindPtr(SYMSTR(SAVE_NAME),0);const unsigned char *p;
    if(!e)return 0;p=(const unsigned char *)HeapDeref(e->handle);
    return *(const unsigned short *)p>=SAVE_FLOW_OFFSET+8&&!memcmp(p+2,SAVE_MAGIC,4)?p[2+SAVE_FLOW_OFFSET]:0;
}
void _main(void)
{
    // The scene player (fftscn) shows what lies between battles: Orbonne's title and chapel
    // (ffs0), the chapter card and the Academy (ffs1).
    // The battle program (fftbat) plays every chapter: 100 + an Orbonne door, 200 + Gariland's.
    // After Gariland, the world map (fftmap) leads: it returns 0 (quit), 1 (the management
    // menus, fftmng) or a battle's door (fftbat), then it opens again.
    static const char *const steps[]={"fftscn(0)","fftbat(100)","fftscn(1)","fftdep(200)","fftscn(2)","fftbat(200)"};
    char cmd[16];
    ESI args;short arg=0,k=0,saved=0;
    InitArgPtr(args);if(GetArgType(args)==POSINT_TAG)arg=GetIntArg(args);
    if(!arg&&!chapter("ffttitle()")){push_shortint(0);return;}
    // Skip Orbonne only; retain deployment, Gariland's dialogue and the world map.
    if(arg==99){if(!chapter("fftbat(105)")){push_shortint(0);return;}k=2;}
    else{
        // fftstory(1): a new game, the save deleted; else the story resumes after the last
        // victory saved (Orbonne's: the card; Gariland's: the world map)
        if(arg==1){SYM_ENTRY *e=SymFindPtr(SYMSTR(SAVE_NAME),0);
            if(e){if(e->flags.bits.archived)EM_moveSymFromExtMem(SYMSTR(SAVE_NAME),HS_NULL);SymDel(SYMSTR(SAVE_NAME));}}
        else if(won(1))k=6;
        else if(won(0)){short p=flow_phase();k=p>=3?5:p>=2?4:p>=1?3:2;}
    }
    while(k<6&&chapter(steps[k]))k++;
    while(k==6){short r=chapter(saved?"fftmap(1)":"fftmap()");saved=0;
        if(!r)break;
        if(r==1)chapter("fftmng()");
        else if(r==2)saved=chapter("ffttitle(3)")==3;
        else{sprintf(cmd,"fftbat(%d)",r);chapter(cmd);}}
    push_shortint(0);
}
