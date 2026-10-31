# Source elemental traits, derived cache and stat residency

2026-10-27. Follow-up to CORE_MAGIC.md. This checkpoint supplies the data and
cache producers needed by elemental formulas. It does not yet apply elemental
strengthening/resistance, convert absorption into healing, nullify a cast or
change production ability enablement. Formula08 still has the preceding
non-elemental scope; production Holy/Fire and every AF_CAST record stay disabled.

## Original evidence and data contract

`tools/affinity_oracle.py` runs unchanged original main_unit_copy_job_data and
main_unit_set_equipment_attributes under Unicorn. Actual-stat arithmetic is
stubbed: this fixture observes the affinity writes, not unrelated growth/HP.
178 profiles cover all160 internal job-template bases,13 distinct nonzero item
masks, overlapping equipment and duplicate items. Arbitrary fixture equipment
is not proof of equip legality. Recorded function/disc hashes, decompilation
commit, inputs and expected masks live in x/core-affinity/oracle/source.json.
The160 templates include special actors/monsters; generic playable careers
remain20. This does not implement monster equipment policy or persistent type.

The original copies four job masks, clears Strengthen, then ORs all equipped
item attribute masks. Native order matches source: Absorb, Nullify, Half, Weak,
Strengthen. These are elemental masks, not status immunity/innate-support bits.
Read source also distinguishes absorption (a healing result) from nullification
(a failed effect), and applies Half before Weak; native effect integration still
needs an original full result/rounding/finalization fixture.

FCJ9 keeps the existing14 section offsets and appends FEA1 after SPECIAL's
20 innate Two Swords bytes. FEA1 has a six-byte header,160 job references,
256 item references and32 deduplicated five-byte masks. Row0 is neutral; all
references are bytes. Sorting exact mask tuples makes export deterministic;
job/item IDs remain the source IDs, independent of dictionary row numbering.
The extension occupies582 bytes versus1920 for separate four-byte job/five-byte
item masks. No art, portrait, geometry or map change is required.

`cj_valid` rejects FCJ8, invalid spans/count/index references, nonzero reserved
flags and a nonneutral row0. It checks the416 references once at load; cold
stat rebuilding then trusts the validated bank. Existing source bodies,
normalized ability records, names and skill sets are preserved. Names' absolute
offsets change as the extension shifts later sections; the shared source reader
checks decoded equality rather than asserting those pointer bytes unchanged.

## Derived cache

A separate13x5-byte State cache preserves the112-byte native Unit stride and
its hot indexing. Native State grows2124->2190 (+66 including alignment),
while Work45516, SaveUnit176 and Save3138(FSV3) stay unchanged. The five shipping
heap requests are retained; only their State/save-tail block grows5262->5328.
These are compiler layouts/allocation formulas, not heap high-water, stack,
fragmentation or combined supervisor/program/grayscale residency measurements.

Every existing battle/party/stat rebuild replaces the cache from the current
job before merging current equipment. Removed/spare actors clear their cache;
scratch Units do not overwrite a live actor's entry. Saved traits are derived
again from stable job/item IDs, so FSV3 needs no new serialized fields. Future
in-battle equipment/job producers must refresh derived traits and preserve live
HP/MP when recalculating stats. The current human gear merge does not guess a
monster's rules from its job number; monster identity/equipment policy remains
an explicit future gate.

Cache reads must be cheap when formulas are connected. Do not scan five items
and archive attribute records for every AI target. Conversely, a populated cache
alone must never imply that resistance/absorption/nullification already works.
Source halfword narrowing, signed intermediate arithmetic and final caps need
fresh evidence before extending the preceding bounded MULS/DIVS magic kernel.

## Native module ownership and budget

The preceding resident program had17 bytes of packer margin. Stat recomputation
now compiles locally in its actual callers: PARTY, ACTION and RESPONSE. Primary
and response-owned level-ups therefore return into their own unchanged image.
The old unit_stats context callback is removed: an accidental call from another
image must fail linkage rather than silently overwrite the caller. The resident
injection door uses a small preload/PARTY9 wrapper and a Unit pointer in the
existing transient input union; COMMAND retains its cast-argument view.

Unsigned growth division uses the same existing compiler helper, routed through
a stack-ABI callback in the stat images. No new assembler was authored. Compiler
inspection and actual native image-ID assertions cover both level-up paths,
party load, scratch pointers and all13 slots. End-turn CT stays resident.

ABI22/ModCtx348/signature61226 reject ABI21. Twelve images remain; PARTY is now
the largest at6060 bytes. Raw image bank56968 (+2062), packed battle42165 (-187).
Private42-variable group495758 (+2709), including582 new catalogue bytes and
shared-loader code growth in management/deployment. All27 other non-code banks
remain identical. TTPack input64859/headroom77; further resident additions still
require a budget. This is a measured packing tradeoff, not a speedup.

## Scoped verification and next gate

`make -C games/fft/campaign core-affinity-check` runs1881 PC/ASan/UBSan/68000
assertions, C0B66CFB. Source-backed cache profiles, stale-trait replacement,
scratch Units, malformed banks, party loading and real ACTION/RESPONSE level-up
ownership pass. Stat zone43 averages10940 cycles across178 varied cold rebuilds;
this includes the resident test wrapper and is not per-frame or worst-case
high-level gameplay timing.

Prior magic1352, incantation799, actions295, dual Charge586 and management16
checks retain their hashes. The shared catalogue reader covers shifted spans/
names; one native Gariland/Orbonne flow checks saving and identical pictures.
Fresh layout/compiler evidence, old bank rejection, data comparison and image
byte/date preservation are collected in x/core-affinity/verification.json.
Unchanged geography/status/timer evidence is reused; no full census or live
TiEmu run is needed. Standard466045-byte playtest group remains untouched.

The bigger ACTION image raises the scoped Holy due-launch CPU zone47900->51788
and cold coordinate setup61474->65914; the scalar zone stays1972 average. These
are isolated CPU regions, not complete-frame latency certificates. No claim of
zero runtime cost or a closed frame/residency budget follows from unchanged
pictures or successful packing.

Next: a typed elemental result carried consistently through forecast/AI,
command preparation, impact, response save/restore and presentation. A healed
target must not take damage, wake from damage or trigger a damage-only reaction;
a nullified effect must still respect source payment/RNG/reward rules. Then
magic evasion, areas and reviewed spell/empty-centre presentation can proceed.
Broader status/reaction/Quick/all-freeze/campaign/formats/editor/residency gates
remain open. Jump refinements stay low priority.
