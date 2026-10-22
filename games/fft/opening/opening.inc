// Native opening adapter for the common tactical engine: the beats played on the battle map
// (gate, banter, aftermath) and the battle. The title and chapel before it and the chapter
// card after it are the scene player's (campaign/scene.c, ffs0/ffs1).
#include "data/story.h"
typedef char OpeningDiskLayout[(sizeof(UnitDef)==24 && sizeof(Item)==23 && sizeof(Job)==14 && sizeof(Ability)==24 && sizeof(SkillSet)==18)?1:-1];
enum { OPEN_PLAY, OPEN_TITLE, OPEN_CHAPEL, OPEN_GATE, OPEN_AFTER, OPEN_DONE, OPEN_BANTER };
static const u8 *opening_story, *pgfx;   // pgfx: the speakers' portraits (ffcp)
static void opening_actor(u8 gfx,u8 slot,s16 x,s16 y,u8 face,u16 logical);
static void opening_stage(u8 stage);
#include "../beats.inc"

enum { DEFER_NONE, DEFER_CHARGING, DEFER_AIRBORNE, DEFER_PERFORMING };
static u8 opening_pending(void)
{
    u8 i;
    for (i=0;i<NUNIT;i++) if (st.pending[i].state) {
        const Unit *u=&st.unit[i];
        if (!u->hp || u->z*MAP_W+u->x!=st.pending[i].origin || u->eq[EQ_WEAPON]!=st.pending[i].weapon) {
            st.pending[i].state=DEFER_NONE;continue;
        }
        if (!st.pending[i].ticks) return i;
    }
    return 255;
}

static void opening_resolve(void)
{
    u8 a=opening_pending(),t,ab=st.pending[a].ability,c=st.pending[a].tile;
    st.pending[a].state=DEFER_NONE;
    t=unit_at(c%MAP_W,c/MAP_W);
    if (!t || !st.unit[t-1].hp) { turn_start(); return; }
    st.open_resolving=1;st.act=a;st.ai=AI_OFF;
    st.cx=c%MAP_W;st.cz=c/MAP_W;
    skill(a,t-1,ab);
}


static void opening_units(void)
{
    u8 i,k;
    for (i=0;i<NUNIT;i++) {
        const UnitDef *d=&unit_defs[i];Unit *u=&st.unit[i];
        memset(u,0,sizeof *u);
        u->x=d->x;u->z=d->z;u->gfx=d->gfx;u->job=d->job;u->team=d->team;u->face=d->face;
        u->level=u->lv0=d->level;u->brave=d->brave;u->faith=d->faith;u->female=d->type==1;
        u->zodiac=opening_zodiac[i];u->set=d->set;u->learned=d->learned;u->dc=3;
        for(k=0;k<5;k++){u->eq[k]=d->eq[k][0];u->raw[k]=RD32(igfx+OPENING_RAW,(u16)(i*10+2*k));}
        unit_stats(u);
    }
}

static u8 opening_attack_range(u8 a,u8 o,u8 c)
{
    const Unit *u=&st.unit[a];
    u8 range=u->eq[EQ_WEAPON]==255 ? 1:opening_range[u->eq[EQ_WEAPON]];
    s16 dx=(s16)TILE_X[c]-TILE_X[o],dz=(s16)TILE_Z[c]-TILE_Z[o],h=(s16)map_tiles[c].stand-map_tiles[o].stand;
    u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));
    if(!map_tiles[c].walk)return 0;
    if(range==1)return d==1 && h>=-6 && h<=5;
    // FFT bow range grows downhill. Minimum range three; terrain collision is
    // checked separately during targeting (the bounded native projectile rule).
    range=(u8)(range+(h<0?(-h)>>2:0));
    return d>=3 && d<=range;
}

static u8 opening_hit(u8 a,u8 t)
{
    const Unit *u=&st.unit[t];u8 side=attack_side(a,t),base=u->status&ST_DEFEND?50:100;
    u16 hit=100;u8 ev=side==SIDE_FRONT?jobs[u->job].cev:0;
    if(ev>base)ev=base;
    hit=(u16)((base-ev)*100/base);
    if(side!=SIDE_BACK&&u->eq[EQ_SHIELD]!=255){
        ev=opening_evade[u->eq[EQ_SHIELD]];if(ev>base)ev=base;
        hit=(u16)(hit*(base-ev)/base);
    }
    return (u8)hit;
}

