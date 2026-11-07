// All ten Draw Out producers: source math/rolls, frozen effects and inventory.
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-draw-out/oracle/source.h"
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


static u16 prepared_status(const CastVictim *v)
{
    if(!(v->effect&EFFECT_STATUS))return 0;
    return v->amount>>10==1?0x304:v->amount>>10==2?0x110:0x401;
}
static void prepare_draw(AbId ab)
{
    prepare_cleanse(ab);st.unit[U_RAMZA].ma=17;
    st.unit[10].team=abilities[ab].kind==AB_HEAL||abilities[ab].kind==AB_STATUS?TEAM_PLAYER:TEAM_ENEMY;
    battle_save.inv[ab-0x26]=20;
    CHECK(can_use(U_RAMZA,ab),1);
}
static void source_profiles(void)
{
    u16 i,c;u8 k;Unit *a,*t;CastBatch b;u32 expected,seed;
    const u8 ids[]={0,27,22,21,3,2,8,31};
    for(i=0;i<sizeof source_draw_out/sizeof source_draw_out[0];i++){
        const typeof(source_draw_out[0]) *p=&source_draw_out[i];
        prepare_draw(p->ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p->ma;a->faith=0;t->faith=100;a->zodiac=p->az;t->zodiac=p->tz;t->female=p->sex;
        if(p->mime)a->job=SOURCE_JOB_MIME;
        st.support[U_RAMZA]=p->up?SUPPORT_MAGIC_ATTACK_UP:0;st.support[10]=p->defense?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);
        if(p->mode&&p->mode<8)t->statuses.inflicted[ids[p->mode]>>3]|=128>>(ids[p->mode]&7);
        if(p->mode==8){const u8 *r=cj_section(common,CJ_STATUS)+6*cj_range(common,p->ab)[11];for(k=0;k<5;k++)t->statuses.immune[k]=r[k+1];}
        unit_status_refresh(t);
        // The isolated effect entry may inspect a distant radial centre; the
        // real range0 command below is always centred on its caster.
        for(c=0;c<MAP_N;c++)if(map_tiles[c].walk&&abs((s16)TILE_X[c]-t->x)+abs((s16)TILE_Z[c]-t->z)>2){
            unit_place(a,(u8)c);if(cast_targets(U_RAMZA,10,p->ab,1)==((u16)1<<10))break;
        }
        CHECK(c<MAP_N,1);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p->ab;
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(rng32==seed,1);CHECK(t->hp,999);CHECK(t->mp,999);
        rng32=p->seed;for(k=0;k<p->draws;k++)rand15();expected=rng32;rng32=p->seed;b.phase=CAST_BATCH_EMPTY;
        ZB(61);CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);ZE(61);
        CHECK(b.victim[10].hit,p->hit);CHECK(rng32==expected,1);CHECK(st.crit,p->broken);
        if(p->hit){
            CHECK(b.victim[10].effect&EFFECT_KIND_MASK,p->kind);
            CHECK(p->kind==AB_STATUS?b.victim[10].amount:b.victim[10].amount&1023,p->amount);
            CHECK(p->kind==AB_STATUS?b.victim[10].amount:prepared_status(&b.victim[10]),p->packed);
        }else CHECK(b.victim[10].effect,AB_NULL);
        CHECK(t->hp,999);CHECK(t->mp,999);CHECK(battle_save.inv[p->ab-0x26],20);
    }
}
static void geometry_profiles(void)
{
    u16 bytes,i,n,length,c;const u8 *file=rt_file("fdo00",&bytes),*p;
    CHECK(file!=RT_NULL,1);if(!file)return;n=RD16(file,2);CHECK(n,12);p=file+6;
    for(i=0;i<n;i++,p+=length){
        length=RD16(p,0);p+=2;bind_profile(p,i&1);
        CHECK(cast_targets(0,(u8)node_at(p[3],p[4],0),83,0),RD16(p+14+150+78,0));
        st.act=0;st.abil=83;st.mode=M_AREA;memset(&W->area_ui,0,sizeof W->area_ui);
        W->area_ui.centre=(u8)node_at(p[3],p[4],0);W->area_ui.forecast.actor=0;
        W->area_ui.forecast.ability=83;W->area_ui.forecast.ref=W->area_ui.centre;
#ifdef FFT_MODULES
        CHECK(mod_call(MOD_TARGETS,4,0),1);
#else
        CHECK(area_preview(),1);
#endif
        for(c=0;c<MAP_N;c++){
            u16 src=(u16)node_layer((u8)c)*256+W->grid_row[TILE_Z[c]]+TILE_X[c];
            const u8 *marks=p+14+150+80;
            CHECK(!!(W->area_ui.panels[c>>3]&(1U<<(c&7))),!!(marks[src>>3]&(1U<<(src&7))));
        }
    }
    chapter_now=0;reset();
}
static void compound_rolls(void)
{
    u16 i;u8 k;u32 expected;AbId ab;
    for(i=0;i<sizeof source_draw_groups/sizeof source_draw_groups[0];i++){
        const u16 *p=source_draw_groups[i];ab=p[0];prepare_draw(ab);
        st.unit[8]=st.unit[10];unit_place(&st.unit[8],second);
        if(p[2]){st.unit[8].hp=st.unit[10].hp=0;st.unit[8].statuses.inflicted[0]|=32;st.unit[10].statuses.inflicted[0]|=32;unit_status_refresh(&st.unit[8]);unit_status_refresh(&st.unit[10]);}
        rng32=p[1];for(k=0;k<p[3];k++)rand15();expected=rng32;rng32=p[1];
        CHECK(cast_order(U_RAMZA,U_RAMZA,ab,1),1);CHECK(rng32==expected,1);CHECK(st.crit,p[4]);
        CHECK(battle_save.inv[ab-0x26],20);impact();CHECK(battle_save.inv[ab-0x26],20-p[4]);
        impact();CHECK(battle_save.inv[ab-0x26],20-p[4]);
    }
}
static void draw_production(void)
{
    AbId ab;u8 i,k,found;u16 n,amount,hp,mp,packed;u32 seed;
    for(ab=76;ab<=85;ab++){
        Unit *a,*t;prepare_draw(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];a->ma=17;
        if(ab==79)t->hp=500;
        rng32=1;CHECK(skill_cost(U_RAMZA,ab),0);CHECK(abilities[ab].flags&AF_EVADE,0);
        ZB(62);CHECK(cast_order(U_RAMZA,ab==83?second:first,ab,0),1);ZE(62);
        CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(st.push,0);CHECK(a->mp,999);
        CHECK(W->cast.victim[10].hit,1);amount=W->cast.victim[10].amount&1023;
        packed=abilities[ab].kind==AB_STATUS?W->cast.victim[10].amount:prepared_status(&W->cast.victim[10]);
        hp=t->hp;mp=t->mp;seed=rng32;a->ma=1;
        ZB(63);impact();ZE(63);CHECK(rng32==seed,1);CHECK(battle_save.inv[ab-0x26],19);
        if(ab==78){CHECK(t->mp,mp-amount);CHECK(t->hp,hp);}
        else if(ab==79){CHECK(t->hp,hp+amount);CHECK(t->mp,mp);}
        else if(ab!=81&&ab!=84){CHECK(t->hp,hp>amount?hp-amount:0);CHECK(t->mp,mp);}
        if(packed)CHECK(core_status_current(&t->statuses,(u8)(packed>>8))&(u8)packed,(u8)packed);
        hp=t->hp;mp=t->mp;impact();CHECK(t->hp,hp);CHECK(t->mp,mp);CHECK(battle_save.inv[ab-0x26],19);
    }
    // Learned entries stay visible, but only stock in the bag enables them.
    prepare_draw(76);battle_save.inv[38]=0;st.unit[U_RAMZA].eq[EQ_WEAPON]=38;
    CHECK(known(U_RAMZA,0),76);CHECK(can_use(U_RAMZA,76),0);seed=rng32;
    CHECK(cast_order(U_RAMZA,first,76,0),0);CHECK(rng32==seed,1);
    battle_save.inv[38]=1;CHECK(can_use(U_RAMZA,76),1);
    change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,76),1);change(U_RAMZA,SID_FROG);CHECK(can_use(U_RAMZA,76),0);
    prepare_draw(76);st.unit[U_RAMZA].team=TEAM_ENEMY;battle_save.inv[38]=0;
    st.unit[U_RAMZA].level=8;CHECK(can_use(U_RAMZA,76),0);st.unit[U_RAMZA].level=9;CHECK(can_use(U_RAMZA,76),1);
    // Reflect and faithful modifiers never redirect these immediate techniques.
    prepare_draw(76);change(10,SID_REFLECT);CHECK(magic_reflects(10,76),0);
    // Healing reverses on Undead, with no Faith/support/zodiac stage.
    prepare_draw(79);change(10,SID_UNDEAD);CHECK(skill_kind(U_RAMZA,10,79),AB_DAMAGE);
    CHECK(skill_amount(U_RAMZA,10,79,0),204);
    // Actual cached AI admits damage, MP denial, healing and status production.
    for(i=0;i<4;i++){
        ab=i==0?76:i==1?79:i==2?81:78;prepare_draw(ab);st.unit[U_RAMZA].ma=40;
        if(i==1)st.unit[10].hp=1;
        if(i==3){st.unit[10].set=11;st.unit[10].learned=0xffff;}
        st.act=U_RAMZA;st.moved=1;st.acted=0;W->ai_k=0;n=0;
        while(!ai_think()&&++n<2048){}CHECK(n<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);
    }
}
static void draw_flush_save(void)
{
#ifdef __m68k__
    // The TI runtime queues writes until return to AMS. Flush the fixture's
    // private VAT at the same boundary before inspecting persisted bytes.
    if(sv_data){save_write();sv_data=0;}
#endif
}
static void inventory_roundtrip(void)
{
    Save back;u8 team;u32 seed;
    for(team=TEAM_ENEMY;team<=TEAM_GUEST;team++){
        prepare_draw(76);st.unit[U_RAMZA].team=team;st.unit[U_RAMZA].level=9;
        st.unit[10].team=team==TEAM_ENEMY?TEAM_PLAYER:TEAM_ENEMY;
        battle_save.inv[38]=2;rng32=1;CHECK(cast_order(U_RAMZA,first,76,0),1);
        impact();CHECK(battle_save.inv[38],team==TEAM_ENEMY?2:1);
    }
    // Cures/buffs with no eligible status produce no break even if the roll passed.
    prepare_draw(81);st.unit[U_RAMZA].statuses.immune[3]=48;st.unit[10].statuses.immune[3]=48;
    rng32=1;CHECK(cast_order(U_RAMZA,first,81,0),1);CHECK(st.crit,0);impact();CHECK(battle_save.inv[43],20);
    reset();save_campaign();draw_flush_save();CHECK(rt_load("ffsave",&back,sizeof back),1);
    back.inv[38]=3;CHECK(rt_save("ffsave",&back,sizeof back),1);draw_flush_save();
    prepare_draw(76);battle_save.inv[38]=3;rng32=1;
    CHECK(cast_order(U_RAMZA,first,76,0),1);CHECK(battle_save.inv[38],3);
    seed=rng32;impact();CHECK(rng32==seed,1);CHECK(battle_save.inv[38],2);
    CHECK(rt_load("ffsave",&back,sizeof back),1);CHECK(back.inv[38],3);
    save_campaign();draw_flush_save();CHECK(rt_load("ffsave",&back,sizeof back),1);CHECK(back.inv[38],2);
    save_campaign();draw_flush_save();CHECK(rt_load("ffsave",&back,sizeof back),1);CHECK(back.inv[38],2);
    // Retry before saving discards live losses; victory/reload retains them.
    battle_save.inv[38]=1;reset();CHECK(battle_save.inv[38],2);
}
static void draw_trials(void)
{
#ifndef DRAW_OUT_RUNTIME_ONLY
    source_profiles();
#endif
#ifndef DRAW_OUT_MATH_ONLY
    f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
    geometry_profiles();compound_rolls();draw_production();inventory_roundtrip();free(f);
#endif
}
#ifndef DRAW_OUT_LABEL
#define DRAW_OUT_LABEL "draw_out"
#endif
#ifndef __m68k__
int main(void){sw_init(61);draw_trials();printf(DRAW_OUT_LABEL " %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();draw_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
