// The scene player (fftscn, ENGINE.md): what a chapter shows between battles, read from a scene
// set (tools/scenes.py): a title, scripted beats over a fixed picture with depth (Orbonne's
// chapel: actors walking behind its pillars, dialogue), a chapter card. fftscn(k) plays ffs<k>
// and returns 1 once it is over. ESC asks "Skip scene?" (No/Yes): Yes ends it as if played
// through (same flow written). The beats, dialogue, text and actor bank are the
// battle engine's own code (../beats.inc, ../text.inc, ../actors.inc).
#include "../../../runtime/core/rt.h"
#include "../core_limits.h"
#include "../units.h"
#include "../ui_common.h"
#include <stdlib.h>
#include <string.h>
#include "../base.inc"
#include "../resource_file.h"
#include "../identity_pool.h"
#include "save.h"
#include "story_choice.h"
#include "story_choice_file.h"
#include "flow.h"
#include "flow_draw.inc"
#include "../../../lib/tilog.h"

#ifdef RT_CYCLES
#include "../../../tools/m68kbench/bench.h"
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif

enum { STEP_TITLE = 1, STEP_STORY, STEP_CARD, STEP_NARRATE, STEP_OVERLAY, STEP_CHOICE, STEP_BRANCH };
enum { OPEN_PLAY, OPEN_TITLE, OPEN_CHAPEL, OPEN_GATE, OPEN_AFTER, OPEN_DONE, OPEN_BANTER, OPEN_CHOICE };
#define NUNIT FFT_CORE_UNIT_MAX         // scripted role capacity; beats carry their own counts
#define COVER_H UNIT_SH
typedef struct { s16 x, y; u8 depth, rot, r0, r1, wet; u16 cm[COVER_H]; } Cover;
typedef struct { s16 x, y; u8 depth; } Feet;

typedef struct {
    u8 step, open_stage, open_beat, open_clock, open_white;
    u16 open_reveal, open_offset, open_seen, tick;
    u8 open_pan, open_fade, open_next, done;
    s16 camx, camy;                     // (beats.inc: map scenes only)
    u8 choice_row,choice_error,choice_armed;
} State;
State st;

#define ACTOR_SLOTS (NUNIT + 2)
#define ACTOR_UNITS NUNIT
#define ACTOR_KEY(logical) RD16(amap, logical)   // the set's pose table (ffoa: logical -> frame)
#include "../actors.h"

typedef struct {
    ACTOR_FIELDS
    char dialog[6][48];
    u8 dialog_n, dialog_w, dialog_h, dialog_next;
    u16 dialog_total, dialog_key, dialog_drawn;
    u8 dialog_white, dialog_part, dialog_face;
    Cover cover[NUNIT];
    Feet feet[NUNIT];
    u16 feet_k[NUNIT];
    u16 status[2][RT_H * RT_PBYTES / 2];   // the dialogue's own planes, copied box by box
} Work;
static Work *W;
static u8 skip_ask;            // shared prompt: 0 closed, 1 No / 2 Yes
static u8 *vbuf;                        // the picture: colour, colour, depth/mask x 2
static const u8 *set, *amap, *cgfx, *pgfx;   // pgfx: the speakers' portraits (ffcp)
static const u8 *scene_assets,*scene_steps;
static const u8 *opening_story, *opening_title;
static const u8 *scene_flow;
static Save scene_save;
static u8 scene_save_loaded,scene_save_dirty;
static const u8 *picture_source;
static u8 scene_set_id;
#define PIC_STRIDE_MAX 40                // a picture is 320 pixels wide at most
static u16 pic_stride, pic_plane, picture_bytes;
#define CHAPEL_STRIDE pic_stride
#define CHAPEL_PLANE pic_plane
#define OPENING_BEATS RD16(opening_story, 0)
#define UI_SHARED(s) (cgfx + UIC_##s)
static const u8 walk_seq[16] = { 2, 1, 1, 0, 0, 0, 1, 1, 2, 3, 3, 4, 4, 4, 3, 3 };

