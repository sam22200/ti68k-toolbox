#ifndef FFT_OBJECTIVE_BANK_H
#define FFT_OBJECTIVE_BANK_H
#include "objective_rules.h"
// FGO1: chapter-wide route directory, then interned named strips and identities.
// All entry offsets are positive byte displacements from that entry. No RAM copy.
#define GO_TARGET(g) RD16(g,1)
#define GO_PROTECT(g) RD16(g,2)
static u8 objective_bank_valid(const u8 *bank,u16 size)
{
    u16 count,i,j,head;
    if(!bank||size<8||RD32(bank,0)!=0x46474f31UL||RD16(bank,2)!=size)return 0;
    count=RD16(bank,3);
    if(!count||count>254||count>(size-8)/16)return 0;
    head=(u16)(8+16*count);
    for(i=0;i<count;i++){
        const u8 *e=bank+8+16*i;u16 target=GO_TARGET(e),protect=GO_PROTECT(e);
        if(!((e[0]>='a'&&e[0]<='z'&&e[0]!='c')||(e[0]>='0'&&e[0]<='9'))||e[1]||
           !(target|protect)||!core_objective_valid(target,protect,UNIT_CAPACITY)||
           (protect&(protect-1)))return 0;
        for(j=0;j<i;j++)if(bank[8+16*j]==e[0])return 0;
        for(j=0;j<4;j++){
            u16 off=RD16(e,j<2?3+j:4+j),at=(u16)(e-bank);
            u8 needed=j==0||(j==1&&protect)||(j==2&&target)||(j==3&&protect);
            if(!needed){if(off||(j==1&&e[11]))return 0;continue;}
            if(!off||(off&1)||off<head-at||off>size-at)return 0;
            at+=off;
            if(j<2){
                const u8 *g=bank+at;u16 bytes;
                if(size-at<4||!e[10+j]||e[10+j]>152||g[0]&15||g[1]!=(j?25:3)||
                   !g[2]||g[2]>18||!g[3]||(g[3]&1)||g[3]>20||g[0]+8*g[3]>160)return 0;
                bytes=(u16)(g[2]*g[3]);
                if(bytes>(size-at-4)/3)return 0;
            }else{
                u8 n=0;while(at<size&&bank[at]){
                    u8 c=bank[at++];if(c<32||c>126||++n>31)return 0;
                }
                if(!n||at==size)return 0;
            }
        }
    }
    return 1;
}
#endif
