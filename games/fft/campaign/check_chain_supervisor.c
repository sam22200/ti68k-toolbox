// Separate real PC child processes, matching the supervisor's program lifecycle.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pack_story.h"
#include "save_progress.h"
#include "flow.h"
typedef struct {char name[9];u8 *bytes;u16 size;} Cached;
static Cached cache[64];static u8 cached,variant,failed_once;
static u16 checks,failures,battles,prompts,continues,calls;static u32 digest=17;
static const char *battle_exe,*title_exe;static u16 programs[16],arguments[16];
static void check(u8 ok,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^ok;
    if(!ok){failures++;printf("chain-supervisor failure line%u\n",line);}
}
#define CHECK(x) check(!!(x),__LINE__)
static void release_files(void)
{while(cached)free(cache[--cached].bytes);}
static const u8 *read_file(const char *name,u16 *size)
{
    u8 i;char path[64];FILE *f;long n;
    for(i=0;i<cached;i++)if(!strcmp(name,cache[i].name)){*size=cache[i].size;return cache[i].bytes;}
    if(cached==64)return RT_NULL;
    sprintf(path,"%s.payload",name);f=fopen(path,"rb");if(!f)return RT_NULL;
    fseek(f,0,SEEK_END);n=ftell(f);rewind(f);if(n<0||n>65520){fclose(f);return RT_NULL;}
    i=cached++;strcpy(cache[i].name,name);cache[i].size=(u16)n;cache[i].bytes=malloc(n);
    if(!cache[i].bytes||fread(cache[i].bytes,1,n,f)!=(size_t)n)exit(111);
    fclose(f);*size=(u16)n;return cache[i].bytes;
}
static u8 read_save(const char *name,Save *s)
{
    FILE *f=fopen(name,"rb");size_t n;if(!f)return 0;
    memset(s,0,sizeof *s);n=fread(s,1,sizeof *s,f);fclose(f);return n==sizeof *s&&!memcmp(s->magic,SAVE_MAGIC,4);
}
static void progress(FpProgress *s)
{
    u8 raw[sizeof(Save)];FILE *f=fopen("ffsave.sav","rb");u16 n=0;
    if(f){n=(u16)fread(raw,1,sizeof raw,f);fclose(f);}
    CHECK(save_progress_read(n?raw:RT_NULL,n,s));
}
static void reset(void){CHECK(0);}
static u16 child(u8 program,u16 argument)
{
    pid_t pid;int status;char arg[16],ordinal[16];char *command[5];u8 losing=0;
    CHECK(calls<16);if(calls>=16)return 0;
    programs[calls]=program;arguments[calls++]=argument;
    if(program==FP_BATTLE){
        battles++;losing=variant==2&&argument==316&&!failed_once;if(losing)failed_once=1;
        sprintf(arg,"%u",argument);sprintf(ordinal,"%u",battles);
        command[0]=(char*)battle_exe;command[1]=arg;command[2]=losing?"lose":"win";command[3]=ordinal;command[4]=RT_NULL;
    }else if(program==FP_TITLE){
        if(argument==3)prompts++;else continues++;
        command[0]=(char*)title_exe;command[1]=argument==3?(variant==1?"decline":"save"):"continue";command[2]=RT_NULL;
    }else{CHECK(0);return 0;} // no world-map or deployment detour in this chain
    fflush(NULL);pid=fork();if(!pid){execv(command[0],command);_exit(111);}
    if(pid<0||waitpid(pid,&status,0)!=pid||!WIFEXITED(status))exit(111);
    CHECK(WEXITSTATUS(status)!=111);
    // Model AMS relocation: every borrowed archive pointer from before the
    // child is invalid now. ASan catches any supervisor read of that old view.
    release_files();return (u16)WEXITSTATUS(status);
}
int main(int argc,char **argv)
{
    FpNotice notice;Save initial,first,second,session,slot;u16 i;
    if(argc!=4)return 111;battle_exe=argv[1];title_exe=argv[2];variant=(u8)atoi(argv[3]);
    CHECK(read_save("ffsave.sav",&initial));
    CHECK(fp_story(read_file,child,progress,reset,variant==2?0:2,&notice)==FP_OK);
    CHECK(prompts==1&&battles==(variant==2?3:2));CHECK(continues==(variant==2?2:0));
    CHECK(read_save("battle-1-after.sav",&first));CHECK(read_save("battle-2-before.sav",&second));
    CHECK(!memcmp(&first,&second,sizeof first)); // actual cold child reads exactly the preceding receipt
    CHECK(read_save("ffsave.sav",&session)&&read_save("ffsave1.sav",&slot));
    CHECK((session.flags[0]&0x80)&&(session.flags[1]&1));
    CHECK(qlong(session.money)>qlong(first.money));
    CHECK(!memcmp(&slot,variant==1?&initial:&first,sizeof slot));
    if(variant==2){
        CHECK(read_save("battle-2-after.sav",&second));CHECK(!memcmp(&first,&second,sizeof first));
        CHECK(read_save("battle-3-before.sav",&second));CHECK(!memcmp(&first,&second,sizeof first));
        CHECK(programs[0]==FP_TITLE&&programs[1]==FP_BATTLE&&programs[2]==FP_TITLE&&
              programs[3]==FP_BATTLE&&programs[4]==FP_TITLE&&programs[5]==FP_BATTLE);
        CHECK(arguments[1]==216&&arguments[3]==316&&arguments[5]==316);
    }else CHECK(calls==3&&programs[0]==FP_BATTLE&&programs[1]==FP_TITLE&&programs[2]==FP_BATTLE);
    for(i=5;i<16;i++)CHECK(!memcmp(&initial.unit[i],&session.unit[i],sizeof(SaveUnit)));
    release_files();printf("chain-supervisor variant%u hash%08lX checks%u failures%u\n",variant,(unsigned long)digest,checks,failures);
    return failures?111:0;
}
