#ifndef FFT_GUEST_BANK_H
#define FFT_GUEST_BANK_H
// FGP1: magic4, byte length2, count2, then route/actor-slot/stable-ID2.
// A shared directory covers all encounters in the active chapter.
static u8 guest_bank_valid(const u8 *p,u16 size)
{
    u16 n,i,j;
    if(!p||size<8||memcmp(p,"FGP1",4)||RD16(p,2)!=size)return 0;
    n=RD16(p,3);if(!n||n>(65512U-8)/4||size!=8+4*n)return 0;
    for(i=0;i<n;i++){
        const u8 *e=p+8+4*i;u8 count=1;
        if(!((e[0]>='a'&&e[0]<='z'&&e[0]!='c')||(e[0]>='0'&&e[0]<='9'))||e[1]>=FFT_CORE_UNIT_MAX||!RD16(e,1))return 0;
        for(j=0;j<i;j++){
            const u8 *q=p+8+4*j;if(q[0]!=e[0])continue;
            if(q[1]==e[1]||RD16(q,1)==RD16(e,1)||++count>GUEST_MAX)return 0;
        }
    }
    return 1;
}
#endif