static void opening_effect(void)
{
    u8 t=st.act_t;
    if(st.mode!=M_ACT)return;
    if((st.abil==NO_ABIL||(abilities[st.abil].flags&AF_CHARGE))&&st.unit[st.atk].eq[EQ_WEAPON]!=255&&opening_range[st.unit[st.atk].eq[EQ_WEAPON]]>1){
        if(t>=ACT_HIT-8&&t<ACT_HIT){
            Feet a=unit_feet(st.rot,st.atk),b=unit_feet(st.rot,st.tgt);u8 k=t-(ACT_HIT-8);
            s16 x=a.x+((b.x-a.x)*k>>3)-st.camx,y=a.y+((b.y-a.y)*k>>3)-st.camy-12;
            static const u8 arc[8]={0,3,5,6,6,5,3,0};y-=arc[k];
            draw_rect(x-2,y-1,6,3,C_WHITE);draw_rect(x-1,y,4,1,C_BLACK);
        }
    }else if(st.abil!=NO_ABIL&&st.abil!=ABIL_DEFEND&&(abilities[st.abil].flags&AF_WEAPON)&&t>=ACT_HIT-5&&t<ACT_HIT+6){
        Feet b=unit_feet(st.rot,st.tgt);s16 x=b.x-st.camx,y=b.y-st.camy;
        u8 h=(u8)(6+(t%5)*4);draw_rect(x-3,y-h,6,h,C_WHITE);draw_rect(x-1,y-h,2,h,C_DGRAY);
    }
}

static void opening_damage(void)
{
    const Ability *b;
    Unit *a=&st.unit[st.atk],*t=&st.unit[st.tgt];
    if(st.abil==NO_ABIL)return;
    b=&abilities[st.abil];
    if(b->flags&AF_DRAIN){u16 n=st.dmg; if(n>a->max_hp-a->hp)n=a->max_hp-a->hp; a->hp+=n;}
    if(b->flags&AF_AREA){
        u8 i;
        // Cross-shaped Stasis Sword splash, including allies (no team filtering).
        for(i=0;i<NUNIT;i++){
            Unit *v=&st.unit[i];s16 dx=(s16)v->x-t->x,dz=(s16)v->z-t->z;
            u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));u16 n;
            if(i==st.tgt||!v->hp||d!=1)continue;
            n=skill_amount(st.atk,i,st.abil,0);v->hp=n>=v->hp?0:v->hp-n;
            if(!v->hp)v->kos++;
        }
    }
}


static void opening_battle(u8 direct)
{
    u8 i;
    st.open_stage=0;st.mode=M_BROWSE;st.end=0;st.banner=0;st.ai=0;
    scene_rot=vbuf_rot=0xFF;W->bg_ok=W->frz=W->hold=0;
    for(i=0;i<NUNIT;i++){W->cover[i].rot=0xFF;W->feet_k[i]=0;}
    turn_start();
    if(direct){
        while(st.act!=U_RAMZA)turn_end(0,0);
        st.ai=AI_OFF;st.mode=M_MENU;
    }else bn_start(BN_START);
    {u8 saved=W->actor_budget;W->actor_budget=255;for(i=0;i<NUNIT;i++)unit_frame(st.rot,i);W->actor_budget=saved;}
    cam_target(&st.camx,&st.camy);hl_seq++;
}

static void opening_start(u16 n)
{
#if defined(FFT_MEASURE) && defined(RT_CYCLES)
    frame_stall=1; // restart/scene-bank preparation, not an ordinary animation frame
#endif
    memset(&st,0,sizeof st);if(!W)return;
    st.want=st.prot=st.ai_view=255;rt_seed=97;rand_seed(97);
    opening_story=rt_file("ffost",RT_NULL);
    pgfx=rt_file("ffcp",RT_NULL);                    // (none: name-only boxes)
    if(opening_story)opening_story+=2;
    opening_units();memcpy(st.bag,bag_init,NBAG);
    actor_reset();W->dialog_key=0xFFFF;
    W->bg_ok=W->hold=W->frz=W->status_i=0;W->pan_ok[0]=W->pan_ok[1]=0;
    W->win_k[0]=0;W->pan_k[0][0]=W->pan_k[1][0]=W->fc_key=0;
    scene_rot=vbuf_rot=0xFF;
    st.rot=2;st.act=st.sel=U_RAMZA;st.cx=st.unit[0].x;st.cz=st.unit[0].z;st.mode=M_BROWSE;
    cam_target(&st.camx,&st.camy);
    if(n==5){st.open_stage=OPEN_DONE;return;} // AMS chapter-loader injection door: over at once
    if(n==1||n==2||n==3){
        opening_battle(n==1);
        if(n==2)st.ai_all=1;
        if(n==3)opening_stage(OPEN_AFTER);
    }else{opening_stage(OPEN_GATE);st.open_fade=9;}   // the end of the scene player's fade
}


static void opening_stage(u8 stage)
{
    st.open_stage=stage;st.open_clock=0;st.open_seen=0;
    memset(W->actor_key,255,sizeof W->actor_key);
    W->dialog_key=0xFFFF;W->bg_ok=W->hold=W->frz=0;
    if(stage==OPEN_GATE){opening_find(4);scene_rot=vbuf_rot=255;}
    else if(stage==OPEN_BANTER)opening_find(5);
    else if(stage==OPEN_PLAY)opening_battle(0);
    else if(stage==OPEN_AFTER){
        opening_find(6);st.mode=M_BROWSE;st.banner=st.end=0;st.rot=0;
        st.camx=108;st.camy=68;scene_rot=vbuf_rot=255;
        st.morph_mask=0;memset(st.pending,0,sizeof st.pending);
    }
}

