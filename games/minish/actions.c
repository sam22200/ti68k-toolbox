/* Ordinary source sword: 15 updates, input-edge restart, three tile samples.
 * Art, point offsets and bush identification are generated offline. */
#include "minish.h"
#include "actions_generated.h"
#ifdef __m68k__
#include <extgraph.h>
#endif
static const u8 *cut_grid;
static const u32 *patch_pixels;
static const u32 *sword_pixels;

u8 minish_actions_init(void)
{
    u16 size;
    const u8 *bank;
    cut_grid=RT_NULL;patch_pixels=RT_NULL;sword_pixels=RT_NULL;
    bank=rt_file("mizact",&size);
    if (!bank || size<ACTION_SIZE || size>ACTION_SIZE+6) return 0;
    cut_grid=bank;
    patch_pixels=(const u32 *)(bank+ACTION_PATCH_OFFSET);
    sword_pixels=(const u32 *)(bank+ACTION_ACTOR_OFFSET);
    return 1;
}

u8 minish_cut_cell(u16 cell)
{
    u16 id=cut_grid[cell];
    if (!id) return 0;
    --id;
    return (st.cut_flags[id>>3]>>(id&7))&1;
}

static void cut_point(s16 x,s16 y)
{
    u16 id;
    if ((u16)x>=WOODS_W || (u16)y>=WOODS_H) return;
    id=cut_grid[(((u16)y&0x3f0)<<2)|((u16)x>>4)];
    if (!id) return;
    --id;
    if (st.cut_flags[id>>3]&(1<<(id&7))) return;
    st.cut_flags[id>>3]|=1<<(id&7);
    st.cut_list[st.cut_count++]=id;
    minish_fx_spawn(cut_x[id]+8,cut_y[id]+8,cut_fx_kind[id]);
}

u8 minish_action_step(u16 keys)
{
    u16 face=st.anim_face,phase;
    u8 a=(keys&K_A)!=0,pressed=a&&!st.last_a;
    st.last_a=a;
    if (pressed) {
        /* A fresh press restarts a swing and can choose a new facing. */
        {
            s16 dx=((keys&K_RIGHT)!=0)-((keys&K_LEFT)!=0);
            s16 dy=((keys&K_DOWN)!=0)-((keys&K_UP)!=0);
            u16 d=dy<0 ? (dx<0 ? 7 : dx>0 ? 1 : 0) :
                  dy>0 ? (dx<0 ? 5 : dx>0 ? 3 : 4) : dx<0 ? 6 : dx>0 ? 2 : 255;
            if (d!=255 && (!(d&1) || ((d+1-(face<<1))&4))) face=(d&6)>>1;
        }
        st.attack=1;st.anim_face=face;st.facing=face<<3;
        st.anim_walk=st.anim_phase=0;st.anim_timer=3;
    } else if (st.attack) {
        if (++st.attack==16) { st.attack=0;return 0; }
    } else return 0;
    st.display_pose=st.pose;st.display_cover=st.cover;
    st.moving=0;st.direction=255;
    st.pose=44+face*10+sword_phase[st.attack-1];
    if (st.attack==2 || st.attack==5 || st.attack==11) {
        phase=face*3+(st.attack==2 ? 0 : st.attack==5 ? 1 : 2);
        cut_point((st.x>>8)+sword_dx[phase],(st.y>>8)+sword_dy[phase]);
    }
    return 1;
}

void minish_draw_cuts(void)
{
    u16 i,id,h;
    for (i=0;i<st.display_cut_count;i++) {
        s16 x,y;
        const u32 *p;
        id=st.cut_list[i];x=cut_draw_x[id]-st.camx;y=cut_draw_y[id]-st.camy;
        h=cut_patch_h[id];
        if (x<=-16 || x>=RT_W || y<=-(s16)h || y>=RT_H) continue;
        p=patch_pixels+cut_patch_offset[id];
#ifdef __m68k__
        { u16 shift=(u16)x&15,offset=(u16)h*shift;
          s16 aligned=x&~15;
          const u32 *l=p+(offset<<1),*d=l+h;
          if (aligned>=0 && aligned<=RT_W-32 && y>=0 && y+(s16)h<=RT_H) {
            u16 r,row=((u16)y<<5)-((u16)y<<1)+((u16)aligned>>3);
            u8 *dl=(u8 *)rt_light+row,*dd=(u8 *)rt_dark+row;
            for (r=0;r<h;r++,dl+=RT_PBYTES,dd+=RT_PBYTES) {
                *(u32 *)dl^=*l++;*(u32 *)dd^=*d++;
            }
          } else GrayClipSprite32_XOR_R(aligned,y,h,l,d,rt_light,rt_dark);
        }
#else
        { u16 r,b;
          for (r=0;r<h;r++) if ((u16)(y+r)<RT_H) {
            u16 row=(u16)(y+r)*RT_PBYTES;
            for (b=0;b<16;b++) if ((u16)(x+b)<RT_W) {
                u16 col=(u16)(x+b),bit=0x8000>>b;
                if ((p[r]>>16)&bit) ((u8 *)rt_light)[row+(col>>3)]^=0x80>>(col&7);
                if ((p[r+h]>>16)&bit) ((u8 *)rt_dark)[row+(col>>3)]^=0x80>>(col&7);
            }
          }
        }
#endif
    }
}

void minish_draw_sword(s16 *x,s16 *y,u16 *w,u16 *h)
{
    u16 pose=st.display_pose-44,i,height=sword_h[pose];
    const u32 *p=sword_pixels+sword_offset[pose];
    RtSprite actor;
    *x=minish_scaled(st.x>>8)-st.camx+sword_x[pose];
    *y=minish_scaled(st.y>>8)-st.camy+sword_y[pose];
    *w=sword_width[pose];*h=height;
    actor.w=32;actor.h=height;
    for (i=0;i<sword_parts[pose];i++,p+=height*3) {
        actor.light=p;actor.dark=p+height;actor.mask=p+(height<<1);
        minish_link_sprite(*x+(i<<5),*y,&actor);
    }
}
