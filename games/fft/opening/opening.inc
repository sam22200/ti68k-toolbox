// Native opening adapter for the common tactical engine. Immutable content is in
// local archive banks; the chapel reuses the tactical view and Status buffers.
#include "data/story.h"
typedef char OpeningDiskLayout[(sizeof(UnitDef)==24 && sizeof(Item)==23 && sizeof(Job)==14 && sizeof(Ability)==24 && sizeof(SkillSet)==18)?1:-1];
enum { OPEN_PLAY, OPEN_TITLE, OPEN_CHAPEL, OPEN_GATE, OPEN_AFTER, OPEN_DONE, OPEN_BANTER };
static const u8 *opening_story, *opening_chapel, *opening_title;
static void opening_actor(u8 gfx,u8 slot,s16 x,s16 y,u8 face,u16 logical);
static void opening_stage(u8 stage);

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

static const u8 *opening_beat(void)
{
    return opening_story + RD16(opening_story + 2, st.open_beat);
}
static const char *opening_speaker(void)
{
    const u8 *b = opening_beat();
    return (const char *)(b + 4 + b[1] * 8);
}
static const char *opening_text(void)
{
    const char *s = opening_speaker();
    while (*s++) ;
    return s + st.open_offset;
}

static const u8 *opening_frame(u16 logical, u8 slot)
{
    static const u8 rev[16] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
    u16 frame_key = RD16(ugfx, logical), id = frame_key & 0x7FFF;
    u8 *out = (u8 *)W->actor_gfx[slot];
    if (W->actor_key[slot] != frame_key) {
        u8 other;
        // Shared job sprites often request a pose another actor already decoded.
        // Reuse it without adding a second cache or consuming the decode quota.
        for(other=0;other<NUNIT+2;other++)if(W->actor_key[other]==frame_key) {
            memcpy(out,W->actor_gfx[other],UNIT_FB);W->actor_key[slot]=frame_key;
            W->actor_copies++;return out;
        }
        // Animation alone can wait a frame; a new actor never borrows another's
        // cached appearance. One ordinary miss per frame bounds decompression.
        if(!W->actor_budget && W->actor_key[slot]!=0xFFFF && slot<NUNIT)return out;
        if(W->actor_budget)W->actor_budget--;
        ZB(20);
        zx0_unpack(ugfx + ACTOR_DATA + RD16(ugfx + ACTOR_DIR, id), out);
        if (frame_key & 0x8000) {
            u8 *p = out, *end = out + UNIT_FB;
            for (; p < end; p += 2) {
                u8 a = p[0], b = p[1];
                p[0] = (u8)(rev[b & 15] << 4 | rev[b >> 4]);
                p[1] = (u8)(rev[a & 15] << 4 | rev[a >> 4]);
            }
        }
        W->actor_key[slot] = frame_key;
        W->actor_misses++;
        ZE(20);
    }
    return out;
}

