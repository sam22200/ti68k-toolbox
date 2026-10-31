// Shared deployment state: local edits until explicit confirmation, real world
// coordinates from the observed oriented squad bitmap, and no reserve losses.
typedef struct {u8 row,col,unit,n,confirm,note,done;u8 placed[25];} Deployment;
static Deployment dp;
static const u8 *flowbank;
static const u8 *deploy_rules(void){return qpart(flowbank,Q_DEPLOY);}
static const u8 *deploy_cell(u8 n){return deploy_rules()+4+3*n;}
static void deploy_move(s16 dr,s16 dc)
{
    s16 row=dp.row+dr,col=dp.col+dc;
    while((u16)row<5&&(u16)col<5) {
        if(deploy_cell((u8)(row*5+col))[0]){dp.row=(u8)row;dp.col=(u8)col;return;}
        row+=dr;col+=dc;
    }
}
static u8 deploy_where(u8 unit)
{
    u8 i;for(i=0;i<25;i++)if(dp.placed[i]==unit)return i;
    return 255;
}
static u8 deploy_ready(void)
{
    u8 i,ramza=0;
    if(!dp.n||dp.n>deploy_rules()[2])return 0;
    for(i=0;i<25;i++)if(dp.placed[i]!=255&&save.unit[dp.placed[i]].type==2)ramza=1;
    return ramza;
}
static void deploy_init(void)
{
    const u8 *boot=qpart(flowbank,Q_ROSTER);u8 i;
    memset(&dp,0,sizeof dp);memset(dp.placed,255,sizeof dp.placed);
    if(!save.n) {
        save.n=boot[0];memcpy(save.money,boot+1,4);memcpy(save.inv,boot+5,256);
        memcpy(save.unit,boot+261,(u16)save.n*sizeof(SaveUnit));
        memcpy(save.magic,SAVE_MAGIC,4);
    }
    dp.row=2;dp.col=2;
    // Restore only a placement for this encounter; never commit a default party.
    if(qword(save.deploy_battle)==qword(deploy_rules())) {
        u8 k;
        for(k=0;k<save.deploy_n&&k<5;k++)for(i=0;i<25;i++) {
            const u8 *c=deploy_cell(i),*p=save.deploy[k];
            if(c[0]&&c[1]==p[1]&&c[2]==p[2]&&p[0]<save.n&&dp.placed[i]==255&&deploy_where(p[0])==255) {
                dp.placed[i]=p[0];dp.n++;break;
            }
        }
    }
    if(!dp.n) {
        u8 cell=deploy_rules()[79];
        for(i=0;i<save.n;i++)if(save.unit[i].type==2&&cell<25&&deploy_cell(cell)[0]) {
            dp.placed[cell]=i;dp.n=1;dp.unit=i;dp.row=cell/5;dp.col=cell%5;break;
        }
    }
    if(!deploy_cell((u8)(dp.row*5+dp.col))[0])for(i=0;i<25;i++)if(deploy_cell(i)[0]) {
        dp.row=i/5;dp.col=i%5;break;
    }
}
static u8 deploy_place(void)
{
    u8 cell=(u8)(dp.row*5+dp.col),old=deploy_where(dp.unit);
    if(!deploy_cell(cell)[0]){dp.note=1;return 0;}
    if(dp.placed[cell]!=255&&dp.placed[cell]!=dp.unit){dp.note=2;return 0;}
    if(old==255&&dp.n==deploy_rules()[2]){dp.note=3;return 0;}
    if(old!=255)dp.placed[old]=255;else dp.n++;
    dp.placed[cell]=dp.unit;return 1;
}
static u8 deploy_commit(void)
{
    u8 i,n=0;
    if(!deploy_ready()){dp.note=4;return 0;}
    for(i=0;i<25;i++)if(dp.placed[i]!=255) {
        const u8 *c=deploy_cell(i);u8 *p=save.deploy[n++];
        p[0]=dp.placed[i];p[1]=c[1];p[2]=c[2];p[3]=deploy_rules()[3];
    }
    save.deploy_n=n;memcpy(save.deploy_battle,deploy_rules(),2);save.flow=2;
    if(!rt_save(SAVE_NAME,&save,sizeof save)){dp.note=5;return 0;}
    dp.done=1;return 1;
}
