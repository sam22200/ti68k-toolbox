# FFT interaction rules — required before UI or rendering work

User decisions,2026-10-24. Read this before changing combat, deployment,
management, world map, title or save UI. These rules take priority over older
review candidates. Performance and visibility remain the platform constraints.

Core content envelope:13 battle actors; omit the eleven-Monk random encounter.
Use256 compact8-bit tactical-node IDs across both floors,16-bit counts and
explicit validity/coordinate/elevation fields. This target and its evidence are
in [campaign/PORT_LIMITS.md](campaign/PORT_LIMITS.md); do not inherit the source's
21-slot/512-cell storage as the TI allocation requirement.

- **AI turns have a deliberate frozen pause of1–2 seconds.** Target1.5 seconds
  of elapsed time including calculation and initial camera settling. If the plan
  is already ready, wait only for the remainder; if it takes longer, do not add
  another full pause. Keep the stable picture while thinking/waiting. The user
  accepts the measured roughly one-second setup calculation here; do not force
  this frozen workload into the interactive30fps budget. Fast-forward cannot
  substitute update count for elapsed time. A replan after acting gets no fresh
  turn pause. This exception does not relax player-menu/input feedback below.
- **Input feedback is mandatory work on the first rendered frame.** Moving a
  selector, opening/cancelling a range, removing an overlay and confirming a
  choice must update the visible picture immediately. Do not queue cleanup
  behind optional background work. Cache static geometry/stats; measure native
  warm input frames and cold transitions separately. PC speed is not evidence
  of calculator responsiveness.
- **Status restrictions share one eligibility policy** across commands, previews
  and AI. A blocked command is not a spent command; rejection cannot consume
  inventory, RNG or a pending action. Invalidate affected range/menu caches on
  application/removal and return illegal targeting to a safe choice boundary.
- **MP uses the same resource policy for player, preview and AI.** Eligibility
  queries spend nothing; reject insufficient or overlapping orders before
  changing State/RNG/pending casts. A valid execution pays once even on a miss.
  Deferred arming spends no MP; its eventual resolution must revalidate the
  cost and restrictions. After paying, effect hit evaluation must not reject
  against the remaining MP again. Enemy unlimited-item policy does not imply free MP.
- **One live action owns its costs, impact and settlement.** A new command must
  not reset an unfinished action. A hit or miss consumes its impact once; final
  victory/turn checks wait until responses have settled. Keep preview/AI queries
  free of costs and effects. A raw injected impact needs an explicit root context.
- **Resource outcomes share preparation, preview, AI and commit.** Freeze both
  HP and MP quantities before wind-up; a paired target quantity must never be
  treated as a caster cost. Issuing an action invalidates the scalar forecast
  cache, so unchanged target/ability IDs cannot retain stale HP/MP predictions.
  A restorative item pays once even when no reserve changes; no-change HP/MP
  grants no item EXP/JP. Reuse shared resource labels, fonts and menus.
- **Cast binding and interruption are separate policies.** A tile binding
  retains its compact cell; a unit binding follows its stable battle slot.
  Ordinary incantation does not inherit Archer movement/weapon cancellation
  or its movement cost. Unimplemented area/performance producers stay gated.
- **Area membership is separate from effect eligibility and payment.** KO/Petrify/
  Wall can occupy a spell footprint while finalization rejects their effect.
  Unit-following centres follow coordinates even if the unit becomes Crystal/Jump.
  Prepare every target outcome before committing HP, pay once, retain source
  slot order and settle all responses/rewards before terminal checks. A pure
  target mask alone does not enable area spells (campaign/CORE_TARGETS.md).
  The gated60-byte CastBatch backend implements preparation/payment/commit; its
  caller must retain storage and route the actual impact explicitly.
  cast_order/due casts now use a dedicated Work owner and automatic impact;
  isolated packets still need explicit commit. Scenario/retry must clear both
  State ownership and Work phase (campaign/CORE_AREA_CT.md).
- **An effect forecast cannot become an impact.** Area forecasts use a separate
  caller-owned packet/FORECAST phase, report per-victim probability and never
  write the live Work cast owner or consume MP/RNG. Commands still enforce their
  own eligibility before offering/executing a choice; effect inspection after
  payment must not reject against remaining MP. Refresh from live positions and
  traits rather than treating a forecast as a frozen future outcome. Cache and
  budget repeated AI queries (campaign/CORE_AREA_FORECAST.md).
