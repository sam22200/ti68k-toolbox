// Source physical weapon families. Preview random returns the retail midpoint.
#ifndef FFT_WEAPON_MATH_H
#define FFT_WEAPON_MATH_H
static u8 weapon_order(AbId ab){return ab==NO_ABIL||(ab<512&&(abilities[ab].flags&AF_CHARGE));}
static u8 weapon_variable(u8 type){return type==6||type==9||type==17;}
static u8 weapon_plain_gun(u8 weapon){return (u8)(weapon-71)<3;}
static s16 weapon_base_xa(u8 type,const Unit *u,u16 roll)
{
    // PA; PA/Speed; PA*Brave; MA; variable PA; PA/MA.
    static const u8 family[33]={0,1,1,0,2,2,4,0,3,4,0,0,1,5,5,0,3,4,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    u8 f=type<33?family[type]:0;s16 n;
    if(f==1)return (u->pa+u->sp)>>1;
    if(f==2){n=(u16)(u->pa*u->brave)/100;return n?n:1;}
    if(f==3)return u->ma;
    if(f==4)return (s16)(muls16(u->pa,(s16)roll)>>15)+1;
    if(f==5)return (u->pa+u->ma)>>1;
    return u->pa;
}
#endif