#include "../actors.inc"
#include "../text.inc"
#include "../contour.h"
#include "../archive_art.h"
static const u8 *contour_gfx;
#include "../contour_art.inc"
#define HAND_H 14
#ifdef __m68k__
#define hand_gfx CONTOUR_REF_hand_gfx
#else
static u32 scene_hand[3][HAND_H]; // software sprites use aligned host-order words
#define hand_gfx scene_hand
#endif
static u8 scene_text_width(const char *s){u8 n=0;while(*s)n+=UI_SHARED(ADV)[(u8)*s++-32];return n;}
#define CHOICE_TEXT(x,y,s,c,l) ui_text(x,y,s,c,l)
#define CHOICE_WIDTH(s) scene_text_width(s)
#include "../choice_menu.inc"
#include "../scene_skip.h"
#include "story_choice_draw.inc"
static void opening_actor(u8 gfx,u8 slot,s16 x,s16 y,u8 face,u16 logical);
#define BEATS_PICTURE
#define OPENING_DECISION (st.open_stage==OPEN_CHOICE)
#include "../beats.inc"

static const u8 *step_at(u8 i) { return scene_steps + 8 * i; }
static const u8 *choice_record(void){return scene_assets+RD16(step_at(st.step),1);}
static u8 scene_choice_load(Save *s)
{
    if(s!=&scene_save)return 0;
    if(scene_save_loaded)return 1;
    scene_save_loaded=save_load(s);return scene_save_loaded;
}
static u8 scene_choice_store(const Save *s)
{
#ifdef __m68k__
    u8 ok=rt_save(SAVE_NAME,s,sizeof *s);
    if(ok)scene_save_dirty=1;
    return ok;
#else
    u8 ok=story_choice_store_file(s);
    if(!ok)scene_save_loaded=0; // commit's reload must restore the on-disk baseline
    return ok;
#endif
}
static void scene_progress(void)
{
    if(scene_flow&&(scene_set_id==1||scene_set_id==2)&&scene_choice_load(&scene_save)) {
        u8 phase=scene_set_id==1?1:3;
        if(phase==3&&(scene_save.flow<2||!scene_save.deploy_n||qword(scene_save.deploy_battle)!=200))return;
        if(scene_save.flow<phase)scene_save.flow=phase;
        scene_choice_store(&scene_save);
    }
}

