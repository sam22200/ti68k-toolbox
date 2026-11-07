// One source weapon formula; callers have returned before this image executes.
static void weapon_roll_one(u8 a,u8 t)
{
    u8 weapon=current_weapon(a),charge=st.abil<512&&(abilities[st.abil].flags&AF_CHARGE)?abilities[st.abil].y:0;
    const u8 *w=cj_weapon(common,weapon);u8 f=w[2],sid=255;u16 base=16384,tier=16384;
    st.tgt=t;st.self=0;st.crit=st.push=0;
    st.effect_kind=weapon_kind_hand(a,t,st.hand);st.hit=1;
    st.hit=charge?magic_hit(a,t,st.abil,1):roll_hit(a,t);if(!st.hit)return;
    if(f==4)tier=rand15();
    else if(weapon!=255&&weapon_variable(cj_equipment_item(common,weapon)[0]))base=rand15();
    if(f!=4&&f!=6){st.crit=(u16)(muls16((s16)rand15(),100)>>15)<4;}
    st.dmg=weapon_value(a,t,weapon,charge,0,st.crit,st.crit?rand15():0,base,tier);
    if(st.crit)knockback(a,t);
    if(st.effect_kind==AB_NULL){st.hit=0;st.push=0;st.dmg=0;return;}
    if(f==4&&(w[6]&128)&&(core_status_removable(&st.unit[t].statuses,2)&128))st.effect_kind|=EFFECT_OIL;
    if(f==7)st.self=(u16)(16384|(st.effect_kind==AB_HEAL?32768:0));
    if((f==1||f==2)&&weapon_order(st.abil)&&((u16)(muls16((s16)rand15(),100)>>15)<19)){
        if(f==2){st.self=w[7];st.push=0;}
        else if(w[7]){
            const u8 *r=cj_section(common,CJ_STATUS)+(u16)w[7]*6;u8 i,count=0,pick=0;CoreStatusDelta d;
            memset(&d,0,sizeof d);
            if(r[0]&16){
                memcpy(d.remove,r+1,5);
                if(core_status_filter(&st.unit[t].statuses,&d,st.unit[t].status_flags,0,cj_status_rules(common)))st.self=32768|((u16)w[7]<<8);
            }else{
                for(i=0;i<40;i++)if(r[1+(i>>3)]&(128>>(i&7)))count++;
                if(r[0]&64)pick=(u8)(muls16((s16)rand15(),count)>>15);
                for(i=0;i<40;i++)if(r[1+(i>>3)]&(128>>(i&7))){if(!pick){sid=i;break;}pick--;}
                if(sid!=255){
                    d.add[sid>>3]=128>>(sid&7);
                    if(sid!=SID_CHARM||!(cj_allegiance_immune(common,st.unit[t].job,st.unit[t].eq)&2))
                    if(core_status_filter(&st.unit[t].statuses,&d,st.unit[t].status_flags,0,cj_status_rules(common)))
                        st.self=d.remove[sid>>3]?(32768U|((u16)w[7]<<8)):(u16)(sid+1)<<8;
                }
            }
        }
    }
}

#include "weapon_packet.inc"
// FORMULA14 runs only after PLANNER has returned. Prepare every panel victim
// in stable slot order before any of this hand's effects reach live units.
static void weapon_freeze(void)
{
    WeaponPanel *p=&W->weapon_panel;u8 a=st.atk,t=st.tgt,i,first=255;u16 mask=p->todo,bit;
    if(p->resume){p->resume=0;weapon_packet_load(&p->victim[t]);return;}
    if(!(mask&(mask-1))){p->todo=0;weapon_roll_one(a,t);return;}
    p->actor=a;p->hand=st.hand;
    for(i=0,bit=1;i<NUNIT;i++,bit<<=1)if(mask&bit){
        if(first==255)first=i;
        weapon_roll_one(a,i);weapon_packet_store(&p->victim[i]);
    }
    st.tgt=first;p->todo&=(u16)~((u16)1<<first);
    st.action.left|=WEAPON_PANEL_MORE;weapon_packet_load(&p->victim[first]);
}
