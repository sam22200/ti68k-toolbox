// Bounded predecessor reconstruction, shared by pure search tests and TARGETS.
static u16 core_nav_path(u16 count,u8 origin,u8 target,const u8 *dist,const u8 *from,
                         u8 *path,u16 capacity)
{
    u16 n=1,i;
    u8 cursor=target;
    if(!count||count>FFT_CORE_TILE_MAX||origin>=count||target>=count||
       !from||dist[target]==255)return 0;
    while(cursor!=origin) {
        if(n>=count)return 0;
        cursor=from[cursor];if(cursor>=count)return 0;n++;
    }
    if(n>capacity)return 0;
    for(i=n;i;){path[--i]=target;target=from[target];}
    return n;
}
