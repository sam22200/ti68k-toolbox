// Called only by accepted primary constructors, before action_pose_start turns.
static void mime_record(u8 a,u16 centre,AbId ab,u8 follow,u8 ref)
{
    MimeState *m=&W->mime;const Unit *u=&st.unit[a];u8 i;
    if(m->active||u->job==SOURCE_JOB_MIME||st.action.phase!=CTX_IDLE||centre>=MAP_N)return;
    m->pending=0;
    for(i=0;i<NUNIT;i++)if(i!=a&&st.unit[i].job==SOURCE_JOB_MIME&&unit_on_field(&st.unit[i]))m->pending|=(u16)1<<i;
    if(!m->pending)return;
    m->actor=a;m->team=u->team==TEAM_ENEMY;m->face=u->face;m->set=ab==NO_ABIL?0:u->set;
    m->ability=ab;m->dx=(s16)TILE_X[centre]-u->x;m->dz=(s16)TILE_Z[centre]-u->z;
    m->follow=follow;m->ref=ref;m->right=u->eq[EQ_WEAPON];m->left=u->eq[EQ_SHIELD];
}
