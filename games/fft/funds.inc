// Live battle funds reuse the existing Save tail, loaded once by PARTY11.
// Queries never load a file or an image; only victory writes these bytes to disk.
#define battle_save (*(Save *)((u8 *)&st+sizeof(State)))
static void steal_gil_commit(u8 a,u8 t,u16 n)
{
    u32 funds=qlong(battle_save.money);
    // TEAM_GUEST is AI control within the blue camp (Delita's source flags 0x84).
    if(st.unit[t].team!=TEAM_ENEMY)funds=funds>n?funds-n:0;
    if(st.unit[a].team!=TEAM_ENEMY){funds+=n;if(funds>99999999UL)funds=99999999UL;}
    qput(battle_save.money,funds);
}
