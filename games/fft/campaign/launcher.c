// Shared AMS supervisor. The installed ffpack supplies the chapter's sequence.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#include "../../../lib/tilog.h"
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
/* AMS runs an archived child from a RAM copy (twin) and keeps it until HOME's event loop
   regains control, i.e. until the supervisor exits: each child would retain ~1 KB for the
   rest of the chapter, enough to starve the battle's allocations. Free it on return,
   as the event loop would (SymFindPtr on the full name first, as SymDelTwin requires). */
static void release_twin(const char *name)
{
    char sym[16];unsigned short n=(unsigned short)strlen(name);SYM_ENTRY *e;
    sym[0]=0;memcpy(sym+1,"main\\",5);memcpy(sym+6,name,n+1);
    e=SymFindPtr(sym+n+6,0);
    if(e&&e->flags.bits.twin&&!e->flags.bits.hidden)SymDelTwin(e);
}
static unsigned short package_call(unsigned char program,unsigned short arg)
{
    static const char *const names[]={"","fftscn","fftbat","fftdep","ffttitle","fftmap","fftmng"};
    char command[24];unsigned short result;
    sprintf(command,"%s(%u)",names[program],arg);
    tilog("story call %s free %lu max %lu",command,HeapAvail(),HeapMax());
    result=(unsigned short)chapter(command);
    release_twin(names[program]);
    tilog("story back %s -> %u free %lu max %lu",command,result,HeapAvail(),HeapMax());
    return result;
}
static void package_notice(unsigned char result,const FpNotice *notice)
{
    static const char *const messages[]={"","Install FFT core","Invalid FFT core","Install chapter pack",
        "Invalid chapter pack","Core/pack mismatch","Missing file","Damaged/mixed file",
        "Earlier chapter needed","Next chapter needed"};
    tilog("story stop %u %s",result,notice->bad);
    ClrScr();FontSetSys(F_4x6);
    printf_xy(2,25,"%s",messages[result]);
    if(notice->bad[0])printf_xy(2,38,"%s",notice->bad);
    if(notice->requested!=FP_NONE)printf_xy(2,51,"Install chapter %u",notice->requested);
    printf_xy(2,76,"Saves kept. ENTER to exit.");
    GKeyFlush();ngetchx();
}
/* The battle runs its modules from a heap block (fft.c mod_zone): an unpatched Titanium
   freezes there. GCC4TI's _execute_in_ghost_space test: HW3Patch in Flash clears a word of
   EX_stoBCD (ROM table entry 0xC0), in RAM it leaves 0x100 at 0xAC. */
static unsigned char hw3patch_missing(void)
{
    const unsigned char *ex=(*(const unsigned char *const **)0xC8)[0xC0];
    return HW_VERSION>=3&&*(const unsigned short *)(ex+92)&&*(const unsigned long *)0xAC!=0x100;
}
void _main(void)
{
    FpNotice notice;unsigned char result;ESI args;unsigned short arg=0;
    InitArgPtr(args);if(GetArgType(args)==POSINT_TAG)arg=GetIntArg(args);
    tilog("story start %u free %lu max %lu",arg,HeapAvail(),HeapMax());
    if(hw3patch_missing()){
        tilog("story stop HW3Patch missing");
        ClrScr();FontSetSys(F_4x6);
        printf_xy(2,25,"HW3Patch required: run");
        printf_xy(2,38,"hw3patch() once, see INSTALL.");
        printf_xy(2,76,"Saves kept. ENTER to exit.");
        GKeyFlush();ngetchx();push_shortint(0);return;
    }
    result=fp_story(package_file,package_call,progress_read,progress_reset,arg,&notice);
    if(result)package_notice(result,&notice);
    tilog("story end %u",result);
    push_shortint(0);
}
