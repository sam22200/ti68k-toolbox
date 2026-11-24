// Included after the actual battle engine by check_zeklaus.py's private carrier.
// Source terrain/C2 artwork; borrowed actors are diagnostic, not an encounter rip.
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include "../../../runtime/platform-sw/rt_sw.h"

static unsigned long tested;
#define VERIFY(c) do { tested++; if(!(c)){fprintf(stderr,"line %u: %s\n",(unsigned)__LINE__,#c);abort();} } while(0)

static void frame(u32 keys) { VERIFY(sw_step(keys)); }

static void view_pixels(u8 rot)
{
    char name[40];FILE *file;u8 *expected,*actual;unsigned size=6U*SC_PLANE;
    expected=malloc(size);actual=malloc(4U*SC_PLANE);VERIFY(expected&&actual);
    snprintf(name,sizeof name,"expected-%u.bin",rot);file=fopen(name,"rb");VERIFY(file);
    VERIFY(fread(expected,1,size,file)==size);VERIFY(!fclose(file));
    unpack_view(rot);VERIFY(!memcmp(scene_l,expected,2U*SC_PLANE));
    depth_planes(actual);VERIFY(!memcmp(actual,expected+2U*SC_PLANE,4U*SC_PLANE));
    free(expected);free(actual);
}

static void cells(u8 rot)
{
    u16 cell;unsigned stops=0;Unit saved=st.unit[U_RAMZA];
    st.rot=rot;st.mode=M_BROWSE;
    for(cell=0;cell<MAP_N;cell++){
        s16 u,v,x,z;Feet f;u16 mask[COVER_H];
        VERIFY(node_at(TILE_X[cell],TILE_Z[cell],0)==cell);
        view_of(rot,TILE_X[cell],TILE_Z[cell],&u,&v);
        world_of(rot,u,v,&x,&z);VERIFY(x==TILE_X[cell]&&z==TILE_Z[cell]);
        VERIFY(cursor_place((u8)cell));VERIFY(CURSOR_CELL==cell);
        VERIFY(unit_place(&st.unit[U_RAMZA],(u8)cell));f=unit_feet(rot,U_RAMZA);
        // Every source cell centre, including the four formerly clipped corners.
        VERIFY(f.x>=8&&f.x+8<=SC_W);VERIFY(f.y>=0&&f.y<SC_H);
        cover(mask,f.x,f.y,f.depth);
        cam_target(&st.camx,&st.camy);
        VERIFY(f.x-st.camx>=8&&f.x-st.camx+8<=RT_W);
        VERIFY(f.y-st.camy>=UNIT_FOOT&&f.y-st.camy<RT_H);
        if(map_tiles[cell].walk==1)stops++;
    }
    VERIFY(stops==176);st.unit[U_RAMZA]=saved;
    printf("view %u source cells %u stoppable %u colors/depth exact; edges/covers/camera pass\n",rot+1,(unsigned)MAP_N,stops);
}

static void arrows(u8 rot)
{
    static const u32 keys[]={K_LEFT,K_RIGHT,K_UP,K_DOWN};
    static const s16 du[]={0,0,-1,1},dv[]={-1,1,0,0};
    u8 i;st.rot=rot;st.mode=M_BROWSE;
    for(i=0;i<4;i++){
        s16 u,v,x,z;VERIFY(cursor_place((u8)node_at(4,8,0)));
        view_of(rot,4,8,&u,&v);world_of(rot,u+du[i],v+dv[i],&x,&z);
        frame(0);frame(keys[i]);VERIFY(st.cx==x&&st.cz==z);
        if(rot==0&&i==1)VERIFY(st.cx==4&&st.cz==9); // observed source handedness
    }
    frame(0);
}

