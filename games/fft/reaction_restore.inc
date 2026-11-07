// Every saved field belongs to presentation/effect payload; CT, turn history,
// inventory, pending casts and live unit positions are never rolled back.
#define PACKET_FIELDS(X) X(hand) X(effect_kind) X(atk) X(tgt) X(act_t) X(hit) X(crit) X(push) X(kx) X(kz) X(push_cell) X(dmg) X(self) X(abil) X(gain) X(lvup) X(jpg)
static void reaction_return(void)
{
    ActionPacket *p=&st.primary;
    if(st.action.phase!=CTX_SECONDARY&&st.action.phase!=CTX_APPLIED)return;
#define RESTORE_FIELD(n) st.n=p->n;
    PACKET_FIELDS(RESTORE_FIELD)
#undef RESTORE_FIELD
    p->running=0;if(st.action.cursor&128){st.action.cursor=0;st.action.left=0;st.action.pending=0;}core_action_return(&st.action);
    if(W){W->num_ok=W->hold=W->frz=W->frz_h=0;W->fc_key=0;}
}
#undef PACKET_FIELDS