// A step begins (the battle engine's opening_stage for the chapel).
static void step_start(void)
{
    const u8 *s;
    for (;;) {
        if (st.step >= RD16(set, 5)) { st.done = 1;scene_progress();tilog("scene done");return; }
        s = step_at(st.step);
        if(s[0]==STEP_BRANCH||s[0]==STEP_CHOICE){
            if(!scene_choice_load(&scene_save)){set=RT_NULL;return;}
            if(s[0]==STEP_BRANCH){st.step=(u8)RD16(s,sc_flag(&scene_save,RD16(s,1))==s[1]?3:2);continue;}
            {const u8 *p=choice_record();u8 answer=story_choice_result(&scene_save,p);
             if(answer==255){set=RT_NULL;return;}
             if(answer){st.step=(u8)RD16(s,answer==1?2:3);continue;}
             if(sc_word(p+8)&&sc_flag(&scene_save,sc_word(p+8))!=p[10]){st.step=s[1];continue;}}
            // The immediately preceding story supplies the visible question,
            // including its portrait and final actor positions when skipped.
            {const u8 *prior=step_at((u8)(st.step-1));u8 i;const u8 *b,*a;
             if(picture_source!=scene_assets+RD16(prior,1)||pic_stride!=RD16(prior,2)){
                 pic_stride=RD16(prior,2);pic_plane=pic_stride*100U;
                 zx0_unpack(scene_assets+RD16(prior,1),vbuf);picture_source=scene_assets+RD16(prior,1);
             }
             opening_story=scene_assets+RD16(prior,3);st.open_beat=(u8)(OPENING_BEATS-1);
             b=opening_beat();a=b+4;st.open_pan=b[3];st.open_offset=0;
             st.open_seen=0;for(i=0;i<b[1];i++,a+=8){u8 slot=a[0]>>4;
                 W->feet[slot].x=(s16)RD16(a+2,0);W->feet[slot].y=(s16)RD16(a+4,0);
                 W->feet_k[slot]=0;W->cover[slot].rot=255;
             }
            }
            W->dialog_key=65535;opening_layout();st.open_reveal=W->dialog_total;
            if(opening_text()[W->dialog_next]){set=RT_NULL;return;} // the final question must fit, never truncate
            st.open_stage=OPEN_CHOICE;st.open_fade=0;st.choice_row=st.choice_error=0;
            st.choice_armed=!(rt_keys&(K_A|K_ENTER));return;
        }
        st.open_clock = 0; st.open_seen = 0;
        memset(W->actor_key, 255, sizeof W->actor_key);
        W->dialog_key = 0xFFFF;
        if (s[0] == STEP_TITLE) { st.open_stage = OPEN_TITLE; opening_title = scene_assets + RD16(s, 1); return; }
        if (s[0] == STEP_STORY||s[0]==STEP_NARRATE||s[0]==STEP_OVERLAY) {
            opening_story = scene_assets + RD16(s, 3);
            pic_stride = RD16(s, 2); pic_plane = (u16)(pic_stride * 100);
            zx0_unpack(scene_assets + RD16(s, 1), vbuf);
            picture_source=scene_assets+RD16(s,1);
            st.open_stage = OPEN_CHAPEL;
            if(s[0]!=STEP_OVERLAY)opening_find(opening_story[RD16(opening_story, 1)]);
            else st.open_stage=OPEN_DONE;
            // NARRATE bit0 means fade-in, not a one-pixel camera offset.
            st.open_pan = s[0]==STEP_STORY?s[1]:0;
            return;
        }
        if (s[0] == STEP_CARD) { st.open_stage = OPEN_DONE; return; }
        st.step++;
    }
}

static void next_step(void) { st.step++; step_start(); }
static void transition(void) { st.open_fade = 1; }

static void advance(void)
{
    u8 event = opening_beat()[0];
    if (st.open_beat + 1 < OPENING_BEATS) {
        st.open_beat++;
        if (opening_beat()[0] == event) { opening_dialog_reset(); return; }
    }
    if(step_at(st.step)[0]==STEP_NARRATE)next_step();else transition();
}

u8 chapter_completed(void) { return st.done == 1; }

static void release(void)
{
    if (acache) free(acache);
    if (vbuf) free(vbuf);
    if (W) free(W);
    acache = RT_NULL; vbuf = RT_NULL; W = RT_NULL;
    picture_bytes = 0;
}

void game_init(void)
{
    u16 identity_size;
#ifndef __m68k__
    release();
#endif
    W = RT_NULL; vbuf = RT_NULL; acache = RT_NULL;   // statics survive between runs
    picture_bytes = 0;
    scene_save_loaded=scene_save_dirty=0;picture_source=RT_NULL;
    cgfx = rt_file("ffcu", &identity_size);
    contour_gfx=archive_art_tail(cgfx,identity_size);
    if(!contour_gfx){ // historical extraction controls; shipping shares this tail in ffcu
        const u8 *view=resource_file("ffov0",&identity_size);
        contour_gfx=archive_art_tail(view,identity_size);
    }
#ifndef __m68k__
    if(contour_gfx){u8 plane,row;for(plane=0;plane<3;plane++)for(row=0;row<HAND_H;row++)
        scene_hand[plane][row]=RD32((const u8 *)CONTOUR_REF_hand_gfx[plane],(u16)row*2);}
#endif
    scene_flow=rt_file("ffflow",RT_NULL);if(!qvalid(scene_flow))scene_flow=RT_NULL;
    pgfx = rt_file("ffcp", &identity_size);
    if(!identity_valid(pgfx,identity_size))pgfx=RT_NULL;
    actor_banks(abank);
    W = malloc(sizeof(Work));
    acache = W ? malloc(AC_N * (UNIT_FB + 4)) : RT_NULL;
    if (!acache) { release(); return; }
    ac_key = (u16 *)acache[AC_N]; ac_age = ac_key + AC_N;
#ifdef __m68k__
    atexit(release);
#endif
}

