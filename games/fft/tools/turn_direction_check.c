// PC-only geometric oracle: actual low-poly grid versus independent angular/cardinal math.
#ifdef __m68k__
#error This independent floating-point oracle is PC-only.
#endif
#include "turn_audit.inc"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <math.h>

int main(int argc, char **argv)
{
    static const u8 pivots[6][2]={{0,0},{MAP_W-1,0},{0,MAP_H-1},
        {MAP_W-1,MAP_H-1},{4,5},{6,9}};
    unsigned pivot,c,step,x,z,checks=0;
    sw_init(65);
    assert(W && pbank && demo_look);
    for(pivot=0;pivot<6;pivot++) for(c=0;c<8;c++) {
        s16 cu,cv,px,py;
        s16 previous_x[MAP_W+1][MAP_H+1],previous_y[MAP_W+1][MAP_H+1];
        int direction=c&1 ? -1:1;
        turn_setup(c); st.cx=pivots[pivot][0]; st.cz=pivots[pivot][1];
        cam_target(&st.camx,&st.camy);
        view_of(st.rot,st.cx,st.cz,&cu,&cv);
        px=scene_x(st.rot,cu,cv)-st.camx; py=scene_y(cu,cv)+HH-st.camy;
        for(step=0;step<=4;step++) {
            double angle=(45.0+90.0*st.rot+direction*22.5*step)*acos(-1.0)/180.0;
            st.turn=step ? direction:0; st.turn_t=step ? step-1:0;
            draw_turn();
            for(z=0;z<=MAP_H;z++) for(x=0;x<=MAP_W;x++) {
                int dx=2*((int)x-st.cx)-1,dz=2*((int)z-st.cz)-1;
                double ex=px+6.0*sqrt(2.0)*(dx*cos(angle)-dz*sin(angle));
                double ey=py+3.0*sqrt(2.0)*(dx*sin(angle)+dz*cos(angle));
                // Fixed sine-table and shift rounding stay below two native pixels.
                assert(fabs(gx[x][z]-ex)<2.0 && fabs(gy[x][z]-ey)<2.0);
                if(step==0 || step==4) {
                    int rot=(st.rot+(step ? direction:0))&3,rx=dx,rz=dz;
                    if(rot==1) {rx=-dz;rz=dx;}
                    if(rot==2) {rx=-dx;rz=-dz;}
                    if(rot==3) {rx=dz;rz=-dx;}
                    assert(gx[x][z]==px+(rx-rz)*HW/2);
                    assert(gy[x][z]==py+(rx+rz)*HH/2);
                }
                if(step) {
                    long cross=(long)(previous_x[x][z]-px)*(gy[x][z]-py)
                        -(long)(previous_y[x][z]-py)*(gx[x][z]-px);
                    assert(direction*cross>0);
                }
                previous_x[x][z]=gx[x][z]; previous_y[x][z]=gy[x][z]; checks++;
            }
        }
    }
    // Visual endpoints rendered by the same low-poly routine as the moving frames.
    if(argc>1) for(c=0;c<2;c++) {
        char path[1024];
        const Unit *unit;
        Feet feet;
        turn_setup(c); unit=&st.unit[1]; feet=unit_feet(st.rot,1);
        snprintf(path,sizeof path,"%s/%u-source.png",argv[1],c); assert(!sw_write_png(path,1));
        printf("MARK,%u-source,%d,%d\n",c,feet.x-st.camx,feet.y-st.camy);
        for(step=0;step<=4;step++) {
            s16 sx,sy;
            st.turn=step ? (c ? -1:1):0; st.turn_t=step ? step-1:0; draw_turn();
            sx=(gx[unit->x][unit->z]+gx[unit->x+1][unit->z+1])/2;
            sy=(gy[unit->x][unit->z]+gy[unit->x+1][unit->z+1])/2
                -map_tiles[unit->z*MAP_W+unit->x].stand*(HU/2);
            snprintf(path,sizeof path,"%s/%u-low-%u.png",argv[1],c,step); assert(!sw_write_png(path,1));
            printf("MARK,%u-low-%u,%d,%d\n",c,step,sx,sy);
        }
        turn_setup(c);
        do {assert(turn_next());} while(st.turn);
        feet=unit_feet(st.rot,1);
        snprintf(path,sizeof path,"%s/%u-target.png",argv[1],c); assert(!sw_write_png(path,1));
        printf("MARK,%u-target,%d,%d\n",c,feet.x-st.camx,feet.y-st.camy);
    }
    printf("PASS: %u actual projected corners, 48 camera/direction cases, endpoints and signed sweep\n",checks);
    return 0;
}
