// Real cold battle child for the private chain; no render or hardware loop.
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#include "flow_battle.c"
static u16 checks,failures,actions,guest_actions;static u32 digest=17;
static void check(u8 ok,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^ok;
    if(!ok){failures++;printf("chain-battle failure line%u frame%u\n",line,rt_frame);}
}
#define CHECK(x) check(!!(x),__LINE__)
static void snapshot(u16 ordinal,const char *suffix,const Save *s)
{
    char name[64];FILE *f;sprintf(name,"battle-%u-%s.sav",ordinal,suffix);
    f=fopen(name,"wb");CHECK(f!=RT_NULL);if(f){CHECK(fwrite(s,1,sizeof *s,f)==sizeof *s);fclose(f);}
}
int main(int argc,char **argv)
{
    Save entry,exit;u16 scenario,ordinal,frames=0;u8 running=1,i,losing;
    if(argc!=4)return 111;
    scenario=(u16)atoi(argv[1]);losing=!strcmp(argv[2],"lose");ordinal=(u16)atoi(argv[3]);
    sw_init(scenario);CHECK(W&&flow_bank&&W->goal);if(!W)return 111;
    CHECK(save_load(&entry));snapshot(ordinal,"before",&entry);
    CHECK(entry.n==16&&st.party_in&&NUNIT==11&&CD(PARTY)==3);
    CHECK(st.unit[0].team==TEAM_GUEST&&!strcmp(unit_name(0),"Delita"));
    CHECK(st.unit[0].level==unit_defs[0].level&&st.unit[0].hp>0);
    CHECK(!unit_party_member(&st.unit[0])&&unit_home_team(&st.unit[0])!=TEAM_ENEMY);
    CHECK(RD16(W->goal,1)==0&&RD16(W->goal,2)==1);
    for(i=6;i<NUNIT;i++)CHECK(st.unit[i].slot<5); // forced guest consumed no deployed slot
    if(losing){
        // A deliberate source-packet injection at the action door. The actual
        // commit, protection failure, result exit and no-save behavior run normally.
        st.act=st.atk=1;st.ai=AI_OFF;memset(st.reaction,0,sizeof st.reaction);
        attack(1,0);st.hit=1;st.dmg=999;st.push=st.self=0;
    }
    while(running&&frames<30000){
        u8 old=st.mode;
        W->hud_ok=1;rt_prev=rt_keys;
        rt_keys=st.mode==M_END&&!(frames&7)?(losing?K_ESC:K_ENTER):0;
        running=game_update();
        if(st.mode==M_ACT&&old!=M_ACT){actions++;if(st.unit[st.atk].team==TEAM_GUEST)guest_actions++;}
        if(st.mode==M_ACT&&st.unit[st.act].team==TEAM_GUEST)CHECK(st.ai!=AI_OFF);
        rt_frame++;frames++;
    }
    CHECK(!running&&frames<30000);CHECK(st.end==(losing?END_DEFEAT:END_VICTORY));
    CHECK(save_load(&exit));snapshot(ordinal,"after",&exit);
    if(losing){CHECK(!memcmp(&entry,&exit,sizeof entry));CHECK(!st.open_handoff&&!st.reward_ready);}
    else{
        CHECK(st.open_handoff&&st.reward_ready&&st.reward_page==3);
        CHECK(exit.n==16);CHECK(actions>0&&guest_actions>0);
        CHECK(qlong(exit.money)==qlong(entry.money)+st.reward_base+st.reward_bonus);
        CHECK(exit.flags[(CD(WON)-1)>>3]&(1U<<((CD(WON)-1)&7)));
        for(i=5;i<16;i++)CHECK(!memcmp(&entry.unit[i],&exit.unit[i],sizeof(SaveUnit)));
        for(i=6;i<NUNIT;i++){
            const Unit *u=&st.unit[i];u8 career=(u8)(u->job-0x4a);if(career>=CAREER_N)career=0;
            if(u->slot<exit.n&&!DC_OBJ(u->dc)){
                CHECK(exit.unit[u->slot].exp==u->exp&&exit.unit[u->slot].level==u->level);
                CHECK(qword(exit.unit[u->slot].jp[career])==u->jp);
            }
        }
    }
    printf("chain-battle %u %s frames%u actions%u guest-actions%u hash%08lX checks%u failures%u\n",
           scenario,losing?"lose":"win",frames,actions,guest_actions,(unsigned long)digest,checks,failures);
    final_release();return failures?111:!losing;
}
