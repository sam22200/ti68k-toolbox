/* Shared package parser and the actual supervisor policy, PC and real 68000. */
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#include <stdlib.h>
#endif
#include "pack_story.h"

static unsigned char core[4096],pack[8192],saved[3202];
static unsigned short core_n,pack_n,checks,failures,calls,stop_at,map_returns;
static unsigned short programs[64],arguments[64],reset_count;
static unsigned long hash=17;
static const char *hidden,*short_file;
static unsigned char same_name(const char *a,const char *b)
{unsigned short n=(unsigned short)strlen(a);return n==strlen(b)&&!memcmp(a,b,n);}
#ifndef __m68k__
typedef struct {char name[9];unsigned char *p;unsigned short n;} Cached;
static Cached cache[64];static unsigned short cache_n;
#endif
static const unsigned char *actual_file(const char *name,unsigned short *size)
{
#ifdef __m68k__
    char sym[12];unsigned short n=(unsigned short)strlen(name);SYM_ENTRY *e;const unsigned char *p;
    sym[0]=0;memcpy(sym+1,name,n+1);e=SymFindPtr(sym+n+1,0);if(!e)return 0;
    p=HeapDeref(e->handle);*size=*(const unsigned short *)p;return p+2;
#else
    unsigned short i;char path[64];FILE *f;long n;
    for(i=0;i<cache_n;i++)if(same_name(cache[i].name,name)){*size=cache[i].n;return cache[i].p;}
    sprintf(path,"%s.payload",name);f=fopen(path,"rb");if(!f)return 0;
    fseek(f,0,SEEK_END);n=ftell(f);rewind(f);if(n>65520||cache_n==64){fclose(f);return 0;}
    i=cache_n++;strcpy(cache[i].name,name);cache[i].n=(unsigned short)n;
    cache[i].p=malloc(n);if(fread(cache[i].p,1,n,f)!=(unsigned long)n)exit(2);fclose(f);
    *size=cache[i].n;return cache[i].p;
#endif
}
static const unsigned char *read_file(const char *name,unsigned short *size)
{
    const unsigned char *p;
    if(hidden&&same_name(name,hidden))return 0;
    if(same_name(name,"ffcore")){*size=core_n;p=core;}
    else if(same_name(name,"ffpack")){*size=pack_n;p=pack;}
    else p=actual_file(name,size);
    if(p&&short_file&&same_name(name,short_file))(*size)--;
    return p;
}
static void check(unsigned short ok,unsigned short line)
{
    checks++;hash=((hash<<3)|(hash>>29))^ok;
    hash&=0xffffffffUL;
    if(!ok){failures++;
#ifndef __m68k__
        printf("package failure at %u\n",line);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(x) check((x)!=0,__LINE__)
static void repair(unsigned char *p)
{
    unsigned short end=fp_word(p+4),c=fp_check(p,(unsigned short)(end-2));
    p[end-2]=(unsigned char)(c>>8);p[end-1]=(unsigned char)c;
}
static void progress(FpProgress *s)
{memcpy(s->flags,saved+4,32);s->flow=saved[3114];}
static void reset(void)
{memset(saved+4,0,32);saved[3114]=0;reset_count++;}
static unsigned short child(unsigned char program,unsigned short arg)
{
    programs[calls]=program;arguments[calls]=arg;calls++;
    if(calls==stop_at)return 0;
    if(program==FP_BATTLE&&arg/100==1)saved[4]|=1;
    if(program==FP_BATTLE&&arg/100==2){saved[4]|=2;saved[3114]=4;}
    if(program==FP_SCENE&&arg==1)saved[3114]=1;
    if(program==FP_DEPLOY)saved[3114]=2;
    if(program==FP_SCENE&&arg==2)saved[3114]=3;
    if(program==FP_MAP){
        if(map_returns==1)return 1;
        if(map_returns==2)return 2;
        if(map_returns==3)return 999;
        if(map_returns)map_returns--;
        return 0;
    }
    if(program==FP_MANAGE)map_returns=0;
    if(program==FP_TITLE&&arg==3){map_returns=0;return 3;}
    return 1;
}
static void prepare(unsigned char flags,unsigned char flow)
{
    memset(saved,0x5a,sizeof saved);memcpy(saved,"FSV4",4);memset(saved+4,0,32);
    saved[4]=flags;saved[3114]=flow;calls=stop_at=map_returns=reset_count=0;
    hidden=short_file=0;
}
static void trials(void)
{
    unsigned short n,i,j,start;unsigned char original,value;const unsigned char *p;char bad[9];FpNotice notice;
    p=actual_file("ffcore",&core_n);if(!p||core_n>sizeof core){CHECK(0);return;}memcpy(core,p,core_n);
    p=actual_file("ffpack",&pack_n);if(!p||pack_n>sizeof pack){CHECK(0);return;}memcpy(pack,p,pack_n);
    CHECK(fp_size(core,core_n,1));CHECK(fp_size(pack,pack_n,0));
    CHECK(fp_letter(pack,0)=='o');CHECK(fp_letter(pack,1)=='g');CHECK(!fp_letter(pack,2));
    for(i=0;i<48;i++)CHECK(!fp_size(pack,i,0));
    CHECK(fp_size(pack,(unsigned short)(pack_n-6),0));
#ifdef __m68k__
    BENCH_BEGIN(60);
#endif
    CHECK(fp_preflight(read_file,bad)==FP_OK);
#ifdef __m68k__
    BENCH_END(60);
#endif
    for(i=0;i<2;i++){
        unsigned char *data=i?pack:core;
        unsigned short size=i?pack_n:core_n;
        for(j=0;j<size;j++){
            data[j]^=1;CHECK(!fp_size(data,size,(unsigned char)!i));data[j]^=1;
        }
    }
    hidden="ffcore";CHECK(fp_preflight(read_file,bad)==FP_MISSING_CORE);
    hidden="ffpack";CHECK(fp_preflight(read_file,bad)==FP_MISSING_PACK);
    hidden="ffgd";CHECK(fp_preflight(read_file,bad)==FP_MISSING_FILE);CHECK(same_name(bad,"ffgd"));hidden=0;
    short_file="ffgd";CHECK(fp_preflight(read_file,bad)==FP_BAD_FILE);short_file=0;
    pack[8]^=1;repair(pack);CHECK(fp_preflight(read_file,bad)==FP_WRONG_CORE);pack[8]^=1;repair(pack);
    original=pack[48];pack[48]=0;repair(pack);CHECK(!fp_size(pack,pack_n,0));pack[48]=original;repair(pack);
    original=pack[52];pack[52]=pack[48];repair(pack);CHECK(!fp_size(pack,pack_n,0));pack[52]=original;repair(pack);
    original=pack[57];pack[57]=1;repair(pack);CHECK(!fp_size(pack,pack_n,0));pack[57]=original;repair(pack);
    /* Resume every persisted pilot boundary without replaying a committed step. */
    for(i=0;i<6;i++){
        static const unsigned char flags[6]={0,1,1,1,3,3},flow[6]={0,0,1,2,4,4};
        static const unsigned char first[6]={FP_SCENE,FP_SCENE,FP_DEPLOY,FP_SCENE,FP_MAP,FP_MAP};
        prepare(flags[i],flow[i]);
        CHECK(fp_story(read_file,child,progress,reset,2,&notice)==FP_OK);
        CHECK(programs[0]==first[i]);CHECK(!reset_count);
        CHECK(saved[1000]==0x5a&&saved[3201]==0x5a);CHECK(saved[4]==3);CHECK(saved[3114]==4);
    }
    for(i=1;i<=7;i++){
        unsigned char before[3202];prepare(0,0);stop_at=i;memcpy(before,saved,sizeof before);
        CHECK(fp_story(read_file,child,progress,reset,2,&notice)==FP_OK);CHECK(calls==i);
        CHECK(saved[1000]==before[1000]&&saved[3201]==before[3201]);
    }
    prepare(3,4);CHECK(fp_story(read_file,child,progress,reset,1,&notice)==FP_OK);CHECK(reset_count==1);
    prepare(0,0);CHECK(fp_story(read_file,child,progress,reset,99,&notice)==FP_OK);
    CHECK(programs[0]==FP_BATTLE&&arguments[0]==105);CHECK(programs[1]==FP_SCENE&&arguments[1]==1);
    prepare(3,4);map_returns=1;CHECK(fp_story(read_file,child,progress,reset,2,&notice)==FP_OK);CHECK(programs[1]==FP_MANAGE);
    prepare(3,4);map_returns=2;CHECK(fp_story(read_file,child,progress,reset,2,&notice)==FP_OK);
    CHECK(programs[1]==FP_TITLE&&arguments[1]==3);CHECK(programs[2]==FP_MAP&&arguments[2]==1);
    prepare(3,4);map_returns=3;CHECK(fp_story(read_file,child,progress,reset,2,&notice)==FP_NEXT);CHECK(notice.requested==2);
    prepare(0,0);hidden="ffpack";CHECK(fp_story(read_file,child,progress,reset,1,&notice)==FP_MISSING_PACK);
    CHECK(!calls&&!reset_count&&saved[1000]==0x5a);
    prepare(0,0);pack[25]=1;pack[44]=0;pack[45]=7;repair(pack);
    CHECK(fp_story(read_file,child,progress,reset,1,&notice)==FP_PREVIOUS);CHECK(notice.requested==7);
    CHECK(!calls&&!reset_count);CHECK(fp_story(read_file,child,progress,reset,0,&notice)==FP_PREVIOUS);
    CHECK(programs[0]==FP_TITLE&&arguments[0]==2);
    pack[25]=pack[45]=0;pack[44]=255;pack[45]=255;repair(pack);
    /* A future chapter may reuse the same assets under another battle band. */
    original=pack[52];pack[52]=3;
    start=fp_word(pack+38);
    for(i=0;i<fp_word(pack+32);i++){
        unsigned char *s=pack+start+i*8;
        if(s[0]!=FP_SCENE&&fp_word(s+2)==200){s[2]=1;s[3]=44;}
    }
    repair(pack);CHECK(fp_size(pack,pack_n,0));CHECK(fp_letter(pack,2)=='g');CHECK(!fp_letter(pack,1));
    /* Invalid offset/count words must be rejected before any indexed read. */
    for(i=30;i<=40;i+=2){value=pack[i];pack[i]=255;repair(pack);CHECK(!fp_size(pack,pack_n,0));pack[i]=value;repair(pack);}
    (void)n;(void)original;
}
#ifndef __m68k__
int main(void)
{
    unsigned short i;trials();printf("packages %08lX failures %u checks %u\n",hash,failures,checks);
    for(i=0;i<cache_n;i++)free(cache[i].p);
    return failures!=0;
}
#else
void _main(void)
{trials();BENCH_VALUE(hash);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
