#ifndef FFT_UNIT_SHEET_H
#define FFT_UNIT_SHEET_H
// The unit sheet's black plate, shared the way `unit_card.h` shares the formation card.
// Presentation only: the owner reads its own state (a saved SaveUnit, a battle Unit, a
// shop's fitting draft) and fills one snapshot; the plate never sees a bank or a rule.
//
// `v[]` holds every number the plate prints, so one table drives the layout and one loop
// draws it. `shown` says which of them the owner actually has -- a battle knows a unit's
// CT, a saved roster does not, and the cells nobody filled stay blank rather than print a
// zero that means nothing. `soon` (see unit_sheet_plate) is the same snapshot as it WOULD
// be after the change the cursor is pointing at; the cells marked live in US_LIVE that
// moved are then drawn inverted -- black on a white cell -- with a triangle for the
// direction, which is how FFT previews a fitting (fft_guides/orig/mgt_item_list.png).
enum { US_LV, US_HP, US_MP,                        /* column 1, under the name and job */
       US_PA, US_MA, US_AT, US_BRAVE, US_FAITH,    /* column 2: what decides its damage */
       US_MOVE, US_JUMP, US_SPEED, US_JLV, US_CT,  /* column 3: how far it reaches, and when */
       US_CEV, US_SEV, US_AEV, US_WEV, US_MEV,     /* column 4: what it dodges with */
       US_N };
#define US_ALL ((1UL << US_N) - 1)
// The cells an equipment change can move. Lv, Job Lv, Brave, Faith, CT and the class evade
// (the job's own byte) never do.
#define US_LIVE 0x3C73EUL    /* HP MP PA MA AT Move Jump Speed S-EV A-EV W-EV M-EV */
typedef struct UnitSheet {
    const u8 *portrait;              // 32x32 planes, or 0: the owner then draws that corner
    const char *name, *job;
    u32 shown;                       // bit k: v[k] is a number this owner has
    u16 v[US_N];
    u16 max_hp, max_mp;              // when set, the cell prints "cur/max", as a battle must
} UnitSheet;
enum { US_PLATE_H = 32 };   // the portrait's own height, so it never spills

// The sheet's two list pages. Every screen that shows a unit's equipment, or the ability
// sets it fights with, draws them here -- whatever it reads them from (CHOICE_MENU.md rule
// 9): the owner resolves each row to a string and an icon, the renderer owns where they go.
// So the battle's Status screen and the management sheet are literally the same picture.
enum { US_SLOTS = 5, US_LIST_Y = 41, US_EQ_PITCH = 12, US_SET_PITCH = 11 };
// Rows the ability page can hold: its glove starts at US_LIST_Y + 10 = 51 and
// steps by 11, so row 5 would stand at 106 on a screen 100 tall.
enum { US_SET_ROWS = 5 };
typedef struct UnitEquip {
    const u8 *icon[US_SLOTS];        // the item's own 8x8 icon (three planes of 8), 0: none
    const char *name[US_SLOTS];      // 0: the slot is empty
} UnitEquip;
typedef struct UnitSets {
    const char *primary;             // the job's own set, which no menu can change
    const char *set[4];              // Secondary, Reaction, Support, Movement; 0: none
} UnitSets;
// Where the glove goes on each, by choice_menu.h's own rule: its finger stops two pixels
// short of the label instead of landing on it, and sits on the label's own middle row --
// the glove is 14 tall and its finger is on rows 4..6, so the Y is the ink's centre minus
// five. On the sets page a label's ink runs y+1..y+5 of an eleven-pixel row.
#define US_EQ_HAND_X       (-1)
#define US_EQ_HAND_Y(row)  (US_LIST_Y - 1 + (row) * US_EQ_PITCH)
#define US_SET_HAND_X      1
#define US_SET_HAND_Y(row) (US_LIST_Y + 10 + (row) * US_SET_PITCH)
#endif
