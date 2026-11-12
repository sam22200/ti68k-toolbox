// Full production-update replay on source Sweegy terrain. No render or hardware path.
#include "flow_battle.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>

static u16 checks, failures, actions, monster_actions, movements, hp_changes;
static u32 digest=17;
static Save initial, settled;
static Unit previous[FFT_CORE_UNIT_MAX];

static void check(u8 ok,u16 line)
{
    checks++;
    if(!ok){failures++;printf("failure line %u frame %lu\n",line,(unsigned long)rt_frame);}
}
#define CHECK(v) check(!!(v),__LINE__)
static void hash(u32 value){digest=(digest<<1|digest>>31)^value;}

static void state_sample(void)
{
    u8 i,k;
    hash(rt_frame);hash(st.act);hash(st.mode);hash(st.ai);hash(st.abil);hash(st.end);hash(rng32);
    for(i=0;i<NUNIT;i++){
        const Unit *u=&st.unit[i];
        hash(u->hp);hash(u->mp);hash(u->ct);hash(u->x);hash(u->z);hash(u->dc);
        hash(u->level);hash(u->exp);hash(u->jp);hash(u->learned);
        for(k=0;k<5;k++){
            hash(u->statuses.inflicted[k]);
            if(u->statuses.inflicted[k]!=previous[i].statuses.inflicted[k])
                printf("{\"event\":\"status\",\"frame\":%lu,\"unit\":%u,\"byte\":%u,\"from\":%u,\"to\":%u}\n",
                       (unsigned long)rt_frame,i,k,previous[i].statuses.inflicted[k],u->statuses.inflicted[k]);
        }
        if(u->hp!=previous[i].hp){
            printf("{\"event\":\"hp\",\"frame\":%lu,\"unit\":%u,\"from\":%u,\"to\":%u}\n",
                   (unsigned long)rt_frame,i,previous[i].hp,u->hp);
            hp_changes++;
        }
        if(u->x!=previous[i].x||u->z!=previous[i].z){
            movements++;
            CHECK(u->dc==DC_GONE||(u->x<MAP_W&&u->z<MAP_H));
            if(u->dc!=DC_GONE&&u->x<MAP_W&&u->z<MAP_H)CHECK(map_tiles[UNIT_CELL(u)].walk);
        }
        previous[i]=*u;
    }
}

static void initial_checks(void)
{
    static const u8 monster_jobs[6]={97,97,98,100,100,103};
    u8 i,k;
    CHECK(W&&flow_bank&&cdesc);if(!W||!flow_bank||!cdesc)return;
    CHECK(MAP_W==11&&MAP_H==12&&NUNIT==13&&U_RAMZA==0);
    CHECK(CD(LOCATION)!=7&&CD(PARTY)==3&&CD(WON)==8);
    CHECK(st.party_in&&st.ai_all&&!st.end&&!story_after);
    CHECK(rt_load(SAVE_NAME,&initial,sizeof initial));
    for(i=0;i<NUNIT;i++){
        const Unit *u=&st.unit[i];
        CHECK(u->hp&&u->x<MAP_W&&u->z<MAP_H);
        CHECK(unit_at_cell(UNIT_CELL(u))==i+1);
        if(i<5){CHECK(u->team==TEAM_PLAYER&&u->slot<initial.n);}
        else if(i<7){CHECK(u->team==TEAM_GUEST);}
        else{
            CHECK(u->job==monster_jobs[i-7]&&unit_monster(u));
            CHECK(u->team==TEAM_ENEMY&&u->zodiac==12&&u->learned==7);
            CHECK(u->set==job_sets[u->job]);
            for(k=0;k<EQ_N;k++)CHECK(u->eq[k]==255);
            for(k=0;k<3;k++)if(HAS_ABILITY(SET_ABILITY(u->set,k)))
                CHECK(abilities[SET_ABILITY(u->set,k)].kind!=255);
            if(u->job==100)CHECK(u->statuses.innate[2]&64);
        }
        printf("{\"event\":\"initial\",\"unit\":%u,\"job\":%u,\"level\":%u,\"hp\":%u,"
               "\"pa\":%u,\"ma\":%u,\"move\":%u,\"jump\":%u,\"x\":%u,\"z\":%u}\n",
               i,u->job,u->level,u->hp,u->pa,u->ma,u->move,u->jump,u->x,u->z);
        previous[i]=*u;
    }
}

