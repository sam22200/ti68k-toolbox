#ifndef FFT_TRAP_RULES_H
#define FFT_TRAP_RULES_H
enum { TRAP_DEGENERATOR,TRAP_DEATHTRAP,TRAP_SLEEPING_GAS,TRAP_STEEL_NEEDLE,TRAP_NONE=255 };
// Source first enabled bit wins; NO_TRAP overrides all four bits.
static u8 core_trap_id(u8 flags)
{
    u8 i;if(flags&16)return TRAP_NONE;
    for(i=0;i<4;i++)if(flags&((u8)1<<i))return i;
    return TRAP_NONE;
}
#endif
