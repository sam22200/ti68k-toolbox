// Private PC review controller. The generated engine copy changes one HUD call.
// No active renderer, catalogue, module ABI, save or calculator package changes.
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#include <stdlib.h>
static void area_review_hud(void);
#define FFT_SOURCE_CAMERA
#define FFT_BATTLE
#define FFT_DEFAULT_CHAPTER 1
#define game_update area_original_update
#define game_render area_original_render
#define game_scenario area_original_scenario
#include "x/core-area-ui-review/engine.inc"
#undef game_update
#undef game_render
#undef game_scenario

enum { AR_RANGE,AR_TARGETS,AR_CONFIRM,AR_SENT };
#define AR_ROWS 4
static struct {
    CastBatch forecast;
    u8 phase,baseline,ids[UNIT_CAPACITY],count,row,top,answer,centre,follow_id;
    u8 enemy[2],ally;
    char line[UNIT_CAPACITY][28];
    u16 failures,checks;
    u32 digest;
} ar;
static void check(u16 got,u16 expected,u16 line)
{
    ar.checks++;ar.digest=(ar.digest<<1|ar.digest>>31)^got;
    if(got!=expected){ar.failures++;printf("line%u: %u expected%u\n",line,got,expected);}
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)

static void page(void)
{
    if(ar.row<ar.top)ar.top=ar.row;
    if(ar.row>=ar.top+AR_ROWS)ar.top=ar.row-AR_ROWS+1;
}
static void prepare_cards(void)
{
    u8 side,k;
    for(side=0;side<2;side++)if(!side||ar.count){
        u8 who=side?ar.ids[ar.row]:st.act;
        W->pan_n[side]=0;panel_keys(who+1,W->pan_k[side]);
        for(k=0;k<5;k++)make_panel(side,who);
    }
    if(ar.count)make_forecast(st.act,ar.ids[ar.row],st.abil);
}
static void footprint(void)
{
    const u8 *r=cj_range(common,st.abil),*p;
    u16 c;u8 x=TILE_X[ar.centre],z=TILE_Z[ar.centre],height;
    p=W->surface+((u16)(W->grid_row[z]+x)<<3)+4*node_layer(ar.centre);
    height=(u8)core_area_height(p);memset(st.reach,255,sizeof st.reach);
    for(c=0;c<MAP_N;c++){
        s16 dx=(s16)TILE_X[c]-x,dz=(s16)TILE_Z[c]-z;
        if((dx<0?-dx:dx)+(dz<0?-dz:dz)>r[1])continue;
        p=W->surface+((u16)(W->grid_row[TILE_Z[c]]+TILE_X[c])<<3);
        if(core_area_layer(p,height,r[2],r[4]&32,W->surface_flags&SURFACE_MOVE_UPPER)==node_layer((u8)c)+1)
            st.reach[c]=1;
    }
    hl_seq++;
}
static u8 inspect(void)
{
    u8 i,o;
    if(st.reach[CURSOR_CELL]!=1||!cast_can_order(st.act,(u8)CURSOR_CELL,st.abil,0))return 0;
    memset(&ar.forecast,0,sizeof ar.forecast);
    ar.forecast.actor=st.act;ar.forecast.ability=st.abil;ar.forecast.ref=(u8)CURSOR_CELL;
    if(!cast_forecast(&ar.forecast))return 0;
    ar.centre=(u8)CURSOR_CELL;ar.count=ar.row=ar.top=ar.answer=0;
    for(i=0;i<NUNIT;i++)if(ar.forecast.mask&((u16)1<<i)){
        ar.ids[ar.count++]=i;forecast_text(st.act,i,st.abil,ar.line[i]);
    }
    o=unit_at_cell(ar.centre);
    ar.follow_id=o&&!(cj_range(common,st.abil)[4]&16)?o-1:255;
    ar.phase=AR_TARGETS;footprint();prepare_cards();return 1;
}
static void range(void)
{
    ar.phase=AR_RANGE;ar.answer=0;cursor_place(ar.centre);aim(st.abil);
}

