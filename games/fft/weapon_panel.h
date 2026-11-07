#ifndef FFT_WEAPON_PANEL_H
#define FFT_WEAPON_PANEL_H
// One frozen hand at a time. Responses start only after its whole panel and
// the remaining hand have settled. No gameplay/save fields are added.
typedef struct {u16 amount,packet;u8 hit,kind,critical,push,cell,pad;} WeaponVictim;
typedef struct {WeaponVictim victim[FFT_CORE_UNIT_MAX];u16 todo;u8 actor,hand,resume,pad;} WeaponPanel;
typedef char WeaponVictimSize[(sizeof(WeaponVictim)==10)?1:-1];
typedef char WeaponPanelSize[(sizeof(WeaponPanel)==136)?1:-1];
enum { WEAPON_PANEL_MORE=128 };
static u16 core_weapon_panel(const u8 *cells,u16 eligible,u8 cell,u8 count)
{
    u16 mask=0,bit=1;u8 i;
    for(i=0;i<count;i++,bit<<=1)if((eligible&bit)&&cells[i]==cell)mask|=bit;
    return mask;
}
#endif
