// Shared line footprint, original scalar rules and owned production commands.
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-line/oracle/source.h"
#include "../area_geometry.h"
typedef struct {
    CoreGrid grid;CoreTilePosition pos[256];Tile tiles[256];
    u8 row[32],lookup[512],present[64],x[256],z[256],surface[8+8*256];
} AreaFixture;
static AreaFixture *f;
static void bind_profile(const u8 *p,u8 reverse)
{
    u8 w=p[0],h=p[1],layer;u16 area=(u16)w*h,i,count=0,ordinal=0;
    const u8 *terrain=p+14;u8 *surface;
    memset(f,0,sizeof *f);
    for(i=0;i<2*area;i++)if(!terrain[3*i+2])count++;
    CHECK(count>0&&count<=256,1);if(count==256)reverse=0;
    f->grid.count=count;f->grid.width=w;f->grid.height=h;f->grid.position=f->pos;
    f->grid.row=f->row;f->grid.lookup=f->lookup;f->grid.present=f->present;
    for(i=0;i<h;i++)f->row[i]=(u8)(i*w);
    memcpy(f->surface,"FGS1",4);f->surface[4]=w;f->surface[5]=h;f->surface[6]=area>>8;f->surface[7]=area;
    for(layer=0;layer<2;layer++)for(i=0;i<area;i++){
        const u8 *t=terrain+3*(i+(u16)layer*area);u8 x=i%w,z=i/w;
        surface=f->surface+8+(i<<3)+4*layer;
        surface[0]=t[0];surface[1]=t[1];surface[3]=t[2]&1?128:0;
        if(!t[2]){
            u16 id=reverse?count-1-ordinal:ordinal,address=i+((u16)layer<<8);ordinal++;
            f->lookup[address]=(u8)id;f->present[address>>3]|=1U<<(address&7);
            f->pos[id].x=f->x[id]=x;f->z[id]=z;f->pos[id].z_layer=z|(layer<<5);
            f->tiles[id].walk=1;f->tiles[id].stand=(u8)core_area_height(surface);
        }
    }
    CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),1);
    CHECK(battle_bind_surface(f->surface,8+(area<<3)),1);
    memset(&st,0,sizeof st);
    for(i=0;i<NUNIT;i++){
        const u8 *u=terrain+6*area+6*i;Unit *t=&st.unit[i];u16 cell=node_at(u[0],u[1],u[2]);
        t->x=t->z=255;t->dc=DC_GONE;t->hp=t->max_hp=t->mp=t->max_mp=100;t->team=u[3];
        if(u[4]!=255)t->statuses.inflicted[u[4]>>3]|=0x80>>(u[4]&7);
        if(u[4]==SID_DEAD)t->hp=0;
        if(!u[5]&&cell<MAP_N){unit_place(t,(u8)cell);t->dc=3;}
        unit_status_refresh(t);
    }
}