static void opening_transition(u8 stage)
{
    st.open_next=stage;st.open_fade=1;
}


static void opening_advance(void)
{
    u8 event=opening_beat()[0];
    if(st.open_beat+1<OPENING_BEATS){
        st.open_beat++;
        if(opening_beat()[0]==event){opening_dialog_reset();return;}
    }
    if(st.open_stage==OPEN_GATE){opening_stage(OPEN_BANTER);}
    else if(st.open_stage==OPEN_BANTER)opening_transition(OPEN_PLAY);
    else opening_transition(OPEN_DONE);
}

static u8 opening_update(void)
{
    if(!W||!opening_story)return !input_pressed(K_ESC);
    // A completed prologue hands over to the scene player's chapter card, never a replay.
    if(st.open_stage==OPEN_DONE){st.open_handoff=1;return 0;}
    st.tick++;if(st.open_clock<250)st.open_clock++;
    if(!st.open_stage&&!st.open_fade)opening_transition(OPEN_AFTER);
    if(st.open_fade){
        if(++st.open_fade==9){opening_stage(st.open_next);st.open_fade=9;}
        else if(st.open_fade>=17)st.open_fade=0;
        return !input_pressed(K_ESC);
    }
    if(st.open_stage==OPEN_AFTER)opening_positions();
    if(input_pressed(K_CATALOG)){st.open_white^=1;W->dialog_key=0xFFFF;}
    if(input_pressed(K_B)){
        if(st.open_stage==OPEN_GATE||st.open_stage==OPEN_BANTER)opening_transition(OPEN_PLAY);
        else if(st.open_stage==OPEN_AFTER)opening_transition(OPEN_DONE);
        return 1;
    }
    if(opening_beat()[2]){
        if(st.open_clock>=opening_beat()[2])opening_advance();
        return !input_pressed(K_ESC);
    }
    if(W->dialog_key!=st.open_beat)opening_layout();
    if(st.open_reveal<W->dialog_total){
        st.open_reveal+=3;
        if(st.open_reveal>W->dialog_total)st.open_reveal=W->dialog_total;
        if(input_pressed(K_A|K_ENTER))st.open_reveal=W->dialog_total;
    }else if(input_pressed(K_A|K_ENTER)){
        const char *s=opening_text();
        if(W->dialog_drawn!=0xFFFF && W->dialog_drawn<W->dialog_total)return 1;
        if(s[W->dialog_next]){st.open_offset+=W->dialog_next;st.open_reveal=0;W->dialog_key=0xFFFF;return 1;}
        opening_advance();
    }
    return !input_pressed(K_ESC);
}




static void opening_render(void)
{
    u8 i;
    if(!opening_story){draw_rect(0,0,160,100,C_WHITE);ui_text(3,40,"Missing data or memory",C_BLACK,160);return;}
    if(st.open_stage==OPEN_DONE)draw_rect(0,0,160,100,C_BLACK);   // the fade's last, black frame
    else{
        const u8 *b=opening_beat(),*a=b+4;
        ZB(21);
        ensure_scene(st.rot);copy_view();
        if(st.open_stage==OPEN_AFTER){
            for(i=0;i<b[1];i++,a+=8){
                u8 slot=a[0]>>4;
                opening_actor(a[0]&15,slot,W->feet[slot].x,W->feet[slot].y,a[1],RD16(a+6,0));
            }
        }else{
            // Outside dialogue: actors are static, without battle timers/markers.
            for(i=0;i<NUNIT;i++)if(st.unit[i].hp){
                Feet f=unit_feet(st.rot,i);u8 dir=unit_direction(st.rot,i);
                W->feet_k[i]=0;
                opening_actor(st.unit[i].gfx,i,f.x,f.y,(f.depth<<2)|dir,
                    (u16)(st.unit[i].gfx*44+(dir&1?5:0)+(dir&2?22:0)));
            }
        }
        ZE(21);
        if(!b[2]){
            if(W->dialog_key!=st.open_beat)opening_layout();
            ZB(22);opening_dialog_draw();ZE(22);
        }
    }
    if(st.open_fade){
        u8 level=st.open_fade<9?st.open_fade:17-st.open_fade;
        u8 phase=(level+1)>>1,r;u16 pat=phase>=4?0xFFFF:phase==3?0xEEEE:phase==2?0xAAAA:0x8888;
        for(r=0;r<100;r++){
            u8 *l=(u8*)rt_light+(u16)r*RT_PBYTES,*d=(u8*)rt_dark+(u16)r*RT_PBYTES;u8 w;
            u16 bits=r&1?(u16)((pat<<1)|(pat>>15)):pat;
            for(w=0;w<10;w++){RMW16(l+2*w,bits,bits);RMW16(d+2*w,bits,bits);}
        }
    }
}
