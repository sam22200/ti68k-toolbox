#ifndef FFT_MODULE_BANK_H
#define FFT_MODULE_BANK_H
#include "../../runtime/core/rt.h"
static u16 mb_word(const u8 *p,u16 word)
{
#ifdef __m68k__
    return ((const u16 *)p)[word];
#else
    return (u16)((u16)p[2*word]<<8|p[2*word+1]);
#endif
}
// FMB3 contiguous ID ranges; both matching export tokens are required.
// Validate the entire pair before any executable bytes are copied.
// big: the one image id allowed above the zone (the companion, run from its own block; 255: none).
static u8 module_bank_part_valid(const u8 *bank,u16 size,u16 first,u16 n,u16 count,
    u16 context_bytes,u16 signature,u16 zone,u16 big)
{
    u16 id,end=14+2*n;
    if(!bank||size<end||mb_word(bank,0)!=0x464D||mb_word(bank,1)!=0x4233||
       mb_word(bank,2)!=first||mb_word(bank,3)!=n||mb_word(bank,4)!=count)return 0;
    for(id=0;id<n;id++){
        u16 start=mb_word(bank,7+id),bytes,context,entry;const u8 *m;
        if(start!=end||start>size-10)return 0;
        m=bank+start;bytes=mb_word(m,3);context=mb_word(m,4);entry=mb_word(m,2);
        if(mb_word(m,0)!=0x464D||mb_word(m,1)!=0x4432||(bytes&1)||bytes<context_bytes||(first+id!=big&&bytes>zone)||bytes-context_bytes+2>size-start-10||
           (entry&1)||entry>=bytes||(context&1)||context>bytes||context_bytes>bytes-context||
           mb_word(m+10+context,0)!=signature)return 0;
        end=start+12+bytes-context_bytes;
    }
    return size-end==0||size-end==6;
}
static u8 module_banks_valid(const u8 *a,u16 asize,const u8 *b,u16 bsize,
    u16 split,u16 count,u16 context_bytes,u16 signature,u16 zone,u16 big)
{
    if(!a||!b||asize<14||bsize<14||!split||split>=count||count>255||context_bytes<2)return 0;
    return mb_word(a,5)==mb_word(b,5)&&mb_word(a,6)==mb_word(b,6)&&
        module_bank_part_valid(a,asize,0,split,count,context_bytes,signature,zone,big)&&
        module_bank_part_valid(b,bsize,split,count-split,count,context_bytes,signature,zone,big);
}
static u8 module_three_valid(const u8 *a,u16 asize,const u8 *b,u16 bsize,const u8 *c,u16 csize,
    u16 split,u16 end,u16 count,u16 context_bytes,u16 signature,u16 zone,u16 big)
{
    if(!a||!b||!c||asize<14||bsize<14||csize<14||!split||split>=end||end>=count||count>255||context_bytes<2)return 0;
    return mb_word(a,5)==mb_word(b,5)&&mb_word(a,6)==mb_word(b,6)&&
        mb_word(a,5)==mb_word(c,5)&&mb_word(a,6)==mb_word(c,6)&&
        module_bank_part_valid(a,asize,0,split,count,context_bytes,signature,zone,big)&&
        module_bank_part_valid(b,bsize,split,end-split,count,context_bytes,signature,zone,big)&&
        module_bank_part_valid(c,csize,end,count-end,count,context_bytes,signature,zone,big);
}
#endif
