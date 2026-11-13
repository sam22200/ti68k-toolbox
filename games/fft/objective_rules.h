// Settled HP-script objectives, separate from generic source importance flags.
#ifndef FFT_OBJECTIVE_RULES_H
#define FFT_OBJECTIVE_RULES_H
// Identities are encounter slots, including0. One named enemy, optional allies.
static u8 core_objective_valid(u16 target,u16 protect,u8 count)
{
    u16 slots;
    if(!count||count>UNIT_CAPACITY)return 0;
    slots=(u16)((1U<<count)-1);
    return !((target|protect)&~slots)&&!(target&protect)&&!(target&(target-1));
}
// ko/removed are sampled only at an existing settled outcome boundary. Removed
// means a permanent loss, never merely hidden, airborne or outside the camera.
static u8 core_objective_result(u16 target,u16 protect,u16 ko,u16 removed,
                               u8 mandatory_loss,u8 ordinary)
{
    u16 lost=(u16)(ko|removed);
    if(mandatory_loss||ordinary==END_DEFEAT||(protect&lost))return END_DEFEAT;
    if(target){
        if(target&lost)return END_VICTORY;
        return ordinary==END_DEFEAT?END_DEFEAT:END_NONE;
    }
    return ordinary;
}
#endif
