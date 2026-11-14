#ifndef FFT_MONSTER_FORMATION_H
#define FFT_MONSTER_FORMATION_H
// Universal approved FMA1 art. Archive pointers only, no extra cache/allocation.
#define MFA_RECORD 660U
#define MFA_NAMES 64U
#define MFA_FRAMES 574U
#define MFA_POOL 11134U
static u16 mfa_word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static u8 monster_formation_valid(const u8 *p,u16 size)
{
    u16 i,last=MFA_POOL;
    if(p&&size>=6&&!memcmp(p+size-6,"\0dat\0\xf8",6))size-=6;
    if(!p||size<MFA_POOL+2||size>65512U||(size&1)||memcmp(p,"FMA1\x10\x20\x1a\0",8)||
       mfa_word(p+8)!=255||mfa_word(p+10)!=MFA_FRAMES||mfa_word(p+12)!=MFA_NAMES||mfa_word(p+14)!=MFA_POOL)return 0;
    for(i=0;i<48;i++)if(p[16+i]!=i/3)return 0;
    for(i=0;i<255;i++){
        u16 at=mfa_word(p+MFA_NAMES+2*i);u8 n=0;
        if(at<MFA_POOL||at>=size)return 0;
        while(at<size&&p[at]&&n<15){if(p[at]<32||p[at]>126)return 0;at++;n++;}
        if(!n||at>=size||p[at])return 0;
        if(at>last)last=at;
    }
    if(size!=((last+2U)&~1U))return 0;
    if(size>last+1U&&p[last+1])return 0;
    for(i=0;i<16;i++){
        const u8 *g=p+MFA_FRAMES+MFA_RECORD*i;u16 k;u8 j;
        for(j=0;j<2;j++){
            u16 span=j?116:104;
            for(k=0;k<span;k++)if((g[k]|g[span+k])&g[2*span+k])return 0;
            g+=3*span;
        }
    }
    return 1;
}
static const u8 *monster_formation_frame(const u8 *p,u8 job)
{return p&&(u16)(job-94)<48U?p+MFA_FRAMES+(u16)p[16+job-94]*MFA_RECORD:RT_NULL;}
static const char *monster_formation_name(const u8 *p,u8 name)
{return p&&name<255?(const char *)(p+mfa_word(p+MFA_NAMES+2*(u16)name)):RT_NULL;}
#endif
