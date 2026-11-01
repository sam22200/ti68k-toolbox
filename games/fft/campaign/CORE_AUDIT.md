# FFT PS1 to TI-89: core audit before content production

**Current addition — module rebalance, 2026-11-01:** Guts had pushed the
FORMULA image to 8392 bytes; the module zone equals the largest image and is
part of every core program (the real production headroom was negative; the
83 bytes noted for Guts were measured against a stale `data/modules.h`).
Turn-end status runtime (ops 4/5/14/15: tick, upkeep, dead turn) moves from
FORMULA to ORDER, which the turn order loads next anyway (one image load less
per turn); the objective/result banners move from NUMBERS (3/4) to ORDER (6/7),
never drawn with damage numbers. `hand_weapon` and `restore_ability` are
shared through ModCtx instead of copied per image (ABI 46), and
`magic_formula`/`magic_status_formula` are 13-byte bitmaps (all 256 ids checked
equal). Images: FORMULA 7016, NUMBERS 7290, ORDER 5130, AI 8094 (now the
zone), ACTION 7940. Zone 8392 -> 8094; resident headroom 235 (scratch raw build
64701). All 37 family suites green with unchanged hashes (`closure.py`);
talk-skill and area-impact fit again. Behaviour unchanged.

**Historical Guts addition — 2026-10-31:** [CORE_GUTS.md](CORE_GUTS.md).
Ramza's Yell (Speed +1), Cheer Up (Brave +5) and Scream (Brave +10, PA/MA/Speed
+1) are immediate owned area casts that always hit, without a draw; a KO,
Petrified or Wall target is a cancelled miss. New AB_SPEED (cap 50); Scream
also raises PA/MA (cap 99). 117 original profiles; `core-guts-check` 1546
assertions. Production: 120 normalized records, 87 casts; Guts is not among the
201 generic entries (114/201). Resident headroom then negative, restored by the module rebalance. Full core remains
incomplete; Jump deferred. Preserve live TiEmu, standard playtest, saves and
old proof bytes AND dates.

**Historical Spin Fist addition — 2026-10-31:** [CORE_PUNCH_ART.md](CORE_PUNCH_ART.md).
Punch Art's area strike: formula 0x31 on radius 1 around the user (excluded),
Attack's physical evasion per victim, a 4% critical without knockback (the
source knocks back only single targets). It rides the owned area pipeline,
the damage reusing `punch_strike`. `core-punch-strikes-check` 6524 assertions.
All 37 family suites green; `actions` changes (E0988BD3) only through the
Spin Fist entry (FCF28AB1 without it).
Production: 117 normalized records, 84 casts, 114/201 generic entries (87
remaining entries are not 87 effects). Resident headroom about 153.

**Historical Talk Skill family — 2026-10-31:** [CORE_TALK_SKILL.md](CORE_TALK_SKILL.md).
Persuade, Praise, Threaten, Preach, Solution, Death Sentence, Insult and Mimic
Daravon are enabled as immediate owned area casts: hit (zodiac(MA) + zodiac(X))%, no
Magic AttackUP or Faith, a sleeping target misses without a draw. Brave/Faith
records carry a signed loss (Foxbird shares the commit), new AB_FAITH clamps
0..100, Persuade writes CT 0 through AB_QUICK; Death Sentence counts the
unit's turns in the end-of-turn upkeep (FORMULA15) and kills non-Undead units
at 0. 344 original profiles and 12 upkeep traces; `core-talk-skill-check` 4616
assertions. Production: 116 normalized records, 83 casts, 113/201 generic
entries (88 remaining entries are not 88 effects). Invitation and Negotiate
stay gated. Three stale checks
(knockback, targets, batch) were repaired first. Resident headroom about 227,
zone 8182. Full core remains incomplete. Preserve live TiEmu, standard
playtest, saves and old proof bytes AND dates.

**Historical Punch Art family — 2026-10-30:** [CORE_PUNCH_ART.md](CORE_PUNCH_ART.md).
Stigma Magic, Chakra, Revive, Wave Fist and Repeating Fist are enabled.
Support (formulas 0x33/0x34/0x35): immediate owned area casts from PA with
Attack UP x4/3 and Martial Arts x3/2, no Faith (99 original profiles,
`core-punch-art-check` 1256 assertions). Strikes (0x31/0x32): single-target
physical skills with Attack's evasion, a 4% critical with knockback and
Repeating Fist's 1..9 hits, behind the exported `charge_query` (276 original
profiles, `core-punch-strikes-check` 4412 assertions). Resident room first:
chapter grid tables moved into PARTY op 10, and `FFT_PRODUCTION_CORE` drops
twelve test-only exported entries from fftbat. Production: 108 normalized
records, 75 casts, 105/201 generic entries (96 remaining entries are not 96
effects). Spin Fist, Earth Slash and Secret Fist stay gated. Resident headroom
about 323, zone 8170. Full core remains incomplete. Preserve live TiEmu,
standard playtest, saves and old proof bytes AND dates.

