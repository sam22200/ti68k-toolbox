// Portable fixture exported and strictly envelope-validated on the host.
// Native null/truncation guards remain; no native geometry search.
static struct {u8 centre,enemy[2],ally;} fixture_ui;
static struct {u8 centre;s16 camx,camy;} impact_fx;
static void impact_fixture(u8 kind)
{
    u16 size,stride,j;u8 i,k;const u8 *file=rt_file("fip00",&size),*p;
    game_scenario(61);
    if(!file||size<10){failures++;return;}
    stride=RD16(file,3);if(size<10+8*stride){failures++;return;}p=file+10+kind*stride;
    impact_fx.centre=fixture_ui.centre=p[0];st.rot=p[1];
    impact_fx.camx=(s16)RD16(p,2);impact_fx.camy=(s16)RD16(p,3);
    fixture_ui.enemy[0]=p[8];fixture_ui.enemy[1]=p[9];fixture_ui.ally=p[10];
    st.act=U_RAMZA;st.ai=st.auto_units=st.ai_all=st.banner=0;
    st.moved=0;st.acted=1;st.mode=M_BROWSE;st.turn=0;st.want=st.prot=255;st.scene_hide=0;
    p+=12;for(i=0;i<14;i++)st.bag[i]=*p++;
    for(i=0;i<NUNIT;i++,p+=38){
        Unit *u=&st.unit[i];u->x=p[0];u->z=p[1];u->cell=p[2];u->dc=p[3];
        u->hp=RD16(p,2);u->max_hp=RD16(p,3);u->mp=RD16(p,4);u->max_mp=RD16(p,5);
        u->ma=p[12];u->faith=p[13];u->gexp=u->gjp=0;u->status=0;
        for(k=0;k<5;k++){
            u->eq[k]=p[14+k];u->statuses.innate[k]=p[19+k];
            u->statuses.inflicted[k]=p[24+k];u->statuses.immune[k]=0;st.affinity[i][k]=p[29+k];
        }
        st.reaction[i]=RD16(p,17);st.support[i]=RD16(p,18);unit_status_refresh(u);W->feet_k[i]=0;
    }
    cursor_place(impact_fx.centre);aim(16);W->frz=W->hold=0;
    j=RD16(file+10+kind*stride,1);
    memset(&W->cast,0,sizeof W->cast);W->cast.actor=st.act;W->cast.ref=impact_fx.centre;W->cast.ability=16;
    CHECK((st.unit[fixture_ui.enemy[0]].team==TEAM_ENEMY&&st.unit[fixture_ui.enemy[1]].team==TEAM_ENEMY)?2:0,2);
    CHECK(fixture_ui.ally!=255,1);CHECK(impact_fx.centre<MAP_N&&map_tiles[impact_fx.centre].walk==1,1);
    CHECK(mod_call(MOD_TARGETS,2,0),1);CHECK(W->cast.phase,CAST_BATCH_READY);
    CHECK(st.action.phase,CTX_ROOT);CHECK(st.mode,M_ACT);if(W->cast.mask!=j)failures++;
#if AREA_IMPACT_FIRST < 4
    if(kind==1){CHECK(W->cast.victim[fixture_ui.enemy[0]].hit,0);CHECK(W->cast.victim[fixture_ui.enemy[1]].hit,1);}
    if(kind==3)CHECK(W->cast.victim[fixture_ui.ally].effect&EFFECT_KIND_MASK,AB_HEAL);
#endif
#if AREA_IMPACT_LAST > 4
    if(kind==4)CHECK(W->cast.mask,0);
#endif
    st.camx=impact_fx.camx;st.camy=impact_fx.camy;
}
