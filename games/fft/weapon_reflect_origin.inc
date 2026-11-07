static void weapon_reflect_pair(u8 a,u8 t,u8 origin)
{
    if(W->ai_reflect_hands&&(core_status_current(&st.unit[t].statuses,4)&2)){
        u8 receiver=reflect_recipient(t,st.unit[a].x,st.unit[a].z);
        if(receiver!=255)W->ai_reflect[origin][t>>1]|=(u8)((receiver+1)<<((t&1)<<2));
    }
}