void game_scenario(u16 n)
{
    char name[6] = "ffs0";
    u16 size,asset_size,i,required=0;
    memset(&st, 0, sizeof st); skip_ask = 0;
    if(!scene_save_dirty)scene_save_loaded=0;
    picture_source=RT_NULL;
    scene_set_id=(u8)n;
    name[3] = (char)('0' + n);
    set = resource_file(name, &size);asset_size=size;
    if (!W || !set || size<12 || !pgfx || !cgfx || !contour_gfx || !abank[0] || !abank[1] ||
        RD16(set,0)!=0x4653||(RD16(set,1)!=0x4331&&RD16(set,1)!=0x4332)) { set = RT_NULL; return; }
    scene_assets=set;scene_steps=set+12;
    if(RD16(set,1)==0x4332){
        char pool[9];if(size<20){set=RT_NULL;return;}
        memcpy(pool,set+12,8);pool[8]=0;
        scene_assets=resource_file(pool,&asset_size);scene_steps=set+20;
        if(!scene_assets||asset_size<6||RD32(scene_assets,0)!=0x46535031UL){set=RT_NULL;return;}
    }
    if(RD16(set,5)>255||RD16(set,5)*8U>size-(u16)(scene_steps-set)){set=RT_NULL;return;}
    for(i=0;i<RD16(set,5);i++){
        const u8 *s=step_at((u8)i);
        if(s[0]==STEP_BRANCH){
            if(s[1]>1||!RD16(s,1)||RD16(s,1)>FLAG_N||RD16(s,2)<=i||RD16(s,2)>RD16(set,5)||
               RD16(s,3)<=i||RD16(s,3)>RD16(set,5)){set=RT_NULL;return;}
            continue;
        }
        if(s[0]==STEP_CHOICE){
            const u8 *p;u16 at=RD16(s,1),q;const u8 *prior,*beats,*b;u8 j;
            if(!i||step_at((u8)(i-1))[0]!=STEP_STORY||at>=asset_size||
               !story_choice_valid(scene_assets+at,(u16)(asset_size-at))||
               RD16(s,2)<=i||RD16(s,2)>RD16(set,5)||RD16(s,3)<=i||RD16(s,3)>RD16(set,5)){set=RT_NULL;return;}
            p=scene_assets+at;
            if(sc_word(p+8)?s[1]<=i||s[1]>RD16(set,5):s[1]!=0){set=RT_NULL;return;}
            for(j=0;j<2;j++)if(scene_text_width((const char *)p+sc_word(sc_option(p,j)+2))>126){set=RT_NULL;return;}
            prior=step_at((u8)(i-1));q=RD16(prior,3);
            if((q&1)||q>asset_size-2){set=RT_NULL;return;}beats=scene_assets+q;
            if(!RD16(beats,0)||RD16(beats,0)>255||RD16(beats,0)>(asset_size-q-2)/2){set=RT_NULL;return;}
            at=RD16(beats,(u16)RD16(beats,0));
            if((at&1)||at<2+2*RD16(beats,0)||at>asset_size-q||asset_size-q-at<4){set=RT_NULL;return;}
            b=beats+at;at=(u16)(q+at+4);
            if(b[2]||b[3]>(RD16(prior,2)-RT_W/8)*8U||b[1]>NUNIT||b[1]>(asset_size-at)/8){set=RT_NULL;return;}
            for(j=0;j<b[1];j++)if(b[4+8*j]>>4>=NUNIT){set=RT_NULL;return;}
            at+=(u16)b[1]*8;
            for(j=0;j<2;j++){
                u16 start=at;
                while(at<asset_size&&scene_assets[at]){
                    if(scene_assets[at]<32||scene_assets[at]>126||at-start>=(j?127:31)){set=RT_NULL;return;}at++;
                }
                if(at==asset_size||(j&&at==start)){set=RT_NULL;return;}at++;
            }
            continue;
        }
        if(s[0]<STEP_TITLE||s[0]>STEP_OVERLAY||RD16(s,1)>=asset_size||
           ((s[0]==STEP_STORY||s[0]==STEP_NARRATE||s[0]==STEP_OVERLAY)&&
            (RD16(s,2)<RT_W/8||RD16(s,2)>PIC_STRIDE_MAX||RD16(s,3)>=asset_size))){set=RT_NULL;return;}
        if(s[0]==STEP_STORY||s[0]==STEP_NARRATE||s[0]==STEP_OVERLAY){
            u16 bytes=RD16(s,2)*600U;
            if(bytes>required)required=bytes;
        }
    }
    {
        char map[7]; memcpy(map, set + 4, 6); map[6] = 0;
        amap = *map ? resource_file(map, RT_NULL) : RT_NULL;   // (no actors: no pose table)
        if (*map && !amap) { set = RT_NULL; return; }
    }
    // Reserve once for this set's widest picture, including all four depth
    // planes. Titles/cards need none; narrower sets release the old excess.
    if(required!=picture_bytes){
        if(vbuf)free(vbuf);
        vbuf=RT_NULL;picture_bytes=0;
        if(required){vbuf=malloc(required);if(!vbuf){set=RT_NULL;return;}picture_bytes=required;}
    }
    memset(W, 0, sizeof *W);
    actor_reset(); W->dialog_key = 0xFFFF;
    tilog("scene %u start: %u steps", n, RD16(set, 5));
    step_start();
    if (!st.done && (step_at(st.step)[0] == STEP_CARD||step_at(st.step)[0]==STEP_NARRATE) && step_at(st.step)[1] & 1)
        st.open_fade = 9;               // the previous program faded out: this card fades in
}