static void turns(void)
{
    u8 i;st.rot=0;st.mode=M_BROWSE;cursor_place((u8)node_at(4,8,0));
    for(i=0;i<8;i++){
        u8 old=st.rot;unsigned count=0;frame(i<4?K_F5:K_F1);VERIFY(st.turn!=0);
        while(st.turn&&count++<40)frame(0);
        VERIFY(!st.turn);VERIFY(st.rot==((old+(i<4?1:3))&3));
        VERIFY(st.cx==4&&st.cz==8);frame(0);
    }
    printf("input 16 directions and 8 animated rotations pass\n");
}

static void walk(void)
{
    u16 cell,target=256;u8 i;unsigned frames=0;Unit *u=&st.unit[U_RAMZA];
    st.rot=0;st.mode=M_MENU;st.act=st.sel=U_RAMZA;st.ai=AI_OFF;
    compute_reach(U_RAMZA);
    for(cell=0;cell<MAP_N;cell++)if(st.reach[cell]>0&&st.reach[cell]!=255&&!unit_at_cell((u8)cell)){
        if(make_path_cell(U_RAMZA,(u8)cell)>=2){target=cell;break;}
    }
    VERIFY(target<MAP_N);VERIFY(st.path_n>=2);
    for(i=0;i<st.path_n;i++)VERIFY(PATH_CELL(i)<MAP_N);
    st.mode=M_WALK;st.path_i=st.walk_t=0;
    while(st.mode==M_WALK&&frames++<200)frame(0);
    VERIFY(st.mode!=M_WALK);VERIFY(UNIT_CELL(u)==target);
    printf("source terrain movement path %u cells, %u frames, destination %u pass\n",st.path_n,frames,target);
}

static void transition(void)
{
    game_scenario(161);VERIFY(W&&SC_BYTES==40&&SC_H==202&&scene_capacity==16160);
    VERIFY((u8 *)W==sbuf+scene_capacity);frame(0);
    fail_scene_bytes=16544U+sizeof(Work);
    game_scenario(261);VERIFY(!W&&!sbuf&&!scene_l&&!scene_d&&!scene_capacity);
    VERIFY(vbuf&&acache);fail_scene_bytes=0;
    game_scenario(261);VERIFY(W&&SC_BYTES==44&&SC_H==188&&scene_capacity==16544);
    VERIFY((u8 *)W==sbuf+scene_capacity);frame(0);
    printf("352x188 -> 320x202 -> failed growth -> successful retry; buffers/Work pass\n");
}

int main(int argc,char **argv)
{
    u8 rot,pass;VERIFY(argc==2);mkdir(argv[1],0777);setbuf(stdout,NULL);
    for(pass=0;pass<2;pass++){
        sw_init(261);VERIFY(W&&sbuf&&vbuf&&navbuf&&acache);
        VERIFY(chapter_now==2&&MAP_W==10&&MAP_H==18&&MAP_N==180);
        VERIFY(SC_W==352&&SC_BYTES==44&&SC_H==188&&SC_TOP==54&&nav_capacity==3600);
        st.open_stage=st.open_fade=st.banner=st.turn=0;st.ai=AI_OFF;st.want=st.prot=255;
        for(rot=0;rot<4;rot++){view_pixels(rot);cells(rot);arrows(rot);}
        turns();walk();
        for(rot=0;rot<4;rot++){
            char name[4096];st.rot=rot;st.mode=M_BROWSE;cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));
            cam_target(&st.camx,&st.camy);scene_rot=255;game_render();
            if(!pass){snprintf(name,sizeof name,"%s/view-%u.png",argv[1],rot);VERIFY(!sw_write_png(name,1));}
            printf("render %u checksum %04X\n",rot+1,sw_checksum());
        }
        transition();final_release();VERIFY(!battle_state&&!rt_state&&!sbuf&&!vbuf&&!navbuf&&!acache);
        printf("reentry %u released all owned battle buffers\n",pass+1);
    }
    printf("zeklaus wide PC checks %lu failures 0\n",tested);return 0;
}
