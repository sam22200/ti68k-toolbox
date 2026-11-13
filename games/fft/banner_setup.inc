// Cold metadata only; the reviewed resident banner renderer/timing stays shared.
static void banner_setup(u8 kind)
{
    static const u8 widths[4]=BN_WIDTHS;
    u8 l,t=DK_N;
    st.banner=kind;st.banner_t=0;
    if(!W)return;
    for(l=0;l<(kind==BN_START?2:1);l++) {
        W->bn_t0[l]=t;
#ifdef FFT_CHAPTER
        if(kind==BN_START&&W->goal){
            if(!W->goal[10+l])continue;
            t+=(u8)((W->goal[10+l]+BN_SPEED-1)/BN_SPEED);
        }else
#endif
        t+=(u8)((widths[kind==BN_START?l:kind]+BN_SPEED-1)/BN_SPEED);
    }
    W->bn_end=(u8)(t+BN_HOLD+DK_N);
}
