// Immediate effects, including single-recipient Reflect at candidate origins.
// Ranking remains a bounded native heuristic, without future-turn simulation.
#include "area_ai_reduce.inc"
#include "status_ai.h"
static u8 area_ai_step(void)
{
#if MOD_CORE
    if(W->ai_k==4||W->ai_k==8||st.unit[st.act].set==21)return area_ai_prepare();
#endif
    u8 a=st.act,t,home=UNIT_CELL(&st.unit[a]),scans=64,queries=3;
    u8 layers=(W->surface_flags&SURFACE_GRID_UPPER)?2:1;
    AreaAi *q=&W->area_ai;
    const u8 *raw;u16 c,bit;Unit *actor=&st.unit[a];
    raw=cj_range(common,W->ab);
    if(W->ai_k==5){
        for(;q->next<MAP_N&&scans;scans--,q->next++){
            c=q->next;if(!map_tiles[c].walk)continue;
            for(t=0,bit=1;t<NUNIT;t++,bit<<=1)if(q->useful&bit){
                s16 dx=(s16)TILE_X[c]-st.unit[t].x,dz=(s16)TILE_Z[c]-st.unit[t].z;
                if((q->useful&((u16)1<<a))||raw[1]==255||(dx<0?-dx:dx)+(dz<0?-dz:dz)<=raw[1]){
                    q->open[c>>3]|=(u8)(1U<<(c&7));break;
                }
            }
        }
        if(q->next<MAP_N)return 0;
        q->next=0;W->ai_k=6;return 0;
    }
    while(W->ab_k<W->ai_no&&scans&&queries){
        u8 o=W->ai_ord[W->ab_k];unit_place(actor,o);
        // Setup already checked command/status/weapon eligibility. Only the
        // source surface changes between candidate origins: use the pure lookup.
        if(abilities[W->ab].formula==0x24&&magic_heal(a,a,W->ab,4)!=W->ab){W->ab_k++;q->origin=256;scans--;continue;}
        if(q->origin!=o){
            q->origin=o;q->x=q->z=q->layer=0;
            for(t=0,bit=1;t<NUNIT;t++,bit<<=1)if(q->reflect&bit){
                u8 receiver=reflect_recipient(t,actor->x,actor->z);
                q->useful&=(u16)~bit;q->gain[t]=0;q->hit[t]=0;
                if(receiver!=255){
                    q->gain[t]=q->returned_gain[receiver];q->hit[t]=q->returned_hit[receiver];
                    if(q->returned_useful&((u16)1<<receiver))q->useful|=bit;
                }
            }
            q->self_kind=(q->useful&((u16)1<<a))?1:(q->hit[a]?2:0);
        }
        while(q->layer<layers&&scans&&queries){
            u16 mask,counted;AiAct candidate;u8 productive=0;
            // Source order is layer/row/column, independent of compact node IDs.
            c=node_at(q->x,q->z,q->layer);scans--;
            if(++q->x==MAP_W){q->x=0;if(++q->z==MAP_H){q->z=0;q->layer++;}}
            if(c>=MAP_N)continue;
            if(!(q->open[c>>3]&(1U<<(c&7)))||!ai_reaches(a,W->ab,o,(u8)c))continue;
            mask=(u16)cast_targets(a,(u8)c,W->ab,0);queries--;
            if((s16)mask<0)continue;
            if(area_ai_reduce(q,mask,NUNIT,&candidate,&counted)){
                candidate.pri+=W->ai_gain[o];
                candidate.ab=W->ab;candidate.o=o;candidate.t=(u8)c;candidate.f=0;
                productive=candidate.pri>W->ai_base;
                if(W->ab_best.pri==AI_BAD||ai_better(candidate.pri,candidate.hit,o,&W->ab_best))W->ab_best=candidate;
            }
            // First safe origin consumes this centre; adverse self-hit centres
            // remain available until the caster can escape their footprint.
            if(!(raw[4]&4)&&(!(mask&q->reflect)||productive)&&((q->self_kind==1&&(counted&((u16)1<<a)))||
               (q->self_kind!=1&&!(counted&((u16)1<<a)))))q->open[c>>3]&=(u8)~(1U<<(c&7));
            // A whole-map footprint is identical for every valid centre at this
            // origin. Keep its first legal centre and avoid a quadratic search.
            if(raw[1]==255&&!(raw[4]&4)){q->layer=layers;break;}
        }
        if(q->layer>=layers){W->ab_k++;q->origin=256;}
    }
    unit_place(actor,home);
    if(W->ab_k<W->ai_no)return 0;
    if(W->ab_best.pri!=AI_BAD)ai_insert(&W->ab_best);
    return 1;
}
