#include "deploy.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    Save old,back;u8 i,cells[8],n=0;u8 reserve[2][sizeof(SaveUnit)];
    if(access("ffflow.bin",R_OK))assert(!chdir("data"));
    remove("ffsave.sav");sw_init(0);
    assert(sizeof(SaveUnit)==176&&sizeof(Save)==SAVE_V5_BYTES+10+GUEST_MAX*sizeof(SaveUnit));
    assert(save.n==7&&qlong(save.money)==2000&&dp.n==1&&deploy_where(0)==8);
    assert(sw_step(K_F1)&&dp.unit==6);assert(sw_step(0));
    assert(sw_step(K_F5)&&dp.unit==0);assert(sw_step(0));
    // Real direction inputs cannot leave the legal bitmap or raise a tile error.
    for(i=0;i<80;i++) {
        static const u32 arrows[4]={K_LEFT,K_UP,K_RIGHT,K_DOWN};
        assert(sw_step(arrows[(i/5)&3])&&sw_step(0));
        assert(deploy_cell((u8)(dp.row*5+dp.col))[0]&&!dp.note);
    }
    deploy_init();assert(sw_step(0));
    assert(sw_step(K_F1)&&dp.unit==6&&sw_step(0));
    assert(sw_step(K_LEFT)&&sw_step(0));assert(sw_step(K_A)&&dp.n==2&&sw_step(0));
    deploy_init();assert(sw_step(0)&&dp.n==1);
    for(i=0;i<25;i++)if(deploy_cell(i)[0])cells[n++]=i;
    assert(n==8&&deploy_ready());
    dp.row=dp.col=0;assert(!deploy_place()&&dp.n==1);
    for(i=0;i<5;i++){dp.unit=i;dp.row=cells[i]/5;dp.col=cells[i]%5;assert(deploy_place());}
    assert(dp.n==5&&deploy_ready());
    // Confirmation is a real choice menu: wrap, return, SHIFT/ESC cancellation,
    // no file/state commit until Start Battle is explicitly selected.
    assert(sw_step(0)&&sw_step(K_C)&&dp.confirm==1);
    {
        Save frozen=save;Deployment placed=dp;
        u16 first=sw_checksum();u8 f,moved=0;
        for(f=0;f<32;f++){assert(sw_step(0));moved|=sw_checksum()!=first;}
        assert(moved&&sw_checksum()==first);
        assert(!memcmp(&frozen,&save,sizeof save)&&!memcmp(&placed,&dp,sizeof dp));
    }
    assert(sw_step(0)&&sw_step(K_UP)&&dp.confirm==2);
    assert(sw_step(0)&&sw_step(K_DOWN)&&dp.confirm==1);
    assert(sw_step(0)&&sw_step(K_DOWN)&&dp.confirm==2);
    assert(sw_step(0)&&sw_step(K_ENTER)&&!dp.confirm&&!dp.done);
    assert(access("ffsave.sav",F_OK));
    assert(sw_step(0)&&sw_step(K_C)&&dp.confirm==1);
    assert(sw_step(0)&&sw_step(K_ESC)&&!dp.confirm&&!dp.done);
    assert(sw_step(0)&&sw_step(K_C)&&dp.confirm==1);
    assert(sw_step(0)&&sw_step(K_B)&&!dp.confirm&&!dp.done);
    assert(sw_step(0)&&sw_step(K_D)&&dp.confirm==1); // physical ALPHA
    assert(sw_step(0)&&sw_step(K_B)&&!dp.confirm);
    assert(save.flow!=2);
    dp.unit=5;dp.row=cells[5]/5;dp.col=cells[5]%5;assert(!deploy_place()&&dp.n==5);
    memcpy(reserve[0],&save.unit[5],sizeof(SaveUnit));memcpy(reserve[1],&save.unit[6],sizeof(SaveUnit));
    assert(deploy_commit());assert(rt_load(SAVE_NAME,&back,sizeof back));
    assert(back.n==7&&back.deploy_n==5&&back.flow==2&&qword(back.deploy_battle)==200);
    assert(!memcmp(reserve[0],&back.unit[5],sizeof(SaveUnit))&&!memcmp(reserve[1],&back.unit[6],sizeof(SaveUnit)));
    sw_init(0);assert(dp.n==5&&save.n==7&&deploy_ready());
    // Mandatory Ramza is checked on confirmation, including a partially filled squad.
    dp.placed[deploy_where(0)]=255;dp.n--;assert(!deploy_commit());
    // Actual old-format load: same full roster/flags/items, new balance initialized.
    old=back;old.flags[3]=0x40;old.inv[7]=19;memcpy(old.magic,"FSV2",4);
    assert(rt_save(SAVE_NAME,&old,SAVE_V2_BYTES));
    assert(save_load(&back)&&back.flags[3]==0x40&&back.inv[7]==19&&back.n==7&&qlong(back.money)==2000);
    assert(!memcmp(old.unit,back.unit,sizeof old.unit));
    remove("ffsave.sav");puts("Deployment: source Ramza/six recruits, automatic mandatory slot, legal grid, capacity, reserves, explicit save, resume and FSV2 migration passed");
    return 0;
}
