// Compact campaign-flow data shared by deployment, scenes and battle modules.
#ifndef FFT_LEGACY_SERIF
#define FFT_SHARED_SERIF
#endif
enum { Q_ROSTER,Q_DEPLOY,Q_REWARDS,Q_TITLES,Q_PRICES,Q_GARILAND,Q_SCRIPT,Q_FONT };
static u16 qword(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static u32 qlong(const u8 *p){return (u32)qword(p)<<16|qword(p+2);}
static void qput(u8 *p,u32 v){p[0]=(u8)(v>>24);p[1]=(u8)(v>>16);p[2]=(u8)(v>>8);p[3]=(u8)v;}
static const u8 *qpart(const u8 *bank,u8 n){return bank+qword(bank+4+2*n);}
static u8 qvalid(const u8 *bank){return bank&&bank[0]=='F'&&bank[1]=='Q'&&bank[2]=='F'&&bank[3]==
#ifdef FFT_SHARED_SERIF
    '2';
#else
    '1';
#endif
}