// Walking/action poses have priority; rotate the remaining decode opportunities
// among visible actors so the frontmost unit cannot starve the other animations.
static void opening_animate(const u8 *visible)
{
    u8 i,k;
    if(st.mode==M_WALK && visible[st.sel])unit_frame(st.rot,st.sel);
    if(st.mode==M_ACT) {
        if(visible[st.atk])unit_frame(st.rot,st.atk);
        if(visible[st.tgt])unit_frame(st.rot,st.tgt);
    }
    for(i=0;i<NUNIT && W->actor_budget;i++) {
        k=W->actor_next;
        if(++W->actor_next==NUNIT)W->actor_next=0;
        if(visible[k])unit_frame(st.rot,k);
    }
    W->actor_budget=0;
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
    s16 dx=(s16)W->tx[c]-W->tx[o],dz=(s16)W->tz[c]-W->tz[o],h=(s16)map_tiles[c].stand-map_tiles[o].stand;
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

static void opening_dialog_reset(void)
{
    st.open_reveal=0;st.open_offset=0;st.open_clock=0;W->dialog_key=0xFFFF;
}
static u8 opening_find(u8 event)
{
    u8 i;
    for(i=0;i<OPENING_BEATS;i++){
        st.open_beat=i;
        if(opening_beat()[0]==event){opening_dialog_reset();return 1;}
    }
    return 0;
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
    opening_chapel=opening_story ? opening_story+RD16(opening_story,0):RT_NULL;
    if(opening_story)opening_story+=2;
    opening_units();memcpy(st.bag,bag_init,NBAG);
    memset(W->actor_key,255,sizeof W->actor_key);W->actor_budget=255;W->actor_misses=W->actor_next=W->actor_copies=0;W->dialog_key=0xFFFF;
    W->bg_ok=W->hold=W->frz=W->status_i=0;W->pan_ok[0]=W->pan_ok[1]=0;
    W->win_k[0]=0;W->pan_k[0][0]=W->pan_k[1][0]=W->fc_key=0;
    scene_rot=vbuf_rot=0xFF;
    st.rot=2;st.act=st.sel=U_RAMZA;st.cx=st.unit[0].x;st.cz=st.unit[0].z;st.mode=M_BROWSE;
    cam_target(&st.camx,&st.camy);
    if(n==5){opening_stage(OPEN_DONE);return;} // AMS chapter-loader injection door
    if(n==1||n==2||n==3){
        opening_battle(n==1);
        if(n==2)st.ai_all=1;
        if(n==3)opening_stage(OPEN_AFTER);
    }else{
        st.open_stage=OPEN_TITLE;opening_find(2);st.open_pan=48;
        if(opening_chapel)zx0_unpack(opening_chapel,vbuf);
        opening_title=opening_chapel+OPENING_CHAPEL_BYTES;

    }
}

// Fit words with the actual canonical glyph advances; six lines maximum, with
// a continuation page instead of truncation. The box has no spare bottom rows.
static void opening_layout(void)
{
    ZB(23);
    const char *s=opening_text();u16 at=0;u8 row=0,maxw=0;
    memset(W->dialog,0,sizeof W->dialog);W->dialog_total=0;W->dialog_drawn=0xFFFF;W->dialog_part=0;
    while(s[at] && row<6){
        u16 start=at,last=at;u8 width=0,col=0,lastcol=0,lastwidth=0;
        while(s[at] && col<47){
            u8 ch=(u8)s[at],adv=igfx[UI_ADV+(ch>=32&&ch<=126?ch-32:'?'-32)];
            if(width+adv>146)break;
            if(ch==' '){last=at;lastcol=col;lastwidth=width;}
            width+=adv;at++;col++;
        }
        if(s[at] && last>start){at=last;col=lastcol;width=lastwidth;}
        memcpy(W->dialog[row],s+start,col);W->dialog_total+=col;
        if(width>maxw)maxw=width;
        while(s[at]==' ')at++;
        row++;
    }
    W->dialog_n=row;W->dialog_w=maxw+8;W->dialog_h=6+7*row;
    W->dialog_next=(u8)at;W->dialog_key=st.open_beat;W->dialog_white=st.open_white;
    ZE(23);
}

static void opening_stage(u8 stage)
{
    st.open_stage=stage;st.open_clock=0;st.open_seen=0;
    memset(W->actor_key,255,sizeof W->actor_key);
    W->dialog_key=0xFFFF;W->bg_ok=W->hold=W->frz=0;
    if(stage==OPEN_CHAPEL){opening_find(2);st.open_pan=48;}
    else if(stage==OPEN_GATE){opening_find(4);scene_rot=vbuf_rot=255;}
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

static void opening_positions(void)
{
    const u8 *b=opening_beat(),*a=b+4;u8 i;
    for(i=0;i<b[1];i++,a+=8){
        u8 slot=a[0]>>4,gfx=a[0]&15;
        s16 x=(s16)RD16(a+2,0),y=(s16)RD16(a+4,0);
        if(!(st.open_seen&(1U<<slot))){
            W->actor_key[slot]=0xFFFF; // a scripted role never borrows a battle actor's body
            W->feet[slot].x=x;W->feet[slot].y=y;
            if(st.open_stage==OPEN_CHAPEL&&gfx<9){W->feet[slot].x-=40;W->feet[slot].y+=20;}
            if(st.open_stage==OPEN_AFTER&&gfx==11)W->feet[slot].x-=64;
            st.open_seen|=(u16)(1U<<slot);W->cover[slot].rot=255;
        }
        if(st.open_stage==OPEN_AFTER && b[3] && !(st.open_seen&0x8000) && (gfx==1||gfx==9)){
            W->feet[slot].x=x-64;W->feet[slot].y=y;
        }
        W->feet_k[slot]=W->feet[slot].x!=x||W->feet[slot].y!=y;
        if(W->feet[slot].x!=x)W->feet[slot].x+=x>W->feet[slot].x?1:-1;
        if(W->feet[slot].y!=y)W->feet[slot].y+=y>W->feet[slot].y?1:-1;
    }
    if(st.open_stage==OPEN_AFTER&&b[3])st.open_seen|=0x8000;
    if(st.open_stage==OPEN_CHAPEL && !(st.tick&3)){
        if(st.open_pan<b[3])st.open_pan+=8;
        else if(st.open_pan>b[3])st.open_pan-=8;
    }
}

static void opening_advance(void)
{
    u8 event=opening_beat()[0];
    if(st.open_beat+1<OPENING_BEATS){
        st.open_beat++;
        if(opening_beat()[0]==event){opening_dialog_reset();return;}
    }
    if(st.open_stage==OPEN_CHAPEL)opening_transition(OPEN_GATE);
    else if(st.open_stage==OPEN_GATE){opening_stage(OPEN_BANTER);}
    else if(st.open_stage==OPEN_BANTER)opening_transition(OPEN_PLAY);
    else opening_transition(OPEN_DONE);
}

static u8 opening_update(void)
{
    if(!W||!opening_story||!opening_chapel)return !input_pressed(K_ESC);
    st.tick++;if(st.open_clock<250)st.open_clock++;
    if(!st.open_stage&&!st.open_fade)opening_transition(OPEN_AFTER);
    if(st.open_fade){
        if(++st.open_fade==9){opening_stage(st.open_next);st.open_fade=9;}
        else if(st.open_fade>=17)st.open_fade=0;
        return !input_pressed(K_ESC);
    }
    if(st.open_stage==OPEN_TITLE){
        // Do not consume the ENTER which launched the packed program. Finish
        // the source-letter reveal and leave a readable hold before skipping.
        if(st.open_clock>=150||(st.open_clock>=110&&input_pressed(K_A|K_ENTER)))opening_stage(OPEN_CHAPEL);
        return !input_pressed(K_ESC);
    }
    if(st.open_stage==OPEN_DONE){
        // A completed prologue is a handoff, never an implicit replay.
        if(input_pressed(K_A|K_ENTER)){st.open_handoff=1;return 0;}
        return !input_pressed(K_ESC);
    }
    if(st.open_stage==OPEN_CHAPEL||st.open_stage==OPEN_AFTER)opening_positions();
    if(input_pressed(K_CATALOG)){st.open_white^=1;W->dialog_key=0xFFFF;}
    if(input_pressed(K_B)){
        if(st.open_stage==OPEN_CHAPEL)opening_transition(OPEN_GATE);
        else if(st.open_stage==OPEN_GATE||st.open_stage==OPEN_BANTER)opening_transition(OPEN_PLAY);
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

static void opening_actor(u8 gfx,u8 slot,s16 x,s16 y,u8 face,u16 logical)
{
    u8 pose=W->feet_k[slot]?walk_seq[st.tick&15]:0;
    const u8 *g=opening_frame(W->feet_k[slot]&&gfx<11&&!(st.open_stage==OPEN_AFTER&&opening_beat()[3])?(u16)(gfx*44+(face&1?5:0)+pose+(face&2?22:0)):logical,slot);
    RtSprite sp;u16 l[UNIT_SH],d[UNIT_SH],m[UNIT_SH];u8 r;
    Cover *c=&W->cover[slot<NUNIT?slot:0];u8 depth=face>>2;
    // Cached per-byte depth sampling, the tactical cover algorithm at stride 20.
    // Six actors x 26 x 16 per-pixel tests cost ~0.9M cycles in the first version.
    if(c->x!=x||c->y!=y||c->depth!=depth||c->rot!=(st.open_stage==OPEN_CHAPEL?42:43)){
        s16 bx=(x-8)>>3,shift=(x-8)&7;
        if(st.open_stage!=OPEN_CHAPEL)cover(c->cm,x,y,depth);
        else for(r=0;r<UNIT_SH;r++){
            s16 sy=y-UNIT_FOOT+r;u32 bits=0;u8 j;
            for(j=0;j<3;j++){
                u8 bit=0;s16 sx=bx+j;
                if((u16)sy<100&&(u16)sx<CHAPEL_STRIDE){
                    u16 at=(u16)(sy*CHAPEL_STRIDE+sx);
                    if(vbuf[2*CHAPEL_PLANE+at]>depth)bit|=vbuf[3*CHAPEL_PLANE+at];
                    if(vbuf[4*CHAPEL_PLANE+at]>depth)bit|=vbuf[5*CHAPEL_PLANE+at];
                }
                bits=(bits<<8)|bit;
            }
            c->cm[r]=(u16)(bits>>(8-shift));
        }
        c->x=x;c->y=y;c->depth=depth;c->rot=st.open_stage==OPEN_CHAPEL?42:43;
    }
    for(r=0;r<UNIT_SH;r++){
        // The rear abduction is a staged foreground shot: the static battle
        // mesh lacks the original event's moving/hidden parts. Preserve the
        // tactical terrain shader in battle and the chapel's own cover here.
        u16 covered=st.open_stage==OPEN_CHAPEL?c->cm[r]&RD16(g,3*UNIT_SH+r):0;
        l[r]=RD16(g,r)&~covered;d[r]=RD16(g,UNIT_SH+r)&~covered;m[r]=RD16(g,2*UNIT_SH+r);
    }
    sp.w=16;sp.h=UNIT_SH;sp.light=l;sp.dark=d;sp.mask=m;draw_sprite(x-8-(st.open_stage==OPEN_CHAPEL?st.open_pan:st.camx),y-UNIT_FOOT-(st.open_stage==OPEN_CHAPEL?0:st.camy),&sp);
}

static void opening_copy_rect(s16 x,s16 y,u8 w,u8 h)
{
    u8 a=(u8)(x>>3),b=(u8)((x+w-1)>>3),r;
    u8 left=(u8)(0xFF>>(x&7)),right=(u8)(0xFF<<(7-((x+w-1)&7)));
    for(r=0;r<h;r++){
        u16 at=(u16)((y+r)*RT_PBYTES);u8 p;
        for(p=0;p<2;p++){
            u8 *dst=(u8*)(p?rt_dark:rt_light)+at;const u8 *src=(const u8*)W->status[p]+at;
            if(a==b){u8 mask=left&right;dst[a]=(dst[a]&~mask)|(src[a]&mask);}
            else{dst[a]=(dst[a]&~left)|(src[a]&left);dst[b]=(dst[b]&~right)|(src[b]&right);if(b>a+1)memcpy(dst+a+1,src+a+1,b-a-1);}
        }
    }
}

static void opening_dialog_draw(void)
{
    u8 row,col;u16 n=0;u8 w=W->dialog_w,h=W->dialog_h;
    s16 x=(RT_W-w)>>1,y=RT_H-h;const char *name=opening_speaker();s16 nw=8;
    void *saved_l=rt_light,*saved_d=rt_dark;
    u8 cold=W->dialog_drawn==0xFFFF;
    u16 visible=st.open_reveal,from=cold?0:W->dialog_drawn;
    if(visible>from+8)visible=from+8;
    rt_light=W->status[0];rt_dark=W->status[1];
    for(col=0;name[col];col++)nw+=igfx[UI_ADV+(u8)name[col]-32];
    if(nw>156)nw=156;
    if(cold){
        draw_rect(x,y,w,h,st.open_white?C_BLACK:C_WHITE);
        draw_rect(x+1,y+1,w-2,h-2,st.open_white?C_WHITE:C_BLACK);
        // Word masks apply the ordered gradient without a per-pixel loop.
        for(row=2;row<5;row++){
            u16 pat=row<2?0xFFFF:row==2?0xAAAA:row==3?0x8888:0;
            u8 first=(u8)((x+2)>>4),last=(u8)((x+w-3)>>4),word;
            u16 left=(u16)(0xFFFFU>>((x+2)&15)),right=(u16)(0xFFFFU<<(15-((x+w-3)&15)));
            u8 *l=(u8*)rt_light+(y+row)*RT_PBYTES;
            for(word=first;word<=last;word++){
                u16 mask=pat&(word==first?left:0xFFFF)&(word==last?right:0xFFFF);
                RMW16(l+2*word,mask,st.open_white?mask:0);
            }
        }
        draw_rect(x,y-11,nw,11,st.open_white?C_BLACK:C_WHITE);
        draw_rect(x+1,y-10,nw-2,10,st.open_white?C_WHITE:C_BLACK);
        ui_text(x+4,y-8,name,st.open_white?C_BLACK:C_WHITE,156);
    }
    if(cold||W->dialog_drawn!=st.open_reveal||W->dialog_part<W->dialog_n){
        for(row=0;row<W->dialog_n;row++){
            s16 sx=x+4;char ch[2]={0,0};
            for(col=0;W->dialog[row][col];col++,n++){
                ch[0]=W->dialog[row][col];
                if(row==W->dialog_part||(W->dialog_drawn!=0xFFFF&&n>=W->dialog_drawn&&n<visible+3)){
                    u8 color=n<visible?(st.open_white?C_BLACK:C_WHITE):n<visible+3?(st.open_white?C_DGRAY:C_LGRAY):(st.open_white?C_LGRAY:C_DGRAY);
                    ui_text(sx,y+4+row*7,ch,color,156);
                }
                sx+=igfx[UI_ADV+(u8)ch[0]-32];
            }
        }
        if(W->dialog_part<W->dialog_n)W->dialog_part++;
        W->dialog_drawn=visible;
    }
    rt_light=saved_l;rt_dark=saved_d;
    // Only the fitted window rectangle is copied, not a second whole screen.
    opening_copy_rect(x,y,w,h);
    opening_copy_rect(x,y-11,(u8)nw,11);
    if(st.open_reveal>=W->dialog_total && (st.tick&16))draw_rect(x+w-5,96,3,2,st.open_white?C_BLACK:C_WHITE);
}

static void opening_render(void)
{
    u8 i;
    if(!opening_story||!opening_chapel){draw_rect(0,0,160,100,C_WHITE);ui_text(3,40,"Missing opening banks",C_BLACK,160);return;}
    if(st.open_stage==OPEN_TITLE){
        u8 col,y;u16 sweep=(u16)st.open_clock*2;
        draw_rect(0,0,160,100,C_BLACK);
        for(col=0;col<18;col++){
            const u8 *p=opening_title+(u16)col*42;u8 l[14],d[14],m[14],white=0,mid=0,dim=0,x;
            for(x=0;x<8;x++){
                s16 delta=(s16)sweep-col*8-x;u8 bit=0x80>>x;
                if(delta>=16)white|=bit;else if(delta>=8)mid|=bit;else if(delta>=0)dim|=bit;
            }
            for(y=0;y<14;y++){
                u8 ink=(u8)~p[28+y];m[y]=(u8)~(ink&(white|mid|dim));
                l[y]=(p[y]&white)|(ink&mid);d[y]=(p[14+y]&white)|(ink&dim);
            }
            {RtSprite sp={8,14,l,d,m};draw_sprite(8+col*8,42,&sp);}
        }
        return;
    }
    if(st.open_stage==OPEN_DONE){
        draw_rect(0,0,160,100,C_BLACK);
        ui_text(14,28,"Chapter 1",C_WHITE,158);
        ui_text(14,45,"The Meager",C_WHITE,158);
        ui_text(14,74,"ENTER: Military Academy",C_LGRAY,158);
    }else{
        const u8 *b=opening_beat(),*a=b+4;
        ZB(21);
        if(st.open_stage==OPEN_CHAPEL){
            for(i=0;i<100;i++){
                u16 at=(u16)i*CHAPEL_STRIDE+(st.open_pan>>3);
                memcpy((u8*)rt_light+i*RT_PBYTES,vbuf+at,20);
                memcpy((u8*)rt_dark+i*RT_PBYTES,vbuf+CHAPEL_PLANE+at,20);
            }
        }else{ensure_scene(st.rot);copy_view();}
        if(st.open_stage==OPEN_CHAPEL||st.open_stage==OPEN_AFTER){
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
