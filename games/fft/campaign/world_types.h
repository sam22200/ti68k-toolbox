#ifndef FFT_WORLD_TYPES_H
#define FFT_WORLD_TYPES_H
// Serialized bytes shared by every Save consumer, with no bank/file dependency.
#define WB_SLOTS (PARTY_MAX+GUEST_MAX)
typedef struct {
    u8 month,day,era,reserved;
    u8 egg[WB_SLOTS];
    u8 birthday[WB_SLOTS][2];
    u8 egg_color[(WB_SLOTS+3)/4];
} WorldParty;
static const u8 world_month_days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
static u8 world_egg_color(const WorldParty *w,u8 slot)
{return slot<WB_SLOTS?(w->egg_color[slot>>2]>>((slot&3)*2))&3:0;}
static void wb_color_set(WorldParty *w,u8 slot,u8 color)
{
    u8 shift=(slot&3)*2,mask=(u8)(3U<<shift);
    w->egg_color[slot>>2]=(w->egg_color[slot>>2]&(u8)~mask)|((color&3)<<shift);
}
static void world_party_clear_slot(WorldParty *w,u8 slot)
{if(slot<WB_SLOTS){w->egg[slot]=0;w->birthday[slot][0]=w->birthday[slot][1]=0;wb_color_set(w,slot,0);}}
static void world_party_move_slot(WorldParty *w,u8 to,u8 from)
{
    if(to>=WB_SLOTS||from>=WB_SLOTS||to==from)return;
    w->egg[to]=w->egg[from];w->birthday[to][0]=w->birthday[from][0];w->birthday[to][1]=w->birthday[from][1];
    wb_color_set(w,to,world_egg_color(w,from));world_party_clear_slot(w,from);
}
#endif
