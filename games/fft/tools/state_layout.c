// Compiled with tigcc -S by fftmem.py only: the 68000 offsets of the campaign battle's
// State/Unit, from the battle program's own translation unit (the whole of
// campaign/native_battle.c, so every configuration macro matches), read from RAM by fftmem.py.
#include "../campaign/native_battle.c"
#define S(n,v) const unsigned short layout_##n = (unsigned short)(v);
#define F(t,f) S(t##__##f, offsetof(t,f))
S(State, sizeof(State)) S(Unit, sizeof(Unit)) S(Save, sizeof(Save)) S(UNITS, UNIT_CAPACITY) S(ORDER, ORDER_N)
F(State,rot) F(State,cx) F(State,cz) F(State,mode) F(State,sel) F(State,tick) F(State,unit)
F(State,act) F(State,turns) F(State,moved) F(State,acted) F(State,atk) F(State,tgt) F(State,hit)
F(State,crit) F(State,dmg) F(State,abil) F(State,end) F(State,ai) F(State,ai_t) F(State,auto_units)
F(State,rounds) F(State,banner) F(State,open_handoff) F(State,open_stage) F(State,reward_ready)
F(State,camx) F(State,camy) F(State,menu) F(State,dbg) F(State,banner_t)
F(Unit,x) F(Unit,z) F(Unit,gfx) F(Unit,team) F(Unit,face) F(Unit,job) F(Unit,level) F(Unit,name)
F(Unit,female) F(Unit,status) F(Unit,hp) F(Unit,max_hp) F(Unit,mp) F(Unit,max_mp) F(Unit,ct)
F(Unit,dc) F(Unit,exp) F(Unit,kos) F(Unit,slot) F(Unit,cell)
