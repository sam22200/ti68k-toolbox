// Offline portable fixture writer for the compact full-render native checker.
#define main impact_check_main
#include "check_area_impact.c"
#undef main
static FILE *output;
static void byte(u8 n){fputc(n,output);}
static void word(u16 n){byte(n>>8);byte(n);}
int main(int argc,char **argv)
{
    u8 kind,i,k;sw_init(61);output=fopen(argv[1],"wb");if(!output)return 2;
    fwrite("FIP1",1,4,output);word(8);word(12+14+38*NUNIT);byte(NUNIT);byte(14);
    for(kind=0;kind<8;kind++){
        impact_fixture(kind);byte(impact_fx.centre);byte(st.rot);word(W->cast.mask);
        word(impact_fx.camx);word(impact_fx.camy);
        byte(fixture_ui.enemy[0]);byte(fixture_ui.enemy[1]);byte(fixture_ui.ally);byte(0);
        for(i=0;i<14;i++)byte(st.bag[i]);
        for(i=0;i<NUNIT;i++){
            const Unit *u=&st.unit[i];byte(u->x);byte(u->z);byte(UNIT_CELL(u));byte(u->dc);
            word(u->hp);word(u->max_hp);word(u->mp+(i==st.act?skill_cost(i,16):0));word(u->max_mp);
            byte(u->ma);byte(u->faith);
            for(k=0;k<5;k++)byte(u->eq[k]);
            for(k=0;k<5;k++)byte(u->statuses.innate[k]);
            for(k=0;k<5;k++)byte(u->statuses.inflicted[k]);
            for(k=0;k<5;k++)byte(st.affinity[i][k]);
            word(st.reaction[i]);word(st.support[i]);
        }
    }
    fclose(output);return failures!=0;
}
