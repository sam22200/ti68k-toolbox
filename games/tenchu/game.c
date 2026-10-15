#include "engine.h"
#include <string.h>
void game_init(void) {
    memset(&te_state,0,sizeof(te_state));
    te_state.nsprites=5;te_state.target=te_state.hit=-1;
    rt_state=&te_state;rt_state_size=sizeof(te_state);
    if(!te_world_init()){te_state.moving=255;return;}
    te_scene(0);te_camera_defaults(0);
    if(!te_render_init()||!te_poly_init()||!te_sprites_init())te_state.moving=255;
    te_training_load();
#ifdef RT_CYCLES
    BENCH_NAME(3,"ray walls");BENCH_NAME(4,"sprites");
    BENCH_NAME(8,"project");BENCH_NAME(9,"visibility");BENCH_NAME(10,"fill");BENCH_NAME(11,"floor");
    BENCH_NAME(5,"present");BENCH_NAME(6,"clear");BENCH_NAME(7,"aim");
    BENCH_NAME(12,"background cache");BENCH_NAME(13,"sprite occlusion");BENCH_NAME(14,"sprite scale");
    BENCH_VALUE(0x61000000UL|te_render_bytes());BENCH_VALUE(0x62000000UL|te_poly_bytes());
    BENCH_VALUE(0x63000000UL|te_sprite_cache_bytes());BENCH_VALUE(0x64000000UL|sizeof(te_world));
    BENCH_VALUE(0x65000000UL|te_tables_bytes());
#endif
}
void game_scenario(u16 n) {
    if(te_state.moving==255)return;
    memset(&te_state,0,sizeof(te_state));te_state.target=te_state.hit=-1;
    /* Four scenes x three resolutions x camera poses. Lower 2 bits = scene;
     * bits 2..3 = resolution; bits 4..6 = renderer; bits 7..8 = pose;
     * bit 9 = ten sprites. Phase 1 has renderer 0. */
    te_state.scene=n&3;te_state.resolution=((n>>2)&3)%3;te_state.renderer=(n>>4)&7;
#ifdef TE_ONLY_RENDERER
    te_state.renderer=TE_ONLY_RENDERER;
#endif
    te_state.nsprites=(n&512)?10:5;
    te_state.sprite_mode=(n>>12)&3;
    te_state.style=(n>>14)&3;
    te_state.training=(n&1024)!=0;te_state.missing=0;te_state.jump_frame=0;
    te_scene(te_state.scene);te_camera_defaults(te_state.scene);
    if(n&128){te_state.camera.yaw+=16;te_state.camera.pitch=8;}
    if(n&256){te_state.camera.y=384;te_state.camera.z=96;te_state.feet_z=48;te_state.camera.pitch=-8;}
    if(n&2048) {
        u16 i;te_state.scene=5;te_world.scene=5;
        te_world.nbox=te_world.nanchor=0;memset(te_world.floor,0,sizeof(te_world.floor));
        for(i=0;i<10;i++){te_world.actor[i].x=352+i*32;te_world.actor[i].y=384;te_world.actor[i].z=0;}
    }
    if(te_state.training) {
        if(!te_mesh.nf)te_state.missing=1;
        else {
            te_state.scene=4;te_state.nsprites=0;
            te_state.camera.x=144;te_state.camera.y=120;te_state.feet_z=te_ground(144,120,32767);
            te_state.camera.z=te_state.feet_z+32;te_training_scene();
        }
    }
}
u8 game_update(void) {
    TeCamera *c=&te_state.camera;
    if(te_state.moving==255||input_pressed(K_ESC))return 0;
    if(input_pressed(K_F1)){te_state.training=te_state.missing=0;te_state.scene=(te_state.scene+1)&3;te_state.nsprites=5;te_scene(te_state.scene);te_camera_defaults(te_state.scene);}
    if(input_pressed(K_F5))te_state.resolution=(te_state.resolution+1)%3;
    if(input_pressed(K_APPS))te_state.renderer=(te_state.renderer+1)%5;
    if(input_pressed(K_DIGIT(1)))te_state.renderer=0;
    if(input_pressed(K_DIGIT(2)))te_state.renderer=1;
    if(input_pressed(K_DIGIT(3)))te_state.renderer=2;
    if(input_pressed(K_DIGIT(4)))te_state.renderer=3;
    if(input_pressed(K_DIGIT(5)))te_state.renderer=4;
    if(input_pressed(K_DIGIT(6)))te_state.style=(te_state.style+1)%5;
    if(input_pressed(K_DIGIT(7)))game_scenario(1044);
    if(input_pressed(K_MODE))te_state.view^=1;
    if(input_pressed(K_ENTER))te_state.door_open^=1;
    if(input_pressed(K_C))te_state.scope^=1;
    if(input_pressed(K_D)) {
        if(te_state.training&&!te_state.view&&!te_state.jump_frame) {
            te_state.jump_base=te_state.feet_z;te_state.jump_frame=1;
        } else te_state.orientation=(te_state.orientation+1)&3;
    }
    if(input_held(K_LEFT))c->yaw+=2;
    if(input_held(K_RIGHT))c->yaw-=2;
    if(input_held(K_B)) {
        if(input_held(K_UP)&&c->pitch<24)c->pitch++;
        if(input_held(K_DOWN)&&c->pitch>-24)c->pitch--;
    } else {
        s16 speed=te_state.training?((rt_frame&1)?5:4):8;
        s16 d=input_held(K_UP)?speed:input_held(K_DOWN)?-speed:0;
        if(d)te_move((s16)(te_mul(te_sin[(u8)(c->yaw+64)],d)>>8),(s16)(te_mul(te_sin[c->yaw],d)>>8));
    }
    if(input_pressed(K_A)&&te_state.scene==2)te_state.hit=te_aim_guard();
    else if(input_pressed(K_A)&&te_state.target>=0) {
        const TeAnchor *a=&te_world.anchor[(u8)te_state.target];
        /* Native traversal adaptation, not a measured Tenchu trajectory. */
        te_state.hook_x=a->x;te_state.hook_y=a->y;te_state.hook_z=a->z;
        te_state.grapple_ticks=12;te_state.jump_frame=0;
    }
    if(te_state.grapple_ticks) {
        s16 t=te_state.grapple_ticks;
        if(t==1){c->x=te_state.hook_x;c->y=te_state.hook_y;te_state.feet_z=te_state.hook_z;}
        else {
            c->x+=(s16)(te_mul(te_state.hook_x-c->x,te_recip[t])>>14);
            c->y+=(s16)(te_mul(te_state.hook_y-c->y,te_recip[t])>>14);
            te_state.feet_z+=(s16)(te_mul(te_state.hook_z-te_state.feet_z,te_recip[t])>>14);
        }
        c->z=te_state.feet_z+(te_state.training?32:48);te_state.grapple_ticks--;
    } else if(te_state.training&&te_state.jump_frame) {
        static const u8 jump[12]={0,8,15,20,23,24,24,22,18,13,6,0};
        s16 ground=te_ground(c->x,c->y,te_state.feet_z+16);
        if(te_state.jump_frame<12)te_state.feet_z=te_state.jump_base+jump[te_state.jump_frame++];
        else te_state.feet_z-=8;
        if(te_state.jump_frame>=7&&ground>=0&&te_state.feet_z<=ground) {
            te_state.feet_z=ground;te_state.jump_frame=0;
        }
        c->z=te_state.feet_z+32;
        if(te_state.feet_z<-32)game_scenario(1044);
    }
    if(te_state.training&&te_mesh.nf)te_training_anchors();
#ifdef TE_ONLY_RENDERER
    te_state.renderer=TE_ONLY_RENDERER;
#endif
    return 1;
}
static void map_view(void) {
    u16 x,y,i;
    draw_clear();
    for(y=0;y<16;y++)for(x=0;x<16;x++) {
        u16 rx=x,ry=y;u8 r=te_state.orientation;
        if(r==1){rx=15-y;ry=x;}if(r==2){rx=15-x;ry=15-y;}if(r==3){rx=y;ry=15-x;}
        if(te_world.floor[(y<<4)+x])draw_rect(32+rx*6,ry*6,5,5,C_BLACK);
    }
    i=((u16)te_state.camera.y>>6)*16+((u16)te_state.camera.x>>6);
    x=i&15;y=i>>4;
    if(te_state.orientation==1){i=x;x=15-y;y=i;}
    else if(te_state.orientation==2){x=15-x;y=15-y;}
    else if(te_state.orientation==3){i=x;x=y;y=15-i;}
    draw_rect(31+x*6,y*6-1,7,7,C_WHITE);draw_rect(33+x*6,y*6+1,3,3,C_BLACK);
}
void game_render(void) {
#ifdef RT_CYCLES
    u32 frame_start=BENCH_CYCLES;
#endif
    if(te_state.missing){draw_clear();draw_text(4,32,"Training data missing",F_SMALL,C_BLACK);draw_text(4,44,"make extract; send tnmap",F_SMALL,C_BLACK);return;}
    if(te_state.view){map_view();return;}
    TE_BEGIN(12);
    {u8 hit=te_background_get();TE_END(12);
     if(!hit) {
        TE_BEGIN(6);te_begin();TE_END(6);
        if(te_renderer==3&&te_state.style!=2)te_floor();
        if(te_renderer==0){te_ray();te_sprites();}
        else te_poly();
        TE_BEGIN(7);te_state.target=te_target();te_markers();TE_END(7);
        TE_BEGIN(12);te_background_put();TE_END(12);
     }}
    TE_BEGIN(5);te_present();TE_END(5);
#ifdef RT_CYCLES
    BENCH_VALUE(0x41000000UL|te_metrics.walls);BENCH_VALUE(0x42000000UL|te_metrics.sprite_pixels);
    BENCH_VALUE(0x43000000UL|te_metrics.segment_overflow);BENCH_VALUE(0x44000000UL|te_metrics.polygons);
    BENCH_VALUE(0x45000000UL|te_metrics.rejected);BENCH_VALUE(0x46000000UL|te_metrics.cells);
    BENCH_VALUE(0x47000000UL|te_metrics.portals);BENCH_VALUE(0x48000000UL|te_metrics.cache_hits);
    BENCH_VALUE(0x49000000UL|te_metrics.cache_misses);
    BENCH_VALUE(0x5F000000UL|(BENCH_CYCLES-frame_start));
#endif
}