static u8 scene_choice_update(void)
{
    const u8 *p=choice_record();u8 result;u32 pressed=rt_keys&~rt_prev;
    W->actor_budget=1;W->actor_misses=0;W->ac_now++;
    st.tick++;
    if(!(rt_keys&(K_A|K_ENTER)))st.choice_armed=1;
    if(pressed&(K_ESC|K_B)){st.choice_row=st.choice_error=0;return 1;}
    if(pressed&(K_UP|K_DOWN)){st.choice_row=choice_move(st.choice_row,2,pressed);st.choice_error=0;}
    if(st.choice_armed&&(pressed&(K_A|K_ENTER))&&W->dialog_drawn!=65535&&W->dialog_drawn>=W->dialog_total){
        result=story_choice_commit(&scene_save,p,sc_word(p+4),st.choice_row,scene_choice_load,scene_choice_store);
        if(result==SC_APPLIED||result==SC_REPLAY){
            u8 answer=story_choice_result(&scene_save,p);
            st.step=(u8)RD16(step_at(st.step),answer==1?2:3);step_start();return !st.done;
        }
        st.choice_error=result;
    }
    return 1;
}
static u8 scene_skip_remaining(void)
{
    while(st.step<RD16(set,5)){
        const u8 *s=step_at(st.step);
        if(s[0]==STEP_BRANCH||s[0]==STEP_CHOICE){
            step_start();
            if(!set)return 1;
            if(st.done)return 0;
            if(st.open_stage==OPEN_CHOICE)return 1; // explicit answer is mandatory
        }else st.step++;
    }
    step_start();return 0;
}
u8 game_update(void)
{
    if(st.done) {
        return 0;
    }
    if (!set) return !input_pressed(K_ESC);
    if(st.open_stage==OPEN_CHOICE)return scene_choice_update();
    {u8 skip=scene_skip_input(&skip_ask);if(skip){
        if(skip==2){tilog("scene skipped at step %u",st.step);return scene_skip_remaining();}
        return 1;
    }}
    W->actor_budget = 1; W->actor_misses = 0; W->ac_now++;
    st.tick++; if (st.open_clock < 250) st.open_clock++;
    if (st.open_fade) {                 // out over 8 frames, the next step in over 8 more
        if (++st.open_fade == 9) { next_step(); if (st.done) return 0; st.open_fade = 9; }
        else if (st.open_fade >= 17) st.open_fade = 0;
        return 1;
    }
    if (st.open_stage == OPEN_TITLE) {
        // Do not consume the ENTER which launched the program. Finish the source-letter
        // reveal and leave a readable hold before skipping.
        if (st.open_clock >= 150 || (st.open_clock >= 110 && input_pressed(K_A | K_ENTER))) next_step();
        return !input_pressed(K_ESC);
    }
    if (st.open_stage == OPEN_DONE) {
        if ((step_at(st.step)[0]==STEP_OVERLAY&&st.open_clock>=150)||
            (input_pressed(K_A | K_ENTER)&&(step_at(st.step)[0]!=STEP_OVERLAY||st.open_clock>=30))) {
            next_step();return 1;
        }
        return !input_pressed(K_ESC);
    }
    opening_positions();
    if (input_pressed(K_CATALOG)) { st.open_white ^= 1; W->dialog_key = 0xFFFF; }
    if (input_pressed(K_B)) { transition(); return 1; }
    if (opening_beat()[2]) {
        if (st.open_clock >= opening_beat()[2]) advance();
        return !input_pressed(K_ESC);
    }
    if (W->dialog_key != st.open_beat) opening_layout();
    if (st.open_reveal < W->dialog_total) {
        st.open_reveal += 3;
        if (st.open_reveal > W->dialog_total) st.open_reveal = W->dialog_total;
        if (input_pressed(K_A | K_ENTER)) st.open_reveal = W->dialog_total;
    } else if (input_pressed(K_A | K_ENTER)) {
        const char *s = opening_text();
        if (W->dialog_drawn != 0xFFFF && W->dialog_drawn < W->dialog_total) return 1;
        if (s[W->dialog_next]) { st.open_offset += W->dialog_next; st.open_reveal = 0; W->dialog_key = 0xFFFF; return 1; }
        advance();
    }
    return !input_pressed(K_ESC);
}

