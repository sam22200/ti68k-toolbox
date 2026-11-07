// Private PC visual candidate. The active engine and calculator images stay intact.
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char hit_flash(unsigned char i);
static unsigned char unit_pose(unsigned char i);
static void draw_number(void);
static void impact_effect(void);
#define FFT_SOURCE_CAMERA
#define FFT_BATTLE
#define FFT_DEFAULT_CHAPTER 1
#define game_render impact_original_render
#include "x/core-area-impact-review/engine.inc"
#undef game_render

static u16 checks,failures;
static u32 digest=17;
static void check(u16 got,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^got;
    if(got!=expected){failures++;printf("line%u: %u expected%u\n",line,got,expected);}
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
#include "area_ui_fixture.inc"

// Glyphs only: outline masks are reconstructed from the same seven source rows.
// This review owns storage; a native version must prove scratch lifetime first.
static struct {
    u32 glyph[FFT_CORE_UNIT_MAX][7],oil[7];
    u8 width[FFT_CORE_UNIT_MAX],oil_width,baseline,enabled,centre;
    char label[FFT_CORE_UNIT_MAX][8];
    s16 camx,camy;
    u16 shown,flashed,oil_shown;
    u8 pulse;
} ir;

static u8 impact_root(void)
{
    return ir.enabled&&st.mode==M_ACT&&W->cast.phase!=CAST_BATCH_EMPTY&&
        st.atk==W->cast.actor&&st.abil==W->cast.ability&&
        (st.action.phase==CTX_ROOT||st.action.phase==CTX_IMPACT);
}
static u8 impact_harm(u8 i)
{
    const CastVictim *v=&W->cast.victim[i];
    u8 kind=v->effect&EFFECT_KIND_MASK,harm=kind==AB_DAMAGE;
    if(!v->hit||kind==AB_NULL)return 0;
    if((s16)v->amount<0)harm=!harm;
    return harm&&v->amount!=0;
}
static u8 hit_flash(u8 i)
{
    u8 t=(u8)(st.act_t-ACT_HIT);
    if(ir.baseline||!impact_root())return original_hit_flash(i);
    if(W->cast.phase!=CAST_BATCH_APPLIED||!(W->cast.mask&((u16)1<<i))||
       !impact_harm(i)||t>=FLASH_FRAMES)return 0;
    ir.flashed|=(u16)1<<i;
    return t<2||t>=10?1:2;
}
static u8 unit_pose(u8 i)
{
    const Unit *u=&st.unit[i];
    if(ir.baseline||!impact_root())return original_unit_pose(i);
    if(i==st.atk&&st.act_t>=ACT_WIND&&st.act_t<ACT_HIT)return P_WIND;
    if(W->cast.phase==CAST_BATCH_APPLIED&&W->cast.mask&((u16)1<<i)&&impact_harm(i)&&
       st.act_t>=ACT_HIT&&st.act_t<ACT_HIT+HIT_FRAMES)return P_HIT;
    if(!u->hp)return P_KO;
    return (u16)(u->hp*5)<=u->max_hp?P_KNEEL:0;
}
static void impact_label(u8 i,char *s)
{
    const CastVictim *v=&W->cast.victim[i];u8 kind=v->effect&EFFECT_KIND_MASK,n=0;
    u8 heal=kind==AB_HEAL;u16 amount=v->amount;
    if(!v->hit||kind==AB_NULL)n=put_s(s,0,"Miss");
    else{
        if((s16)amount<0){heal=!heal;amount=(u16)(0U-amount);}
        if(heal)s[n++]='+';
        n=put_n(s,n,amount);
    }
    s[n]=0;
}
static void impact_cache(void)
{
    u8 i,r;
    for(i=0;i<NUNIT;i++)if(W->cast.mask&((u16)1<<i)){
        impact_label(i,ir.label[i]);make_number(0,ir.label[i]);ir.width[i]=W->num_w[0];
        for(r=0;r<7;r++)ir.glyph[i][r]=W->num[0][0][r];
    }
    make_number(0,"Oil-");ir.oil_width=W->num_w[0];
    for(r=0;r<7;r++)ir.oil[r]=W->num[0][0][r];
    W->num_ok=0;
}
static void impact_glyph(const u32 *g,u8 width,s16 x,s16 y)
{
    u32 mask[7];u8 r;RtSprite s;
    for(r=0;r<7;r++){
        u32 m=g[r]|(r?g[r-1]:0)|(r<6?g[r+1]:0);
        mask[r]=~(m|m<<1|m>>1);
    }
    s.w=32;s.h=7;s.light=s.dark=g;s.mask=mask;
    draw_sprite(x-((width+2)>>1),y,&s);
}
static void draw_number(void)
{
    u8 i,t=(u8)(st.act_t-ACT_HIT);
    if(ir.baseline||!impact_root()||(st.gain&&t>=GAIN_T)){original_draw_number();return;}
    if(W->cast.phase!=CAST_BATCH_APPLIED||t>=NUM_T)return;
    for(i=0;i<NUNIT;i++)if(W->cast.mask&((u16)1<<i)){
        Feet f=unit_feet(st.rot,i);s16 x=f.x-st.camx;
        s16 y=f.y-st.camy-UNIT_FOOT-6-(t<NUM_RISE?t:NUM_RISE);
        impact_glyph(ir.glyph[i],ir.width[i],x,y);ir.shown|=(u16)1<<i;
        if(W->cast.victim[i].hit&&(W->cast.victim[i].effect&EFFECT_OIL)){
            impact_glyph(ir.oil,ir.oil_width,x,y+8);ir.oil_shown|=(u16)1<<i;
        }
    }
}
static void impact_effect(void)
{
    Marker m;u8 t=st.act_t;
    if(ir.baseline||!impact_root()||t<ACT_BLOW||t>=ACT_HIT+FLASH_FRAMES)return;
    // Reuse the selected ground tile, its real slope and existing cover mask.
    // The centre remains a tile even when it has no occupant or no victims.
    marker(st.rot,ir.centre,(t&4)!=0,&m);
    blit24(m.x,m.y,m.rows,m.l,m.d,cursor_mask(&m,ir.centre|((t&4)?0:0x1000)));
    ir.pulse=1;
}
void game_render(void)
{
    State before;CastBatch packet;u32 seed=rng32;
    // Common camera for before/after: this review compares effects, not framing.
    if(ir.enabled){st.camx=ir.camx;st.camy=ir.camy;}
    before=st;packet=W->cast;ir.shown=ir.flashed=ir.oil_shown=ir.pulse=0;
    impact_original_render();
    CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(memcmp(&packet,&W->cast,sizeof packet)==0,1);
    CHECK(rng32==seed,1);
}

static void begin(u8 kind,u8 baseline)
{
    u8 i,r=kind==7?2:kind==5||kind==6?2:kind==1?1:kind==2?2:kind==3?3:kind==4?4:0;
    s16 u,v;
    memset(&ir,0,sizeof ir);ui_fixture(r);ir.baseline=baseline;ir.enabled=1;
    ir.centre=fixture_ui.centre;st.scene_hide=0;st.acted=1;
    for(i=0;i<NUNIT;i++){
        st.reaction[i]=st.support[i]=0;st.unit[i].gexp=st.unit[i].gjp=0;
        memset(st.unit[i].eq,255,sizeof st.unit[i].eq);
    }
    if(kind==1){
        Unit *u=&st.unit[fixture_ui.enemy[0]];
        // Source25% accessory evade reaches the25% base after Defend/Abandon:
        // a real guaranteed evasion, not zero damage caused by zero Faith.
        u->eq[EQ_ACC]=232;st.reaction[fixture_ui.enemy[0]]=REACTION_ABANDON;
        u->statuses.innate[0]|=2;unit_status_refresh(u);
    }
    if(kind==5){
        st.unit[fixture_ui.enemy[0]].hp=1;
        st.unit[fixture_ui.enemy[1]].statuses.inflicted[2]|=128;
        unit_status_refresh(&st.unit[fixture_ui.enemy[1]]);
    }
    if(kind==6){
        for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_ENEMY)
            st.unit[i].hp=st.unit[i].x<MAP_W?1:0;
    }
    if(kind==7){st.reaction[fixture_ui.ally]=REACTION_POTION;st.unit[fixture_ui.ally].brave=100;st.bag[0]=1;}
    memset(&W->cast,0,sizeof W->cast);W->cast.actor=st.act;W->cast.ref=ir.centre;W->cast.ability=16;
    CHECK(batch_owned_prepare(),1);CHECK(W->cast.phase,CAST_BATCH_READY);
    CHECK(st.action.phase,CTX_ROOT);CHECK(st.mode,M_ACT);
    impact_cache();
    if(kind==1){CHECK(W->cast.victim[fixture_ui.enemy[0]].hit,0);CHECK(W->cast.victim[fixture_ui.enemy[1]].hit,1);}
    if(kind==3)CHECK(ir.label[fixture_ui.ally][0],'+');
    if(kind==4)CHECK(W->cast.mask,0);
    view_of(st.rot,TILE_X[ir.centre],TILE_Z[ir.centre],&u,&v);
    ir.camx=scene_x(st.rot,u,v)-80;
    ir.camy=scene_y(u,v)+HH-map_tiles[ir.centre].stand*(HU/2)-65;
    {
        s16 top=32767,bottom=-32767;
        for(i=0;i<NUNIT;i++)if(W->cast.mask&((u16)1<<i)){
            Feet f=unit_feet(st.rot,i);if(f.y<top)top=f.y;if(f.y>bottom)bottom=f.y;
        }
        if(top!=32767){
            if(top-ir.camy<45)ir.camy=top-45;
            if(bottom-ir.camy>76)ir.camy=bottom-76;
        }
    }
    cam_clamp(&ir.camx,&ir.camy);st.camx=ir.camx;st.camy=ir.camy;
}
static const char *output;
static void shot(u8 kind,u8 baseline,u16 frame)
{
    char p[512];if((frame&1)||frame>=(kind==7?160:96))return;
    snprintf(p,sizeof p,"%s/%u-%u-%03u.png",output,kind,baseline,frame);
    CHECK(sw_write_png(p,1),0);
}
static void simulate(u8 kind,u8 baseline,u32 *states,u16 limit)
{
    Unit before[UNIT_CAPACITY];u16 n,hp[UNIT_CAPACITY],mp,cost;u8 i,committed=0,secondary=0;
    begin(kind,baseline);memcpy(before,st.unit,sizeof before);mp=st.unit[st.act].mp;cost=skill_cost(st.act,16);
    for(i=0;i<NUNIT;i++)hp[i]=st.unit[i].hp;
    CHECK(mp,30-cost);
    for(n=0;n<limit;n++){
        u32 h=17;const u8 *p;u16 k;u8 was=impact_root();
        if(st.mode==M_ACT)CHECK(game_update(),1);
        else st.tick++;
        st.camx=ir.camx;st.camy=ir.camy;
        p=(const u8 *)&st;for(k=0;k<sizeof st;k++)h=(h<<1|h>>31)^p[k];
        h^=rng32;
        if(!baseline)CHECK(h==states[n],1);else states[n]=h;
        game_render();shot(kind,baseline,n);
        if(!committed&&W->cast.phase==CAST_BATCH_APPLIED){
            committed=1;CHECK(st.unit[st.act].mp,mp);
            for(i=0;i<NUNIT;i++){
                const CastVictim *e=&W->cast.victim[i];
                CHECK(st.unit[i].hp,(W->cast.mask&((u16)1<<i))&&e->hit?
                    magic_hp(e->effect&EFFECT_KIND_MASK,hp[i],before[i].max_hp,e->amount):hp[i]);
            }
        }
        if(st.action.phase==CTX_SECONDARY||st.action.phase==CTX_APPLIED){
            secondary=1;if(!baseline){CHECK(impact_root(),0);CHECK(ir.shown,0);CHECK(ir.pulse,0);}
        }
        if(!baseline&&impact_root()&&W->cast.phase==CAST_BATCH_APPLIED&&
           st.act_t<ACT_HIT+NUM_T){CHECK(ir.shown,W->cast.mask);}
        if(kind==6&&was&&st.mode==M_ACT)CHECK(st.end,END_NONE);
    }
    CHECK(committed,1);CHECK(st.action.phase,CTX_IDLE);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
    CHECK(st.unit[st.act].mp,mp);if(kind==7){CHECK(secondary,1);CHECK(st.bag[0],0);}
    if(kind==6)CHECK(st.end,END_VICTORY);
}
int main(int argc,char **argv)
{
    u8 kind;u32 states[160];if(argc!=2)return 2;output=argv[1];sw_init(61);
    for(kind=0;kind<8;kind++){simulate(kind,1,states,160);simulate(kind,0,states,160);}
    printf("area_impact_review %08lX failures%u checks%u\n",(unsigned long)digest,failures,checks);
    return failures!=0;
}
