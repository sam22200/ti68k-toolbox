// Scalar forecasts are independent of hypothetical AI origins. Reach and hit
// chance remain candidate-local. The resident prepares only rows on this plan.
static u8 weapon_kind(u8 a,u8 t,AbId ab){(void)a;(void)ab;return W->ai_weapon_kind[t];}
static u16 weapon_total(u8 a,u8 t,AbId ab)
{
    u8 row=ab==NO_ABIL?0:(u8)(ab-0x195);
    (void)a;return W->ai_weapon_value[row][t];
}
static u16 weapon_forecast(u8 a,u8 t,AbId ab,u8 hand,u8 awake)
{(void)awake;return hand?0:weapon_total(a,t,ab);}
static u16 attack_prediction(u8 a,u8 t){return weapon_total(a,t,NO_ABIL);}
