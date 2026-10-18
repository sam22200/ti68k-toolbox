// Native presentation controls: numbered frames, extra wait, or fixed plane addresses.
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#include <extgraph.h>
#ifdef FFT_TURN_ORDER_COPY
#undef GrayDBufToggleSync
#define GrayDBufToggleSync() ({ GrayWaitNSwitches(1); \
    FastCopyScreen_R(GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayGetPlane(LIGHT_PLANE)); \
    FastCopyScreen_R(GrayDBufGetHiddenPlane(DARK_PLANE),GrayGetPlane(DARK_PLANE)); })
#endif
#ifdef FFT_TURN_ORDER_WAIT
#undef GrayDBufToggleSync
#define GrayDBufToggleSync() ({ GrayWaitNSwitches(1); GrayDBufToggle(); GrayWaitNSwitches(2); })
#endif
#include "turn_audit_tiemu.c"
