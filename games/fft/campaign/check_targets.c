// Source radial footprint and compact-grid integration; no multi-target effect producer.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "../area_geometry.h"
static u32 world_hash(void)
{
    const u8 *p=(const u8 *)&st;u16 i;u32 h=17;
    for(i=0;i<sizeof st;i++)h=(h<<1|h>>31)^*p++;
    return h;
}
typedef struct {
    CoreGrid grid;CoreTilePosition pos[256];Tile tiles[256];
    u8 row[32],lookup[512],present[64],x[256],z[256],surface[8+8*256];
} AreaFixture;
static AreaFixture *f;
static void column_trials(void)
{
    u16 bytes,n,i;u8 col[8];const u8 *p=rt_file("fal00",&bytes);
    CHECK(p!=RT_NULL,1);if(!p)return;n=RD16(p,2);CHECK(n,42);p+=6;
    for(i=0;i<n;i++,p+=10){
        col[0]=p[0];col[1]=p[1];col[2]=0;col[3]=p[2]?128:0;
        col[4]=p[3];col[5]=p[4];col[6]=0;col[7]=p[5]?128:0;
        ZB(61);CHECK(core_area_layer(col,p[6],p[7],p[8],1),p[9]);ZE(61);
        if(p[5])CHECK(core_area_layer(col,p[6],p[7],p[8],0),p[9]);
    }
}
static void bind_profile(const u8 *p,u8 reverse)
{
    u8 w=p[0],h=p[1],layer;u16 area=(u16)w*h,i,count=0,ordinal=0;
    const u8 *terrain=p+14;u8 *surface;
    memset(f,0,sizeof *f);
    for(i=0;i<2*area;i++)if(!terrain[3*i+2])count++;
    CHECK(count>0&&count<=256,1);if(count==256)reverse=0;
    f->grid.count=count;f->grid.width=w;f->grid.height=h;f->grid.position=f->pos;
    f->grid.row=f->row;f->grid.lookup=f->lookup;f->grid.present=f->present;
    for(i=0;i<h;i++)f->row[i]=(u8)(i*w);
    memcpy(f->surface,"FGS1",4);f->surface[4]=w;f->surface[5]=h;f->surface[6]=area>>8;f->surface[7]=area;
    for(layer=0;layer<2;layer++)for(i=0;i<area;i++){
        const u8 *t=terrain+3*(i+(u16)layer*area);u8 x=i%w,z=i/w;
        surface=f->surface+8+(i<<3)+4*layer;
        surface[0]=t[0];surface[1]=t[1];surface[3]=t[2]&1?128:0;
        if(!t[2]){
            u16 id=reverse?count-1-ordinal:ordinal,address=i+((u16)layer<<8);ordinal++;
            f->lookup[address]=(u8)id;f->present[address>>3]|=1U<<(address&7);
            f->pos[id].x=f->x[id]=x;f->z[id]=z;f->pos[id].z_layer=z|(layer<<5);
            f->tiles[id].walk=1;f->tiles[id].stand=(u8)core_area_height(surface);
        }
    }
    CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),1);
    CHECK(battle_bind_surface(f->surface,8+(area<<3)),1);
    memset(&st,0,sizeof st);
    for(i=0;i<NUNIT;i++){
        const u8 *u=terrain+6*area+6*i;Unit *t=&st.unit[i];u16 cell=node_at(u[0],u[1],u[2]);
        t->x=t->z=255;t->dc=DC_GONE;t->hp=t->max_hp=t->mp=t->max_mp=100;t->team=u[3];
        if(u[4]!=255)t->statuses.inflicted[u[4]>>3]|=0x80>>(u[4]&7);
        if(u[4]==SID_DEAD)t->hp=0;
        if(!u[5]&&cell<MAP_N){unit_place(t,(u8)cell);t->dc=3;}
        unit_status_refresh(t);
    }
}
static void runtime_profiles(void)
{
    u16 bytes,n,i,length,ref,area;const u8 *p=rt_file("fat00",&bytes);u32 seed,hash;u8 j;
    CHECK(p!=RT_NULL,1);if(!p)return;n=RD16(p,2);CHECK(n,79);p+=6;
    for(i=0;i<n;i++,p+=length+2){
        length=RD16(p,0);p+=2;bind_profile(p,i&1);area=(u16)p[0]*p[1];
        ref=p[6]?p[7]:node_at(p[3],p[4],p[5]);
        if(ref>=MAP_N&&!p[6])ref=255;
        hash=world_hash();seed=rng32;
#ifdef FFT_MODULES
        mod_call(MOD_PARTY,255,0);
#endif
        ZB(62);CHECK(cast_targets(p[2],(u8)ref,0x100+i,p[6]),RD16(p+14+6*area+78,0));ZE(62);
        CHECK(world_hash()==hash,1);CHECK(rng32==seed,1);
        ZB(63);CHECK(cast_targets(p[2],(u8)ref,0x100+i,p[6]),RD16(p+14+6*area+78,0));ZE(63);
        CHECK(world_hash()==hash,1);CHECK(st.pending[p[2]].state,0);CHECK(st.unit[p[2]].mp,100);
        // Repeated queries/rotations do not own the footprint or the RNG.
        for(j=0;j<4;j++){st.rot=j;CHECK(cast_targets(p[2],(u8)ref,0x100+i,p[6]),RD16(p+14+6*area+78,0));}
        if(MAP_N==256){CHECK(ref,12);CHECK(UNIT_CELL(&st.unit[12]),255);}
        p-=2; // compensate the record-length prefix for the for-loop increment
    }
}
static void rejected_queries(void)
{
    u16 bytes;const u8 *file=rt_file("fat00",&bytes),*p=file+8;u32 hash=world_hash(),seed=rng32;u8 flags;
    bind_profile(p,0);hash=world_hash();
    CHECK(cast_targets(13,0,0x100,0),TARGETS_INVALID);CHECK(cast_targets(0,255,0x100,1),TARGETS_INVALID);
    CHECK(cast_targets(0,0,NO_ABIL,0),TARGETS_INVALID);CHECK(cast_targets(0,0,0x100,2),TARGETS_INVALID);
    CHECK(cast_targets(0,0,0x100,255),TARGETS_INVALID);CHECK(cast_targets(0,255,0x100,0),TARGETS_INVALID);
    for(flags=0;flags<4;flags++)CHECK(cast_targets(0,0,0x150+flags,0),TARGETS_UNSUPPORTED);
    CHECK(world_hash()==hash,1);CHECK(rng32==seed,1);
}
static void target_trials(void)
{
    f=malloc(sizeof *f);CHECK(f!=RT_NULL,1);if(!f)return;
    column_trials();runtime_profiles();rejected_queries();free(f);
}
#ifndef BATCH_CHECK
#ifndef __m68k__
int main(void){sw_init(61);target_trials();printf("targets %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();target_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif

#endif // BATCH_CHECK
