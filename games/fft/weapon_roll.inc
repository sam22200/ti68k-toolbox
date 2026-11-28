// Native owners return before the weapon image prepares a strike.
static void attack_roll(u8 a,u8 t)
{
#ifdef FFT_MODULE
    (void)a;(void)t;W->nature_pending|=4;
#elif defined(FFT_MODULES)
    st.atk=a;st.tgt=t;mod_call(MOD_PLANNER,1,0);
#elif defined(FFT_CHAPTER)
    weapon_prepare(a,t);
#else
    // The demo has no weapon panel, but it still has to ROLL: weapon_prepare()
    // is what decides hit, damage and knockback, and it only exists under
    // FFT_CHAPTER. Stubbing this out left st.hit at 0 for ever, and the
    // scenario doors that retry until a blow lands (9, 17..25, 37) spun on it.
    st.hit=roll_hit(a,t);if(!st.hit)return;
    st.crit=(u8)(((u32)rand15()*100>>15)<4);
    st.dmg=attack_damage(a,t,st.crit,st.crit?rand15():0);
    if(st.crit)knockback(a,t);
#endif
}
