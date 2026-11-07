// Same source terrain, unit positions and review camera as the accepted candidate.
static struct {u8 centre;s16 camx,camy;} impact_fx;
static void impact_fixture(u8 kind)
{
    u8 i,r=kind==7?2:kind==5||kind==6?2:kind==1?1:kind==2?2:kind==3?3:kind==4?4:0;
    s16 u,v;
    memset(&impact_fx,0,sizeof impact_fx);ui_fixture(r);
    impact_fx.centre=fixture_ui.centre;st.scene_hide=0;st.acted=1;
    for(i=0;i<NUNIT;i++){
        st.reaction[i]=st.support[i]=0;st.unit[i].gexp=st.unit[i].gjp=0;
        memset(st.unit[i].eq,255,sizeof st.unit[i].eq);
    }
    if(kind==1){
        Unit *u=&st.unit[fixture_ui.enemy[0]];
        // Source25% accessory evade reaches the25% base after Defend/Abandon:
        // a real guaranteed evasion, not zero damage caused by zero Faith.
        u->eq[EQ_ACC]=232;st.reaction[fixture_ui.enemy[0]]=REACTION_ABANDON;
        u->statuses.innate[0]|=2;unit_status_refresh(u);
    }
    if(kind==5){
        st.unit[fixture_ui.enemy[0]].hp=1;
        st.unit[fixture_ui.enemy[1]].statuses.inflicted[2]|=128;
        unit_status_refresh(&st.unit[fixture_ui.enemy[1]]);
    }
    if(kind==6){
        for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_ENEMY)
            st.unit[i].hp=st.unit[i].x<MAP_W?1:0;
    }
    if(kind==7){st.reaction[fixture_ui.ally]=REACTION_POTION;st.unit[fixture_ui.ally].brave=100;st.bag[0]=1;}
    memset(&W->cast,0,sizeof W->cast);W->cast.actor=st.act;W->cast.ref=impact_fx.centre;W->cast.ability=16;
#ifdef FFT_MODULES
    CHECK(mod_call(MOD_TARGETS,2,0),1);
#else
    CHECK(batch_owned_prepare(),1);
#endif
    CHECK(W->cast.phase,CAST_BATCH_READY);
    CHECK(st.action.phase,CTX_ROOT);CHECK(st.mode,M_ACT);

    if(kind==1){CHECK(W->cast.victim[fixture_ui.enemy[0]].hit,0);CHECK(W->cast.victim[fixture_ui.enemy[1]].hit,1);}
    if(kind==3)CHECK(W->cast.victim[fixture_ui.ally].effect&EFFECT_KIND_MASK,AB_HEAL);
    if(kind==4)CHECK(W->cast.mask,0);
    view_of(st.rot,TILE_X[impact_fx.centre],TILE_Z[impact_fx.centre],&u,&v);
    impact_fx.camx=scene_x(st.rot,u,v)-80;
    impact_fx.camy=scene_y(u,v)+HH-map_tiles[impact_fx.centre].stand*(HU/2)-65;
    {
        s16 top=32767,bottom=-32767;
        for(i=0;i<NUNIT;i++)if(W->cast.mask&((u16)1<<i)){
            Feet f=unit_feet(st.rot,i);if(f.y<top)top=f.y;if(f.y>bottom)bottom=f.y;
        }
        if(top!=32767){
            if(top-impact_fx.camy<45)impact_fx.camy=top-45;
            if(bottom-impact_fx.camy>76)impact_fx.camy=bottom-76;
        }
    }
    cam_clamp(&impact_fx.camx,&impact_fx.camy);st.camx=impact_fx.camx;st.camy=impact_fx.camy;
}
