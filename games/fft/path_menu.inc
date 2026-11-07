#include "movement_rules.h"
u8 make_path_cell(u8 who,u8 c)
{
    u8 s,n,i,kept;
    if(who>=NUNIT||c>=MAP_N){st.path_n=0;return 0;}
#ifdef FFT_CHAPTER
    if(!unit_can_move(&st.unit[who])){st.path_n=0;W->from_who=0;return 0;}
#endif
    s=UNIT_CELL(&st.unit[who]);kept=W->from_who==who+1;
#ifdef FFT_CHAPTER
    if(W->move_kind>=2){
        if(!kept)bfs(who,bfs_from);
        if(st.reach[c]==255||c==s||unit_at_cell(c)||map_tiles[c].walk==2||(W->hw[c]&64)){st.path_n=0;return 0;}
        st.path[0]=s;st.path[1]=c;st.path_n=2;W->from_who=0;
        if(W->move_kind==2){
            s16 dx=(s16)TILE_X[c]-TILE_X[s],dz=(s16)TILE_Z[c]-TILE_Z[s];
            u8 chance=core_teleport_chance((u8)((dx<0?-dx:dx)+(dz<0?-dz:dz)),st.unit[who].move);
            if((u16)(muls16((s16)rand15(),100)>>15)<100-chance)st.path[1]=s;
        }
        return 2;
    }
#endif
    W->from_who = 0;
    if (!kept) bfs(who, bfs_from);
    n=(u8)core_nav_path(MAP_N,s,c,st.reach,bfs_from,order,PATH_MAX);
    if(!n){st.path_n=0;return 0;}
    for (i = 0;i<n;i++) {
        c=order[i];
#ifdef FFT_CHAPTER
        st.path[i]=c;
#else
        st.path[i][0]=TILE_X[c];st.path[i][1]=TILE_Z[c];
#endif
    }
    st.path_n = n;                                                 // path[0] = the start
    if (!kept) { compute_reach(who); W->from_who = 0; }
    else hl_seq++;
    return st.path_n;
}
