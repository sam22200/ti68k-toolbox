#ifndef FFT_UNIT_CARD_H
#define FFT_UNIT_CARD_H
// Presentation-only snapshot: tactical rules and saved-party statistics stay with their owners.
typedef struct {
    const u8 *portrait;
    const char *name,*job;
    u16 hp,max_hp,mp,max_mp;
    u8 level,exp,brave,faith;
} UnitCard;
enum { UC_FORMATION,UC_MANAGEMENT,UC_BATTLE_LEFT,UC_BATTLE_RIGHT,UC_STATUS };
// The first five parts retain the battle HUD's existing incremental cache schedule.
enum { UC_PORTRAIT,UC_NAME,UC_HP,UC_MP,UC_LEVEL,UC_JOB,UC_EXP,UC_BRAVE,UC_FAITH,UC_PARTS };
enum { UC_BAR_FULL,UC_BAR_FLAT,UC_BAR_ROUND };
#endif