static void area_review_hud(void)
{
    char text[48];u8 k,n=0,allies=0;
    if(ar.baseline||ar.phase==AR_SENT){draw_hud();return;}
    if(ar.phase==AR_RANGE)draw_hud();
    else{
        draw_panel(0);
        if(ar.count){
            u8 t=ar.ids[ar.row];draw_panel(1);draw_forecast();
            if(st.unit[t].team!=TEAM_PLAYER)draw_tag(1,st.unit[t].team==TEAM_GUEST);
        }
    }
    draw_rect(0,0,160,8,C_BLACK);
    n=put_s(text,0,ABIL_NAME(st.abil));
    if(ar.phase==AR_RANGE)n=put_s(text,n," / Select panel");
    else{
        for(k=0;k<ar.count;k++)if(st.unit[ar.ids[k]].team!=TEAM_ENEMY)allies++;
        n=put_s(text,n," / ");n=put_n(text,n,ar.count);n=put_s(text,n," targets");
        if(allies){n=put_s(text,n," / ");n=put_n(text,n,allies);n=put_s(text,n," ally");}
    }
    text[n]=0;ui_text(2,1,text,C_WHITE,158);
    if(ar.phase==AR_TARGETS){
        ChoiceLayout p={61,11,98,0,3,CHOICE_TAB,3,1,0,7,10,8,157,-16,7};
        u8 shown=ar.count>AR_ROWS?AR_ROWS:ar.count;
        p.height=(shown?shown:1)*8+14;choice_frame(&p,"Targets");
        if(!ar.count)choice_row(&p,0,"No targets",C_BLACK);
        for(k=0;k<shown;k++){
            u8 t=ar.ids[ar.top+k];s16 y=p.y+p.row_y+k*p.pitch;
            ui_text(p.x+p.text_x,y,unit_name(t),C_BLACK,104);
            ui_text(106,y,ar.line[t],C_BLACK,157);
        }
        if(ar.count)choice_cursor_animated(&p,ar.row-ar.top,rt_frame);
        for(k=0;k<2;k++)if(k?ar.top+shown<ar.count:ar.top){
            u8 y=k?p.y+p.height-4:p.y+4,r;
            for(r=0;r<3;r++)draw_rect(153-(k?2-r:r),y+r,k?5-2*r:1+2*r,1,C_BLACK);
        }
    }else if(ar.phase==AR_CONFIRM){
        static const char *const follow[]={"Unit","Panel","Cancel"};
        static const char *const fixed[]={"Panel","Cancel"};
        u8 count=ar.follow_id==255?2:3;
        ChoiceLayout p={95,17,64,0,3,CHOICE_TAB,3,1,0,6,10,8,157,-16,7};
        p.height=count*8+14;choice_frame(&p,"Target");
        choice_list(&p,ar.follow_id==255?fixed:follow,count,ar.answer,C_BLACK,C_BLACK,rt_frame);
    }
}

