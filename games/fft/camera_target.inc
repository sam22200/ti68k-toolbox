// Shared camera target; native BEATS4 owns calculation, resident owns easing.
#include "camera_clamp.h"

// is scene point (x, y) (a unit's feet) on the screen with a margin (its sprite, the HUD)?
static u8 cam_sees(s16 x, s16 y)
{
    x -= st.camx; y -= st.camy;
    return x >= CAM_MX && x < RT_W - CAM_MX && y >= CAM_MT && y < RT_H - CAM_MB;
}

// Where the camera goes: the cursor centred; while walking or choosing a facing, it stays
// until the unit nears the edge, then centres it; while a unit aims or acts, it stays when the unit and its
// target are both on screen, else centres the point between them
static void cam_target(s16 *tx, s16 *ty)
{
    s16 u, v, x, y;
    if (st.morph_mask) {
        u8 first=0;while(!(st.morph_mask&((u16)1<<first)))first++;
        Feet f = unit_feet(st.rot, first);
        *tx = f.x - RT_W / 2; *ty = f.y - RT_H / 2 - 6;
        cam_clamp(tx, ty); return;
    }
    view_of(st.rot, st.cx, st.cz, &u, &v);
    x = scene_x(st.rot, u, v);
    y = scene_y(u, v) + HH - map_tiles[CURSOR_CELL].stand * (HU / 2);
    if (st.mode == M_WALK || st.mode == M_FACE) {
        Feet f = unit_feet(st.rot, st.mode == M_FACE ? st.act : st.sel);
        if (cam_sees(f.x, f.y)) { *tx = st.camx; *ty = st.camy; return; }
        x = f.x; y = f.y;
    } else if (st.banner == BN_START) {           // the objective: the enemies it lights up
        s16 sx = 0, sy = 0, n = 0, i;
        for (i = 0; i < NUNIT; i++)
            if (st.unit[i].team == TEAM_ENEMY && st.unit[i].hp) { Feet f = unit_feet(st.rot, (u8)i); sx += f.x; sy += f.y; n++; }
        if (n) { *tx = sx / n - RT_W / 2; *ty = sy / n - 68; cam_clamp(tx, ty); return; }
    } else if (st.mode == M_AIM || st.mode == M_ACT || st.mode==M_CAST_RANGE || st.mode==M_AREA) {
        Feet a = unit_feet(st.rot, st.mode == M_ACT ? st.atk : st.act);
        s16 target_y,target_x=x;
        if (st.mode == M_ACT) {
#ifdef FFT_CHAPTER
            if(area_impact_root()&&W->area_impact.centre<MAP_N){
                u16 cell=W->area_impact.centre;
                view_of(st.rot,TILE_X[cell],TILE_Z[cell],&u,&v);
                x=scene_x(st.rot,u,v);y=scene_y(u,v)+HH-map_tiles[cell].stand*(HU/2);
            }else
#endif
            {Feet t=unit_feet(st.rot,st.tgt);x=t.x;y=t.y;}
        }
        if (st.mode!=M_CAST_RANGE&&st.mode!=M_AREA&&cam_sees(a.x, a.y) && cam_sees(x, y)) { *tx = st.camx; *ty = st.camy; return; }
        target_y=y;
        x = (x + a.x) >> 1; y = (y + a.y) >> 1;
        *tx=x-RT_W/2;*ty=y-RT_H/2-6;
        {
            s16 top=a.y<target_y?a.y:target_y,bottom=a.y>target_y?a.y:target_y;
            // Preserve both actors' headroom when they fit. Otherwise the target
            // and its damage number take priority over the distant attacker.
            if(bottom-top<RT_H-CAM_MT-CAM_MB) {
                if(*ty>top-CAM_MT)*ty=top-CAM_MT;
                if(*ty<bottom-(RT_H-CAM_MB-1))*ty=bottom-(RT_H-CAM_MB-1);
            } else *ty=target_y-RT_H/2-6;
        }
        if(st.mode==M_CAST_RANGE||st.mode==M_AREA)*tx=target_x-44;
        cam_clamp(tx,ty);return;
    }
    *tx = x - RT_W / 2;
    *ty = y - RT_H / 2 - 6;
    cam_clamp(tx, ty);
}
