#include "../engine.h"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    u16 s,r,y,x;
    for(s=0;s<4;s++)for(r=0;r<3;r++) {
        sw_init(s+(r<<2));assert(sw_step(0));
        assert(te_world.nbox<=TE_MAX_BOXES);assert(te_metrics.walls>0);
        assert(te_metrics.segment_overflow==0);
        for(x=0;x<te_width;x++)for(y=0;y<te_ncover[x];y++) {
            assert(te_cover[x][y].top<=te_cover[x][y].bottom);
            assert(te_cover[x][y].bottom<te_height);
        }
        assert(sw_checksum());
    }
    sw_init(0);assert(te_move(0,8));assert(te_state.camera.y==200);
    te_state.camera.x=248;te_state.camera.y=480;
    assert(!te_move(16,0)); /* tall wall */
    te_state.camera.x=672;te_state.camera.y=500;te_state.feet_z=16;
    assert(te_move(0,20));assert(te_state.camera.z==80); /* step +16 */
    te_state.camera.x=512;te_state.camera.y=632;te_state.feet_z=0;
    assert(te_move(0,16));assert(te_state.feet_z==0); /* beneath common bridge */
    te_state.camera.x=352;te_state.camera.y=544;te_state.feet_z=176;
    assert(te_move(0,8));assert(te_state.feet_z==172); /* walk down the gable */
    assert(!te_los(512,192,48,352,600,48));
    assert(te_los(512,192,48,512,384,48));
    sw_init(2);te_state.camera.y=440;te_state.camera.x=384;
    assert(!te_move(0,16));te_state.door_open=1;assert(te_move(0,16));
    sw_init(2);te_state.camera.x=448;assert(te_aim_guard()==2);
    sw_step(K_A);assert(te_state.hit==2);
    {TeBox b={432,336,0,464,368,64,0,0,0,0};
     s16 t=te_ray_box(&te_state.camera,0,640,0,&b);assert(t>=57&&t<=60);
     assert(te_ray_box(&te_state.camera,640,0,0,&b)==-1);}
    sw_init(0);sw_step(0);
    {TeCamera c=te_state.camera;sw_step(K_MODE);sw_step(0);sw_step(K_MODE);
     assert(c.x==te_state.camera.x&&c.y==te_state.camera.y&&c.z==te_state.camera.z&&c.yaw==te_state.camera.yaw);}
    /* Opacity is independent of white ink; near wall clips a sprite by row. */
    te_ncover[3]=1;te_cover[3][0].top=20;te_cover[3][0].bottom=40;te_cover[3][0].depth=80;
    assert(te_occluded(3,30,160));assert(!te_occluded(3,10,160));assert(!te_occluded(3,30,64));
    for(s=0;s<4;s++)for(r=0;r<3;r++) {
        sw_init(16+s+(r<<2));assert(sw_step(0));assert(te_metrics.polygons>0);
        assert(te_metrics.segment_overflow==0);
    }
    sw_init(34);sw_step(0);assert(te_metrics.cells==1);
    te_state.door_open=1;sw_step(0);assert(te_metrics.cells>=2);
    {u16 a;
     sw_init(16);
     for(a=0;a<256;a+=8){te_state.camera.yaw=a;te_state.camera.pitch=(a&32)?24:-24;sw_step(0);assert(!te_metrics.segment_overflow);}}
    /* Every rendering path and quantised scaling method; equality is checked
     * against independent baked pixels, not merely a self-generated checksum. */
    for(s=0;s<5;s++)for(r=0;r<3;r++) {
        u16 baseline,mode;
        sw_init((s<<4)+(r<<2)+2048+512);sw_step(0);baseline=sw_checksum();
        for(mode=1;mode<3;mode++) {
            sw_init((s<<4)+(r<<2)+2048+512+(mode<<12));sw_step(0);
            assert(sw_checksum()==baseline);assert(!te_metrics.segment_overflow);
        }
    }
    sw_init(20);sw_step(0);
    {u16 hash=sw_checksum();sw_step(0);assert(!te_metrics.polygons);assert(sw_checksum()==hash);
     te_state.camera.yaw++;sw_step(0);assert(te_metrics.polygons);}
    {s16 yaw,pitch;u8 found=0;TeAnchor goal;
     sw_init(20);te_begin();
     for(yaw=0;yaw<256&&!found;yaw+=4)for(pitch=-24;pitch<=24;pitch+=2) {
        te_state.camera.yaw=yaw;te_state.camera.pitch=pitch;te_state.target=te_target();
        if(te_state.target>=0){goal=te_world.anchor[(u8)te_state.target];found=1;break;}
     }
     assert(found);sw_step(K_A);
     for(y=0;y<12;y++)sw_step(0);
     assert(!te_state.grapple_ticks&&te_state.camera.x==goal.x&&te_state.camera.y==goal.y&&te_state.feet_z==goal.z);}
    if(te_mesh.nf) {
        sw_init(1044);assert(!te_state.missing);assert(te_ground(144,120,100)==0);
        assert(te_ground(144,200,100)==16);assert(te_ground(144,240,100)==0);
        te_state.camera.x=144;te_state.camera.y=188;
        assert(!te_move(0,5)); /* first original step, walking blocked */
        te_state.jump_frame=5;te_state.feet_z=24;
        assert(te_move(0,5)); /* jump clears +16 */
        te_state.camera.y=220;assert(te_move(0,5)); /* can leave the raised block */
        sw_init(1044);sw_step(K_D);
        {u16 peak=0,k;for(k=0;k<16;k++){sw_step(0);if(te_state.feet_z>peak)peak=te_state.feet_z;}
         assert(peak==24);assert(!te_state.jump_frame);assert(te_state.feet_z==0);}
        for(s=0;s<5;s++) {
            sw_init(1028+(s<<4));sw_step(0);assert(!te_metrics.segment_overflow);
        }
    } else puts("Native training tests skipped: make extract to provide the local disc bank");
    printf("memory: world=%lu, state=%lu, render_heap=%u, polygon_heap=%u, tables=%u bytes\n",
        (unsigned long)sizeof(te_world),(unsigned long)sizeof(te_state),te_render_bytes(),te_poly_bytes(),te_tables_bytes());
    te_finish();
    printf("engine: scenes, resolutions, stairs, collision, doors, LOS, camera return, height occlusion pass\n");
    return 0;
}