static u8 cluster(u8 *x,u8 *z)
{
    u16 c,best=65535;u8 found=0;
    for(c=0;c<MAP_N;c++){
        u8 cx=TILE_X[c],cz=TILE_Z[c],i;u16 cells[4],score;
        s16 dx=(s16)cx-st.unit[U_RAMZA].x,dz=(s16)cz-st.unit[U_RAMZA].z;
        if(node_layer((u8)c)||cx<1||cx+1>=MAP_W||cz<2||cz+1>=MAP_H||map_tiles[c].walk!=1)continue;
        cells[0]=node_at(cx-1,cz,0);cells[1]=node_at(cx+1,cz,0);
        cells[2]=node_at(cx,cz+1,0);cells[3]=node_at(cx,cz-2,0);
        for(i=0;i<4;i++)if(cells[i]>=MAP_N||map_tiles[cells[i]].walk!=1||map_tiles[cells[i]].stand!=map_tiles[c].stand)break;
        if(i<4)continue;score=(dx<0?-dx:dx)+(dz<0?-dz:dz);
        if(score<best){best=score;*x=cx;*z=cz;found=1;}
    }
    return found;
}
void game_scenario(u16 door)
{
    u8 i,x=0,z=0,ne=0,kind=door>=800?door-800:0;u16 c,best=65535;
    area_original_scenario(61);ar.phase=AR_RANGE;ar.baseline=0;
    st.act=U_RAMZA;st.ai=st.auto_units=st.ai_all=st.banner=0;
    st.moved=st.acted=0;st.mode=M_BROWSE;st.turn=0;st.want=st.prot=255;
    ar.ally=255;
    for(i=0;i<NUNIT;i++)if(i!=st.act){
        if(st.unit[i].team==TEAM_ENEMY&&ne<2)ar.enemy[ne++]=i;
        else if(st.unit[i].team!=TEAM_ENEMY&&ar.ally==255)ar.ally=i;
    }
    CHECK(ne,2);CHECK(ar.ally!=255,1);CHECK(cluster(&x,&z),1);
    c=node_at(x,z,0);ar.centre=(u8)c;
    for(i=0;i<NUNIT;i++){
        st.unit[i].x=st.unit[i].z=255;st.unit[i].dc=DC_GONE;
        st.unit[i].hp=st.unit[i].max_hp=100;st.unit[i].mp=st.unit[i].max_mp=30;
        st.unit[i].ma=5;st.unit[i].faith=70;
        memset(&st.unit[i].statuses,0,sizeof st.unit[i].statuses);st.unit[i].status=0;
        memset(st.affinity[i],0,5);
    }
    unit_place(&st.unit[st.act],(u8)node_at(x,z-2,0));st.unit[st.act].dc=3;
    if(kind!=4){
        unit_place(&st.unit[ar.enemy[0]],kind==1?(u8)c:(u8)node_at(x-1,z,0));
        unit_place(&st.unit[ar.enemy[1]],(u8)node_at(x+1,z,0));
        st.unit[ar.enemy[0]].dc=st.unit[ar.enemy[1]].dc=3;
    }
    if(kind==2||kind==3){
        unit_place(&st.unit[ar.ally],(u8)node_at(x,z+1,0));st.unit[ar.ally].dc=3;
        if(kind==3){st.affinity[ar.ally][0]=128;st.unit[ar.ally].hp=60;}
    }
    // Choose one source camera for the fixture, leaving all tactical positions
    // intact. Before/after share this view; do not hide the review behind a roof.
    for(i=0;i<4;i++){
        u8 j;u16 hidden=0;
        for(j=0;j<NUNIT;j++)if(st.unit[j].x<MAP_W)hidden+=view_hides(i,j);
        if(hidden<best){best=hidden;st.rot=i;}
    }
    st.abil=16;cursor_place(ar.centre);aim(16);W->frz=W->hold=0;
    cam_target(&st.camx,&st.camy);
    printf("review fixture%u centre%u,%u enemies%u,%u ally%u view%u\n",kind,x,z,ar.enemy[0],ar.enemy[1],ar.ally,st.rot);
}
u8 game_update(void)
{
    u32 pressed=rt_keys&~rt_prev;
    if(ar.phase==AR_SENT)return area_original_update();
    if(ar.phase==AR_RANGE){
        if(pressed&(K_A|K_ENTER)){if(!inspect())st.deny=DENY_FRAMES;return 1;}
        if(pressed&(K_ESC|K_B))return 0;
        return area_original_update();
    }
    if(pressed&(K_ESC|K_B)){
        if(ar.phase==AR_CONFIRM)ar.phase=AR_TARGETS;else range();
    }else if(ar.phase==AR_TARGETS){
        u8 old=ar.row;ar.row=choice_move(ar.row,ar.count,pressed);page();
        if(old!=ar.row)prepare_cards();
        if(pressed&(K_A|K_ENTER)){ar.phase=AR_CONFIRM;ar.answer=0;}
    }else{
        u8 count=ar.follow_id==255?2:3;
        ar.answer=choice_move(ar.answer,count,pressed);
        if(pressed&(K_A|K_ENTER)){
            if(ar.answer==count-1)ar.phase=AR_TARGETS;
            else{
                u8 follow=ar.follow_id!=255&&ar.answer==0;
                if(cast_order(st.act,follow?ar.follow_id:ar.centre,st.abil,follow))ar.phase=AR_SENT;
                else st.deny=DENY_FRAMES;
            }
        }
    }
    if(ar.phase!=AR_SENT){
        u32 keys=rt_keys,prev=rt_prev;rt_keys&=K_F1|K_F5;rt_prev&=K_F1|K_F5;
        area_original_update();rt_keys=keys;rt_prev=prev;
    }
    return 1;
}
void game_render(void)
{
    if(ar.phase!=AR_SENT&&!st.turn){
        // Leave the aimed panels beside the right-hand menu. The baseline
        // uses the same camera; tactical positions and projection stay intact.
        s16 u,v;cam_target(&st.camx,&st.camy);
        view_of(st.rot,st.cx,st.cz,&u,&v);st.camx=scene_x(st.rot,u,v)-44;
        cam_clamp(&st.camx,&st.camy);
    }
    area_original_render();
}

