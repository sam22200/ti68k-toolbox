// Native owners return before the weapon image prepares a strike.
static void attack_roll(u8 a,u8 t)
{
#ifdef FFT_MODULE
    (void)a;(void)t;W->nature_pending|=4;
#elif defined(FFT_MODULES)
    st.atk=a;st.tgt=t;mod_call(MOD_PLANNER,1,0);
#else
    weapon_prepare(a,t);
#endif
}
