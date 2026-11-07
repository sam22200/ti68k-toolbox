// Frozen weapon payloads retain the existing scalar presentation and commits.
static void weapon_packet_load(const WeaponVictim *p)
{
    st.dmg=p->amount;st.self=p->packet;st.hit=p->hit;st.effect_kind=p->kind;
    st.crit=p->critical;st.push=p->push;st.push_cell=p->cell;
    if(st.push){st.kx=TILE_X[p->cell];st.kz=TILE_Z[p->cell];}
}
static void weapon_packet_store(WeaponVictim *p)
{
    p->amount=st.dmg;p->packet=st.self;p->hit=st.hit;p->kind=st.effect_kind;
    p->critical=st.crit;p->push=st.push;p->cell=st.push_cell;
}
