// Cold State/archive/catalogue/view/cache bootstrap, before chapter Work binding.
#include "chapter_validate.h"
static u8 bootstrap_files(void)
{
    const CoreBoot *p=M.boot;const u8 *cj;u16 size;
    *p->p_battle_state=malloc(BATTLE_STORAGE_BYTES);
    if(!*p->p_battle_state)return 0;
    memset(*p->p_battle_state,0,BATTLE_STORAGE_BYTES);
    *p->p_rt_state=*p->p_battle_state;*p->p_rt_state_size=sizeof(State);
    *p->p_flow_bank=RT_NULL;*p->p_flow_ui=RT_NULL;
    if(p->campaign_flow){
        *p->p_flow_bank=rt_file("ffflow",RT_NULL);
        if(!qvalid(*p->p_flow_bank))*p->p_flow_bank=RT_NULL;
        if(*p->p_flow_bank)*p->p_flow_ui=rt_file("ffmg",RT_NULL);
    }
    *p->p_wgfx=rt_file("ffcw",RT_NULL);
    {                                          // actors.h actor_banks(), with this image's rt_file
        char name[5];u8 k;
        name[0]=name[1]='f';name[2]='c';name[4]=0;
        for(k=0;k<ACTOR_BANKS;k++){name[3]=(char)('a'+k);p->p_abank[k]=k&&!p->p_abank[k-1]?RT_NULL:rt_file(name,RT_NULL);}
        p->p_abank[ACTOR_BANKS]=RT_NULL;
    }
    *p->p_cgfx=rt_file("ffcu",RT_NULL);cj=rt_file("ffcj",&size);
    if(!cj_valid(cj,size))return 0;
    *p->p_common=cj;*p->p_abilities=(const Ability *)cj_section(cj,CJ_BATTLE);
    *p->p_charge_ticks=(const u8 *)*p->p_abilities+CJ_EXECUTABLES*12U;
    *p->p_jobs=(const Job *)(cj+RD16(cj,2));*p->p_job_names=(const char *)cj+RD16(cj,3);
    *p->p_job_sets=cj+RD16(cj,4);*p->p_items=(const Item *)(cj+RD16(cj,5));
    if(!*p->p_wgfx||!p->p_abank[0]||!p->p_abank[1]||!*p->p_cgfx)return 0;
    *p->p_sl_l=(const PlaneWord *)(*p->p_cgfx+UIC_SLOPES);
    *p->p_sl_m=*p->p_sl_l+SLOPE_ROWS;*p->p_sc_l=*p->p_sl_m+SLOPE_ROWS;*p->p_sc_m=*p->p_sc_l+SLOPE_ROWS;
    *p->p_vbuf=malloc(FFT_CORE_DEPTH_MAX);
    *p->p_sbuf=*p->p_vbuf?malloc(2UL*SC_BYTES*SC_HMAX+sizeof(Work)):RT_NULL;
    *p->p_acache=*p->p_sbuf?malloc(AC_N*(UNIT_FB+4)):RT_NULL;
    if(!*p->p_acache&&*p->p_sbuf){free(*p->p_sbuf);*p->p_sbuf=RT_NULL;}
    if(!*p->p_sbuf){if(*p->p_vbuf)free(*p->p_vbuf);*p->p_vbuf=RT_NULL;return 0;}
    *p->p_ac_key=(u16 *)(*p->p_acache)[AC_N];*p->p_ac_age=*p->p_ac_key+AC_N;
    if(!*p->p_navbuf){                         // the core's map envelope, once: any map fits
        *p->p_navbuf=malloc(NAV_ENVELOPE);*p->p_nav_capacity=*p->p_navbuf?NAV_ENVELOPE:0;
    }
    return 1;
}

