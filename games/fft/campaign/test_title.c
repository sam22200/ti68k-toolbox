#include "title.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
static Save sample;
static void reset_files(void)
{
    u8 i;remove("ffslot.sav");remove("ffsave.sav");
    for(i=1;i<=3;i++){char name[24];snprintf(name,sizeof name,"%s.sav",save_slot_name(i));remove(name);}
}
static void press(u32 key){assert(sw_step(key));assert(sw_step(0));}
static void finish(u8 expected){assert(title_finish()==expected&&title_verified()==expected);title_cleanup();}
int main(void)
{
    char origin[PATH_MAX],folder[]="/tmp/fft-title-test-XXXXXX",path[PATH_MAX];Save back;
    FILE *f;u8 i;
    assert(getcwd(origin,sizeof origin));assert(mkdtemp(folder));
    for(i=0;i<2;i++) {
        const char *name=i?"ffmg.bin":"fftitle.bin";
        snprintf(path,sizeof path,"%s/data/%s",origin,name);char target[PATH_MAX];
        snprintf(target,sizeof target,"%s/%s",folder,name);assert(!symlink(path,target));
    }
    assert(!chdir(folder));sw_init(0);assert(title_bank&&title_mg&&title_back&&title_draft);
    assert(title.screen==TITLE_MAIN&&!strcmp(SAVE_NAME,SAVE_LEGACY_NAME));
    press(K_DOWN);press(K_ENTER);assert(title.action&&title.screen==TITLE_FILES);
    press(K_UP);assert(title.row==2);press(K_ENTER);assert(title.notice==1&&!title.result);
    press(K_B);press(K_B);assert(title.screen==TITLE_MAIN);press(K_UP);press(K_ENTER);
    press(K_DOWN);assert(title.row==1);assert(!sw_step(K_ENTER));finish(1);
    assert(!strcmp(SAVE_NAME,"ffsave2")&&save_load(&back)&&!back.n);
    assert(!save_load_from("ffsave1",&sample)&&!save_load_from("ffsave3",&sample));
    // Every caller's ordinary SAVE_NAME now addresses only the chosen slot.
    back.n=1;back.unit[0].type=2;back.unit[0].level=17;back.flags[0]=3;back.loc=7;
    assert(rt_save(SAVE_NAME,&back,sizeof back));sample=back;sample.unit[0].level=31;
    assert(rt_save("ffsave3",&sample,sizeof sample));
    sw_init(0);assert(title_slots[1].level==17&&title_slots[2].level==31);
    press(K_ENTER);press(K_DOWN);press(K_ENTER);assert(title.screen==TITLE_OVERWRITE&&title.answer==1);
    press(K_ENTER);assert(title.screen==TITLE_FILES);assert(save_load_from("ffsave2",&back)&&back.unit[0].level==17);
    press(K_ENTER);press(K_UP);assert(!sw_step(K_ENTER));finish(1);
    assert(save_load_from("ffsave2",&back)&&!back.n);
    assert(save_load_from("ffsave3",&back)&&back.unit[0].level==31);
    sw_init(0);press(K_DOWN);press(K_ENTER);press(K_DOWN);press(K_DOWN);
    assert(!sw_step(K_ENTER));finish(2);assert(!strcmp(SAVE_NAME,"ffsave3"));
    assert(save_load(&back)&&back.unit[0].level==31);
    // Map Data calls this exact file module. Full snapshot copy, selected slot,
    // explicit overwrite and two-stage metadata write leave other files intact.
    sw_init(3);assert(title.action==2&&title.row==2&&title_save_ready);
    assert(sw_step(K_ENTER)&&title.screen==TITLE_OVERWRITE&&title.answer==1);
    assert(sw_step(0));
    press(K_ENTER);assert(title.screen==TITLE_FILES&&title_draft->unit[0].level==31);
    press(K_UP);assert(title.row==1); // slot2 already exists, requires explicit overwrite
    press(K_ENTER);assert(title.screen==TITLE_OVERWRITE);press(K_UP);
    assert(!sw_step(K_A));finish(3);assert(!strcmp(SAVE_NAME,"ffsave2"));
    assert(save_load(&back)&&back.unit[0].level==31&&save_load_from("ffsave3",&sample)&&sample.unit[0].level==31);
    sw_init(3);assert(!sw_step(K_B)&&!title.result);title_cleanup();
    // Import an old-format legacy save into slot1 only on an explicit Continue.
    reset_files();memcpy(sample.magic,"FSV2",4);sample.unit[0].level=22;
    assert(rt_save("ffsave",&sample,SAVE_V2_BYTES));sw_init(2);
    assert(title_slots[0].legacy&&title_slots[0].level==22);assert(!sw_step(K_ENTER));finish(2);
    assert(!strcmp(SAVE_NAME,"ffsave1")&&save_load(&back)&&back.unit[0].level==22&&back.money[2]==7&&back.money[3]==208);
    f=fopen("ffsave.sav","rb");assert(f);assert(fseek(f,0,SEEK_END)==0&&ftell(f)==SAVE_V2_BYTES);fclose(f);
    // Invalid metadata never redirects a save; corrupt data cannot be continued.
    {const u8 bad[]={ 'F','S','L','1',4,0 };assert(rt_save(SAVE_SELECT_NAME,bad,sizeof bad));}
    assert(!strcmp(SAVE_NAME,SAVE_LEGACY_NAME));reset_files();memcpy(sample.magic,"BAD!",4);
    assert(rt_save("ffsave3",&sample,sizeof sample));sw_init(2);assert(title_slots[2].kind==SLOT_DAMAGED);
    press(K_UP);press(K_ENTER);assert(title.notice==2&&!title.result);
    title_cleanup();reset_files();remove("fftitle.bin");remove("ffmg.bin");assert(!chdir(origin));assert(!rmdir(folder));
    puts("Title: three slots, selection, isolated writes, overwrite cancellation/confirmation, legacy migration, invalid data and metadata passed");
    return 0;
}