static void geometry_profiles(void)
{
    u16 bytes,n,i,length,area,c;const u8 *file=rt_file("flg00",&bytes),*p;
    CHECK(file!=RT_NULL,1);if(!file)return;n=RD16(file,2);CHECK(n,27);p=file+6;
    for(i=0;i<n;i++,p+=length){
        length=RD16(p,0);p+=2;bind_profile(p,i&1);area=(u16)p[0]*p[1];
        ZB(61);CHECK(cast_targets(0,(u8)node_at(p[3],p[4],p[5]),103,0),RD16(p+14+6*area+78,0));ZE(61);
        st.act=0;st.abil=103;st.mode=M_AREA;memset(&W->area_ui,0,sizeof W->area_ui);
        W->area_ui.centre=(u8)node_at(p[3],p[4],p[5]);
        W->area_ui.forecast.actor=0;W->area_ui.forecast.ability=103;
        W->area_ui.forecast.ref=W->area_ui.centre;
#ifdef FFT_MODULES
        CHECK(mod_call(MOD_TARGETS,4,0),1);
#else
        CHECK(area_preview(),1);
#endif
        for(c=0;c<MAP_N;c++){
            u16 src=(u16)node_layer((u8)c)*256+W->grid_row[TILE_Z[c]]+TILE_X[c];
            const u8 *marks=p+14+6*area+80;
            CHECK(!!(W->area_ui.panels[c>>3]&(1U<<(c&7))),!!(marks[src>>3]&(1U<<(src&7))));
        }
        CHECK(W->area_ui.follow_id,255);

    }
    chapter_now=0;reset();
}
static void prepare_line(void)
{
    prepare_cleanse(103);st.unit[10].team=TEAM_ENEMY;
    st.unit[U_RAMZA].pa=23;st.unit[10].hp=st.unit[10].max_hp=999;
    // Two equal-distance victims are used by the production multi-hit fixture.
    st.unit[8]=st.unit[10];unit_place(&st.unit[8],second);st.unit[8].hp=999;
}
static void formula_profiles(void)
{
    u16 i;u8 k;CastBatch b;Unit *a,*t;
    const u8 ids[]={0,26,35,4,17,16,18,5,2,8,31};
    for(i=0;i<sizeof source_line/sizeof source_line[0];i++){
        const typeof(source_line[0]) *p=&source_line[i];u32 expected,seed;
        prepare_line();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=p->pa;a->faith=0;t->faith=100;a->zodiac=p->az;t->zodiac=p->tz;t->female=p->sex;
        st.support[U_RAMZA]=p->up==1?SUPPORT_ATTACK_UP:p->up==2?SUPPORT_MARTIAL_ARTS:p->up==3?SUPPORT_MAGIC_ATTACK_UP:0;
        st.support[10]=p->defense==1?SUPPORT_DEFENSE_UP:p->defense==2?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->mode)t->statuses.inflicted[ids[p->mode]>>3]|=128>>(ids[p->mode]&7);
        unit_status_refresh(t);
        if(p->affinity)st.affinity[p->affinity==5?U_RAMZA:10][p->affinity==5?4:p->affinity-1]=8;
        st.weather=p->weather;
        // Compute exact expected state from the original per-target draw count.
        // Direct formula fixtures isolate one present victim; there is no fall.
        st.unit[8].dc=DC_GONE;st.unit[8].x=255;
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=second;b.ability=103;
        CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);
        CHECK(b.victim[10].hit,p->hit);CHECK(b.victim[10].amount,p->hit?p->amount:0);
        CHECK(rng32==expected,1);CHECK(t->hp,999);CHECK(t->mp,999);
        if(p->hit)CHECK(b.victim[10].effect&EFFECT_KIND_MASK,p->kind);

    }
}
static void line_production(void)
{
    u16 hp,amount,c;u8 k,found=0;u32 seed;
    prepare_line();CHECK(abilities[103].kind,AB_DAMAGE);CHECK(abilities[103].flags,AF_CAST|AF_AREA);
    CHECK(abilities[103].element,8);CHECK(skill_cost(U_RAMZA,103),0);
    CHECK(cast_targets(U_RAMZA,third,103,0),((u16)1<<8)|((u16)1<<10));
    // No self, cardinal selection only. Legal selector height is unrestricted;
    // the footprint still uses the origin's two-level source tolerance.
    CHECK(cast_order(U_RAMZA,first,103,0),0);
    for(c=0;c<MAP_N;c++)if(TILE_X[c]!=st.unit[U_RAMZA].x&&TILE_Z[c]!=st.unit[U_RAMZA].z&&map_tiles[c].walk)break;
    if(c<MAP_N)CHECK(cast_order(U_RAMZA,(u8)c,103,0),0);
    seed=rng32;skill_range(U_RAMZA,103);CHECK(rng32==seed,1);
    for(c=0;c<MAP_N;c++)if(TILE_X[c]!=st.unit[U_RAMZA].x&&TILE_Z[c]!=st.unit[U_RAMZA].z)CHECK(st.reach[c],255);
    rng32=10000;ZB(62);CHECK(cast_order(U_RAMZA,third,103,0),1);ZE(62);
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(W->cast.mask,((u16)1<<8)|((u16)1<<10));
    CHECK(st.push,0);hp=st.unit[10].hp;amount=W->cast.victim[10].amount;CHECK(amount>0,1);
    st.unit[U_RAMZA].pa=1;seed=rng32;ZB(63);impact();ZE(63);
    CHECK(st.unit[10].hp,hp-amount);CHECK(rng32==seed,1);CHECK(st.unit[U_RAMZA].mp,999);
    hp=st.unit[10].hp;impact();CHECK(st.unit[10].hp,hp);
    // A single critical can push, with its destination fixed at preparation.
    prepare_line();st.unit[8].dc=DC_GONE;st.unit[8].x=255;rng32=9;
    CHECK(cast_order(U_RAMZA,second,103,0),1);CHECK(st.crit,1);CHECK(st.push,1);
    CHECK(st.push_cell,third);hp=st.unit[10].hp;amount=W->cast.victim[10].amount;
    impact();CHECK(UNIT_CELL(&st.unit[10]),third);CHECK(st.unit[10].hp,hp-amount);
    // The nearer high-slot victim receives the first source draws, even if
    // the farther actor has a lower native ID. Both formulas freeze before HP.
    prepare_line();unit_place(&st.unit[8],third);rng32=9;
    CHECK(cast_order(U_RAMZA,third,103,0),1);
    CHECK(W->cast.victim[10].amount,286);CHECK(W->cast.victim[8].amount,253);
    CHECK(st.push,0);
    prepare_line();change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,103),1);
    change(U_RAMZA,SID_FROG);CHECK(can_use(U_RAMZA,103),0);
    prepare_line();st.unit[10].statuses.inflicted[2]|=64;unit_status_refresh(&st.unit[10]);rng32=9;CHECK(cast_order(U_RAMZA,third,103,0),1);
    CHECK(W->cast.victim[10].hit,0);impact();CHECK(st.unit[10].hp,999);
    prepare_line();st.unit[10].hp=200;st.affinity[10][0]=8;rng32=10000;
    CHECK(cast_order(U_RAMZA,third,103,0),1);CHECK(W->cast.victim[10].effect,AB_HEAL);impact();CHECK(st.unit[10].hp>200,1);
    prepare_line();st.unit[U_RAMZA].pa=40;st.act=U_RAMZA;st.moved=1;st.acted=0;W->ai_k=0;
    for(c=0;c<2048&&!ai_think();c++){}CHECK(c<2048,1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==103)found=1;
    CHECK(found,1);
}
static void line_trials(void)
{
    f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
    geometry_profiles();formula_profiles();line_production();free(f);
}
#ifndef __m68k__
int main(void){sw_init(61);line_trials();printf("line %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();line_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