- **Area targeting chooses the centre, never a subset of victims.** The approved
  list inspects the footprint, signed HP forecasts and allied victims. Browsing
  changes the detail card only. Unit follows the centre actor during charging;
  Panel fixes a tile. Empty centres and source range NO_FOLLOW_TARGET bit0x10
  offer Panel/Cancel only. Use shared cards/animated choice menus, both ENTER/2nd,
  cancel without payment and independent forecast storage. Native integration:
  campaign/CORE_AREA_UI.md. Per-target/empty impact presentation remains separate.
- **Area painting uses execution results, never rerolls.** Show each retained
  victim result only after batch APPLIED; do not cache a fake scalar Miss during
  READY. Empty footprints still pulse their resolution tile and never pretend
  the caster was hit. Stage glyphs during wind-up; suppress root FX during
  secondary reactions and preserve existing EXP/JP/terminal draining. Approved
  implementation: campaign/CORE_AREA_IMPACT.md; Oil is the supported extra status
  line, not a complete status presentation system.
- **Derived traits rebuild from base data.** Clear to the current job, then merge
  current equipment; refresh on load/job/equipment/stat changes and clear absent
  actors. Keep the battle cache outside the hot Unit stride. Noninitial stat
  rebuilds must preserve live HP/MP. An affinity cache alone does not implement
  damage, healing, nullification or status immunity.
- **Magical formulas have their own rules.** Share prediction, execution and
  AI math; preserve modifier rounding and live delayed stats/Faith. Never
  inherit physical vulnerability, criticals or knockback. Holy bypasses magic
  evasion in its source flags; Fire does not. Scalar support does not enable
  status/element/target producers or waive reviewed spell presentation.
- **Prepare the effect category once.** Damage, healing and nullification must
  agree in prediction, AI and impact. Keep the prepared category through query/
  trait changes and response save/restore. Absorption is a healing event;
  nullification is no effect, with source cost/RNG/reward policy. Preserve source
  signed quantities separately from the event category; do not mistake a wrapped
  signed field for large positive damage. Healing does not offer damage-only
  responses or damage-type wake. Unsupported target/weather/area producers stay gated.
- **Responses suspend the primary action without rewinding the world.** Preserve
  its effect/presentation packet; keep live HP, positions, stock, CT and pending
  casts. Revalidate queued eligibility/range/stock before execution, consume
  secondary effects once and forbid recursive responses. Settle primary rewards
  after responses and terminal checks after the root presentation. Counter and
  Auto-Potion currently implement this contract; further families need fixtures.
- **Consecutive strikes precede post-action responses.** Each impact is unique;
  pending reactions wait for the last primary strike. Keep the highest earned
  EXP amount and apply EXP/JP once after responses. The bounded two-strike
  engine entry now feeds ordinary Two Swords for the source-checked hand subset.
- **Evaluate each hand without swapping equipment.** Ordinary Attack, Counter,
  Archer Charge, forecast and AI share hand selection and supported formulas. Both hands
  contribute stats once; a left weapon provides no shield evade. Recheck the
  fixed tile after recoil. Management returns an illegal left weapon to stock
  when its capability/job/right-hand compatibility changes. Deferred Charge
  validates both actual weapons; each strike retains its power and rolls its
  own critical. Other unsupported dual weapon skills stay unavailable until
  their own fixtures pass.
- **One choice-menu implementation:** use `choice_menu.inc` with layout options
  for commands and confirmations, including job changes. The animated white
  glove stays close to the start of the text, outside it. Cancel preserves the
  pre-dialogue state. No bespoke "ENTER:yes" confirmation panels.
- **One three-file selector:** title New Game/Continue and world-map Data use
  the same slot presentation, metadata and overwrite confirmation. Saving to a
  different slot changes the active campaign slot only after the save succeeds.
  Other files and the legacy save are preserved.
- **ENTER and2nd are equivalent confirm keys.** Use ALPHA for deployment's
  displayed "go" command (Diamond may be an additional alias). Labels must
  match the real keyboard mapping.
- **Deployment:** direction keys stay on legal placement cells. The current
  cell uses a white floor tile, not an overhead arrow. Selecting an already
  deployed roster member blinks that actor in place. Preserve the approved
  ground-plane orientation triangle. No placement/save occurs while merely
  browsing units/cells.
- **Management:** animate the floor diamond between roster positions. Use the
  common confirmation menu and animate a completed job change.
  The observed PS1 job effect is a light column around a stationary actor;
  use the four-grey adaptation, not a jump. Confirm skips; cancel can return.
  Jobs revealed by the party but unavailable to this unit use white silhouettes; fully
  undiscovered jobs stay hidden. Learn always shows the current job's available
  JP, the action's cost/learned state, and its updated JP after purchase.
