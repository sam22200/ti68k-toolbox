# Four traps and persistent level-down (2026-11-13)

Delivery branch `feat/fft-tile-traps`, based on main566a7ba. The user approved
`four-traps.png` and `trap-before-after.png` with "tres bien le visuel trapw et
contine". The shared box keeps its reviewed layout; its title is Trap and its
row names the trap. Evidence and matching private installation groups:
`x/tile-traps/verification.json` and `x/tile-traps/release/`.

## Rules and ownership

- Move-Find uses the first matching x/z/layer record. NO_TRAP disables it;
  the lowest enabled trap bit wins. Item discovery shares this lookup.
- Degenerator lowers the level, retains EXP, consumes the original five draws
  and applies all five raw-stat losses. Level1 still loses raw growth. Derived
  HP/MP clamp to their new maxima.
- Deathtrap applies Death Sentence through the existing status immunity,
  cancellation and lifetime rules. Sleeping Gas applies Sleep, including
  cancellation of charge/perform state when its command status disappears.
- Steel Needle deals floor(max HP/5), removes damage-sensitive statuses and
  enters the existing KO lifecycle. MP Switch redirects resources, including
  an MP shortfall without HP spill. Original resource/post-damage reaction
  rolls are retained; the original drain skips the acting unit, so its queued
  self responses do not execute. Mount/rider behavior remains outside scope.
- A committed move settles crystal/chest pickup, then the movement-benefit
  message, then item discovery or a trap. An unfound item suppresses the trap
  for that visit; later visits can trigger it. Sleep/KO cannot reopen the
  incapacitated mover's menu after its message, and settled moves cannot undo.

NATURE owns trap effects. EQUIPMENT returns before the resident calls NATURE;
NATURE returns before a requested EQUIPMENT stat rebuild. An executing image
never evicts itself through a nested stat callback. The three cold settlement
bytes consume two additional Work bytes because of existing alignment padding.
Hot renderer/cache offsets and every cache/capacity remain unchanged.

## Encounter and save formats

FCD5 appends SOURCE_MAP at word52, header106 bytes. Original map identity is
independent of world-map location, so repeated encounters at one location use
correct source trap/item records. IDs0..127 are accepted;255 explicitly disables
Move-Find data. Other values, truncated headers and old versions are rejected.
The two pilot descriptors were migrated without regenerating art: descriptor
spans/footer move by two bytes; interface offsets and payloads are preserved.
New generators emit FCD5 and the Sweegy fixture declares74. Package/core specs
bind this format and reject reuse of an incompatible installed core.

The former native growth model retains level-one seeds. First Degenerator
materializes those current native raw values once; subsequent levels and job
changes preserve current values using source growth arithmetic. This does not
retroactively reconstruct a legacy record's PS1 growth history.
Unit.raw's spare high byte marks that mode in RAM. FSV5 appends a two-byte
party-slot bitmap: Save3204, SaveUnit176 and old field offsets unchanged.
FSV2/3/4 migrate on load with a zero bitmap; normal saves write FSV5. Party
compaction carries the bitmap with reserve records/recruits and clears the
removed tail. Management and Continue/Save preserve it across jobs and slots.

ABI89, ModCtx352/signatureF3CB, Work29310, Unit112, State2224. All28 images
bind; the actual88 export is rejected before execution. PARTY is now16 and
ORDER8 to balance the existing three archive ranges. Companion27 and its
3072-byte reservation remain; no new execution zone or allocation family.

Final raw shards62758/64232/60004 bytes, split9/18, zone8698. Seven battle
blocks request98444 bytes (+4 from the scene/save lot), separate from runtime/
program/grayscale allocations. True production input57621 leaves7315 bytes.
Private groups: core410040 + pilot chapter291735 =701775 transport bytes,
key337e2236ab07dd8664c93c4725eabde7. Install matching groups, never source.89g.
This remains the pilot, not an assembled full Chapter I package.

## Focused evidence

- Original MIPS:36 trap lookups,54 raw-growth boundaries and20 resource/
  post-damage reaction cases with acting-slot drain. Function/disc hashes are
  recorded. Occupancy/map variables, eligibility and derived-stat refresh are
  isolated; this is not a complete original trap finalizer or battle replay.
- PC/ASan/UBSan:632 checks (1CDD88FE) on effects, move ordering, discovery,
  Sleep/charge, KO, reactions/RNG, levels/jobs, FSV4 migration, FSV5 reload,
  reserve compaction and source-map bounds. Current-raw management and the
  existing party/KO/inventory and title/session/slot suites also pass.
- Independent arithmetic/bitmap PC/68000 carrier:570 checks, D0B6D0A7, reused
  because those inputs/helpers did not change during the final header work.
- Final normalized-data actual-overlay carrier:83 checks, BAB6BBC8; four trap
  screens agree (88068E1D/9BD94EAB/6972BEA3/4297F361). It tests NATURE/EQUIPMENT
  handoffs, rebuilds, job-change/level-up and real virtual-VAT FSV5 roundtrips.
  Only the private trap table/source-map fixture differs from shipping data;
  this is a direct carrier, not an installed-supervisor checksum certification.
- All28 binding/context checks pass (356 assertions); actual88 banks reject.
  Actual native layout is measured; its evidence remains valid after the
  presentation-only Status-header factorization.
- Two selected13-actor render cases keep ECF7/AA26. Ordinary peaks337162 and
  357620 cycles, averages248848/148482, below360000. Cold rotation remains a
  documented4961118-cycle stall. The first candidate's equipment entry exceeded
  the budget by416 cycles. Reusing one snapshot for its four identical Status
  header parts brought that frame from360416 to357620, with unchanged pixels.
- One complete frozen Sweegy fight passes PC/sanitizers:14697 frames,43 actions,
  16 monster actions,382 checks,453E560F. Its entire trace and decoded save
  semantics match the prior delivery; only FCD5 metadata and the FSV5 empty
  bitmap were added to the reused fixture. No art/census extraction repeated.
- Final normalized package ownership/continuation/save tests pass PC/sanitizers;
  metadata-only migration and module bank boundaries are separately checked.

No full TI sweep or new TiEmu session. Existing hardware-path evidence is reused
only for unchanged hardware paths. The16KiB free-RAM goal is still open; the
older installed14058/largest13042 and estimated15171 figures predate this lot.
Difficult RAM recovery remains deferred as requested. New maps/content and
installed-flow peak/contiguous/reentry certification remain separate work.

Commands: `make campaign-traps-check` is PC/source-MIPS/sanitizers only.
`check_traps.py --review` captures the box; `--native-numbers` explicitly opts
into the independent numeric check. `check_traps_native.py --data-dir DIR`
opts into the single actual-overlay carrier on normalized .bin/.89y data.
See [the remaining user requests](CAMPAIGN_PREREQUISITES.md) before route automation.
