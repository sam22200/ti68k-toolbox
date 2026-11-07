static void action_rewards(u8 kill)
{
#ifdef FFT_CHAPTER
    if(st.action.phase!=CTX_IMPACT)return;
    if(st.action.pending||st.action.left||st.primary.reward){
        u8 e=earned_exp(st.atk,st.tgt,kill);
        if(e>st.primary.reward)st.primary.reward=e;
        return;
    }
#endif
    rewards(st.atk,st.tgt,kill);
}
