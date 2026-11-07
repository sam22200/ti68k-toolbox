// Native two-arc obstruction. Fixed64 samples replace the PS1's trigonometric
// impulse stepping; actor collision boxes and source column endpoints are kept.
#include "arc_rules.h"
typedef struct {s16 x,z,feet;u8 body,id;} ArcBody;
typedef struct {s16 x,z,dx,dz,start,rise,curve;u8 target,count;ArcBody body[UNIT_CAPACITY];} ArcFlight;
static u8 arc_trace(const ArcFlight *f)
{
    s16 x=f->x<<6,z=f->z<<6,py,px,pz;u8 step,k,c;CoreArcStep path;
    core_arc_begin(&path,f->start,f->rise,f->curve);
    for(step=1;step<=64;step++){
        x+=f->dx;z+=f->dz;px=x>>6;pz=z>>6;py=core_arc_next(&path);
        c=TILE_COL[px]+TILE_ROW[pz];
        if(W->surface){
            if(surface_hit_layers(W->surface+((u16)c<<3),py,(W->surface_flags&SURFACE_HIT_UPPER)?2:1))return 255;
        }else{
            u16 cell=c;if(W->grid)cell=(W->grid->present[c>>3]&(1U<<(c&7)))?W->grid->lookup[c]:MAP_N;
            if(cell<MAP_N&&py>0&&(u16)py<=W->top[cell])return 255;
        }
        for(k=0;k<f->count;k++){
            const ArcBody *b=&f->body[k];
            if(py>b->feet&&py<b->feet+b->body&&px>b->x-8&&px<b->x+8&&pz>b->z-8&&pz<b->z+8)return b->id;
        }
    }
    return f->target;
}
static u8 arc_target(u8 a,u8 t)
{
    const Unit *u=&st.unit[a],*v=&st.unit[t];
    s16 dx=((s16)v->x-u->x)*28,dz=((s16)v->z-u->z)*28;
    s16 body=(core_status_current(&u->statuses,2)&6)?24:36;
    s16 start=W->hs[UNIT_CELL(u)]*6+body*2/3,feet=W->hs[UNIT_CELL(v)]*6;
    s16 aim=(core_status_current(&v->statuses,2)&6)?12:18,rise,curve;
    u8 low,high,k;ArcFlight f;
    if(!v->hp)aim=6;
    rise=feet+aim-start;curve=core_arc_curve(dx,dz,rise,0);
    if(curve<0||feet<rise){rise=feet-start;curve=core_arc_curve(dx,dz,rise,0);}
    if(curve<0||feet<rise)return 255;
    f.x=u->x*28+14;f.z=u->z*28+14;f.dx=dx;f.dz=dz;f.start=start;f.rise=rise;f.curve=curve;f.target=t;f.count=0;
    for(k=0;k<NUNIT;k++)if(k!=a&&k!=t&&unit_on_field(&st.unit[k])&&!unit_airborne(&st.unit[k])){
        const Unit *b=&st.unit[k];ArcBody *box=&f.body[f.count++];
        box->x=b->x*28+14;box->z=b->z*28+14;box->feet=W->hs[UNIT_CELL(b)]*6;box->id=k;
        box->body=!b->hp?12:(core_status_current(&b->statuses,2)&6)?24:36;
    }
    low=arc_trace(&f);if(low==t)return t;
    f.curve=core_arc_curve(dx,dz,rise,1);
    high=arc_trace(&f);if(high==t)return t;
    return core_arc_recipient(low,high,low!=255&&unit_original_enemy(&st.unit[low])!=(u->team==TEAM_ENEMY),
        high!=255&&unit_original_enemy(&st.unit[high])!=(u->team==TEAM_ENEMY));
}
// One eligible projectile pair/update; cursor is origin*16+slot. Direct and arc
// weapons accept only their intended recipient, never an intercepted shot.
#include "weapon_reflect_origin.inc"
static u8 projectile_cache(u8 flags)
{
    u8 a=st.act,home=UNIT_CELL(&st.unit[a]),scans=64;u16 c;u8 t;
    while(W->ai_melee_at<(MAP_N<<4)&&scans--){
        c=W->ai_melee_at>>4;t=W->ai_melee_at++&15;
        if(!t){W->ai_melee_mask[c]=0;memset(W->ai_reflect[c],0,7);}
        if((!W->ai_m[c]&&c!=home)||t>=NUNIT||t==a||!st.unit[t].hp||!unit_on_field(&st.unit[t])||
            !ranged_reach(a,(u8)c,UNIT_CELL(&st.unit[t])))continue;
        unit_place(&st.unit[a],(u8)c);
        weapon_reflect_pair(a,t,(u8)c);
        if(flags&16?arc_target(a,t)==t:direct_target(a,t)==t)W->ai_melee_mask[c]|=(u16)1<<t;
        unit_place(&st.unit[a],home);break;
    }
    if(W->ai_melee_at==(MAP_N<<4))W->ai_weapon_left&=(u16)~512;
    return W->ai_weapon_left==0;
}
