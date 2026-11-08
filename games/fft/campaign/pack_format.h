#ifndef FFT_PACK_FORMAT_H
#define FFT_PACK_FORMAT_H
/* Cold, allocation-free package readers. All serialized words are big endian;
   byte reads also accept unaligned or truncated input on the 68000. */
#include <string.h>
typedef const unsigned char *(*FpRead)(const char *, unsigned short *);
#define FP_CORE_HEADER 24U
#define FP_HEADER 48U
#define FP_DEP_BYTES 12U
#define FP_STEP_BYTES 8U
#define FP_ROUTE_BYTES 4U
#define FP_NONE 65535U
enum { FP_SCENE=1, FP_BATTLE, FP_DEPLOY };
#define FP_SAVE_PROMPT 1   /* step byte 1: offer the Save screen once the step completes */
enum { FP_OK, FP_MISSING_CORE, FP_BAD_CORE, FP_MISSING_PACK, FP_BAD_PACK,
       FP_WRONG_CORE, FP_MISSING_FILE, FP_BAD_FILE, FP_PREVIOUS, FP_NEXT };
static unsigned short fp_word(const unsigned char *p)
{ return (unsigned short)((unsigned short)p[0]<<8|p[1]); }
static unsigned short fp_check(const unsigned char *p,unsigned short n)
{
    unsigned short a=0,b=0,k;
    while(n){k=n>20?20:n;n-=k;
        do{a+=*p++;b+=a;}while(--k);
        a=(a&255)+(a>>8);b=(b&255)+(b>>8);
    }
    a=(a&255)+(a>>8);b=(b&255)+(b>>8);
    if(a==255)a=0;
    if(b==255)b=0;
    return (unsigned short)(b<<8|a);
}
static unsigned short fp_size(const unsigned char *p,unsigned short n,unsigned char core)
{
    unsigned short end,count,routes,steps,i,j,at;
    if(!p||n<(core?FP_CORE_HEADER:FP_HEADER)+2||memcmp(p,core?"FKR1":"FKP1",4))return 0;
    end=fp_word(p+4);
    if(end<(core?FP_CORE_HEADER:FP_HEADER)+2||end>n)return 0;
    /* rt_file includes the AMS dat trailer; portable raw .bin files do not. */
    if(end!=n&&(n-end!=6||memcmp(p+end,"\0dat\0\xf8",6)))return 0;
    if(fp_check(p,(unsigned short)(end-2))!=fp_word(p+end-2))return 0;
    count=fp_word(p+(core?6:34));if(!count||count>255)return 0;
    if(core)return end==FP_CORE_HEADER+count*FP_DEP_BYTES+2?end:0;
    routes=fp_word(p+30);steps=fp_word(p+32);
    if(!routes||routes>254||steps>255||fp_word(p+24)>256||fp_word(p+26)>256||
       fp_word(p+42)>1||fp_word(p+46)||fp_word(p+36)!=FP_HEADER||
       fp_word(p+38)!=FP_HEADER+routes*FP_ROUTE_BYTES||
       fp_word(p+40)!=FP_HEADER+routes*FP_ROUTE_BYTES+steps*FP_STEP_BYTES||
       end!=fp_word(p+40)+count*FP_DEP_BYTES+2)return 0;
    for(i=0;i<routes;i++){
        const unsigned char *r=p+FP_HEADER+i*FP_ROUTE_BYTES;
        if(!r[0]||r[0]>254||r[1]=='c'||
           !((r[1]>='a'&&r[1]<='z')||(r[1]>='0'&&r[1]<='9'))||r[2]||r[3])return 0;
        for(j=0;j<i;j++)if(p[FP_HEADER+j*FP_ROUTE_BYTES]==r[0])return 0;
    }
    at=fp_word(p+38);
    for(i=0;i<steps;i++){
        const unsigned char *s=p+at+i*FP_STEP_BYTES;
        unsigned short arg=fp_word(s+2);
        if(s[0]<FP_SCENE||s[0]>FP_DEPLOY||s[1]>FP_SAVE_PROMPT||s[7]||fp_word(s+4)>256)return 0;
        if(s[0]==FP_SCENE){if(arg>9)return 0;}
        else{
            for(j=0;j<routes;j++)if(p[FP_HEADER+j*FP_ROUTE_BYTES]==arg/100)break;
            if(j==routes)return 0;
        }
    }
    return end;
}
static unsigned char fp_dependencies(const unsigned char *p,unsigned char core,FpRead read,char *bad)
{
    unsigned short i,n,count=fp_word(p+(core?6:34));
    const unsigned char *d=p+(core?FP_CORE_HEADER:fp_word(p+40)),*file;
    for(i=0;i<count;i++,d+=FP_DEP_BYTES){
        unsigned char k;
        memcpy(bad,d,8);bad[8]=0;
        for(k=0;k<8&&bad[k];k++)
            if(!((bad[k]>='a'&&bad[k]<='z')||(k&&bad[k]>='0'&&bad[k]<='9')))return FP_BAD_FILE;
        if(!k)return FP_BAD_FILE;
        for(;k<8;k++)if(bad[k])return FP_BAD_FILE;
        file=read(bad,&n);if(!file)return FP_MISSING_FILE;
        if(n!=fp_word(d+8)||fp_check(file,n)!=fp_word(d+10))return FP_BAD_FILE;
    }
    return FP_OK;
}
static unsigned char fp_preflight(FpRead read,char *bad)
{
    const unsigned char *core,*pack;unsigned short n;unsigned char result;
    bad[0]=0;core=read("ffcore",&n);if(!core)return FP_MISSING_CORE;
    if(!fp_size(core,n,1))return FP_BAD_CORE;
    result=fp_dependencies(core,1,read,bad);if(result)return result;
    /* No allocations or archive moves while borrowed manifest pointers live. */
    pack=read("ffpack",&n);if(!pack)return FP_MISSING_PACK;
    if(!fp_size(pack,n,0))return FP_BAD_PACK;
    if(memcmp(core+8,pack+8,16))return FP_WRONG_CORE;
    return fp_dependencies(pack,0,read,bad);
}
static unsigned char fp_flag(const unsigned char *flags,unsigned short plus_one)
{
    if(!plus_one)return 0;
    plus_one--;
    return (unsigned char)((flags[plus_one>>3]>>(plus_one&7))&1);
}
static unsigned char fp_needed(const unsigned char *step,const unsigned char *flags,unsigned char flow)
{ return !fp_flag(flags,fp_word(step+4))&&(!step[6]||flow<step[6]); }
static unsigned char fp_letter(const unsigned char *pack,unsigned char encounter)
{
    unsigned short i,n=fp_word(pack+30);
    for(i=0;i<n;i++)if(pack[FP_HEADER+i*FP_ROUTE_BYTES]==(unsigned short)encounter+1)
        return pack[FP_HEADER+i*FP_ROUTE_BYTES+1];
    return 0;
}
#endif
