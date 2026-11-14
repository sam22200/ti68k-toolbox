#ifndef FFT_SAVE_PROGRESS_H
#define FFT_SAVE_PROGRESS_H
#include "save_types.h"
// Supervisor view only: no Save allocation and no pointer retained across a child.
// Keep accepted versions/minimum lengths consistent with save_load_from.
static u8 save_progress_read(const u8 *p,u16 size,FpProgress *state)
{
    u16 need;u8 old;
    memset(state,0,sizeof *state);
    if(!p||size<4)return 0;
    old=!memcmp(p,"FSV2",4);
    if(old)need=SAVE_V2_BYTES;
    else if(!memcmp(p,"FSV3",4))need=SAVE_V3_BYTES;
    else if(!memcmp(p,"FSV4",4))need=SAVE_V4_BYTES;
    else if(!memcmp(p,"FSV5",4))need=SAVE_V5_BYTES;
    else if(!memcmp(p,SAVE_MAGIC,4))need=sizeof(Save);
    else return 0;
    if(size<need||p[36]>PARTY_MAX)return 0;
    memcpy(state->flags,p+SAVE_FLAGS,32);
    state->flow=old?(state->flags[0]&2?4:0):p[SAVE_FLOW_OFFSET];
    return 1;
}
#endif
