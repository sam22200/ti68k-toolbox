# Control statuses: Frog, Berserk, Confusion and Foxbird/Chicken

2026-10-30. The control family after CORE_TERMINAL. Frog (29), Blind Rage (53,
Berserk) and Confusion Song (55) are enabled by the shared status-spell
compiler; Foxbird (54) adds a Brave-loss effect, AB_BRAVE, whose Chicken shares
the lost-control handlers. Production: 101 normalized records, 70 casts,
98/201 generic action entries (103 remaining entries are not 103 effects).
No battle/save field, image count, context field or heap allocation is added.

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- `battle_status_remove_control`: Confusion, Blood Suck, Berserk, Chicken and
  Charm remove player control. `status_control.h` derives the same mask into a
  new cached bit, `STATUS_NO_CONTROL`; `turn_start` then hands the unit to the AI
  whatever its team. Blood Suck and Charm stay gated (no producer).
- `battle_formula_apply_physical_xa_modifying_statuses_support` order: attacker
  Berserk XA x3/2, then Frog XA = 1, then target Protect, Sleep/Charging and
  Chicken/Frog x3/2, then zodiac. Magical target Chicken/Frog x3/2 already existed.
- `battle_formula_calculate_dark_confuse`: Confusion halves the attacker's base
  hit like Darkness. A confused target already could not evade (CORE_CONTROL cache).
- Frog fights bare-handed (`battle_action_init_current_ability_strike_data`,
  `battle_target_calculate_weapon_range`): no weapon, reach 1, PA*Br/100 power;
  Two Swords still strikes twice. `battle_action_check_and_consume_mp`: only Attack
  and Frog are usable; `skill_blocked` greys every other ability.
- `battle_action_finalize_attack_and_flag_reactions`: HP damage removes Confusion
  (with Sleep, Charm and Transparent). The existing Sleep wake hook now also
  clears mutable Confusion; Berserk and Frog persist.
- `battle_ai_choose_status_action`: Chicken flees without acting, Confusion draws
  a random usable ability and a random target of any team, Berserk Attacks its
  nearest enemy. Adapted on the existing AI: a Chicken has no action and
  retreats (safest tile); Berserk keeps only Attack and only its focus (nearest
  enemy) as a target; Confusion targets any standing unit, its base value is
  AI_BAD so harming an ally still ranks, a coin keeps one candidate per ability
  and the ability is drawn at random. No control status Defends. This is not an
  original AI replay.
- Frog toggles (`battle_status_modify_inflictions`): Frog on a Frog removes it,
  an innate Frog stays. The source keeps that removal in its removal bytes; the
  prepared native packet carries the requested Frog bit (0x202) and the shared
  bitmap filter turns it into the removal on commit. Frog's AI flags are 0x30
  (adds and cancels); the immediate area AI accepts them, without valuing the
  undoing of an allied Frog.

- Foxbird is formula 0x61, `battle_formula_lower_brave_y`: formula 0x0a's
  magical evasion and Faith/MA accuracy (Shell/Magic DefendUP apply), then a
  Brave loss Y=30. `battle_action_finalize_attack_and_flag_reactions` clamps
  Brave to 0..100 (a rider's floor 10 is not modelled: no mounts) and sets
  Chicken below 10, removes it otherwise. `battle_status_build_upkeep_action`
  gives Brave +1 at the end of each turn below 10, so Chicken lifts at 10.
  The prepared packet carries AB_BRAVE and the loss; numbers read "-30Br".
  Immediate area AI values Foxbird only when it drops an enemy into Chicken.

## Shared implementation and budget

The family needed resident room first: the battle core had 7 bytes of TTPack
headroom, and the module zone is the largest image (AI). Two changes pay for it:

- `ai_clear` (line of sight, AI image) squared 16-bit offsets with `__mulsi3`;
  `muls16` is exact for them, smaller and faster (identical surface/reaction
  hashes, PC and 68000).
- `ai_choose` moves from the AI image to AI_SETUP op 5. The resident AI driver
  calls it right after the AI image returns; AI_SETUP is loaded next anyway for
  the camera, so no extra image load happens. PC builds call it in place.
- `unit_physical_base` and `weapon_reach` are single functions instead of
  expanded macros.
- `opening_after` (aftermath scene setup, ~550 bytes) was resident but is only
  called by map beats: it now compiles into the BEATS image (and the PC core).
  Its context slot stays, unused, so ModCtx and the signature keep their size.
- The Wait facing arrows move from NUMBERS to ORDER (never needed while the
  facing is chosen), so NUMBERS stays below the AI image that sizes the zone.

Resident TTPack input falls from 64929 (headroom 7) to about 64450 (headroom
about 487) with the whole family included; the zone is the AI image, 8116.
This also brings the area-impact native check binary back under the program
limit.

## Evidence

`make core-control-status-check`: 56 original formula0a/0x61 and finalizer
profiles for the four spells (`tools/control_status_oracle.py`, controlled RNG),
production casts and the Frog toggle, Foxbird's Brave/Chicken commit and upkeep, physical XA/reach/hit rules, Frog command limits,
lost-control turns and AI policies, ordinary AI use and MP/Silence guards.

## Not done

No frog/chicken sprite
or palette: the Status screen and impact labels name the state. Reraise and
Golem follow in CORE_PROTECT.md; Charm, Blood Suck, Float and the remaining
job families stay gated. The full
core is not complete; Jump stays deferred.
