# Split executable archive banks

Current follow-up: [CORE_AREA_CT.md](CORE_AREA_CT.md) adds the durable area owner
and command/CT/impact routing. Numbers below are the historical archive checkpoint.

2026-10-27. Follow-up to [CORE_BATCH.md](CORE_BATCH.md). The thirteen existing
modules now occupy two archived variables while retaining one execution zone.
This removes the near64K module-variable limit before more core code is added.
Gameplay, retail ability enablement, maps, art and save formats do not change.

## Format and deterministic export

Active command: `tools/module.py data/ffbm.bin battle.c 13 --second data/ffbn.bin`
FMB3 replaces active FMB2. All fields are
big-endian and word aligned: magic4, first ID/count/total as three u16, matching
export token as u32, local directory of u16 offsets, then contiguous FMD1 records.
FMD1 still stores entry/image length/context offset and the position-independent
image. No module ID changes and no per-map or per-ability exception is added.

The exporter chooses the contiguous split minimizing the larger archived shard,
including headers/directory bytes; ties choose the earliest split. Current split
is IDs0..6 in ffbm33772 bytes, IDs7..12 in ffbn31234. Largest image remains6204.
Each shard has a64000-byte production budget; exceeding it fails before output.
Omitting --second emits FMB2 for explicitly historical controls only, never an
active asset/default. Matching outputs keep their bytes/timestamps unchanged.

The shared32-bit token is CRC32 over the complete ordered FMD1 record sequence.
It identifies one export and rejects accidentally mixed shard generations. The
runtime compares tokens; it does not recompute a content checksum or authenticate
arbitrary modified code. Python verification records the actual token1156443084
and reproduces it over both shards. Different module content changes the token.

Both banks are required. Startup validates magic/ranges/counts, matching token,
all contiguous/even offsets and image lengths, aligned/in-range entry/context,
context signature and execution-zone bounds. Only no trailing bytes or the
existing optional six-byte AMS trailer is accepted. No image executes until the
entire pair passes. FMB2, a missing/swapped/mixed/truncated bank or bad signature
returns through the existing missing-data path; there is no single-bank fallback.

## Runtime and binding

After validation, startup resolves a13-entry table of archived record pointers.
Module calls and context lookup index that table directly; warm calls have no
shard-selection branch, offset-directory decode or file lookup. rt_file keeps
archive data in place and locks unarchived variables under the existing runtime
contract. One module still executes in the same6204-byte resident zone. A caller
must return before another module is loaded, as before.

The pointer table is52 native bytes, replacing the former pointer/size pair of6
bytes: +46 resident directory bytes. It is not a State/Work field or heap block.
The format validator initially exceeded the resident packing limit; fixed module
bindings now use one364-byte const ModCtx template rather than seventy repeated
callback/address assignments. Twenty-three map/State/asset fields still refresh
on each cold load. Copying the template clears transient request input on a cold
load; a warm preload preserves that input. No new assembly was authored.

ABI28/ModCtx364/signature62790 rejects27. Every one of the thirteen executable
images is byte-identical to CORE_BATCH except its two-byte ABI signature.
The explicit old binding initializer is retained only in check_banks.c, where
all364 context bytes are compared after every cold load. Its second pass changes
map sizes/count/scenery dimensions; pilot flow checks actual chapter rebinding.
The production template never caches a previous chapter's dynamic asset pointers.

## Evidence and budget

`make core-banks-check`: three offline partition/token/capacity tests;
130 PC/ASan/UBSan/actual68000 parser checks (hash00000047);112 native binding
checks (842D45F9), with26 cold loads and26 warm preloads spanning all thirteen
modules. Six additional full native startups reject a missing companion, mismatched
token, truncated companion, bad signature, old FMB2 or swapped banks before any
binding/load call. Four targeted package transport/classification tests also pass.

Native preload-only average33419/max40726 cycles cold; warm296. Compound fixture
still passes1202 PC/sanitizer/68000 checks94558E80. Cold preparation average84821/
max97692 (previous84287/97158); compound commit average58170/max104998 (previous
57485/103810). The extra template copy therefore adds a small measured cold cost;
these are backend zones, not full-frame/animation timing or a new60K HUD certificate.
No claim of a measured warm speedup against a new baseline is made.

Private group503606 bytes, -27 from503633, now43 variables. Changed payloads:
ffbcore-79, ffbm-31212, new ffbn31240 (including its AMS data trailer). Splitting
adds22 raw metadata bytes and one transport entry; all28 non-code banks stay
identical. Resident TTPack input64845 leaves91 bytes (previous64783/153). Archive
sharding fixes the module variable limit, not future resident-code growth: enabling
new callers still needs its own cold-code/zone/residency budget.

Fresh layout probes keep native State2192/Unit112/Work45516/ActionPacket22/
SaveUnit176/FSV3 Save3138, host State2248, and the same five battle allocation
requests, first5330 native. The directory/template are resident program storage,
not free RAM; these layout/allocation formulas are not measured peak RAM, stack,
fragmentation or combined residency.

Affected closure checks batch execution, ordinary actions, Charge and both pilot
save-flows; twelve existing action/reaction frame hashes and eleven flow pictures
remain identical. Existing gallery bytes/dates, live TiEmu and the standard
playtest group remain untouched. Exact inputs/hashes: x/core-banks/verification.json.
Geometry/census/Jump/scalar magic sweeps retain their earlier evidence.

Next: durable batch ownership and area CT/command/AI/explicit impact routing,
then reviewed per-target presentation before retail activation. Broader original
reaction/reward/status families, campaign/formats/editors and C07/L4 full-frame/
cold-HUD/stack/residency gates remain open. Jump stays deferred.
