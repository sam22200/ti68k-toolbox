// Separate bounded carrier keeps each native test program below the AMS limit.
#define BUFF_CAPACITY_CHECK
#include "check_buffs.c"
static void full_capacity(void)
{
    u16 centre,c;u8 cells[FFT_CORE_UNIT_MAX],n=0,i,rh;Unit sample;
    prepare_buff(66);st.unit[U_RAMZA].ma=99;sample=st.unit[U_RAMZA];
    for(centre=0;centre<MAP_N;centre++){
        n=0;if(map_tiles[centre].walk!=1)continue;
        rh=(u8)core_area_height(W->surface+((u16)(W->grid_row[TILE_Z[centre]]+TILE_X[centre])<<3)+4*node_layer((u8)centre));
        for(c=0;c<MAP_N&&n<NUNIT;c++)if(map_tiles[c].walk==1&&
            abs(TILE_X[c]-TILE_X[centre])+abs(TILE_Z[c]-TILE_Z[centre])<=2&&
            core_area_layer(W->surface+((u16)(W->grid_row[TILE_Z[c]]+TILE_X[c])<<3),rh,2,0,W->surface_flags&SURFACE_MOVE_UPPER)==node_layer((u8)c)+1)cells[n++]=(u8)c;
        if(n==NUNIT)break;
    }
    CHECK(n,NUNIT);if(n!=NUNIT)return;
    for(i=0;i<NUNIT;i++){st.unit[i]=sample;unit_place(&st.unit[i],cells[i]);}
    st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    ZB(61);CHECK(cast_order(U_RAMZA,(u8)centre,66,0),1);ZE(61);
    CHECK(W->cast.mask,((u16)1<<NUNIT)-1);
    ZB(62);impact();ZE(62);
    for(i=0;i<NUNIT;i++){
        CHECK(core_status_current(&st.unit[i].statuses,4)&2,2);
        CHECK(st.unit[i].hp,999);CHECK(st.unit[i].status_ct[14],cj_status_rules(common)[(SID_REFLECT<<4)+3]);
    }
    CHECK(st.action.pending,0); // Newly granted Reflect cannot bounce frozen peers.
    CHECK(st.unit[U_RAMZA].mp,999-cj_range(common,66)[13]);
}
static void physical_hooks(void)
{
    u8 i;u16 expected;Unit *a;
    for(i=0;i<sizeof source_protect/sizeof source_protect[0];i++){
        const u16 *p=source_protect[i];prepare_buff(9);a=&st.unit[U_RAMZA];
        a->pa=p[0];a->brave=100;change(10,SID_PROTECT);if(p[1])change(10,SID_SLEEP);
        expected=(p[2]?p[2]:1)*p[0]; // Shared source zodiac stage clamps XA to at least1.
        CHECK(attack_damage(U_RAMZA,10,0,0),expected>999?999:expected);
        if(p[0]>1){
            a->pa=p[0]-1;CHECK(charge_query(U_RAMZA,10,0x196,0,0),p[2]*(p[0]-1));
        }
    }
}
static void capacity_trials(void){full_capacity();protect_profiles();physical_hooks();status_numbers();}
#ifndef __m68k__
int main(void){sw_init(61);capacity_trials();printf("buff_capacity %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();capacity_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