static void draw_title(void)
{
#ifdef FFT_SHARED_SERIF
    draw_rect(0,0,160,100,C_BLACK);
    if(scene_flow)qtitle_reveal(scene_flow,12,42,(u16)st.open_clock*2);
#else
    u8 col, y; u16 sweep = (u16)st.open_clock * 2;
    draw_rect(0, 0, 160, 100, C_BLACK);
    for (col = 0; col < 18; col++) {
        const u8 *p = opening_title + (u16)col * 42; u8 l[14], d[14], m[14], white = 0, mid = 0, dim = 0, x;
        for (x = 0; x < 8; x++) {
            s16 delta = (s16)sweep - col * 8 - x; u8 bit = 0x80 >> x;
            if (delta >= 16) white |= bit; else if (delta >= 8) mid |= bit; else if (delta >= 0) dim |= bit;
        }
        for (y = 0; y < 14; y++) {
            u8 ink = (u8)~p[28 + y]; m[y] = (u8)~(ink & (white | mid | dim));
            l[y] = (p[y] & white) | (ink & mid); d[y] = (p[14 + y] & white) | (ink & dim);
        }
        { RtSprite sp = { 8, 14, l, d, m }; draw_sprite(8 + col * 8, 42, &sp); }
    }
#endif
}

static void draw_card(void)
{
    const u8 *p = scene_assets + RD16(step_at(st.step), 1);
    u8 n = *p++;
    draw_rect(0, 0, 160, 100, C_BLACK);
    while (n--) {
        ui_text(p[0], p[1], (const char *)p + 3, p[2], 158);
        p += 3; while (*p++) ;
    }
}

