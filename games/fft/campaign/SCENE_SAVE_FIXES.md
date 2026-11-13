# Scene controls and permanent battle stats (2026-11-13)

ESC now opens the same **Skip scene? / No / Yes** box in map scenes and
standalone scenes. No is selected initially. ESC, SHIFT or No resumes; only an
explicit Yes skips. Scene clocks, dialogue reveal and fades pause in the box.
Skipping an aftermath enters the normal result/reward handoff, including when
the scene is still fading in. Battle quitting retains its own confirmation.
The user approved the common box with "ok pour le visuel, continue".

The existing choice renderer and archived glove are shared. `archive_art.h`
factors the existing checked art-tail reader; no new artwork or duplicate bank.
PC sprites decode aligned host-order glove words once; native sprites read the
same archive in place. The modal animation reuses idle scene state in battle.

Party settlement now preserves permanent Brave/Faith as
`base + trunc((current - base) / 4)`, following the original post-battle routine
`require_apply_permanent_brave_faith_changes_and_collect_warnings.c`. Settlement
also resets the live baseline, making repeated settlement idempotent, and copies
raw stat bytes back to existing party slots. Original warnings/departure UI and
Degenerator itself are still pending; this delivery does not claim them.

## Layout and package

The26-byte baseline array sits at the end of Work, preserving hot renderer and
cache offsets. ABI88, ModCtx352, Work29308, Unit112, State2224 and FSV4/3202.
All28 images bind; old ABI87 is rejected before execution. Zone8698 and the
3072-byte companion stay fixed. Seven battle blocks request98440 bytes, plus
the separately measured runtime/grayscale/program allocations.

True production TTPack input57275 leaves7661 bytes. Private normalized groups:
core406084 + pilot chapter291731 =697815 transport bytes; core key
`575203168a32cb23e67b966cde4b6eea`. Install matching groups; this remains the
Orbonne/Academy/Gariland pilot, not the full chapter. Never distribute source.89g.

Raw FMB3 banks are64522/61648/57504 bytes. The exporter now enforces65000,
as used by other large banks, rather than the former arbitrary64000 guard.
The inclusive boundary and rejection above it are tested. Container overhead
still fits the TI variable and package resource limits. The planning warning
at64000 in the budget report is intentionally distinct.

## Scoped verification

Evidence: `x/campaign-prerequisites/verification.json`, including source/input
hashes, visual approval and preserved unrelated files/save timestamps.

- PC and ASan/UBSan scene controls/save/standalone suites: held ESC, default No,
  both confirm keys, cancel, paused reveal/fades, before/after battle handoffs,
  signed Brave/Faith deltas, repeated settlement and raw-stat round trips.
- Final shipping-data PC/real68000 carrier:198 checks, digest5501566B;
  prompt screens0383BD29/FD052883 agree. Three standalone native prompt,
  resume and confirmed-skip replays agree with PC.
- All28 native bindings pass; the prior87 export is rejected. Actual layout
  and packed resident sizes were measured after the final rebuild.
- Two selected13-actor render cases retain checksumsECF7/AA26. Ordinary frame
  peaks336050/357848 stay below360000; averages248758/148672. Cold rotation
  reaches4961116 and is still a documented stall, not an ordinary-frame claim.
- Final normalized package host42 + C819 checks agree PC/sanitizers,
  digest94924924, including continuation/save preservation.
- The unchanged frozen Sweegy fight fixture passes PC/sanitizers:14697 frames,
  43 actions,16 monster actions,382 checks, digest453E560F; trace and save bytes
  match the previous checkpoint. No unchanged art or complete census rebuilt.
- Seven module-bank boundary tests pass. No full TI suite sweep and no new
  TiEmu/hardware session; prior hardware evidence is reused only for unchanged
  hardware paths.

The16KiB free-RAM goal remains open. The previous installed diagnostic14058/
largest13042 and production estimate15171 predate this26-byte addition; neither
is a new measured peak or a claim that the target passes. Further difficult RAM
recovery remains deferred at the user's request.

Next: the [remaining user prerequisites](CAMPAIGN_PREREQUISITES.md), starting
with the four traps, before installed-chapter route automation.