static void replay(u32 seed)
{
    u8 running=1,old_mode=255;u16 limit=30000; // rt_frame is u16; keep the bound below wrap
    rand_seed(seed);
    initial_checks();if(failures)return;
    while(running&&rt_frame<limit){
        u32 keys=st.mode==M_END&&!(rt_frame&7)?K_ENTER:0;
        // The rules-only driver acknowledges the HUD just as a completed frame
        // does. No unit, action, AI choice, HP or CT is injected during combat.
        W->hud_ok=1;
        rt_prev=rt_keys;rt_keys=keys;
        old_mode=st.mode;
        running=game_update();
        if(st.mode==M_ACT&&old_mode!=M_ACT){
            actions++;if(unit_monster(&st.unit[st.atk]))monster_actions++;
            printf("{\"event\":\"action\",\"frame\":%lu,\"actor\":%u,\"job\":%u,"
                   "\"ability\":%u,\"target\":%u,\"mask\":%u}\n",
                   (unsigned long)rt_frame,st.atk,st.unit[st.atk].job,st.abil,st.tgt,W->cast.mask);
        }
        state_sample();rt_frame++;
        if(!(rt_frame&4095))printf("{\"event\":\"progress\",\"frame\":%lu,\"mode\":%u,\"ai\":%u,\"act\":%u}\n",
                                  (unsigned long)rt_frame,st.mode,st.ai,st.act);
    }
    printf("{\"event\":\"end\",\"running\":%u,\"mode\":%u,\"ai\":%u,\"act\":%u,"
           "\"end\":%u,\"rounds\":%u,\"reward_base\":%lu,\"reward_bonus\":%lu}\n",
           running,st.mode,st.ai,st.act,st.end,st.rounds,
           (unsigned long)st.reward_base,(unsigned long)st.reward_bonus);
    CHECK(!running&&rt_frame<limit);
    CHECK(st.end==END_VICTORY||st.end==END_DEFEAT);
    CHECK(actions>5&&monster_actions>0&&movements>0&&hp_changes>0);
    CHECK(rt_load(SAVE_NAME,&settled,sizeof settled));
    if(st.end==END_VICTORY){
        u32 funds=qlong(settled.money);u8 i,k,progress=0;
        CHECK(st.open_handoff&&st.reward_ready&&st.reward_page==3);
        CHECK(settled.flags[0]==(initial.flags[0]|128));
        CHECK(settled.loc==CD(LOCATION)&&settled.deploy_n==0);
        CHECK(funds==qlong(initial.money)+st.reward_base+st.reward_bonus);
        CHECK(settled.n>0&&settled.n<=initial.n);
        for(i=0;i<5;i++){
            const Unit *u=&st.unit[i];const SaveUnit *s;u8 career=(u8)(u->job-0x4a);
            if(u->slot>=settled.n||DC_OBJ(u->dc))continue;
            s=&settled.unit[u->slot];if(career>=CAREER_N)career=0;
            CHECK(s->level==u->level&&s->exp==u->exp);
            CHECK(qword(s->jp[career])==u->jp&&qword(s->tjp[career])==u->tjp);
            progress|=!!(u->gexp||u->gjp);
        }
        CHECK(progress);
        // Slots 5 and 6 were not deployed: compaction may move them, never edit them.
        for(i=5;i<initial.n;i++){
            for(k=0;k<settled.n;k++)if(!memcmp(&initial.unit[i],&settled.unit[k],sizeof(SaveUnit)))break;
            CHECK(k<settled.n);
        }
        // A repeated receipt cannot pay the same encounter flag twice.
        save_campaign();CHECK(rt_load(SAVE_NAME,&settled,sizeof settled));
        CHECK(qlong(settled.money)==funds);
    }else{
        CHECK(!st.open_handoff);
        CHECK(!memcmp(&initial,&settled,sizeof initial));
    }
    printf("{\"frames\":%lu,\"actions\":%u,\"monster_actions\":%u,\"movements\":%u,"
           "\"hp_changes\":%u,\"outcome\":%u,\"checks\":%u,\"failures\":%u,\"hash\":\"%08lX\"}\n",
           (unsigned long)rt_frame,actions,monster_actions,movements,hp_changes,st.end,checks,failures,
           (unsigned long)digest);
}

int main(int argc,char **argv)
{
    setvbuf(stdout,0,_IOLBF,0);
    sw_init(216);
    replay(argc>1?(u32)strtoul(argv[1],0,0):0x9e3779b1UL);
    final_release();
    return failures!=0;
}