static void draw_picture(void)
{
    u8 i;
    for(i=0;i<100;i++) {
        u16 at=(u16)i*pic_stride+(st.open_pan>>3);
        memcpy((u8 *)rt_light+(u16)i*RT_PBYTES,vbuf+at,20);
        memcpy((u8 *)rt_dark+(u16)i*RT_PBYTES,vbuf+pic_plane+at,20);
    }
}
static void draw_narration(void)
{
    u8 row;u16 reveal=st.open_reveal;
    if(W->dialog_key!=st.open_beat)opening_layout();
    for(row=0;row<W->dialog_n;row++) {
        char line[48];u8 n=(u8)strlen(W->dialog[row]),k;u16 width=0;
        for(k=0;k<n;k++)width+=UI_SHARED(ADV)[(u8)W->dialog[row][k]-32];
        if(n>reveal)n=(u8)reveal;
        memcpy(line,W->dialog[row],n);line[n]=0;reveal-=n;
        ui_text((160-width)/2,(100-W->dialog_n*7)/2+row*7,line,C_WHITE,158);
    }
}

void game_render(void)
{
    u8 i;
    if (!set) { draw_rect(0, 0, 160, 100, C_WHITE); ui_text(3, 40, "Missing scene set", C_BLACK, 160); return; }
    if (st.done) return;
    if (st.open_stage == OPEN_TITLE) draw_title();
    else if (st.open_stage == OPEN_DONE&&step_at(st.step)[0]==STEP_OVERLAY) {
        const u8 *p=scene_assets+RD16(step_at(st.step),3);u8 n=*p++;
        draw_picture();qshade();
        if(!scene_flow)ui_text(3,40,"Missing flow data",C_WHITE,158);
        else {
            u8 line=0;
            while(n--){
                s16 sweep=(s16)st.open_clock*3-line*48;
                qtitle_reveal(scene_flow,p[0],p[1],sweep>0?(u16)sweep:0);
                p+=2;line++;
            }
        }
    }
    else if (st.open_stage == OPEN_DONE) draw_card();
    else {
        const u8 *b = opening_beat(), *a = b + 4;
        ZB(21);
        draw_picture();
        for (i = 0; i < b[1]; i++, a += 8) {
            u8 slot = a[0] >> 4;
            opening_actor(a[0] & 15, slot, W->feet[slot].x, W->feet[slot].y, a[1], RD16(a + 6, 0));
        }
        ZE(21);
        if(step_at(st.step)[0]==STEP_NARRATE){qshade();draw_narration();}
        else if (!b[2]) {
            if (W->dialog_key != st.open_beat) opening_layout();
            ZB(22); opening_dialog_draw(); ZE(22);
        }
    }
    if (st.open_fade) {
        u8 level = st.open_fade < 9 ? st.open_fade : 17 - st.open_fade;
        u8 phase = (level + 1) >> 1, r; u16 pat = phase >= 4 ? 0xFFFF : phase == 3 ? 0xEEEE : phase == 2 ? 0xAAAA : 0x8888;
        for (r = 0; r < 100; r++) {
            u8 *l = (u8 *)rt_light + (u16)r * RT_PBYTES, *d = (u8 *)rt_dark + (u16)r * RT_PBYTES; u8 w;
            u16 bits = r & 1 ? (u16)((pat << 1) | (pat >> 15)) : pat;
            for (w = 0; w < 10; w++) { RMW16(l + 2 * w, bits, bits); RMW16(d + 2 * w, bits, bits); }
        }
    }
    if(skip_ask)scene_skip_draw(skip_ask,rt_frame);
    if(st.open_stage==OPEN_CHOICE)scene_choice_draw(choice_record(),st.choice_row,st.choice_error,rt_frame);
}
