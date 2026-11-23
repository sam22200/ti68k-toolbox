#ifndef FFT_SERVICES_H
#define FFT_SERVICES_H
#include "proposition_rules.h"
static u8 service_bank_valid(const u8 *p,u16 n)
{return p&&n>=8&&!memcmp(p,"FSV1",4)&&!p[6]&&!p[7]&&(p[4]==255||p[5]==255||p[4]!=p[5]);}
static u8 service_open(const Save *s,const u8 *p,u8 kind)
{u8 flag;if(!p||kind>1)return 0;flag=p[4+kind];return flag!=255&&(s->flags[flag>>3]&(1U<<(flag&7)));}
static u8 service_text_valid(const u8 *p,u16 size)
{
    u16 n,i,j,total;
    if(!p||size<738||memcmp(p,"FPT1",4)||p[6]!=96||p[7]!=77)return 0;
    total=prop_word(p+4);if(total>size||total<738)return 0;
    for(i=0;i<173;i++) {
        u16 at=i<96?prop_word(p+8+6*i):prop_word(p+584+2*(i-96));
        if(at<738||at>=total)return 0;
        for(n=at;n<total&&p[n];n++)if(p[n]<32||p[n]>126)return 0;
        if(n==total||n-at>4096)return 0;
        if(i<96) {
            u16 index=prop_word(p+10+6*i),last=0;u8 pages=p[12+6*i];
            if(!pages||pages>37||p[13+6*i]||index<738||index>=total||2*(u16)pages>total-index)return 0;
            for(j=0;j<pages;j++){u16 off=prop_word(p+index+2*j);if(off>=n-at||(!j&&off)||(j&&off<=last))return 0;last=off;}
        }
    }
    return 1;
}
static u16 service_workers(const Save *s)
{
    u8 i;u16 mask=0;
    for(i=0;i<s->n;i++)if(save_generic(s,i)&&!save_egg(s,i)&&!prop_away(&s->missions.dispatches,i)&&s->unit[i].type<2)
        mask|=(u16)(1U<<i);
    return mask;
}
static u16 service_world_draw(void *ctx)
{
    Save *s=(Save *)ctx;u32 n=prop_long(s->world_rng);
    n^=n<<13;n^=n>>17;n^=n<<5;prop_put_long(s->world_rng,n);return (u16)(n>>17);
}
// Both Fur purchases and sales use source half-price. The latter puts the
// sold item into poached stock, including a saturating255 stock already full.
enum { FUR_OK,FUR_NO_STOCK,FUR_FULL,FUR_NO_GIL,FUR_BAD_ITEM };
static u8 fur_trade(Save *s,u8 item,u16 price,u8 buy)
{
    u32 money=prop_long(s->money);
    if(!item||!price)return FUR_BAD_ITEM;
    price>>=1;
    if(buy) {
        if(!s->poach[item])return FUR_NO_STOCK;
        if(s->inv[item]>=99)return FUR_FULL;
        if(money<price)return FUR_NO_GIL;
        money-=price;s->poach[item]--;s->inv[item]++;
    } else {
        if(!s->inv[item])return FUR_NO_STOCK;
        money+=price;if(money>99999999UL)money=99999999UL;
        s->inv[item]--;if(s->poach[item]<255)s->poach[item]++;
    }
    prop_put_long(s->money,money);return FUR_OK;
}
#endif
