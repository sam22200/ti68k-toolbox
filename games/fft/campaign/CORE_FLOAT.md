# Float: the final ordinary generic spell

2026-11-02. Float (id38, Time Magic, formula0x0b) now uses the shared owned
cast/status pipeline. Production has175 executable records,129 casts and169/201
generic entries. The remaining32 are Throw12, Jump12 (deferred) and Arithmeticks8.
FCJB stays41822 bytes: only normalized row38 changes. FGS2/FCD3/FSV3 stay fixed.
The complete content/release gate remains open.

## Original evidence

`tools/float_oracle.py` executes the unchanged retail friendly-accuracy formula,
status filter and finalizer on16 focused profiles, plus the actual unit movement
configuration, adjacent-panel propagation and effective-height helper. Local JSON
retains original executable/overlay/function hashes and all observations.

The strict14-byte Float record is `04 01 01 00 00 e2 00 00 0b 8c 00 2e 02 08`.
Status set46 is `80 00 00 40 00 00`: canonical Float17, third bitmap byte0x40.
All14 one-byte record mutations reject. Range4, radius1, vertical area tolerance1,
no centre-height restriction, CT2 and MP8 come directly from that record.

- Ordinary formula0x0b uses caster/target Faith and the source140 accuracy
  parameter, clamps to100 and follows its original RNG/filter order. Forecasts
  stay pure. MA/support/zodiac/gear and current/immune/innate cases use the same
  source routine as existing friendly spells.
- Float has no CT expiry slot. Successful application changes the status at
  impact; an already present or immune Float produces no new infliction.
  Damage retains Float. Dispel uses the ordinary cancellation packet.
- With no equipped special movement, Float selects movement class2 and terrain
  row2. All64 row costs are1 except the blocked surface63 sentinel255. Jump
  remains capped at7, body clearance stays6 and water/lava restrictions remain
  distinct. Float status and the equipped Float movement ability are different
  config paths: the latter also changes clearance/water flags and is outside
  this producer milestone.
- Twelve source Float adjacent moves cover all dry/wet endpoint pairs at
  budgets1/2/3; native narrow and wide queues reproduce all twelve. Twelve
  original non-Float moves are retained as comparative source evidence.
  The original normal dry-to-water witness costs2; the existing native normal
  adapter charges1 while requiring spare movement. This preexisting difference
  is recorded rather than silently changing all ordinary paths in this family.
- The already implemented elemental policy nullifies Earth for Float. The
  new real producer is checked against Titan; actual Dispel removes that
  protection through the shared status path.

These are bounded original-function trials with controlled RNG and isolated
external sorting. They do not execute the full original action/AI/map traversal.
The original effective-height helper rises2 half-units (plus water depth), but
the native renderer and targeting retain their existing tile-height convention.
Hover/bob animation, a full unit-height model, equipped movement abilities,
mounts and unrestricted terrain/monster traversal remain separate work.

## Shared implementation and cost

`float_record` admits exactly this source row and status set. No new effect
kind, scalar formula or packet is needed. Generic actorless status application
now permits Float additions/removals. Existing status refresh invalidates cached
reach/forecast/AI state. CT/MP payment, followed or fixed targets, Non-Charge,
Half MP, Silence/Frog restrictions, source bitmap immunity, once-only impact,
Earth nullification, Dispel, rewards and AI all reuse the common paths.

`CORE_NAV_FLOAT` occupies bit2 of the existing search `out` byte. `spread_start`
sets it from the actor's current canonical status. Each search computes one
byte terrain mask outside its node/edge loops. The mask clears destination
water and explicit-cost3 tags only in Float mode; ordinary geometry, actor
blockers, span lengths, direction bits, predecessor order, queue yielding and
cached jump topology remain shared. Water ceases to add cost or require spare
movement under Float. Both narrow and wide searches use the same policy.
No extra per-unit/map cache, heap block or navigation layout is introduced.

ABI55 retains ModCtx388,18 images, the9+9 split and8312-byte static execution
zone. Both archive banks remain63026/63708 bytes. Image sizes0..17 are:

```
8228 3566 7878 6304 8254 8240 6366 6640 7428
8106 8312 5642 6850 8214 4876 8240 5212 8134
```

The raw native variable is64771 bytes. The actual TTPack input includes its
length prefix:64773, leaving163 below64936 (paired prior input64683/253).
The shared navigation implementation adds90 packed-input bytes in total.
Unit112, State2210, Work45576, Save3138, all member offsets and five allocation
formulas remain identical to the preceding verified build. Existing artwork,
portraits, fonts, projection and package inputs retain their formats.

## Verification

`make core-float-check`:525 assertions,BB53A5B9, equal on PC, ASan/UBSan
and actual68000 modules. It compares source formula outputs/RNG, the strict record,
owned delayed/followed/fixed casts, once-only payment/commit, actual damage and
Dispel, Earth protection, no CT timer, capability/MP restrictions and actual AI
admission. Real Gariland reach is a superset while floating, cached reach is
invalidated on status change and removing Float restores every baseline byte.
The24 native narrow/wide movement trials compare against the twelve unchanged
original Float observations. Normal costs and predecessor ties retain their
existing independent navigation regression controls.

Native measured zones: scalar forecast16 calls, peak70144 cycles; the two marked
Float impacts137032 each;24 narrow/wide source movement trials peak8932.
Real Gariland reach takes62716 normal /73922 floating cycles in this bounded
fixture (Float reaches more tiles). Generated `spread_go`/`spread_start` contain
no32-bit multiply/division or float helper calls.

Only diagnostic transport changes were needed for older near-limit checkers:
Geomancy math keeps its full final failure count/hash while omitting per-failure
native ordinal output; PC source-line diagnostics remain. Its tiny lookup is
static. Area-impact carries scenarios0..3,4..5,6..7 separately and recombines
all assertion/picture digests and every sampled frame. The case6 carrier primes
presentation caches at unchanged State/RNG/clock before sampling, matching the
warm unchanged-camera cache of the full sequence. A cold carrier previously
postponed one HUD detail on its first frame; PC State bytes were identical.
The complete64-frame reference is retained.

The local `x/core-float/verification.json` records final assertion counts/digests,
scoped regression results, source/artifact hashes, measured native zones,
actual ABI54-bank rejection, both private chapter flows, catalogue rollback
and preservation. It also distinguishes raw-variable and TTPack-input bytes.
All50 scoped suites pass. Area pixels remainF9BACFF5 and bounds9DE20073;
Geo/Talk/previous-family digests remain unchanged. Restoring only normalized
row38 reproduces the exact previous catalogue and actions688793BE/295; enabled
production gives6FF41691/295.508 protected save/group hashes and dates match;
112 of120 data files retain their bytes, with only four catalogue files and
four code-bank files changing. The generated module header is byte-identical.

Backend timings are not whole-frame, stack high-water or heap-fragmentation
certificates. No live emulator, user save or standard chapter group is used.
