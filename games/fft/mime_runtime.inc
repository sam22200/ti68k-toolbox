#include "mime_rules.h"
// Cold selector: return to the resident before loading an effect owner.
// Source facings -> native world facings (the permutation is its own inverse).
static u16 mime_cell(u8 who)
{
    static const u8 source_face[4]={3,1,2,0};
    const MimeState *m=&W->mime;const Unit *u=&st.unit[who];
    s16 dx=m->dx,dz=m->dz,x,z;u16 lo,hi;u8 h,l,r,rotation;
    rotation=(source_face[u->face&3]-source_face[m->face&3])&3;
    core_mime_rotate(&dx,&dz,rotation);
    x=u->x+dx;z=u->z+dz;
    if(x<0||x>=MAP_W||z<0||z>=MAP_H)return MAP_N;
    lo=node_at(x,z,0);hi=node_at(x,z,1);
    if(lo>=MAP_N||!map_tiles[lo].walk)lo=MAP_N;
    if(hi>=MAP_N||!map_tiles[hi].walk)hi=MAP_N;
    if(lo>=MAP_N)return hi;
    if(hi>=MAP_N)return lo;
    // Original chooses by integer source height, with higher/upper ties.
    h=W->surface[((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u))];
    l=W->surface[((u16)(W->grid_row[z]+x)<<3)];r=W->surface[((u16)(W->grid_row[z]+x)<<3)+4];
    return core_mime_layer(h,l,r,0,jump_ability(m->ability))==2?hi:lo;
}
static u8 mime_next(void)
{
    MimeState *m=&W->mime;Unit *u;const Ability *b;const u8 *r;
    u16 cell;u8 who,t,menu;s16 dx,dz,h;AbId ab;
    if(m->active){
        u=&st.unit[m->who];u->set=m->saved_set;u->eq[EQ_WEAPON]=m->saved_right;u->eq[EQ_SHIELD]=m->saved_left;m->active=0;
    }
    while(m->pending){
        for(who=0;!(m->pending&((u16)1<<who));who++){}
        m->pending&=(u16)~((u16)1<<who);u=&st.unit[who];ab=m->ability;
        if(u->job!=SOURCE_JOB_MIME||(u->team==TEAM_ENEMY)!=m->team||!reaction_live(who)||!unit_can_act(u))continue;
        if(ab!=NO_ABIL){
            if(ab>=NABIL||abilities[ab].kind==255)continue;
            menu=cj_action_menu(common,m->set);r=cj_range(common,ab);
            if((menu==0||menu==3||menu==6)&&(!r||(r[5]&16)))continue;
        }
        cell=mime_cell(who);if(cell>=MAP_N)continue;
        m->saved_set=u->set;m->saved_right=u->eq[EQ_WEAPON];m->saved_left=u->eq[EQ_SHIELD];
        m->active=1;m->who=who;u->set=m->set;u->eq[EQ_WEAPON]=m->right;u->eq[EQ_SHIELD]=m->left;
        if(ab==NO_ABIL){
            t=unit_at_cell((u8)cell);
            if(t&&st.unit[t-1].hp&&ranged_reach(who,UNIT_CELL(u),(u8)cell)){
                st.formula.actor=who;st.formula.target=t-1;return 7;
            }
        }else{
            // Elements selects the imitator's terrain, just as source prepare_attack.
            if(cj_action_menu(common,m->set)==4)ab=magic_heal(who,who,ab,4);
            if(!ab||!can_use(who,ab))goto rejected;
            b=&abilities[ab];r=cj_range(common,ab);
            if(!b->range&&(b->flags&AF_AREA))cell=UNIT_CELL(u);
            dx=(s16)TILE_X[cell]-u->x;dz=(s16)TILE_Z[cell]-u->z;
            h=(s16)map_tiles[cell].stand-map_tiles[UNIT_CELL(u)].stand;
            if(m->follow==MATH_BIND||
               (jump_ability(ab)?jump_reaches(who,UNIT_CELL(u),(u8)cell):
                (b->flags&AF_CHARGE)?ranged_reach(who,UNIT_CELL(u),(u8)cell):
                (!r||!(r[3]&16)||!dx||!dz)&&
                (dx<0?-dx:dx)+(dz<0?-dz:dz)<=(((b->flags&AF_WEAPON)&&b->formula!=99)?WEAPON_REACH(u):ability_range(who,ab))&&
                (b->vert==255||(h<0?-h:h)<=b->vert)&&
                (cell!=UNIT_CELL(u)||(b->flags&AF_SELF)))){
                // Mime executes the prepared effect immediately, including delayed families.
                st.pending[who].state=DEFER_NONE;unit_status_action(u,0);
                if(m->follow==MATH_BIND||(b->flags&AF_AREA)){
                    W->cast.actor=who;W->cast.ability=ab;W->cast.follow=m->follow==MATH_BIND?MATH_BIND:0;
                    W->cast.ref=m->follow==MATH_BIND?m->ref:(u8)cell;W->cast.phase=CAST_BATCH_EMPTY;
                    return CAST_AREA_DISPATCH;
                }
                t=unit_at_cell((u8)cell);
                if(t&&!unit_airborne(&st.unit[t-1])&&(st.unit[t-1].hp||b->kind==AB_REVIVE)){
                    st.formula.actor=who;st.formula.target=t-1;st.formula.ability=ab;return CAST_EFFECT_DISPATCH;
                }
            }
        }
rejected:
        u->set=m->saved_set;u->eq[EQ_WEAPON]=m->saved_right;u->eq[EQ_SHIELD]=m->saved_left;m->active=0;
    }
    return 0;
}