#ifndef AREA_REVIEW_GAME
static const char *output;
static void settle(u8 frames)
{
    u8 i;for(i=0;i<frames;i++)sw_step(0);
}
static void press_review(u32 keys){sw_step(keys);sw_step(0);}
static void shot(const char *name)
{
    char path[512];game_render();
    if(!strcmp(name,"before-empty")){
        u8 i;for(i=0;i<NUNIT;i++)if(st.unit[i].x<MAP_W){
            Feet f=unit_feet(st.rot,i);
            printf("review actor%u screen%d,%d hidden%u flags%u\n",i,f.x-st.camx,f.y-st.camy,view_hides_now(i),st.scene_hide);
        }
    }
    snprintf(path,sizeof path,"%s/%s.png",output,name);CHECK(sw_write_png(path,1),0);
}
static void trials(void)
{
    u8 i;u32 seed;Unit before[UNIT_CAPACITY];CastBatch live;
    for(i=0;i<5;i++){
        game_scenario(800+i);settle(24);seed=rng32;memcpy(before,st.unit,sizeof before);live=W->cast;
        press_review(i&1?K_ENTER:K_A);CHECK(ar.phase,AR_TARGETS);CHECK(ar.count,i==4?0:i>=2?3:2);
        CHECK(st.acted,0);CHECK(rng32==seed,1);CHECK(memcmp(before,st.unit,sizeof before)==0,1);
        CHECK(memcmp(&live,&W->cast,sizeof live)==0,1);CHECK(ar.follow_id==255,i!=1);
        press_review(K_B);CHECK(ar.phase,AR_RANGE);CHECK(st.reach[ar.centre],1);
        press_review(K_ENTER);press_review(K_ENTER);CHECK(ar.phase,AR_CONFIRM);
        press_review(K_B);CHECK(ar.phase,AR_TARGETS);CHECK(st.acted,0);
    }
    game_scenario(801);press_review(K_ENTER);press_review(K_ENTER);press_review(K_ENTER);
    CHECK(ar.phase,AR_SENT);CHECK(st.pending[st.act].state,DEFER_CAST_UNIT);
    CHECK(st.pending[st.act].tile,ar.enemy[0]);CHECK(st.unit[st.act].mp,30);
    game_scenario(800);press_review(K_ENTER);press_review(K_ENTER);press_review(K_ENTER);
    CHECK(ar.phase,AR_SENT);CHECK(st.pending[st.act].state,DEFER_CAST_TILE);CHECK(st.pending[st.act].tile,ar.centre);
    game_scenario(800);press_review(K_ENTER);press_review(K_ENTER);st.unit[st.act].mp=0;press_review(K_ENTER);
    CHECK(ar.phase,AR_CONFIRM);CHECK(st.acted,0);CHECK(st.pending[st.act].state,DEFER_NONE);
    ar.count=13;ar.row=ar.top=0;
    for(i=0;i<13;i++){ar.row=choice_move(ar.row,13,K_DOWN);page();CHECK(ar.row>=ar.top&&ar.row<ar.top+AR_ROWS,1);}
    CHECK(ar.row,0);CHECK(ar.top,0);
}
int main(int argc,char **argv)
{
    u8 i;char name[48];if(argc!=2)return 2;output=argv[1];ar.digest=17;
    sw_init(800);trials();
    game_scenario(800);settle(24);ar.baseline=1;shot("before-empty");ar.baseline=0;shot("range-empty");
    press_review(K_ENTER);settle(4);shot("area-empty");press_review(K_ENTER);settle(4);shot("confirm-empty");
    for(i=0;i<32;i++){snprintf(name,sizeof name,"glove-%02u",i);shot(name);sw_step(0);}
    game_scenario(801);settle(24);ar.baseline=1;shot("before-unit");ar.baseline=0;
    press_review(K_ENTER);settle(4);press_review(K_ENTER);settle(4);shot("confirm-unit");
    game_scenario(802);settle(24);press_review(K_ENTER);settle(4);shot("area-ally");
    press_review(K_DOWN);settle(4);shot("detail-enemy");
    game_scenario(803);settle(24);press_review(K_ENTER);settle(4);shot("area-absorb");
    game_scenario(804);settle(24);press_review(K_ENTER);settle(4);shot("area-none");
    printf("area_review %08lX failures%u checks%u\n",(unsigned long)ar.digest,ar.failures,ar.checks);
    return ar.failures!=0;
}
#endif
