// Unit adapter: source bitmap state is authoritative; status is its four-bit cache.
#ifndef FFT_STATUS_UNIT_H
#define FFT_STATUS_UNIT_H
#ifdef FFT_CHAPTER
// These are separate queries: a statue still occupies its tile and retains HP.
static inline u8 unit_petrified(const Unit *u){return core_status_current(&u->statuses,1)&0x80;}
static inline u8 unit_on_field(const Unit *u)
{
    return u->x<MAP_W&&u->z<MAP_H&&UNIT_CELL(u)<MAP_N&&!DC_OBJ(u->dc);
}
// Real clock gain: Slow wins a simultaneous pair; Dead ignores both. Unlike
// the AT forecast, Slow at Speed1 may genuinely produce zero progress.
static inline u8 unit_ct_gain(const Unit *u)
{
    u8 gain=u->sp;
    if(u->hp){
        u8 rate=core_status_current(&u->statuses,3);
        if(rate&4)gain>>=1;
        else if(rate&8)gain+=gain>>1;
    }
    return gain;
}
static inline void unit_status_refresh(Unit *u)
{
    u->status=core_status_compact(&u->statuses);
    u->status_flags=(u8)((u->status_flags&STATUS_CONTEXT)|core_status_controls(&u->statuses));
}
static inline u8 unit_can_move(const Unit *u){return !(u->status_flags&(STATUS_CT_FROZEN|STATUS_NO_MOVE));}
static inline u8 unit_can_act(const Unit *u){return !(u->status_flags&(STATUS_CT_FROZEN|STATUS_NO_ACT));}
// Source target XA vulnerability is applied once, before zodiac compatibility.
static inline s16 unit_protect_xa(const Unit *u,s16 xa)
{
    return core_status_current(&u->statuses,3)&32?(s16)(muls16(xa<<1,21846)>>16):xa;
}
static inline s16 unit_status_xa(const Unit *u,s16 xa)
{
    xa=unit_protect_xa(u,xa);
    if((core_status_current(&u->statuses,0)&8)||(core_status_current(&u->statuses,4)&0x10))xa+=xa>>1;
    return xa;
}
static inline void unit_status_seed(Unit *u,u8 compact)
{
    compact&=15;
    memset(&u->statuses,0,sizeof u->statuses);u->status_flags=0;
    memset(u->status_ct,0,sizeof u->status_ct);
    if(compact&ST_POISON)u->status_ct[0]=cj_status_rules(common)[(SID_POISON<<4)+3];
    core_status_compact_mask(u->statuses.inflicted,compact);u->status=compact;
}
static inline u8 status_curable(const Unit *u,u8 mask)
{
    return core_status_curable(&u->statuses)&mask;
}
// Only implemented effect families may enter the running battle.
static inline u8 unit_status_request_supported(const CoreStatusDelta *d,u8 removal_only)
{
    if(removal_only>1)return 0;
    if(removal_only&&(d->add[0]|d->add[1]|d->add[2]|d->add[3]|d->add[4]))return 0;
    return !(((d->add[0]|d->remove[0])&0xed)|((d->add[1]|d->remove[1])&0x47)|
        ((d->add[2]|d->remove[2])&0xd1)|((d->add[3]|d->remove[3])&1)|((d->add[4]|d->remove[4])&0x20));
}
static inline u8 unit_status_cure(Unit *u,u8 mask)
{
    u8 removed=status_curable(u,mask);
    u->statuses.inflicted[0]&=(u8)~((removed&1)<<1);
    u->statuses.inflicted[1]&=(u8)~(((removed&4)<<3)|(removed&8));
    u->statuses.inflicted[3]&=(u8)~((removed&2)<<6);
    if(removed&ST_POISON)u->status_ct[0]=0;
    u->status&=(u8)~removed;return removed;
}
// Charging/Jump/Defend/Perform are command state, not ordinary inflictions.
// Replace their mutable bits together; source innate bits remain effective.
static inline void unit_status_action(Unit *u,u8 source_mask)
{
    u->statuses.inflicted[0]=(u8)((u->statuses.inflicted[0]&0xf0)|source_mask);
    u->status=(u8)((u->status&0xfe)|((core_status_current(&u->statuses,0)&2)>>1));
    // A command replaces only the low nibble: only its no-evasion cache can change.
    u->status_flags=(u8)((u->status_flags&~STATUS_NO_EVADE)|
        (core_status_no_evade(&u->statuses)?STATUS_NO_EVADE:0));
}
#else
static inline void unit_status_seed(Unit *u,u8 compact){u->status=compact;}
static inline u8 status_curable(const Unit *u,u8 mask){return u->status&mask;}
#endif
#endif
