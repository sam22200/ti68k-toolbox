// Shared AMS supervisor. The installed ffpack supplies the chapter's sequence.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#define SAVE_OFFSETS_ONLY
#include "save.h"
#include "pack_story.h"
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
static const unsigned char *package_file(const char *name,unsigned short *size)
{
    char sym[12];unsigned short n=(unsigned short)strlen(name);
    SYM_ENTRY *e;const unsigned char *p;
    if(n>8)return 0;
    sym[0]=0;memcpy(sym+1,name,n+1);e=SymFindPtr(sym+n+1,0);
    if(!e||!e->handle)return 0;
    p=(const unsigned char *)HeapDeref(e->handle);*size=*(const unsigned short *)p;
    return p+2;
}
static void progress_read(FpProgress *state)
{
    unsigned short n;const unsigned char *p=package_file(SAVE_NAME,&n);
    memset(state,0,sizeof *state);
    if(!p||n<SAVE_V2_BYTES+6||
       (memcmp(p,SAVE_MAGIC,4)&&memcmp(p,"FSV3",4)&&memcmp(p,"FSV2",4)))return;
    memcpy(state->flags,p+SAVE_FLAGS,32);
    if(n>=SAVE_V3_BYTES+6&&memcmp(p,"FSV2",4))state->flow=p[SAVE_FLOW_OFFSET];
}
static void progress_reset(void)
{
    SYM_ENTRY *e=SymFindPtr(SYMSTR(SAVE_NAME),0);
    if(e){if(e->flags.bits.archived)EM_moveSymFromExtMem(SYMSTR(SAVE_NAME),HS_NULL);SymDel(SYMSTR(SAVE_NAME));}
}
static unsigned short package_call(unsigned char program,unsigned short arg)
{
    static const char *const names[]={"","fftscn","fftbat","fftdep","ffttitle","fftmap","fftmng"};
    char command[24];
    sprintf(command,"%s(%u)",names[program],arg);
    return (unsigned short)chapter(command);
}
static void package_notice(unsigned char result,const FpNotice *notice)
{
    static const char *const messages[]={"","Install FFT core","Invalid FFT core","Install chapter pack",
        "Invalid chapter pack","Core/pack mismatch","Missing file","Damaged/mixed file",
        "Earlier chapter needed","Next chapter needed"};
    ClrScr();FontSetSys(F_4x6);
    printf_xy(2,25,"%s",messages[result]);
    if(notice->bad[0])printf_xy(2,38,"%s",notice->bad);
    if(notice->requested!=FP_NONE)printf_xy(2,51,"Install chapter %u",notice->requested);
    printf_xy(2,76,"Saves kept. ENTER to exit.");
    GKeyFlush();ngetchx();
}
void _main(void)
{
    FpNotice notice;unsigned char result;ESI args;unsigned short arg=0;
    InitArgPtr(args);if(GetArgType(args)==POSINT_TAG)arg=GetIntArg(args);
    result=fp_story(package_file,package_call,progress_read,progress_reset,arg,&notice);
    if(result)package_notice(result,&notice);
    push_shortint(0);
}
