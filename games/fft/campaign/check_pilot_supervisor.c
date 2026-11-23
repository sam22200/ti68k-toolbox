// Actual fp_story, a cold real child per call, and invalidated archive views.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "pack_story.h"
#include "save_progress.h"
typedef struct {char name[9];u8 *p;u16 n;} File;
static File cache[96];static u8 count,variant,loss;
static unsigned calls[7],total;static const char *prefix;
static void clear_files(void){while(count)free(cache[--count].p);}
static const u8 *read_file(const char *name,u16 *size)
{
    u8 i;char path[64];FILE *f;long n;
    for(i=0;i<count;i++)if(!strcmp(cache[i].name,name)){*size=cache[i].n;return cache[i].p;}
    snprintf(path,sizeof path,"%s.payload",name);f=fopen(path,"rb");if(!f)return 0;
    fseek(f,0,SEEK_END);n=ftell(f);rewind(f);assert(n>0&&n<65536&&count<96);
    i=count++;strcpy(cache[i].name,name);cache[i].p=malloc(n);assert(cache[i].p);
    cache[i].n=(u16)n;assert(fread(cache[i].p,1,n,f)==(size_t)n);fclose(f);*size=(u16)n;return cache[i].p;
}
static void progress(FpProgress *s)
{u8 b[sizeof(Save)];u16 n;FILE *f=fopen("ffsave.sav","rb");assert(f);n=(u16)fread(b,1,sizeof b,f);fclose(f);assert(save_progress_read(b,n,s));}
static void reset(void){assert(0);}
static u16 child(u8 program,u16 arg)
{
    static const char *const names[]={"","scene","battle","deploy","title","map","manage"};
    char exe[1024],argument[16];const char *action="run";pid_t pid;int status;
    assert(program>0&&program<7&&total++<30);calls[program]++;
    if(program==FP_TITLE)action=arg==3?"save":variant||calls[program]>1?"continue":"new";
    if(program==FP_BATTLE){action=variant==2&&arg==200&&!loss?"lose":"win";if(!strcmp(action,"lose"))loss=1;}
    snprintf(exe,sizeof exe,"%s-%s",prefix,names[program]);snprintf(argument,sizeof argument,"%u",arg);
    printf("pilot-call %s(%u) %s\n",names[program],arg,action);fflush(NULL);
    pid=fork();assert(pid>=0);if(!pid){execl(exe,exe,argument,action,(char *)0);_exit(111);}
    assert(waitpid(pid,&status,0)==pid&&WIFEXITED(status));assert(WEXITSTATUS(status)!=111);
    clear_files();return (u16)WEXITSTATUS(status);
}
int main(int argc,char **argv)
{
    FpNotice notice;FpProgress s;assert(argc==3);prefix=argv[1];variant=(u8)atoi(argv[2]);
    assert(fp_story(read_file,child,progress,reset,0,&notice)==FP_OK);progress(&s);
    assert(s.flags[0]&2);
#ifdef PILOT_AUTOPLAY
    assert(calls[FP_MAP]==3&&calls[FP_MANAGE]==1);
#else
    assert(calls[FP_MAP]==2&&!calls[FP_MANAGE]);
#endif
    assert(calls[FP_SCENE]==(variant==0?3:0));assert(calls[FP_DEPLOY]==(variant==0?1:0));
    assert(calls[FP_BATTLE]==(variant==1?0:2));assert(calls[FP_TITLE]==(variant==2?3:2));
    assert((variant==2)==loss);clear_files();
    printf("pilot-supervisor variant%u calls%u scenes%u deploy%u battles%u titles%u maps%u passed\n",
           variant,total,calls[FP_SCENE],calls[FP_DEPLOY],calls[FP_BATTLE],calls[FP_TITLE],calls[FP_MAP]);return 0;
}