- **World map:** keep the approved67% texture/actor scale, strong outlined roads
  and circles. The free glove visibly catches nearby shown destinations and
  never starts travel on hover. Hidden nodes must not attract it.
  Confirming empty space returns the glove to Ramza without entering a node
  or starting travel (current interpretation of the user's report, pending reply).
- **Battle scenery:** keep the selected1px contours only in the active package.
 2px comparisons are historical controls, not another active MODE setting.
- **Typography:** the shared glyph-font before/after is accepted. Reuse its
  archived letters and dynamic left-to-right reveal for white headings; avoid
  storing a new bitmap per heading. Title A/B remains a separate choice.

Current defects, reproducers and validation limits are tracked in
`campaign/QA_STATUS.md`. A passing logic test alone does not close a reported
native latency or presentation defect.

## Magical prediction and target validity

- Forecasts and AI queries must not consume battle RNG or change costs/equipment.
  Use the shared magical evaluator; facing/class/weapon physical evade does not
  apply. Executing evadeable magic rolls accessory/right/left/class in source
  order, including zero channels, and stops on the first failure.
- Separate absent targets from blocked present targets. Ordinary formula08
  Dead/Petrify/Wall rejection happens after formula/proc work; skipping it early
  would alter subsequent RNG. Misses do not apply HP or earn effect rewards.
- Elemental nullification follows evasion. Thus Fire null can consume evade draws
  while Holy null consumes none. Do not generalize the older Holy-only no-RNG
  observation to every spell. See campaign/CORE_MAGIC_EVASION.md.
- Packed catalogue metadata (FCJA) is shared by battle, management and deployment;
  preserve Item's existing dual-weapon bit when reading magic evade/shield tags.

## Formula08 environment and prepared status effects

- Apply source weather after the signed XA*power halfword, before resistance/
  Faith/caps; narrow after each sequential weather operation. Snow/indoor flags
  come through one validated weather boundary, never a map-ID exception.
- Preserve PS1 Oil's ordering bug: a successful fire removes mutable Oil but
  does not double formula08 damage. Innate protection survives; misses/nulls do
  not consume Oil. Preview and AI queries do not mutate statuses or RNG.
- Float nullifies formula08 earth without making the unit absent. This does not
  activate levitation, movement traits or a Float infliction producer.
- Carry prepared status removals through the primary/response packet. Classify
  effects through act_kind/masked category; the packed effect byte also has flags.
  Later queries/climate changes cannot rewrite prepared damage/removals.
- Cold full forecast preparation must be measured with drawing and image copying;
  a formatter-only cycle zone is not the whole HUD task or frame budget.

## Area AI planning and ownership

- Use shared source-backed footprints/effect math for ordinary hostile magic,
  including empty centres, friendly fire and absorption. Cache current victim
  effects once per ability; re-evaluate self after a simulated move. Distinguish
  this immediate model from unimplemented future CT/Reflect/status simulation.
- Reuse idle search scratch only after setup/focus ends. Every quantum restores
  simulated unit positions before rendering. Queries do not consume battle RNG;
  only the normal final action chooser retains its selection draws.
- An image must return before another loads. Cache must survive a pure forecast
  evicting AREA_AI. Rejecting the final command returns to thinking without
  spending the turn action. Route only CAST+AREA; AREA-only weapon splash is not
  a spell. Whole-map effects reuse the first legal centre at each origin.
- Measure general setup and area scoring separately, plus total plan latency.
  General setup may use the requested frozen pause (campaign/CORE_AI_PACING.md);
  its measured one-second quantum is accepted. Do not add another full pause
  after planning or infer total plan latency from that peak alone.

## Campaign prerequisites requested2026-11-12

Before installed-route automation, resolve the user's list in
[campaign/CAMPAIGN_PREREQUISITES.md](campaign/CAMPAIGN_PREREQUISITES.md).
ESC during a scenario scene must offer a safe confirmation; it must not silently
quit to the title. A skip passes to the normal next stage. Any actual quit path
needs No/Yes confirmation too. Keep the common choice renderer and both confirm
keys. Four traps, monster-specific KO poses, persistent results/Brave/Faith/
level-down, chained battles with save proposals, imposed guests, objective types
and story choices need explicit audited status rather than a generic core-ready
claim. The shared scene confirmation layout was approved2026-11-13; scene/save delivery
and remaining prerequisites are tracked in the linked audit.
The trap message layout was approved2026-11-13; all four traps and persistent
Degenerator stats are delivered in [campaign/TRAPS.md](campaign/TRAPS.md).
