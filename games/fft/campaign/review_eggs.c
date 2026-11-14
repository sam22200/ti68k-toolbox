// PC-only visual prototype. No production save/schema, renderer or package edits.
#include <assert.h>
#include <stdio.h>
#include "manage.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"
static const u8 *egg_bank;
static u8 egg_timer[PARTY_MAX],egg_color[PARTY_MAX];
static void egg_actor(s16 x,s16 y,u8 slot)
{
    const u8 *p=egg_bank+8+((u16)egg_color[slot]*2+((egg_timer[slot]&15)<2))*156;
    u16 rows[3][26];u8 i,j;
    RtSprite sprite={16,26,rows[0],rows[1],rows[2]};
    for(i=0;i<3;i++)for(j=0;j<26;j++)rows[i][j]=b16(p+52*i+2*j);
    draw_sprite(x,y,&sprite);
}
static void egg_formation(u8 selected)
{
    u8 i,left=egg_timer[selected]&15,total=egg_timer[selected]>>4;
    mg.unit=selected;mg.top=selected>=10?1:0;mg.screen=S_FORM;
    page(); // reviewed formation layout/humans, then replace egg-only regions
    for(i=7;i<10;i++)egg_actor(8+(i%5)*32,7+(i/5)*27,i);
    // The existing diamond still goes under the egg's feet, in the same cell.
    floor_diamond(16+(selected%5)*32,29+(selected/5)*27);
    egg_actor(8+(selected%5)*32,7+(selected/5)*27,selected);
    draw_rect(0,63,160,37,C_WHITE);
    egg_actor(7,64,selected);
    black(30,66,"Monster Egg");
    number(30,76,left);black(37,76,left==1?"day to hatch":"days to hatch");
    draw_rect(30,85,122,5,C_BLACK);draw_rect(31,86,120,3,C_WHITE);
    draw_rect(31,86,(s16)((u16)(total-left)*120/total),3,C_DGRAY);
    black(37,95,"ALPHA:menu SHIFT:back");
}
static void egg_menu(u8 confirmation)
{
    const ChoiceLayout menu={100,39,56,21,3,CHOICE_TAB,3,1,0,6,10,6,152,-16,7};
    const ChoiceLayout confirm={92,33,64,27,3,CHOICE_TAB,3,1,0,6,10,6,152,-16,7};
    egg_formation(7);
    if(confirmation){
        const char *labels[]={"No","Yes"};
        choice_frame(&confirm,"Dismiss?");choice_list(&confirm,labels,2,0,C_BLACK,C_BLACK,0);
    }else{
        choice_frame(&menu,"Egg");choice_row(&menu,0,"Dismiss",C_BLACK);choice_cursor_animated(&menu,0,0);
    }
}
static void shot(const char *directory,const char *name)
{
    char path[4096];snprintf(path,sizeof path,"%s/%s.png",directory,name);
    assert(!sw_write_png(path,1));printf("%s %04X\n",name,sw_checksum());
}
int main(int argc,char **argv)
{
    FILE *file;u16 size;u8 i;
    if(argc!=3)return 2;
    sw_init(0);assert(bank&&common&&cache);
    file=fopen(argv[2],"rb");assert(file);assert(read_save_fixture(file,&save));fclose(file);
    assert(save.n>=7);save.n=10;
    egg_bank=rt_file("ffeg",&size);assert(egg_bank&&size==1256&&!memcmp(egg_bank,"FEG1",4));
    for(i=7;i<10;i++){
        memset(&save.unit[i],0,sizeof save.unit[i]);save.unit[i].type=3;save.unit[i].job=94;
        save.unit[i].level=1;save.unit[i].gfx=255;
        egg_timer[i]=i==7?0x32:i==8?0x31:0x55;egg_color[i]=i-7;
    }
    mg.unit=7;mg.screen=S_FORM;page();formation_cursor();shot(argv[1],"before");
    egg_formation(7);shot(argv[1],"egg");
    egg_menu(0);shot(argv[1],"menu");egg_menu(1);shot(argv[1],"confirm");
    free(cache);cache=RT_NULL;return 0;
}
