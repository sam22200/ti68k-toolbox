#ifndef FFT_MOVEMENT_RULES_H
#define FFT_MOVEMENT_RULES_H
// Source bitsets remain byte-sized; no floating point or dynamic rule caches.
typedef struct {u8 flags[3],kind,row,jump,jump_half,body,stored,water,lava;} MoveConfig;
static void core_movement_config(MoveConfig *c,const u8 *flags,u8 jump,u8 status)
{
    u8 f2=flags[1],f3=flags[2],i;
    for(i=0;i<3;i++)c->flags[i]=flags[i];
    c->lava=(f3&40)!=0;c->body=status&6?4:6;
    if(status&6){f2&=243;f3&=251;}
    c->stored=f3;if(f3&8){c->body+=2;c->stored|=128;}
    if(status&64)f3|=8;
    if(f2&16)f3&=175;
    c->flags[1]=f2;c->flags[2]=f3;
    if(f3&4){c->kind=0;c->row=1;}
    else if(f2&12){c->kind=1;c->row=1;}
    else if(f3&8){c->kind=2;c->row=2;}
    else if(f3&32){c->kind=3;c->row=5;}
    else if(f3&128){c->kind=4;c->row=3;}
    else if(f3&64){c->kind=5;c->row=3;}
    else if(f3&16){c->kind=6;c->row=4;}
    else {c->kind=7;c->row=0;}
    jump=jump<8?jump:7;c->jump_half=jump>>1;
    c->jump=c->kind==0||(flags[0]&2)?31:jump;
    c->water=(c->stored&192)!=0;
}
static void core_movement_bonus(u8 flags,u8 *move,u8 *jump)
{
    u16 m=*move,j=*jump;
    if(flags&128)m++;if(flags&64)m+=2;if(flags&32)m+=3;
    if(flags&16)j++;if(flags&8)j+=2;if(flags&4)j+=3;
    *move=m>252?252:(u8)m;*jump=j>7?7:(u8)j;
}
static u8 core_movement_cost(u8 cost,u8 flags,u8 weather)
{
    if(!cost)return flags&2?1:weather;
    return (flags&1)&&cost!=255?1:cost;
}
static u8 core_teleport_chance(u8 distance,u8 move)
{return distance<=move?100:distance-move>=10?0:(u8)(100-(distance-move)*10);}
#endif
