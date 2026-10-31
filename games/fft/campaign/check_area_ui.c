// Actual player inputs, owned modal cache, native frames and bounded cold work.
#define AREA_UI_CHECK
#include "check_area_ai.c"
#include "area_ui_fixture.inc"
static u32 pictures[8];
static void ui_tick(u32 keys)
{
    rt_prev=rt_keys;rt_keys=keys;
    ZB(54);CHECK(game_update(),1);ZE(54);
    ZB(55);draw_clear();
    if(st.mode==M_AREA){
#ifdef FFT_MODULES
        mod_call(MOD_AREA_UI,AREA_DRAW,0);
#else
        area_draw(0);
#endif
    }
    ZE(55);rt_frame++;
}
static void ui_key(u32 keys){ui_tick(keys);ui_tick(0);}
static void settle_ui(void){u8 k;for(k=0;k<12;k++)ui_tick(0);}
static void picture(u8 slot)
{
    u16 x,y;u32 h=17;const u8 *l=rt_light,*d=rt_dark;
    for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){
        h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];
    }
    pictures[slot]=h;
}
static void real_inputs(void)
{
    u8 i;u32 seed;Unit before[UNIT_CAPACITY];CastBatch live;u8 reach[256];
    for(i=0;i<5;i++){
        ui_fixture(i);CHECK(st.mode,M_CAST_RANGE);settle_ui();
        if(!i){ui_key(K_APPS);CHECK(st.mode,M_ORDER);ui_key(K_B);CHECK(st.mode,M_CAST_RANGE);}
        seed=rng32;memcpy(before,st.unit,sizeof before);live=W->cast;memcpy(reach,st.reach,256);
        ui_key(i&1?K_ENTER:K_A);CHECK(st.mode,M_AREA);CHECK(W->area_ui.ready,1);
        CHECK(W->area_ui.count,i==4?0:i>=2?3:2);CHECK(W->area_ui.follow_id==255,i!=1);
        CHECK(st.acted,0);CHECK(rng32==seed,1);CHECK(memcmp(before,st.unit,sizeof before)==0,1);
        CHECK(memcmp(&live,&W->cast,sizeof live)==0,1);settle_ui();picture(i);
        if(i==2){
            CastBatch saved=W->area_ui.forecast;u16 mask=saved.mask;
            ui_key(K_DOWN);CHECK(W->area_ui.row,1);CHECK(W->area_ui.forecast.mask,mask);
            CHECK(memcmp(&saved,&W->area_ui.forecast,sizeof saved)==0,1);
        }
        ui_key(K_B);CHECK(st.mode,M_CAST_RANGE);CHECK(memcmp(reach,st.reach,256)==0,1);
        ui_key(K_ENTER);ui_key(K_ENTER);CHECK(st.mode,M_AREA);CHECK(W->area_ui.confirm,1);
        settle_ui();if(i==0)picture(5);if(i==1)picture(6);
        ui_key(K_B);CHECK(W->area_ui.confirm,0);CHECK(st.acted,0);
    }
    ui_fixture(1);ui_key(K_ENTER);ui_key(K_ENTER);ui_key(K_ENTER);
    CHECK(st.pending[st.act].state,DEFER_CAST_UNIT);CHECK(st.pending[st.act].tile,fixture_ui.enemy[0]);
    CHECK(st.acted,1);CHECK(st.unit[st.act].mp,30);CHECK(st.mode,M_MENU);
    ui_fixture(0);ui_key(K_ENTER);ui_key(K_ENTER);ui_key(K_ENTER);
    CHECK(st.pending[st.act].state,DEFER_CAST_TILE);CHECK(st.pending[st.act].tile,fixture_ui.centre);
    ui_fixture(0);ui_key(K_ENTER);ui_key(K_ENTER);st.unit[st.act].mp=0;ui_key(K_ENTER);
    CHECK(st.mode,M_AREA);CHECK(W->area_ui.confirm,1);CHECK(st.acted,0);
    CHECK(st.pending[st.act].state,DEFER_NONE);CHECK(st.unit[st.act].mp,0);
    // Tile-only policy comes from range flags, not from a spell-ID exception.
    ui_fixture(1);aim(17);ui_key(K_ENTER);CHECK(W->area_ui.follow_id,255);
    ui_key(K_ENTER);ui_key(K_ENTER);CHECK(st.pending[st.act].state,DEFER_CAST_TILE);
}
static void invalidation(void)
{
    char expected[28];u8 i;u16 mask;
    ui_fixture(2);ui_key(K_ENTER);mask=W->area_ui.forecast.mask;
    for(i=0;i<W->area_ui.count;i++){
        u8 t=W->area_ui.ids[i];forecast_text(st.act,t,st.abil,expected);
        {u8 n=0;while(expected[n])n++;CHECK(memcmp(W->area_ui.line[i],expected,n+1)==0,1);}
    }
    CHECK(battle_weather(3,0),1);ui_tick(0);CHECK(W->area_ui.weather,st.weather);
    unit_place(&st.unit[fixture_ui.enemy[0]],UNIT_CELL(&st.unit[st.act]));
    CHECK(W->area_ui.dirty,1);ui_tick(0);CHECK(W->area_ui.forecast.mask!=mask,1);
    {CoreStatusDelta d;memset(&d,0,sizeof d);d.add[SID_DARK>>3]=(u8)(0x80>>(SID_DARK&7));
        CHECK(battle_status_apply(fixture_ui.enemy[1],&d,0)!=0,1);CHECK(W->area_ui.dirty,1);ui_tick(0);}
    unit_stats(&st.unit[fixture_ui.enemy[1]]);CHECK(W->area_ui.dirty,1);ui_tick(0);
    // Rotation never changes the tactical centre or the list's victim mask.
    mask=W->area_ui.forecast.mask;ui_key(K_F5);settle_ui();
    CHECK(st.mode,M_AREA);CHECK(W->area_ui.forecast.mask,mask);
    CHECK(CURSOR_CELL,W->area_ui.centre);
}
static void capacity(void)
{
    u8 k;f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
    fixture(0,1);st.ai=AI_OFF;st.moved=1;st.acted=0;st.mode=M_BROWSE;
    aim(18);CHECK(cursor_place(255),1);CHECK(pickable(255,1),1);
    ui_key(K_ENTER);CHECK(st.mode,M_AREA);CHECK(W->area_ui.count,13);
    CHECK(W->area_ui.centre,255);CHECK(W->area_ui.forecast.ref,255);CHECK(MAP_N,256);
    for(k=0;k<12;k++){ui_key(K_DOWN);CHECK(W->area_ui.row>=W->area_ui.top&&W->area_ui.row<W->area_ui.top+4,1);}
    CHECK(W->area_ui.row,12);CHECK(W->area_ui.top,9);settle_ui();picture(7);
    ui_key(K_DOWN);CHECK(W->area_ui.row,0);CHECK(W->area_ui.top,0);
    CHECK(chapter_load(1),1);reset();free(f);f=RT_NULL;
}
static void ui_trials(void){real_inputs();invalidation();capacity();}
#ifndef __m68k__
int main(void)
{
    u8 i;sw_init(61);ui_trials();
    printf("area_ui %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    for(i=0;i<8;i++)printf("picture%u %08lX\n",i,(unsigned long)pictures[i]);return failures!=0;
}
#else
void _main(void){u8 i;reaction_runtime_main();ui_trials();for(i=0;i<8;i++)BENCH_VALUE(pictures[i]);BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
