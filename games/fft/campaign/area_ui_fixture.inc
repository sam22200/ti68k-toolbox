// Same terrain/actor fixtures as the accepted PC area review.
static struct {u8 centre,enemy[2],ally;} fixture_ui;
static u8 cluster(u8 *x,u8 *z)
{
    u16 c,best=65535;u8 found=0;
    for(c=0;c<MAP_N;c++){
        u8 cx=TILE_X[c],cz=TILE_Z[c],i;u16 cells[4],score;
        s16 dx=(s16)cx-st.unit[U_RAMZA].x,dz=(s16)cz-st.unit[U_RAMZA].z;
        if(node_layer((u8)c)||cx<1||cx+1>=MAP_W||cz<2||cz+1>=MAP_H||map_tiles[c].walk!=1)continue;
        cells[0]=node_at(cx-1,cz,0);cells[1]=node_at(cx+1,cz,0);
        cells[2]=node_at(cx,cz+1,0);cells[3]=node_at(cx,cz-2,0);
        for(i=0;i<4;i++)if(cells[i]>=MAP_N||map_tiles[cells[i]].walk!=1||map_tiles[cells[i]].stand!=map_tiles[c].stand)break;
        if(i<4)continue;score=(dx<0?-dx:dx)+(dz<0?-dz:dz);
        if(score<best){best=score;*x=cx;*z=cz;found=1;}
    }
    return found;
}
static void ui_fixture(u8 kind)
{
    u8 i,x=0,z=0,ne=0;u16 c,best=65535;
    game_scenario(61);
    st.act=U_RAMZA;st.ai=st.auto_units=st.ai_all=st.banner=0;
    st.moved=st.acted=0;st.mode=M_BROWSE;st.turn=0;st.want=st.prot=255;
    fixture_ui.ally=255;
    for(i=0;i<NUNIT;i++)if(i!=st.act){
        if(st.unit[i].team==TEAM_ENEMY&&ne<2)fixture_ui.enemy[ne++]=i;
        else if(st.unit[i].team!=TEAM_ENEMY&&fixture_ui.ally==255)fixture_ui.ally=i;
    }
    CHECK(ne,2);CHECK(fixture_ui.ally!=255,1);CHECK(cluster(&x,&z),1);
    c=node_at(x,z,0);fixture_ui.centre=(u8)c;
    for(i=0;i<NUNIT;i++){
        st.unit[i].x=st.unit[i].z=255;st.unit[i].dc=DC_GONE;
        st.unit[i].hp=st.unit[i].max_hp=100;st.unit[i].mp=st.unit[i].max_mp=30;
        st.unit[i].ma=5;st.unit[i].faith=70;
        memset(&st.unit[i].statuses,0,sizeof st.unit[i].statuses);st.unit[i].status=0;
        memset(st.affinity[i],0,5);
    }
    unit_place(&st.unit[st.act],(u8)node_at(x,z-2,0));st.unit[st.act].dc=3;
    if(kind!=4){
        unit_place(&st.unit[fixture_ui.enemy[0]],kind==1?(u8)c:(u8)node_at(x-1,z,0));
        unit_place(&st.unit[fixture_ui.enemy[1]],(u8)node_at(x+1,z,0));
        st.unit[fixture_ui.enemy[0]].dc=st.unit[fixture_ui.enemy[1]].dc=3;
    }
    if(kind==2||kind==3){
        unit_place(&st.unit[fixture_ui.ally],(u8)node_at(x,z+1,0));st.unit[fixture_ui.ally].dc=3;
        if(kind==3){st.affinity[fixture_ui.ally][0]=128;st.unit[fixture_ui.ally].hp=60;}
    }
    // Choose one source camera for the fixture, leaving all tactical positions
    // intact. Before/after share this view; do not hide the review behind a roof.
    for(i=0;i<4;i++){
        u8 j;u16 hidden=0;
        for(j=0;j<NUNIT;j++)if(st.unit[j].x<MAP_W)hidden+=view_hides(i,j);
        if(hidden<best){best=hidden;st.rot=i;}
    }
    st.abil=16;cursor_place(fixture_ui.centre);aim(16);W->frz=W->hold=0;
    cam_target(&st.camx,&st.camy);
    
}