#ifndef FFT_PACK_STORY_H
#define FFT_PACK_STORY_H
#include "pack_format.h"
enum { FP_TITLE=4, FP_MAP, FP_MANAGE };
typedef struct {unsigned char flags[32],flow;} FpProgress;
typedef unsigned short (*FpCall)(unsigned char,unsigned short);
typedef void (*FpProgressRead)(FpProgress *);
typedef struct {unsigned short requested;char bad[9];} FpNotice;
/* Child programs may compact the AMS heap or write saves. Reacquire all archive
   and save pointers after every child; no manifest pointer crosses a call. */
static unsigned char fp_story(FpRead read,FpCall call,FpProgressRead progress,
                             void (*reset)(void),unsigned short arg,FpNotice *notice)
{
    const unsigned char *p,*step;
    unsigned short n,k,id,prerequisite,next;unsigned char result,saved=0;
    FpProgress state;
    notice->requested=FP_NONE;
    result=fp_preflight(read,notice->bad);if(result)return result;
    notice->bad[0]=0;
    p=read("ffpack",&n);id=fp_word(p+6);prerequisite=fp_word(p+24);
    if(arg==1&&prerequisite){notice->requested=fp_word(p+44);return FP_PREVIOUS;}
    /* Later packs expose Continue directly, so an unavailable New Game cannot
       overwrite a slot before the first chapter has been installed. */
    /* A normal start goes back to the title when a step ends without handing over (a lost
       or quit battle, an abandoned scene): Game Over -> title as in FFT. Only the session
       changed since the last Save; Continue reloads a slot. */
title:
    if(!arg&&!call(FP_TITLE,prerequisite?2:0))return FP_OK;
    if(arg==1)reset();
    if(arg==99){
        p=read("ffpack",&n);
        if(fp_letter(p,0)=='o'&&!call(FP_BATTLE,105))return FP_OK;
    }
    progress(&state);p=read("ffpack",&n);
    if(prerequisite&&!fp_flag(state.flags,prerequisite)){
        notice->requested=fp_word(p+44);return FP_PREVIOUS;
    }
    for(k=0;;k++){
        p=read("ffpack",&n);
        if(!fp_size(p,n,0)||fp_word(p+6)!=id)return FP_BAD_PACK;
        if(k>=fp_word(p+32))break;
        step=p+fp_word(p+38)+k*FP_STEP_BYTES;
        if(fp_needed(step,state.flags,state.flow)){
            unsigned char op=step[0],flags=step[1];unsigned short argument=fp_word(step+2);
            if(!call(op,argument)){if(!arg)goto title;return FP_OK;}
            if(flags&FP_SAVE_PROMPT)call(FP_TITLE,3);   /* SHIFT/ESC declines */
            progress(&state);
        }
    }
    next=fp_word(p+28);
    if(!fp_word(p+42)){
        if(fp_flag(state.flags,fp_word(p+26))&&next!=FP_NONE){notice->requested=next;return FP_NEXT;}
        return FP_OK;
    }
    for(;;){
        unsigned short r=call(FP_MAP,saved);saved=0;
        if(!r)return FP_OK;
        if(r==1)call(FP_MANAGE,0);
        else if(r==2)saved=call(FP_TITLE,3)==3;
        else{
            p=read("ffpack",&n);
            if(!fp_size(p,n,0))return FP_BAD_PACK;
            if(r<100||r>=25500||!fp_letter(p,(unsigned char)(r/100-1))){
                notice->requested=next;return FP_NEXT;
            }
            call(FP_BATTLE,r);
        }
    }
}
#endif