**Historical protection family — 2026-10-30:** [CORE_PROTECT.md](CORE_PROTECT.md).
Reraise and Golem are enabled together. Reraise (status set 33) survives KO;
at a KO unit's turn it revives with max HP/10 (one upkeep draw; 0 HP spends it
and keeps the counter). Golem (formula 0x14) fills its side's pool with the
caster's max HP; a primary Attack/Charge on that side is absorbed whole and
drains the pool (source BLOCKED_BY_GOLEM, ability 0's range data). State gains
`golem[2]` (4 bytes): MOD_SIG changes, older banks are rejected. 28 original
formula/finalizer profiles and six Reraise upkeep traces; `core-protect-check`
460 PC/sanitizer/68000 assertions. Production: 103 normalized records, 72
casts, 100/201 generic entries (101 remaining entries are not 101 effects).
End-of-turn upkeep moves into FORMULA15; resident headroom about 443. Float
waits for L1 movement with Jump. Full core remains incomplete. Preserve live
TiEmu, standard playtest, saves and old proof bytes AND dates.

**Historical control family — 2026-10-30:** [CORE_CONTROL_STATUS.md](CORE_CONTROL_STATUS.md).
Frog, Blind Rage (Berserk), Confusion Song and Foxbird (Brave -30, Chicken
below 10) are enabled together. Shared handlers: a cached STATUS_NO_CONTROL bit
hands Berserk/Confusion/Chicken units to the AI (Chicken flees, Berserk Attacks
its nearest enemy, Confusion draws ability and target of any team); source
physical XA order (Berserk x3/2, Frog XA 1, target Chicken/Frog x3/2);
Confusion halves base hit and is cleared by HP damage; a Frog fights
bare-handed (reach 1) and may only Attack or cast Frog, Frog toggles; Brave
clamps 0..100 and gains +1 per turn below 10. 56 original formula/finalizer
profiles; `core-control-status-check` 1509 PC/sanitizer/68000 assertions.
Production: 101 normalized records, 70 casts, 98/201 generic entries (103
remaining entries are not 103 effects). Resident room was recovered first:
`opening_after` compiles into BEATS, Wait arrows into ORDER, `ai_choose` into
AI_SETUP5, `ai_clear` uses MULS. Resident input about 64451 (headroom about
485, from 7); zone 8116 (AI). ABI 45 rejects older banks. Stale sleep,
incapacity and area-impact checks were repaired (area-impact's native binary
fits again). No frog/chicken sprite. Next grouped family: Reraise and Golem
(Float belongs with deferred L1 movement). Full core remains incomplete.
Preserve live TiEmu, standard playtest, saves and old proof bytes AND dates.

**Historical decisive family — 2026-10-30:** [CORE_TERMINAL.md](CORE_TERMINAL.md).
Death, Quick, Zombie and Petrify are enabled together as one shared target/
status/clock family. 56 original formula/finalizer profiles and 12 upkeep
traces pin Death (Dead plus nominal max-HP damage, Undead healing), Quick
(CT255 handed to the Quick-priority scheduler), Petrify (statue keeps HP/CT/
tile, leaves ordinary elimination, curable) and Zombie (Undead bit; KO-expiry
revival draw). 1702 family assertions (1292/5DEE6FD1 rules, 410/C4AE09F8
edges) agree on PC, sanitizers and 68000. KO-counter and end-turn CT-credit
policies move unchanged from resident code to FORMULA14/15; the Wait arrows
use a 16-byte Jacobian table (-34 bytes). Production: 97 normalized records,
66 casts, 94/201 generic action entries (107 remaining entries are not 107
effects). ABI 44/signature 1438, 17 images split 8+9, static zone 8228 (not
heap). Private group 539377 bytes (+1571); resident input 64929, headroom 7:
the next resident growth needs a relocation first. Unit/State/Work/FSV3 and
five core allocations fixed; 27 other data banks identical. Both native pilot
save/results flows, old frames and 18694 preserved paths pass. Next grouped
family: Reraise/Frog/Float/Berserk/Confusion and Brave/Faith control. Full core
remains incomplete; Jump remains deferred. Preserve live TiEmu, standard
playtest, saves and old proof bytes AND dates.

**Historical resource-restoration family — 2026-10-30:**
[CORE_RESTORATION.md](CORE_RESTORATION.md). Hi-Potion, X-Potion, Ether, Hi-Ether
and Elixir are enabled together; Potion/Phoenix Down use the same seven-producer
scalar pipeline. All 14 ordinary Item records are enabled. Frozen paired HP/MP
quantities reuse existing packet bytes; action-start forecast invalidation and
no-effect item rewards are fixed. Original evidence: 80 formula/finalizer
profiles and 38 actual HP/MP store traces. Undead Elixir damages HP and still
restores MP. The 2481 family assertions agree on PC, sanitizers and 68000;
relocated menu/view controls retain State/Work and the 1.5-second AI pause.
Production: 93 normalized records, 62 casts, 90/201 generic action entries.
The 111 remaining entries include upgrades/categories, not 111 distinct effects.
ABI 43/376/signature 1181, 17 images split 8+9, static resident zone 8172
(+966, not heap). Private group 537806 bytes (+5982); resident input 64829,
headroom 107. Unit/State/Work/FSV3 and five core allocations stay fixed;
27 other data banks stay identical. Scoped callers, 20 layouts, 12 old frames
and both native pilot save/results flows pass. Full core remains incomplete:
remaining control/barrier/job families, R/S/M, special actors, campaign/objectives,
formats/tools and worst-case RAM/stack/frame gates. Jump remains deferred.
Preserve live TiEmu, standard playtest, saves and old proof bytes AND dates.

**Historical cleansing family — 2026-10-29:** [CORE_CLEANSE.md](CORE_CLEANSE.md).
Esuna, Dispel Magic, Maiden's Kiss, Soft, Holy Water and Remedy ship together.
A frozen ordinal subset spans all five status bytes inside the existing effect
quantity: no new Unit/State/Work/save field or heap block. Innate protection,
source cancellation eligibility, timers, statues, one item debit, CT/MP,
Reflect and actual AI have 1711 PC/sanitizer/68000 assertions and 78 original
profiles. Production has 88 executable records, 62 casts (47 spells/15 summons)
and 85/201 generic action entries. The 116 unavailable entries include upgrades
and categories, not 116 distinct effects. ABI 42/ModCtx 376/signature 924,
17 images, split 9+8, static resident zone 7206 (+86, not a heap allocation).
Cold forecast formatting/cache and unchanged Wait arrows move to NUMBERS;
32 arrow cases and six new cache boxes retain exact pixels. Private group:
531824 bytes (+4415), resident input 64885/headroom 51; 27 other banks unchanged.
Full core/content gates remain open: HP/MP item restoration, remaining status/control/barrier and
job families, R/S/M, special actors, campaign/objectives, formats/tools and full
frame/stack/residency budgets. Keep families grouped and Jump deferred. Preserve
live TiEmu, standard playtest, saves and old proof bytes AND modification dates.

**Historical percentage/resource-transfer batch — 2026-10-29:**
[CORE_PERCENT.md](CORE_PERCENT.md). Demi, Demi 2, Lich, Spell Absorb and Life
Drain ship together: 60 casts, 82 normalized records and 79/201 generic action
entries. The 122 remaining entries include upgrades/categories, not 122 effects.
2396 PC/sanitizer/68000 assertions cover 57 original paired-result profiles,
production records, CT/MP, Reflect, Undead, AI, capped/empty pools and bootstrap.
Caster returns reuse the existing packet; MP effects remain separate from HP.
ABI 41/ModCtx 376/signature 667, 17 images, zone 7120. The unchanged catalogue
validator moves to SAVE5 before Work allocation. The image zone is static
resident storage, not a heap allocation; Unit/State/Work/FSV3 and the five heap
allocations stay fixed. See the linked note and private verification report for
sizes and scoped closure. Private group: 527409 bytes (+1392); resident packed
input: 64845, with 91 bytes of headroom. Full core/content gates remain open:
remaining status/control/barrier and job families, R/S/M, campaign/objectives,
formats/tools
and full frame/stack/residency budgets. Keep effects grouped and Jump deferred.
Preserve live TiEmu, standard playtest, saves and old proof bytes AND dates.

**Historical compatible buff/status batch —2026-10-29:** [CORE_BUFFS.md](CORE_BUFFS.md).
22 retail producers ship together:55 casts,77 normalized actions and 74/201
career action-table entries (127 unavailable entries, not 127 effects).
5682 PC/sanitizer/native checks cover the batch, including 13 distinct actors;
67 original formula profiles/10 Protect cases and 4843 bitmap-kernel checks pass.
Skipping empty status bytes reduces 13-victim impact 441270→296906 backend cycles.
ABI 40/376/sig 410,17 images/zone 7020; shared menu composition moves unchanged to
RESULTS. Private 526017 (+2753), resident 64915/headroom 21.
Full core/content gates remain open. Next compatible family: five
percentage/resource-transfer entries. Keep Jump deferred and preserve live
TiEmu, standard playtest, user saves and old gallery/group bytes AND dates.

**Historical approved area-impact integration —2026-10-28:**
[CORE_AREA_IMPACT.md](CORE_AREA_IMPACT.md). Shared per-victim quantities/Miss,
absorbed healing, hit poses/flash, covered centre pulse and Oil feedback now
reuse existing sprites/font. No fake caster result on an empty footprint;
secondary reactions suppress root effects and retain EXP/JP/terminal draining.
416 candidate captures match the accepted review. Native full-render1012 checks
12D9C238/64 individual screens agree;13-victim/two-floor/node255/signed/centre
bounds pass154 checksC56EC8B9. Cache412 aliases idle ai_gain512; State/Work/Unit/
FSV3/five core allocations unchanged. ABI34/ModCtx372/sig64400,16 images/split8+8,
zone6264; NUMBERS2416 also owns unchanged scalar number drawing. Raw shards
41072/38606, private517737(+2173), resident64473/headroom463. Package/affected regression evidence is
in x/core-area-impact/verification.json. No new bitmap bank or retail unlock.
Cold view decompression/full-frame/stack/residency gates stay open;13-number
backend timing is not a full13-actor frame certificate. Next shared scheduler/
status edges (Quick/clock liveness), broader families/AI/campaign/formats/tools.
Full core remains incomplete; mass content production is gated. Jump deferred. Standard playtest,
old review/gallery bytes and dates, user saves and live TiEmu remain untouched.

**Historical approved area-UI integration —2026-10-28:** [CORE_AREA_UI.md](CORE_AREA_UI.md).
The accepted panel/target-list/Unit-or-Panel flow now runs in the shared PC/68000
engine.499 PC/sanitizer/native checks67EED097, eight HUD hashes and four actual
full-screen/state replay checkpoints agree. The list only inspects victims;
Unit/Panel selects centre binding. AreaUi228 aliases idle256-byte AI scratch;
State2192/Work45576/Unit112/FSV3/five allocations stay fixed. ABI33/ModCtx372/
sig64143,15 images/zone6264, shards41066/36084. Private515564(+6146), resident64719/
headroom217;28 non-code banks unchanged. AI camera moved unchanged to AI_SETUP
(1321 native-equal checks); old actions, forecast/CT/magic/AI/pause and pilot saves
pass. First13-unit whole-map HUD peaks375252 cycles: interactive full-frame/
stack/residency gates remain open. Next reviewed per-victim/empty-centre impact
presentation, then remaining families/campaign/formats/tools. Retail casts and
full core/content gates stay closed; Jump deferred. Approved review PNG/GIF and
standard playtest/live TiEmu/gallery bytes/dates remain intact.

**Historical AI-pause checkpoint —2026-10-28:** [CORE_AI_PACING.md](CORE_AI_PACING.md).
User accepts roughly one-second thinking inside the requested frozen AI pause.
The1.5-second elapsed-clock minimum now includes planning instead of adding48
frames afterwards; slow plans add no second full pause.171 PC/sanitizer/native
checks58CBDE4E pass. All14 images/ABI32/State/Work/FSV3/five allocations unchanged;
resident timestamp adds2 static bytes. Private509418(-24), resident64467/headroom469.
The prior general setup peak remains measured, but is not a30fps blocker while
frozen. Next reviewed area targeting/per-target/empty presentation. Worst total
plan latency, other core and production gates remain open; Jump deferred.

**Historical cached area-AI checkpoint —2026-10-28:** [CORE_AREA_AI.md](CORE_AREA_AI.md).
The actual AI now chooses ordinary hostile formula08 area centres, including
empty panels, with cached current effects, friendly-fire and absorption scoring.
676 PC/sanitizer/native checksD04607FA;12 bounded original HP/accounting profiles,
five rank comparisons and three empty-panel visits. This is the immediate-HP
subset, not a complete original future-CT/status/Reflect/MP-utility chooser.
ABI32/ModCtx364/signature63878,14 images/zone6218; State/Work/FSV3/five allocations
unchanged, idle scratch reused. Private509442(+4838), input64447/headroom489,
28 non-code banks unchanged. New area quanta max319968 cycles; general setup
stage0 reaches12059828, so C07/L4 performance explicitly remains open. Next bound
general AI setup/reach, then reviewed targeting/per-target/empty presentation.
Retail casts/full production gates stay closed; Jump deferred. Standard playtest,
live TiEmu and existing gallery bytes/dates are preserved.

**Historical owned-area checkpoint —2026-10-27:** [CORE_AREA_CT.md](CORE_AREA_CT.md).
Supported formula08 areas now use command/CT/automatic impact and existing response/
reward drain, with one60-byte Work packet.353 PC/sanitizer/native checks11D6FCB4,
5 Orbonne retry checks00000230;13 equal-tick roots, explicit unit/tile centres and
pure late rejection. Retail area producers/AI/forecast/presentation remain gated.
ABI29/ModCtx364/signature63107/largest6204; Work45576(+60), State2192/Unit112/packet22/
FSV3 stable; same5 allocations, scene+Work and totals+60. Private504139(+533), shards
33782/31632, input64927/headroom9;28 non-code banks unchanged. Next area forecast/AI
and reviewed per-target/empty presentation, with a resident budget first. Broader
families and C07/L4 frame/residency remain open. Jump deferred; standard group/
live TiEmu/gallery bytes and dates untouched.

**Historical archive-bank checkpoint —2026-10-27:** [CORE_BANKS.md](CORE_BANKS.md).
FMB3 splits the same13 images into33772/31234 archived bytes; one6204-byte execution
zone remains. All image code is unchanged except ABI28 signature62790/ModCtx364.
Startup validates both ranges/export token before execution; six bad/missing/mixed
pair cases reject.130 PC/sanitizer/native parser checks and112 native full-context
checks pass; fixed callbacks share a const template, dynamic fields still refresh.
Native directory52 replaces6 bytes; State2192/Unit112/Work45516/packet22/FSV3 and five
allocations unchanged. Private503606(-27), input64845/headroom91;28 non-code banks
unchanged. Next durable area-batch ownership/CT/command/AI/impact routing and reviewed
presentation. Resident growth and C07/L4 frame/residency still need budgets; Jump
stays deferred. Standard group/live TiEmu/gallery bytes and dates untouched.

**Historical compound-cast checkpoint —2026-10-27:** [CORE_BATCH.md](CORE_BATCH.md).
Formula08 batches prepare all outcomes before HP, pay once, commit once and reuse
stable response/reward drain.13 original compound groups/1202 PC/sanitizer/68000
checks94558E80; private13-slot carrier, no retail area command/AI/presentation.
Caller packet60 bytes; no State/Work/Unit/FSV3 growth or new batch heap allocation.
ABI27/thirteen images/ModCtx364/largest6204; private503633 (+3027), resident input64783/
headroom153, raw bank64984.28 non-code banks unchanged; future growth needs a bank
budget. Next durable batch ownership, area CT/command/AI/impact routing and reviewed
presentation; remaining families and C07/L4 full-frame/residency gates stay open.
Jump deferred; live TiEmu, standard group and gallery bytes/dates untouched.

**Historical radial-target checkpoint —2026-10-27:** [CORE_TARGETS.md](CORE_TARGETS.md).
Read-only ordinary formula08 target sets pass79 original footprints/42 column
boundaries and1193 PC/sanitizer/68000 checksFC02B091. Nearest-layer/ties, whole-map,
follow coordinates, presence and panel-level team filtering are source-checked.
One-floor maps skip upper reads; no damage/MP/RNG/State changes or live area producer.
ABI26/thirteen images/ModCtx364/largest6172; State2192/Unit112/Work45516/packet22/FSV3
unchanged. Private500606 (+1564), input64749/headroom187;28 non-code banks unchanged.
Next batch preparation/commit in source order, one payment, per-target responses/
rewards/presentation before retail activation. C07/L4 cold-HUD/frame/residency and
broader core/content gates remain open. Jump deferred; playtest group/live TiEmu/
existing gallery bytes and dates untouched.

**Historical formula08 environment checkpoint —2026-10-27:** [CORE_WEATHER.md](CORE_WEATHER.md).
Weather/Float and prepared protected Oil removal now share prediction/AI/
execution.254 original profiles and6493 PC/sanitizer/68000 checks pass. Preserve
PS1 Oil's no-double-damage bug and signed per-step weather narrowing. Retail
spells/climate/status producers remain gated; weather/Float movement are separate.
ABI25/ModCtx364/largest6172; State2192 (+2), Unit112/Work45516/packet22/FSV3 stable.
Private499042 (+279), input64735/headroom201; all28 non-code banks unchanged.
FORMULA11 owns unchanged forecast-cache drawing; full cold forecast costs up to
92032 in the fixture, so full-frame/cold-HUD/residency gates stay open. Next area
target sets and per-target effects/reactions/rewards/presentation. Standard group,
live TiEmu and existing image bytes/dates remain untouched.

**Historical affinity-data checkpoint —2026-10-27:** [CORE_AFFINITY.md](CORE_AFFINITY.md).
Source job/equipment affinities now rebuild a65-byte derived battle cache;
Unit stays112 bytes, State grows66 including alignment, FSV3/Work stay unchanged.
178 original aggregation profiles and1881 PC/sanitizer/68000 checks pass. FCJ9
stores416 references to32 shared masks in582 bytes, rejects FCJ8/bad references
and preserves all existing ability semantics. This is data/cache preparation:
elemental damage, absorption/healing, nullification and retail spells stay gated.
Native stats now execute locally in PARTY/ACTION/RESPONSE; no stat callback
can evict an executing image. ABI22/ModCtx348/largest6060; five allocation requests.
Private495758 (+2709), input64859/headroom77;27 non-code banks unchanged.
Next: typed elemental outcomes through prediction/AI/impact/responses, with
source rounding/capping, then magic evasion/areas/presentation. Full core and
residency gates remain open. Standard group/live TiEmu untouched; Jump deferred.

**Historical magical-scalar checkpoint —2026-10-27:** [CORE_MAGIC.md](CORE_MAGIC.md).
Formula08 now shares source-checked MA/support/Shell/zodiac/Faith arithmetic
between prediction, execution and actual AI. A private normalized Holy record
exercises vulnerable casting, one payment, one impact and Auto-Potion settlement;
production Holy/Fire and all AF_CAST producers remain disabled.104 original-MIPS
profiles and1352 PC/sanitizer/68000 assertions pass. No element affinities,
magic-evasion spells, monster zodiac or new status/trait producers are enabled.
ABI21/ModCtx348/largest5882; battle/save layouts and five allocations unchanged.
Private493049 (+503), input64919/headroom17;28 non-code banks unchanged.
Pure math remains resident; cold banner/cache setup uses PARTY7/8. Next:
element/target eligibility and magic evasion, then areas and reviewed casting
presentation. L2/L3/L4 and residency gates remain open; Jump stays deferred.
Standard playtest group and live TiEmu remain untouched.

**Historical incantation checkpoint —2026-10-27:** [CORE_INCANTATION.md](CORE_INCANTATION.md).
Single-centre casts now retain a fixed tile or stable unit slot through actual
CT, interruption, one MP payment and root/reaction settlement. Exact-cost paid
effects also fix the preceding preview-affordability recheck. Private carriers
exercise the backend; no retail Holy/Fire formula or binding UI is enabled.
ABI20/twelve images/ModCtx344/largest5862; State/Unit/Work/FSV3 unchanged.
Private492546 (+1481), input64859/headroom77;28 non-code banks unchanged.
COMMAND3/4 own validation/arming/due preparation, with arguments outside State.
Next magical formulas/traits, area target sets and presentation. Broader core/
residency gates stay open; standard group/live TiEmu untouched. Jump deferred.

**Historical casting-resource checkpoint —2026-10-27:** [CORE_CASTING.md](CORE_CASTING.md).
Player/forecast/AI share effective MP eligibility; COMMAND pays once at effect
start, after arming returns. Half MP and preparation timing have original-MIPS
fixtures. This enables no new Holy/Fire effect or caster producer. ABI19/twelve
images, ModCtx336/largest5860; Unit/State/Work/FSV3/five allocations unchanged.
Private491065 (+686), input64743/headroom193;28 non-code banks unchanged.
Cold range construction moves unchanged to COMMAND2; pure cost callback cannot
evict its caller. Spell targeting/formulas/AoE and broader scheduler gates remain
open. Standard group/live TiEmu untouched; earlier checkpoints are history.

**Historical deferred-dispatch checkpoint —2026-10-26:** [CORE_DEFERRED.md](CORE_DEFERRED.md).
Due Charges drain iteratively, in source slot order, before expiry/next CT.
Busy roots preserve the packet; later invalidated casters are rechecked. No new
State/Work/save fields, queue or ABI: ABI18/twelve images unchanged. Private490379
(+8), input64857/headroom79. Source unit-versus-tile coordinates are observed;
magic/MP/AoE, Quick255 priority and all-permanent-freeze liveness remain open.
Jump stays deferred; standard group/live TiEmu untouched. Earlier notes are history.

**Historical dual-Charge checkpoint —2026-10-26:** [CORE_CHARGE_HANDS.md](CORE_CHARGE_HANDS.md).
All eight Archer Charge powers now resolve per hand with source criticals,
shared potential forecast/AI and delayed validation of both weapons.586 targeted
PC/sanitizer/68000 checks; eight original initializations and sixty scalar cases.
Pending's spare byte holds the off weapon: State2124/Unit112/Work45516/FSV3 and
five allocations unchanged. ABI18/twelve images, ModCtx332, largest5814.
Private490371 (+1570), packed input64845/headroom91;28 non-code banks unchanged.
Pure math stays resident; cold pickup moves to PARTY6. Broader targeting,
weapon procs/elements, action/status families and combined residency remain
open. Jump stays deferred; standard group/live TiEmu untouched. Earlier dated
checkpoints below are historical; see CORE_ROADMAP.md for current priorities.

**Historical hands checkpoint —2026-10-26:** [CORE_HANDS.md](CORE_HANDS.md).
Ordinary supported Two Swords and dual Counter now use independent hands;
forecast/AI share potential damage and Sleep wake. Management returns an illegal
left weapon to stock.196+16 targeted PC/sanitizer/native checks; FSV3 unchanged.
FCJ8 adds20 innate bytes/reuses Item.pad; ABI17/twelve images split COMMAND and
RESPONSE from impact. Native State2124 (+30), Unit112/Work45516 unchanged.
Private488801 (+5387), input64821/headroom115, zone5750. Further resident growth
still needs a module budget. Broader weapon/skill/element/proc and original AI
families remain open; full L2/L3/content/residency gates stay open. Jump deferred.
Standard group/live TiEmu untouched. Earlier checkpoint numbers are historical.

**Current consecutive-strike checkpoint —2026-10-26:** [CORE_STRIKES.md](CORE_STRIKES.md).
The bounded ordinary-Attack engine entry resolves two primary impacts before
one response drain and best-EXP/one-JP settlement;250 PC/sanitizer/native checks.
Normal commands still request one: independent hands, preview/AI and the actual
Two Swords producer remain open. ABI16/ten images, CoreAction34/State2094 (+2),
Unit112/Work45516/FSV3 unchanged. Private483414 (+836), input64923/headroom13,
zone5830; data/pictures unchanged. Relocate further resident growth first.
Next: hand-specific policies and more reaction families; full L2/L3/content and
residency gates remain open. Jump deferred; standard group/live TiEmu untouched.
Earlier checkpoint numbers below are historical.

**Current reaction checkpoint —2026-10-26:** [CORE_REACTIONS.md](CORE_REACTIONS.md).
Counter/Auto-Potion now queue, revalidate, execute and restore the primary packet.
Equipped party IDs load from FSV3. Shared stock, secondary suppression, deferred
root rewards and terminal ordering are covered by267 PC/sanitizer/native checks.
ABI15/ten images, State2092 (+46), Unit112/Work45516/FSV3 and allocation count
unchanged. Private482578 (+4008), input64605/headroom331, zone5742; data unchanged.
Next: source strike sequencing and more reaction families; general L2/L3/content
and timing/residency gates remain open. Jump deferred; live TiEmu untouched.
Earlier checkpoint numbers below are historical.

**Current action-context checkpoint —2026-10-26:** [CORE_CONTEXT.md](CORE_CONTEXT.md).
Live roots protect costs/impact/settlement; a hit or miss commits once, and a new
command cannot reset an active action.357 PC/sanitizer/68000 checks cover source
reaction guards and bounded per-actor response primitives. Reaction effects,
packet restoration, producers and multistrike remain next; full L2 stays open.
ABI14/nine images, State2046 (+32), unchanged Unit112/Work45516/FSV3 and allocation
count. Private478570 (+434), input64727/headroom209, zone5742. FCJ7/data unchanged.
Jump deferred; standard group/live TiEmu untouched. Older notes are historical.

**Current Sleep checkpoint —2026-10-26:** [CORE_SLEEP.md](CORE_SLEEP.md).
Sleep now shares source60-tick CT freeze, Charge cancellation, physical XA
vulnerability before zodiac and protected wake at successful HP-damage impact.
1070 PC/sanitizer/68000 assertions pass. No new producer/art or allocation;
Unit112/State2014/Work45516/FSV3 unchanged. ABI13/nine images, private478136
(+112), input64837/headroom99, zone5742. FCJ7/data banks unchanged.
Next: explicit action/reaction contexts; broader effects, timing/residency and
content-production gates remain open. Jump deferred. Earlier notes are history.

**Current control checkpoint —2026-10-26:** [CORE_CONTROL.md](CORE_CONTROL.md).
Stop pauses CT/Charge; Don't Move/Don't Act share player/preview/AI restrictions,
source CT credit and due-time Charge validation.594 targeted PC/sanitizer/native
checks pass. Cached flags reuse existing fields; ABI12/nine images, unchanged
Unit/State/Work/FSV3. Cold AT/glyph setup moved to existing images, with a safe
scene-to-battle return boundary. Private478024 (+1340), input64847/headroom89;
FCJ7/data banks unchanged. Next: Sleep damage/wake and action/reaction contexts.
Full L2/content gates remain open; Jump deferred. Older notes are historical.

**Current Poison/Regen lifetime checkpoint —2026-10-26:** [CORE_STATUS_LIFETIME.md](CORE_STATUS_LIFETIME.md).
Source counters, application/renewal/removal, clock expiry and end-turn HP upkeep
now pass4140 PC/sanitizer/native assertions with13 actors. KO cancels mutable
timed/command effects; menus pause CT. FCJ7 remains unchanged; ABI11/nine images,
Unit112/State2014 (+210 RAM bytes), unchanged Work/FSV3 and allocation count.
Private group476684 (+1168), packed input64789/headroom147. Next: incapacitation,
cast interruption and action/reaction contexts. Full L2/content gates stay open;
Jump deferred. Earlier checkpoints below are historical.

**Current status-state checkpoint —2026-10-26:** [CORE_STATUS_STATE.md](CORE_STATUS_STATE.md).
Canonical forty-bit storage/filtering, immunity and innate-protected cures now
share source policy; Charge/Defend command state is consistent. Gameplay effects
remain a restricted subset; expiry/upkeep and reaction contexts are next.
FCJ7/ABI10, Unit96/State1804 (+218 RAM bytes), Work45516/FSV3 unchanged.
Private group475516 (+1877), packed input64673/headroom263.4842 targeted
PC/sanitizer/native checks pass. Earlier checkpoints below are historical;
Jump stays deferred. Standard bundle and live TiEmu are unchanged.

**Darkness/Silence checkpoint —2026-10-26:** [CORE_STATUS_RULES.md](CORE_STATUS_RULES.md).
Physical preview and commit now apply attacker Darkness and target Defend through
one rule. Player/preview/AI share the source Silence flag/menu restriction.
FCJ6 adds224 archived menu bytes and strict length/version rejection; no new
battle/save fields or heap allocation.433 native status checks plus existing
primary actions/Charge pass. Private group473639 (+1174), ABI9/nine images,
packed input64575/headroom361. Full status policy, poison upkeep and reaction
contexts remain open; Jump deferred. Standard bundle/live TiEmu stay unchanged.

**Shared primary actions checkpoint —2026-10-25:** [CORE_ACTIONS.md](CORE_ACTIONS.md).
Impact commit is separated from animation; combat, AI and healing forecasts share
pure HP/status transitions. Existing primary effects and six action pictures
remain identical before/after on PC/68000. ABI8/nine images; no State/Work/Save or
heap growth. Private group472465 bytes (+464); resident packed input64177,
headroom759. Impact relocation costs extra CPU (scoped peak33480 cycles).
Next: source-backed status policy/action contexts. Full L2/L1 and production
gates remain open; Jump refinements deferred; standard group/live TiEmu untouched.

**Common compact topology closed —2026-10-25:** [CORE_COMPACT.md](CORE_COMPACT.md).
ABI7/nine images use36-byte lower-only or68-byte two-floor row storage, with
four cached neighbors per node. Native Gariland heap requests fall1798 bytes;
Orbonne1678. Actual ordinary search CPU is preserved; wider searches are about
1.6% cheaper in the scoped two-map comparison. Generic/Work search now represents
cost6 and32-link rows; live gameplay still emits one-gap links. Work45516 (+2),
State1586 and FSV3 unchanged. Private group472001 bytes (+119),28 identical data
banks, accepted Status/range views preserved; packed input64825/headroom111.
**User priority: Jump is minor. Finish this common-storage verification, then
advance L2 actions/statuses; defer Jump refinements.** Full L1 and production
readiness remain open; standard group/live TiEmu are unchanged.

**Current consolidated roadmap:** [CORE_ROADMAP.md](CORE_ROADMAP.md). L1 is
advanced but incomplete; L2/L3 still require major rules work, and L4 production
gates remain open. Dated checkpoint notes below retain their historical scope.

**Resident capacity / actor-column checkpoint —2026-10-25:** [CORE_RESIDENT.md](CORE_RESIDENT.md).
SAVE is split from encounter/party initialization (new PARTY image); unchanged
Status keys now run in SCREENS. ABI6/nine FMB2 images reduce the reserved zone
6074→5734. Wider-cost search is now linked by default, with unchanged Work/State/
Save layouts and fast ordinary movement. Private group471882 bytes (+1438),28
unchanged data hashes. Packed input64831 bytes leaves105 below TTPack's limit;
new build reports expose this separately from compressed size. Targeted flow,
save, Status, search and accepted-view checks pass.88 original actor-column
queries also feed a99-check PC/sanitized/68000 prototype. Runtime still has
one-gap links: bounded longer links, actor-record production and wet costs
remain next. Standard group and live TiEmu are unchanged.

**Wider-cost search prerequisite —2026-10-25:** [CORE_SPAN_SEARCH.md](CORE_SPAN_SEARCH.md).
An optional eight-bucket linked frontier supports abstract costs1..5 while
reusing the existing queue storage.27592 focused PC/sanitized/68000 checks pass;
actual Work/caller tests pass1219 Gariland and1065 Orbonne checks, including
cost/predecessor equality, representation restarts and an abstract cost5 path.
Fresh native Search1054/Work45514/State1586/Save3138 sizes are unchanged.
`FFT_NAV_SPANS` is TEST-ONLY in the bound battle build: making it resident exceeds
TTPack's64936-byte input ceiling. The ordinary package builds at470444 bytes
(+37), with28 unchanged non-code variable hashes and accepted views unchanged.
Before enabling longer jumps, solve resident code headroom, bounded links and
all-intermediate actor bodies together. Shipping still has one-gap jumps.
The standard group/live TiEmu are untouched; no exhaustive audit rerun.

**Accepted movement/cache milestone —2026-10-25:** [CORE_STEPS.md](CORE_STEPS.md).
Forward source edges, asymmetric slope/headroom gates and ordinary water exits
replace stand-height approximations. User accepted Gariland's changed rings;
Orbonne remains unchanged.112 original endpoint fixtures feed451 kernel checks;
80 battle checks and cached-versus-reference graph/search checks (628/588) pass
PC/sanitizers/68000. Scratch reuse cuts cold graph CPU by30.77%/28.18% over
Jump1..4 with no extra allocation or fields. Private group470407 bytes (+178),
42 variables; ABI5/FMB2/FSV3 unchanged. All28 non-code hashes match the previous
group. Standard bundle and live TiEmu remain untouched; no exhaustive rerun.

**Next long-span gate:** [CORE_LONG_JUMPS.md](CORE_LONG_JUMPS.md).22 original full
spread trials feed278 geometry checks on PC/sanitizers/68000. Ordinary Jump is
capped at7 (static config evidence), allowing up to3 intervening cells. This is
a test-only prototype: shipping still supports one gap. Before activation,
extend bounded link storage, queue cost support and all-intermediate actor
checks together. Actor/special movement, raster, action/status pipeline, peak
combined residency and production formats/editors remain open.

**Previous recoil/test-scope checkpoint —2026-10-25:** [CORE_KNOCKBACK.md](CORE_KNOCKBACK.md).
Source-bound recoil selects the highest eligible floor by facing edges and
headroom, with no fallback after chosen-floor occupancy/targetability rejection.
The cached single-floor fast path remains.68 focused original queries feed172
PC/sanitized/68000 geometry checks;82 actual battle hit/placement/cache checks
pass. Private `x/core-knockback/fftcampaign.89g`:470229 bytes (+395),42 variables;
ABI5/eight modules and storage fields remain unchanged. All28 non-code variable
hashes match the prior group. No full scene/decoder/profile sweep was rerun;
[CORE_TESTING.md](CORE_TESTING.md) is the default impact policy, with2574 focused
profile checks and an explicit `--deep` exhaustive certificate. Endpoint/long-
jump/actor/raster/action contexts remain open; standard group/live TiEmu unchanged.

**Previous per-map layer checkpoint —2026-10-25:** [CORE_MAP_PROFILE.md](CORE_MAP_PROFILE.md).
User accepted CORE_JUMPS; its Decision marker is cleared. The new census covers
109 event/random-referenced maps and214 terrain variants:77 lack upper stop
cells,66 can omit upper movement queries,64 can omit positive-height upper ray
queries. Cached independent flags skip those paths without removing lower-floor
height/slope rules.3,234,825 PC/sanitized/68000 equivalence checks pass;169
accepted images remain identical. ABI5/eight FMB2 images reduce the resident
module zone7406→6074; Work45514 and the five battle heap requests are unchanged.
Private `x/core-single-floor/fftcampaign.89g`:469834 bytes (+734, including the
accepted jump rule). Full upper traversal/raster cover, action/status mechanics,
combined residency and production formats remain open. Standard group/live
TiEmu are unchanged; production stays closed.

**Previous source-surface checkpoint —2026-10-25:** [CORE_SURFACES.md](CORE_SURFACES.md).
Both chapter descriptors now contain validated source solid spans. Straight-ray
AI collision keeps upper solids even without a walkable node; ordinary movement
filters endpoint headroom.9294 original-MIPS queries feed9462 portable/native
checks;55 battle integration checks and1846 boundary checks pass. Existing
movement/replays pass and169 images remain identical. A data-derived classifier
removes Gariland's extra cold ceiling cost with an identical graph. Private
`x/core-terrain/fftcampaign.89g`:469100 bytes (+3033); native Work45514 (+4),
five Gariland blocks116094, maximum nodes/current scene121182. ABI4/FMB2,
FSV3 unchanged. Full source upper paths, raster bridge masks, special movement,
ballistic/player targeting, action/status pipeline and combined residency remain
open. The production gate is closed; standard bundle/live TiEmu are unchanged.

**Previous256-node tactical checkpoint —2026-10-24:** [CORE_FULL_GRID.md](CORE_FULL_GRID.md).
The battle now supports13 actors/256 compact nodes with16-bit counts/endpoints,
valid node255, complete AI origin/selection iteration, range completion and
rotation sorting.3181 PC/sanitized/68000 checks pass, including two floors and
allocation-failure rollback;37 campaign and24 opening/scene/control replays pass
and169 reference images remain identical. AI origin merge sorting preserves the
frozen tie order and makes the pathological setup zone about9.25× faster.
Topology is a fifth heap block; Gariland requests116090 bytes (+4770), maximum
nodes with the current scene121178, excluding full residency/stack. Largest block
61670; Unit remains80 bytes. Private `x/core-256/fftcampaign.89g`:466067 bytes
(+501). Source upper-floor traversal/LoS/cover, full-scene extrema, maximum
combined residency and formats/editors remain open; the production gate stays
closed. Standard groups and user-owned live TiEmu are unchanged.


**Previous battle-cell milestone —2026-10-24:** [CORE_CELL_STATE.md](CORE_CELL_STATE.md).
Canonical cells now drive units/cursor, compact paths, movement undo, Charge,
knockback, AI placement and feet-cache keys.128 actual battle checks plus68
FMB2 module-bank checks pass on PC/sanitizers/68000; the256-node kernel still
passes13887 checks and91186 old movement comparisons agree.37 campaign and24
opening/scene/control replays pass;169 images remain identical. Unit remains80
bytes, State1480, Work48046; four battle blocks request111320 bytes (+3598),
not combined residency/stack. Seven overlays reduce the largest image to7404.
Private `x/core-floors/fftcampaign.89g`:465566 bytes (+746 versus geometry).
Full256-node battle counts/arrays, source upper-floor traversal/LoS/cover and
matching archived topology remain open; the production gate is closed. Standard
groups and the user-owned live TiEmu are untouched.


**Previous partial geometry migration —2026-10-24:**
[CORE_GEOMETRY.md](CORE_GEOMETRY.md): compact identity/search for256 nodes across
two floors passes13887 native checks; active paths match the old search in91186
comparisons. Battle unit/cursor/target/cover/count adapters remain single-floor.
Current Work/save ownership, memory formulas and private package supersede the
earlier baseline figures below. The production gate is still closed.

**Subsequent implementation —2026-10-24:** [CORE_CAPACITY.md](CORE_CAPACITY.md)
records the completed13-capacity/descriptor-count migration,1813 native boundary
checks, rendering controls, current heap formulas and467425-byte private package.
The geometry, action/status and production gates below remain open. Historical
eleven-slot baseline measurements are retained with their original scope.

**Reduced port envelope, subsequent user decision —2026-10-24:**
[PORT_LIMITS.md](PORT_LIMITS.md) supersedes the earlier21-slot/512-node target.
Use13 actors and256 compact8-bit node IDs across both floors,16-bit counts and
explicit validity. Exclude the eleven-Monk encounter and known test maps from
content budgeting. The raw original census below remains research evidence;
it does not require copying the source's storage layout into the TI engine.

2026-10-24. User-authorized workstream4. This document implements the audit brief
dated2026-10-23. **Production gate is closed.** The existing opening loop is a
regression baseline, not a complete battle core. The previous UI follow-up is
accepted; the user's TiEmu and saves were not touched.

## Evidence and reproducibility

Three labels apply throughout:

- **OBSERVED / data**: read directly from the local retail-US disc, with resource
  extents, IDs and SHA256s in the generated census.
- **STATIC / code**: behavior read in the existing local decompilation at commit
  `83c1e00d410277fb0a3f9acf10ca2da1e8dabe22`. This audit did not rebuild or
  independently verify its matching claims. Comments alone are not runtime proof.
- **TARGET / proposed**: the TI design or synthetic stress input. A proposed
  envelope is not an observed original-game maximum.

Primary reference: [pinned original-code study](https://github.com/adamrt/fft_decomp/tree/83c1e00d410277fb0a3f9acf10ca2da1e8dabe22).
Relevant files are `include/fft/{unit,data,map}.h`,
`battle_unit_{load_entd_units,init_deployed_units_data}.c`,
`battle_map_{copy_xy_coords_and_tile_data,get_tile_data_pointer}.c`,
`main_party_find_free_slot.c`, `battle_state_announce_next_charged_action.c`,
`battle_reaction_prepare_next.c` and
`battle_action_init_current_ability_strike_data.c`. The census hashes these
specific local files so later upstream edits cannot silently change the evidence.

From the repository root:

```sh
rtk proxy make -C games/fft/campaign core-audit-test core-audit
rtk proxy make -C games/fft/campaign core-boundaries-check
rtk proxy make -C games/fft/campaign core-audit-bench
rtk proxy tools/pyenv/bin/python games/fft/tools/core_audit.py \
  --state sources/fft_ps1/gariland_t1.state \
  --deployment-state games/fft/campaign/x/mandalia/gariland-deployment.state \
  --out games/fft/campaign/x/core-audit/source-observed.json
```

The optional original-state pass reads21 slots from an existing **controlled**
Gariland fixture; its recipe includes the documented Academy/scenario and
deployment pokes in `oracle.py`. It does not claim an untouched playthrough.
Deployment pointers are read only in the matching original overlay, never from
overlapping battle RAM. No TiEmu interaction is needed.

Local outputs are `x/core-audit/source.json`, `source-observed.json`,
`native-budget.json`, `boundaries/results.json` and `fixtures/*.json`. The initial
native measurement also exists as `budget.json`; future bundle reports use that
name for package size only. Commercial data and generated source snapshots stay
local. Deterministic source/package reports contain no wall-clock timestamps.

## 1. Verified inventory and remaining limit questions

| Domain | Evidence | Implication for the core |
|---|---|---|
| Battle simulation records | STATIC:21, indexed0..20. Normal initialization reads16 ENTD records into0..15 and five deployed records into16..20. OBSERVED in existing Gariland state:0..5 and16..20 present. | Source capacity differs from the user-selected13-actor port limit. Keep source references separate from compact runtime slots. **21 simultaneously fighting actors is not established.** |
| ENTD encounter definitions | OBSERVED:4 files ×128 encounters ×16 records; each record40bytes. Max populated records16 in the512-record census. Retail loader tests `sprite_set !=0`. | Populated is not present: presence flags, scenario scripts, placement and sprite-loading checks alter the actual set. Do not add deployed count to populated count and call it an observed active maximum. |
| Original initial-load filtering | STATIC:loader derives a distinct-sprite allowance from `9-deployment_limit` and a non-off-field encounter allowance from that value+7. Presence`0xC0` is staged off-field. | This explains why21 allocated slots does not prove21 active units. Later script activation and special scenarios need controlled traces. Do not copy PS1 VRAM restrictions into TI game rules. |
| Persistent party | STATIC:20 records =16 Formation members plus4 guest records. Free-slot search uses these two ranges separately. | Current `PARTY_MAX16` correctly describes Formation, but lacks persistent guest records. Guests must not consume or overwrite a recruit's permanent identity. |
| Deployment | STATIC:normal initializer reads five deployed records. OBSERVED:480 scenario records,85 with two squad IDs. Raw overlay capacities include2+2 and3+2. | Model zones with independent world coordinates, elevation and capacity. Raw event445/MAP050 sums5+2: **unresolved outlier**, not proof of seven simultaneous deployments. |
| Terrain storage | STATIC:two fixed256-record layers,8bytes/tile,4096bytes copied after width/height. Lookup is `(layer<<8)+y*width+x`. | Tile identity requires a layer. Our current one-layer Tile/occupancy/path/target model cannot represent bridges correctly. |
| Terrain census | OBSERVED:121 GNS files,228 terrain-bearing mesh resources;121 distinct map IDs. Includes tutorials/noncombat resources and variants. | These are not121 campaign battles. All source variants are inventoried without rendering/adding content. |
| Largest observed grid | OBSERVED:MAP12516×16 =256 cells/layer; axes reach18 in MAP026,039,076.47 map IDs /80 resources have unblocked upper cells. | `map_n:u8`, tile255 as sentinel and150-entry arrays are inadequate.18 is an **observed extent**, not a proven general PS1 axis hard limit. |
| Height | OBSERVED:maximum unblocked source corner31, including MAP036/056; slope half-height and depth are distinct fields. | Current `MAXH10`, scene320px width, fixed202rows and 8-bit standing-height intermediates need a complete range audit. Unblocked does not establish reachable by every movement mode. |
| Jobs | OBSERVED tables / STATIC domain:160 internal job records, **20 generic careers**. | Never describe these as160 selectable careers. Special actors, monsters and generic careers have different progression rules. |
| Ability IDs and lists | OBSERVED:512 primary IDs,368 default range/formula records,176 human sets×22 entries and48 monster sets×4. Human list:16 action positions and6 R/S/M positions. | Table capacity is not the count of learned or executable actions. Sparse/high IDs must survive saves, menus, modules and AI. |
| Status policies | OBSERVED:40 status-policy entries,5-byte masks; STATIC:16 timed-status slots and cancellation/incompatibility masks. | Four flags plus `hp==0` are insufficient. Source policy tables should drive common status transitions. |
| Equipment | STATIC:seven resolved battle equipment fields, including independent hand weapons/shields; native/save currently five fields. | Dual wield, stealing/breaking and per-hand hit order require a versioned schema change. |
| Action phases | STATIC:pre-action Hamedo, acting unit, reacting unit; per-action strike count and per-target result records. | A single `damage then reaction` callback cannot describe the original. Exact multistrike/KO/victory timing still needs source traces. |

The source table census also finds193 default records with an area byte,
144 with nonzero CT and117 with MP cost. These counts include special/internal
records; they measure format requirements, not completed/learnable spell counts.

Additional **STATIC** scalar constraints, read in
`main_unit_calculate_actual_stats`, `main_unit_check_level_up`,
`main_job_calculate_level`, both JP award functions,
`battle_action_increment_item_quantity_for_steal_break`,
`battle_action_add_war_funds` and `world_item_cash_out_excess_inventory`:

| Field / subsystem | Reviewed bound and caveat |
|---|---|
|Level / EXP|level99; level-up threshold100EXP; at level cap EXP clamps99|
|Raw stats|24-bit serialized; growth clamps`0xFFFFFF`|
|Ordinary calculated HP/MP|999; **egg** calculation uses a different divisor and65535 cap, so999 is not a universal serialization bound|
|Calculated Speed / PA / MA|50 /99 /99 before other equipment/status paths; do not infer every intermediate shares these bounds|
|Job points / job level|current and lifetime JP clamp9999 in reviewed award paths; job-level calculation iterates8 thresholds|
|Item ownership|99 including equipped copies; overflow handling differs by path: battle steal/break uses quarter-price funds, world cleanup uses half-price for IDs0..252|
|War funds|0..99999999 in reviewed battle award path|
|Original scheduler/script storage|16 native thread slots;256 signed32-bit script variables. This is not a requirement to reproduce PS1 threading or persist all variables on TI.|

These source-type distinctions belong in the model. They must not turn into
map-name checks or a blanket `u8`/999 clamp. The owning decompilation files are
also SHA256-pinned by the census. Current monster breeding/egg state and complete
equipment/status intermediate arithmetic still require implementation fixtures.

## 2. Current engine assumptions that must be removed

These are specific shared constraints, not requests for per-map workarounds:

- Actor capacity migration is implemented: State/caches/AI/rewards reserve13,
  active loops read the validated FCD3 count, and module context carries it.
  Synthetic13-actor CT/KO/revive/target/result tests pass. Conditional scene
  activation and complete guest/lifecycle semantics still need source traces;
  keep stable source order for CT ties.
- `auto_units`, `scene_hide`, `morph_mask`, `Work.ai_use` and `Work.ab_done` are
  16-bit masks. They remain sufficient for13 compact actors. Retain explicitly
  unsigned shifts and validate slot bounds; source slot20 must not become a
  runtime mask bit20.
- `Work.fc_key` packs the forecast target into four bits, with ability identity
  above it (`draw_hud`). Targets1..13 fit; no widening is required by the revised
  envelope. Keep source identities and runtime target indices distinct.
- `map_n`, tile indices, predecessor/edge arrays, range iterators, cursor keys,
  pending tiles and no-tile markers assume fewer than256 ground tiles. Merely
  widening the count still aliases tile255 with `0xFF`. Adopt compact`u8` node
  IDs across both floors, explicit validity/layer and`u16` counts/loop endpoints
  in shared APIs/module context. See PORT_LIMITS.md for source-address mapping.
- `PATH_MAX16` is a storage limit, not an original-game rule. Refuse invalid
  generated inputs explicitly until streamed/predecessor traversal replaces it.
- Scene beats encode `slot<<4|gfx`; scene storage now uses13 actors as well.
  The separate four-bit scene identity format fits the accepted envelope;
  existing beats keep their own actual actor counts.
- `contour_assets.py` archives tile-coordinate tables specifically for10×15.
  Those tables must become map-owned data. Changing dimensions alone leaves
  movement, cover and source-camera conversion inconsistent.
- `rules.inc:pending_next` cancels every pending state on displacement or weapon
  change. That is only the current Charge rule; reserved Jump/performance enum
  values must not inherit it. `skill_extra` is a special cross splash, not a
  general area/height/target evaluator.
- `save.h` has16 Formation records, no separate guests, five equipment fields,
  five deployment positions and a byte `flow`. Story branches, monster ownership,
  forced parties and chained encounters need explicit versioned data/migration.
- FCD3/FQF2 readers do not yet constitute strict, end-to-end v1 map/scene
  validation. The current audit rejects malformed census input; that does not
  certify every runtime bank reader or freeze the future format.

Keep the original world-coordinate handedness, active isometric views and1px
edges. Perspective stays experimental. Source geometry/cover fixes must apply to
every rotation; no manual actor-offset patches.

## 3. Ranked20-case mechanic/risk/fixture matrix

S = severity, P = probability, each1..5; priority sorts S×P, then severity.
Ratings are engineering judgment. Stable case IDs are in
[`core_fixtures.json`](core_fixtures.json), with executable baseline mappings.
**Pending is not a test pass.** Examples identify a mechanic, not a complete
original-game behavioral trace.

| Rank / ID | S/P | Mechanic and example | Rework risk / minimum fixture | Status |
|---|---|---|---|---|
|1 C01|5/5|13-actor envelope, Zirekile|all camps, KO/resurrection/off-field; reject content above13|synthetic13 native capacity passes; original conditional activation trace pending|
|2 C02|5/5|Two elevations, MAP001|same x/y on different floors; independent occupancy, targeting, landing and cover|actual cell/rules native pass; source traversal/LoS/cover pending|
|3 C03|5/5|256 compact nodes, synthetic boundary|node255 valid, count256; source layer addresses mapped separately|13-actor/256-node tactical native pass; source render/full residency pending|
|4 C04|5/5|Petrify/Undead/Reraise/Charm|duration, immunity, cancellation, effective team and terminal conditions|four-status subset|
|5 C05|5/5|Two Swords/Hamedo/Auto-Potion|pre-action response, miss, two hits, reaction, KO, victory; exact trace|trace and mechanism open|
|6 C06|5/5|Charge vs Fire|tile vs unit identity; move/kill caster/target; simultaneous due ticks|Charge subset checked|
|7 C07|5/5|Large allocation blocks|max scene/AI/poses; repeated loads; largest free block, stack and allocation failure|current layouts/decode measured|
|8 C08|5/4|Formation plus guests|join/leave/reserve/recruit/monster; stable identity across save/compaction|guest model open|
|9 C09|5/4|Consecutive battles|atomic payout/progress, save/reload, defeat/retry, version mismatch|opening receipt subset checked|
|10 C10|5/4|Tall/long maps|all rotations, clipping, slopes/cover after knockback|source extrema inventoried|
|11 C11|4/5|Lancer Jump|untargetable/absent airborne actor; empty landing tile; interruption|reserved enum only|
|12 C12|4/5|AoE/Truth/Math Skill|all eligible targets; friendly fire; multiple hits and rewards|general mechanism open|
|13 C14|4/5|Two hands, Steal/Break|independent hand outcomes, equipment loss and save round-trip|five-field model|
|14 C15|4/4|Bard/Dancer|equal-tick repeated pulses, interruption, reevaluate targets|reserved enum only|
|15 C16|4/4|Teleport/Fly/Float|height/terrain rules, impasse, bounded search|ordinary movement only|
|16 C17|4/4|Concurrent scene actors|four motions, dialogue/camera/barrier, spawn/despawn/condition|VM comparison pending|
|17 C20|4/4|AI/cache pressure|many actions/actors, same evaluator, bounded work, cover invalidation|baseline; B06/B07 open|
|18 C13|5/3|Counter/Reflect/Mime|root/context guards; bounded queue; overflow traps|source traces needed|
|19 C19|5/3|Formats/editors|bad IDs/spans/modules, deterministic round-trip, old data rejection|audit validators only|
|20 C18|4/3|Terrain mutation|opcode40 / geomancy lookup; invalidate topology/LoS/cover together|source opcode known|

Synthetic input generator now creates13-actor,256-node and4×18/two-floor snapshots,
including two ordinary KOs and a fixed-tile due queue.
Its validator checks the draft input envelope/references, **not gameplay**.
These are starting inputs for future core adapters. No missing source expected
trace is synthesized from our own implementation.

## 4. Proposed common core contracts

**Identity and lifecycle.** Reserve13 compact simulation identities; descriptor supplies
the populated/active set. Keep Formation16 and guests4 as distinct persistent
collections. Scene actors may have visual identities without becoming tactical
occupants. Removal, off-field, KO, crystal and chest are distinct transitions.
Ordinary KO returns with the party after victory; converted/gone actors follow
the existing removal/reward rules. Do not compact live battle slots.

**Actions.** One pure evaluator describes validation, target set, cost, outcome
and AI utility; the committing path consumes the same description. Target mode
is independent of timing: `unit_id` versus `TileId`. Execution kinds are immediate,
vulnerable preparation, protected Jump and recurring performance. Include a
pre-action hook (Hamedo), per-strike state and post-action reaction context.
Check terminal conditions at an observed stable boundary; do not announce victory
halfway through an action chain. HP/MP/stat/status/equipment changes flow through
common commit helpers so every target invalidates its current pose/cover cache.

**Scheduler.** CT advances only when simulation requests it. Menus/dialogue/
animation do not tick. Order equal-tick work deterministically by observed phase
and stable slot; retain all due work before advancing again. Keep one pending
mechanism per actor until source evidence requires more. Use a bounded event
queue with root action/context and explicit overflow diagnostics. Derive its
capacity from worst-case strike×target×reaction fixtures; **do not decree an
untested32/64-event bound or recursively append all possible descendants.**
Process one strike/target at a time to bound storage; record queue high-water.
Loop prevention should follow permitted source reaction contexts; a safety
ceiling detects a bug, not silently changes combat rules.

**Statuses.** Five-byte IDs/masks with shared policy data: application, immunity,
cancel/incompatibility, timed expiry and hooks for CT, action, movement, damage,
team and terminal conditions. Store minimal permanent/current fields and timer
slots; keep per-action inflicted/removed scratch separate. Source5-byte status
sets are not the same as the existing native ST_* bit numbers.

**Geometry.** `layer*256 + y*width+x` is a source address; generate a compact
8-bit node ID plus position/elevation data. Count/loop endpoints are16-bit;
missing-node validity is separate. Preserve slope corners, surface/depth, blocked/cannot-stop,
ceiling and targeting policies. Map-owned precomputed topology/coordinates keep
hot loops small; variable runtime terrain uses an override/invalidation layer.
The retained source census supports a256-node budget including conservative
lower/transit geometry and upper candidates. Four archived views need a dimension/origin contract;320px width
cannot simply clip an18-cell-axis map. Keep one documented world-to-screen
conversion shared by input, scenes, projections, facings and masks.

**Scenes/campaign.** Compile dialogue, movement, animation, camera, wait/barrier,
condition, presence and transition offline. Keep compact bytecode as the initial
candidate and compare with specialized sequences using the same four-moving-actor
fixture, native state/plane equality, code+data bytes, RAM and cycles. No runtime
text parsing or new glyph strips. Known speakers always retain shared portraits.
Conditions/objectives/next encounter/save checkpoints are data, separate from
presentation. Commit reward/progress receipts exactly once; migrate saves
explicitly and never rewrite the only supported save with unknown-version data.

## 5. Size, RAM and compression baseline

Measured group SHA256:
`8f98d7470453ca558cde03557d1d80ae9efe99c6d0d5a8ee12e42674d97b4a66`.
Group466045bytes =464955 variable payload bytes +1090 transport bytes;
42 variables. Payloads include their TI type trailers. The report reads/checks
the group itself, not loose files which might belong to another build.

| Packaged category | Bytes |
|---|---:|
| Code/modules/packed cores with embedded constants |131803|
| Actors and pose-index banks |86655|
| Battle views and occlusion, including shared contour tails |88042|
| Catalogue, encounter interfaces and descriptors |46341|
| UI, portraits, weapons, title |73598|
| Scenes/flow/shared font |20842|
| World map |17674|

The6556-byte accepted font is embedded inside flow. These broad categories are
disjoint but not a complete semantic split: embedded constants, per-bank
portraits/scripts and shared tables need section-level accounting. Audio has
no packaged bank. Save state3138bytes is created at runtime, not shipped here.
`ffca` payload64899bytes triggers the provisional64000-byte per-variable planning
alert. This is headroom accounting, not a claim the current valid bank fails AMS.

Actual68000 compiler layout, not the64-bit host ABI:

| Allocation / object | Bytes |
|---|---:|
| Unit / State (eleven units) |80 /1314|
| Work |42636|
| Depth buffer,4×40×202 |32320|
| Scene colors plus Work,2×40×202+Work |58796|
| Actor decoded-pose cache |10176|
| Management cache |7680|
| SaveUnit / Save |176 /3138|

The three battle buffers sum101292bytes, before State, program/module residency,
runtime/grayscale storage, stack and allocator overhead. **This is an allocation
formula, not measured peak RAM.** Repeated load/unload fragmentation, largest
available block and stack high-water remain to instrument on isolated hardware.
Existing native packaging evidence remains in `PACKAGE_CHECK.md`.

Verified existing ASM ZX0 decoder,16 streams ×3 repetitions; decoded length and
checksum matched the PC for all48 runs:

| Map | Four-view raw bytes | Pure ZX0 bytes | Group view payloads | CPU per full two-stream decode at12MHz |
|---|---:|---:|---:|---:|
| Orbonne |184320|35695|42603|162.4–179.6ms|
| Gariland |193920|38530|45439|164.7–178.4ms|

Depth-mask rows are still XOR encoded at this boundary; row reconstruction,
VAT/link/Flash access, drawing and presentation are excluded. Group payloads also
include headers, shared tails and TI trailers. No recompression or visual changes
were performed. Codec CPU times are not loading/rotation/input latency or FPS.

Provisional planning policy: report every bundle, alert above2MiB total or64000
bytes for one variable, and compare size with the previous report's package SHA.
These alerts are adjustable engineering budgets, **not measured free archive**.
At today's two-map view payloads alone,50 similarly sized maps suggest roughly
2.1–2.3MB before shared content and scripts; this is a sensitivity example, not
a forecast. Benchmark all representative heavy/upper-layer maps before setting
the final per-chapter envelope. Reuse/deduplicate poses, portraits and geometry
first; compare layer/surface/depth encodings on identical output. Shared
dictionary or second compression must earn its storage/RAM/decode cost in data.

## 6. Modification costs and regression requirements

Only the baseline sizes above are compiled measurements. Future code ranges are
planning estimates, to replace with linked/native measurements in each slice.

| Change | RAM/data estimate | Code estimate / required check |
|---|---|---|
|11→13 unit-shaped fields|+180 State,+150 Work for current arrays; existing16-bit masks sufficient; excludes new rules|roughly0.5–2KB plus ABI/descriptor changes; preserve existing11-unit pixels, CT order, masks and save bytes|
|40-status policy|640 archived policy bytes; proposed3×5 mask bytes+16 timers per13 units ≈390B above one status byte|roughly1–3KB shared policy/hooks; source fixtures for immunity, cancellation and timed transitions|
|Independent deferred modes|e.g.16-byte proposed record×13 =208B total; event scratch budget undecided|roughly1–3KB mechanism code, formulas separate; Charge baseline plus cast/Jump/song traces|
|Persistent guests|4×176=704B before flags/identity metadata; two extra hand fields add40B over20 persistent records|versioned migration, separate ownership; round-trip every old save and preserve reserves|
|Layered tiles/full map|current per-node State+Work arrays about41B;150→256 adds about4346B before new lookup metadata|retain8-bit compact topology; scene bounds and allocation margin still need native maximum-map fixtures|
|Scenes, formats, editors|not sized yet; code/data tradeoff depends on VM comparison|no invented disk estimate; compare identical script outputs and validate bad blobs before freezing v1|

Do not make all arrays13-wide and call capacity solved. Descriptor counts,
module signatures, signedness, typed bit shifts, occupancy, active filtering,
forecast identities, actor slots, sorting, targeting and AI all move together.

## 7. Format/editor contract and production gates

Reserve a small, documented header for new map/scene containers: magic, version,
declared length, counts, section offsets and compatible core/module ABI. Keep
source map, encounter, scene, unit and ability IDs stable; no dependence on export
order. PC validator must check lengths/overlaps/alignment/ranges/references,
required portrait bindings, target mechanisms, deployment assignments and
supported features. Deterministic export must produce byte-identical blobs.
TI performs a small bounds/version/feature check, not schema parsing.

Current FCD3/FCJ5/FQF2/FSV3 remain current formats; **none is renamed/frozen as
the requested map/scene v1.** FCD3 replaces FCD2 for count/reward metadata and
rejects old descriptors with the new core. Old saves migrate explicitly. Unknown
data revisions fail with a useful diagnostic before allocation or execution.

Minimal PC map editor, after geometry contract: grid/layers/heights/surfaces,
blocked/target policies, placements/zones, four original reference views and
projected actor/cover inspection, validated import/export. Minimal scene editor:
shared IDs, command sequence/timeline, parallel actors, barriers/conditions,
camera/dialogue with portraits, same core preview and validated export. A
generic JSON editor or static screenshot does not meet these editor gates.

| Gate | Current status |
|---|---|
|Two maps, full play after save/load|native flow/save fixtures pass; latest complete hardware playthrough remains pending|
|13-unit stress and all mechanism-family fixtures|synthetic13 rules/rendering pass; conditional lifecycle and remaining mechanism families open|
|RAM/stack/fragmentation/AI worst-case bounded|open; current layouts/decode and historical package probes available|
|Validated map/scene v1 and editor export|open|
|Per-build size/performance reports with comparison|bundle size hook added; native bench is explicit; full per-frame/per-map performance gate still open|
|Every new mechanic has source trace and regression fixture|required by fixture registry; most future families remain pending|
|Known exceptions tracked|B06/B07 still open; deployment event445 and later script activation remain research questions|

## 8. Work order

The consolidated live plan is [CORE_ROADMAP.md](CORE_ROADMAP.md). Current state:

1. **L0 inventory/envelope:** baseline completed;13/256 scope selected. Original
conditional activation and exceptional encounter policies remain explicit.
2. **L1 board/geometry:** actual actor/tile/count/identity migration passes.
Ordinary source edge/headroom/recoil and single-floor profiles pass. Long-span
terrain/body/cost primitives and a wider queue are verified separately; runtime
links/records, full upper-floor targeting/cover, extrema, special movement and
mutable terrain remain open. The six-point cost boundary is now documented in
CORE_JUMP_COSTS.md, not silently mapped into the current1..5 link kinds.
3. **L2 shared rules:** common action pipeline, statuses, reaction contexts,
AoE/multihit, deferred casting/Jump/performance and equal-tick ordering. Basic
and Archer Charge subsets are not family completion.
4. **L3 families/progression/campaign:** expand formulas/passives and shared AI,
independent hands/guests/monsters, objectives/conditions and chained save/retry.
Only22 action execution records are enabled today; later game content needs
its required families, not just existing catalogue IDs.
5. **L4 production readiness:** validated/frozen map/scene v1, scene execution
contract, minimum editors/export, worst-case RAM/stack/CPU and two-map complete
save/load pilot. Only then import maps/scenarios in batches of5 with feature
checks and per-map reports.

This is a dependency order, not a percentage or a three-week completion claim.
A helper/oracle certificate closes its tested slice; it does not close a wider
runtime gate. Content requiring a missing family first gets that mechanic and
a regression fixture; no per-map hard-coded exception.
