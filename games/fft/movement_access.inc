// Shared equipped/innate eligibility, including Frog/Chicken suppression.
static u8 movement_is_teleport(u8 who)
{
    u8 f[3];cj_movement(common,st.unit[who].job,st.movement[who],f);
    return (f[1]&12)&&!(core_status_current(&st.unit[who].statuses,2)&6);
}
