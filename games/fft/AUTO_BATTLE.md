# Auto-battle — 2026-10-18

The turn menu now has a fifth entry, **Auto-battle**:

- **Manual**: remove automatic control from all player units.
- **This unit**: delegate the active player's unit, including subsequent turns.
- **All allies**: delegate every player-controlled unit, including KO units if revived.

ENTER/2nd confirms; Shift/ESC closes the selector without changing the policy.
During AI turns, Shift/ESC cancels all player-unit automation. Enemies/guests continue
their turns. An automated player returns to the ordinary menu; a committed walk,
attack or pickup finishes first, retaining movement, item use, damage and rewards
exactly once. New battles reset to manual. CATALOG keeps its view settings.

## Scope

The original game's orders include Manual, Fight for Life, Protect Allies, Save Fading
Life and Run Like a Rabbit. Reference: [PlayStation manual, Battle commands](https://manuals.plus/m/7c36efdfcdba17704ecdad21e92662edc18dda3837db9bcb534a5998d65b0a72).
Read-only local decompilation references are `battle_menu_store_auto_battle_main_target_id.c`
and `battle_ai_init_target_consideration.c` in `sources/fft_decomp/src/battle/`: automatic
behavior is separate from team. No new original-game runtime oracle was added here.

The TI adaptation exposes the existing measured general-purpose AI: legal movement,
attacks, usable abilities, healing/revival/retreat choices, waiting and facing. It does
**not** implement the four original specialized orders or their target selectors.
Teams, inventory limits, CT, one Move/Act, damage and EXP/JP remain unchanged. Held
2nd/ENTER retains existing budgeted acceleration. Auto-battle itself changes neither
animation speed nor the restriction of gameplay to the tactical ISO view.

## Implementation and memory

A `u16 auto_units` stores unit bits. `turn_start` checks it without changing teams;
test-only `ai_all` remains. Delegation initializes planning scratch and retains
`moved`/`acted`: it never calls `ct_next` or restores CT. Cancellation consumes the key
before back/undo processing, clears the player's plan/frozen image, and preserves
committed action modes. Closing the selector during a manual turn preserves other
units' policies.

Native State grows from 1,014 to **1,016 bytes**; tactical Work stays **44,492 bytes**.
No heap buffer, sprite, table or bank is added. `fft.89z` has 65,585 container bytes:
its 65,495-byte program payload fits the linker's 65,514-byte limit with **19 bytes spare**.
Further code growth needs a size strategy. This is not a stock original-TI-89 24 KB
launch claim. Full tactical+perspective native packaging remains separate.

## Checks

```sh
make test auto-battle-check
make xcheck events-check
make pc ti cycles camera-config
```

The unit suite checks menu input, per-unit/party ownership on later turns, reset,
cancellation during enemy turns, selector cancellation preserving other policies,
committed walks/pickups, and four attack phases (before/on/after damage and rewards).
Damage, rewards, inventory and RNG match uninterrupted actions. Delegation after a
Move or Act cannot grant another. Its scenario-10 autoplay wins after 15,809 updates,
80 turn changes and 48 attacks/abilities in that input sequence.

`check_auto_battle.py` compares the actual TI frame loop against PC at five checkpoints:
selector, thinking, cancellation, one unit and a 16,000-frame party replay. It compares
images and canonical unit position/facing, HP/MP/CT/status/death, EXP/JP and bag signatures,
plus exact RNG, policy and turn/action fields. The party wins; ordinary-frame peak is
**353,624 cycles**. Existing 60-scenario and 101 event/replay comparisons also pass
(defeat peak 359,782). MC68000 datasheet counts exclude gray/display waits; these are
not TiEmu FPS measurements.

Local evidence: `x/auto-battle/menu.png`, case PNGs/logs and `checked.json`. Public
`keys/auto-unit.txt` / `auto-party.txt` start at scenario 10 (Ramza's first turn), reusing
the existing injection door without runtime scenario branches near the size limit.
The user's TiEmu session was left untouched; this logic/UI change is checked headlessly.
