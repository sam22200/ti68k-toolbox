# Geomancy: terrain selection, HP damage and conditional statuses

2026-11-01. All twelve retail Geomancer actions (126..137, formula0x24)
are enabled together: Pitfall, Water Ball, Hell Ivy, Carve Model, Local Quake,
Kamaitachi, Demon Fire, Quicksand, Sand Storm, Blizzard, Gusty Wind and Lava Ball.
Production:146 normalized records,100 owned casts,140/201 generic entries.
The61 remaining entries include categories/upgrades, not61 separate effects.

## Original rules

OBSERVED with `tools/geomancy_oracle.py`, executing the unmodified local MIPS
formula, its target finalizer and the original terrain selector under Unicorn:

- The caster's source surface selects exactly one of the twelve abilities.
  The mapping has64 entries and uses the actual lower/upper layer. Learning
  another terrain ability does not make it usable on the current ground.
- Immediate range5, radius1, footprint height tolerance0. The centre has no
  vertical limit. The caster's panel is excluded; allies are otherwise hit.
  No MP, CT delay, Faith, magical evasion or Reflect stage is involved.
- Damage is `modified MA * ((raw PA + 2) >> 1)`. MA follows elemental
  strengthening, Magic Attack UP, Magic Defense UP, Shell, Frog/Chicken and
  zodiac order. Weather, half/weak/null/absorb affinities, Float versus earth,
  Oil removal, signed halfword results and the positive999 cap reuse the
  already checked elemental pipeline.
- Each non-nullified formula makes one19% conditional-status draw. Status
  immunity or an already present effect can reject the status without
  cancelling the HP result. A forecast consumes no RNG.
- The twelve secondary effects are Don't Move, Frog, Stop, Petrify,
  Confusion, Don't Act, Sleep, Death Sentence, Darkness, Silence, Slow and Dead.
  Frog toggles; timed effects use the existing source CT rules. Lethal HP
  damage prevents an additional ailment. Newly inflicted Sleep/Confusion
  survive this blow's damage wake-up. KO, cast interruption and AT refresh
  remain shared battle behavior.
- Silence permits the family; Frog blocks the caster's command.

## Native implementation and data

`geomancy_record` accepts each exact retail record, including its element and
status set; it never erases unsupported bits to enable a producer. The shared
catalogue is now **FCJB**, appending the original64-byte surface mapping to
the existing battle section. Old catalogues reject at bootstrap.

Chapter descriptors now contain **FGS2** geometry: the established eight-byte
collision column remains unchanged, followed by two source surface bytes per
column. Both layers, including blocked records, come directly from the disc.
Gariland and Orbonne are compared byte for byte with that source extraction.
The tail stays archived; the existing `surface_flags` byte records its presence.
Historical FGS1 geometry fixtures still bind, but cannot invent Geomancy terrain.
Truncated tails and values above63 reject before changing the active binding.

`magic_heal` query4 is a pure terrain lookup used by command eligibility;
query0 supplies damage and query3 supplies the filtered secondary request.
The amount and proc decision are frozen in the existing four-byte CastVictim:
**EFFECT_STATUS=32**, category in the low5 bits, existing return/Oil flags in
the upper two bits. A compile-time bound checks the current outcome range.
There is no Unit/State/Work/FSV3 growth or extra core allocation.

HP/status commit handles the validated single-bit request with source
cancellation rows and timers. It preserves innate effects, invalidates status
caches, clears cancelled pending casts and refreshes turn order. A second
impact cannot apply damage, statuses or rewards again. No executing module
loads another image. ABI49 rejects older module banks; ModCtx remains388 bytes.

The native learned list retains the learned techniques, dimming those unusable
on the current terrain. Original Elemental's special menu lists only the
terrain-selected technique; that menu dispatch is not reproduced here. The
existing area UI, numbers and animation style are reused without new art.

The adapted area AI evaluates immediate HP value and checks **each candidate
origin's terrain**, including an ability unusable before moving. It does not
score the19% secondary-status utility or reproduce FFT's full AI. A regression
also fixes a shared defect: the area's early dispatch previously bypassed the
best-candidate reset, allowing a prior scalar ability's score/ID to leak into
the next action. The reset now precedes either dispatch.
Rejected terrain origins also consume the64-scan budget; an update yields
with the caster restored to its real tile rather than scanning the whole map.

## Room and evidence

The first integration exceeded the resident limit by127 bytes. Cold scripted
start preparation now lives in PARTY12; allocation, actor-cache reset and
subsequent clock/image calls stay resident. PARTY10 also prepares the existing
counter/arrow glyphs and initial cover flags while its chapter data is bound.
This does not add work to ordinary combat frames or change the glyph algorithm.

Production TTPack input64741 bytes, headroom195, static module zone8186 (NUMBERS).
RESPONSE8122 and COMMAND8086 are close to that zone; measure before growing
either. There are still17 images and the same core allocation/layout scheme.
These figures are packaging evidence, not a full frame/stack/RAM certificate.

`core-geomancy-check` checks279 original formula profiles and128 original
terrain-selector calls, then production learned lists, pure forecasts,
frozen one-time impacts, immunities, timers, Frog toggling, terminal effects,
cast interruption, source gates, friendly fire and adapted AI origins. It
includes a synthetic twelve-victim capacity fixture and all64 surfaces on both
native layers. A temporary geometry buffer belongs only to the checker.

The full PC/sanitizer check and two bounded68000 carriers recombine to the
same8927 assertions, digest4BF14699: math6403/229FDD40 and
runtime2524/59D8BB4C. The split is transport only, required by the TI variable
ceiling; no assertion is omitted natively.

Scoped native peaks:180276 cycles for the single-victim preparation calls,
59012 for their impacts; the twelve-victim fixture costs284798 preparation
and143002 impact cycles. These include the current image/cache state, not
complete rendered frames or hardware FPS. The shared catalogue reader matches
PC/68000 (B9686645); management rules run in an isolated temporary save directory.

Full closure:70 suites run; the sole stale test still classified formula24
as unsupported. Its synthetic rejection carrier now uses gated Draw Out0x20,
retaining the historical target digestFC02B091. All69 historical summaries
remain identical except actions60795172 (the twelve new normalized records).
After bounding rejected AI-origin scans, ten direct suites pass again:
Geomancy, area AI, area impact, targets, banks, headroom, actions, offensive
magic, gil and batch. The final pure-origin-lookup check passes Geomancy/area-AI/banks again with
identical digests. Command eligibility was already checked during setup;
per-origin queries now read only terrain. This reduces the64-rejected-origin
case from294256 to167456 native cycles (43.1%), with the same bounded yield.
Both native Orbonne/Gariland pilot flows retain their PC pixels/rules and
settled-save/no-double-payment/trophy-overflow results.

Private evidence lives in `x/core-geomancy/`: oracle source/function hashes, check/results.json,
native zone logs, closure.log, targets-final.log, final-closure.log,
origin-query-closure.log, common.log, management.log, flow.log and headroom.txt.

## Remaining boundary

Counter Flood and other R/S/M producers are separate work. The native special
Elemental menu and original AI status utility remain adaptations described
above. Other job families, effective allegiance/recruitment, campaign/tooling
and full worst-case budgets are still open. Jump remains user-deferred.
No live TiEmu session, standard bundle, user save or historical proof artifact
is updated by this family check.
