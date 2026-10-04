// flow.c: the GB memory, the VBlank, the main thread
#ifndef FLOW_H
#define FLOW_H
#include "gb.h"
extern u8 gb_joy, door_end;
extern void (*read_hook)(void);        // called at every key read (04A4), before it
u8 gb_vblank(u8 joy);                  // one VBlank; returns 1 if a logic frame completed
void gb_reset(void);                   // memory and every protothread to zero
void set_main(u8 (*f)(void));
u8 door_loop(void);                    // the game from play_hall on (a door's memory)
u16 state_hash(void);                  // Fletcher-16 of WRAM/HRAM (the traced part)
#endif
