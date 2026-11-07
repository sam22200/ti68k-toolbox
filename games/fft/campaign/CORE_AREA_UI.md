# Approved area targeting in the shared engine

Historical integration checkpoint. The subsequent approved native impact slice
and current ABI/package evidence are in [CORE_AREA_IMPACT.md](CORE_AREA_IMPACT.md).

2026-10-28. User approval: “ok valide et continue”. The controller now runs in
the shared PC/68000 engine. This closes manual selection/inspection for the
supported ordinary formula08 area subset. Retail AF_CAST records, complete
spell presentation and full core production gates remain open. Jump deferred.

## Interaction and source boundary

1. Select an in-range panel, including an empty one. ENTER/2nd inspect the
   centre using existing cursor, rotation and upper/lower selection rules.
2. Show the depth-masked footprint and affected units. Four visible rows show
   signed HP forecasts/hit percentages, scroll indicators and an ally count.
   Browsing changes only the inspected portrait/stat/forecast card; it never
   chooses or excludes victims. Empty sets explicitly say “No targets”.
3. ENTER/2nd opens the common animated white-glove menu. Unit binds the centre
   to an actor during charging; Panel fixes a tile. Cancel returns to inspection,
   then range selection, without MP payment or a spent action.
4. Empty centres, or source NO_FOLLOW_TARGET, offer Panel/Cancel only. Follow
   policy comes from raw range byte4 bit0x10, not primary ability flags. Late
   insufficient MP keeps confirmation open. Arming reuses cast_order/deferred
   payment; browsing never pays or changes the live cast packet.

STATIC evidence remains the five pinned original functions recorded in
x/core-area-ui-review/verification.json: battle_state_handle_targeting_range_state,
battle_target_select_tile, battle_action_confirm,
battle_state_handle_confirm_action_state and battle_target_set_panels_for_action.
They establish centre selection/binding, not a PS1 four-row list. That list is a
TI adaptation. CORE_TARGETS/AREA_FORECAST/AREA_CT supply the supported geometry,
formulas and ownership. No new full original spell dispatcher trace is claimed.

## Ownership, overlays and memory

- M_CAST_RANGE/M_AREA append modes15/16; earlier IDs stay fixed. Both CAST and
  AREA are required. AREA-only weapon0x9b retains its legacy path.
- AreaUi is228 bytes on both compilers, aliasing idle256-byte ai_dist. It owns
  an independent60-byte forecast, thirteen victim IDs, a32-byte footprint and
  four cached28-byte lines. The live cast owner is separate. No new heap request.
- Native State2192, Work45576, Unit112, FSV3 save3138 and the same five battle
  allocations stay fixed. Twenty layout fields match the previous checkpoint.
- ABI33/ModCtx372/signature64143; fifteen images, FMB3 split8/7. New AREA_UI4096,
  largest execution image6264, raw banks41066/36084 bytes. Older image IDs stable.
- AREA_UI returns QUERY/SEND/BACK; only the resident controller then loads
  TARGETS or COMMAND. The executing image never replaces itself. Pure resident
  area_paint builds at most one existing card part and blits cards/tags without
  loading any image. TARGETS4 fills the private forecast and footprint.
- To recover resident headroom, the unchanged AI occlusion/camera kernel moved
  to AI_SETUP3/4. world_of is a pure callback. Native view_hides uses transient
  ModCtx input, not battle-state scratch. Camera returns before image loading.
- Position, canonical status, rebuilt stats and weather invalidate inspection.
  Cancellation restores range; rotation preserves centre/victims. Existing
  fonts, portraits, card, forecast and shared choice-menu drawing are reused.

The private deployable package is515564 bytes (+6146); resident TTPack input64719
leaves217 bytes of headroom. Details: x/core-area-ui/package.json.
All28 non-code banks stay identical. Archive size
is not peak RAM. No assembly was authored; the standard playtest is separate.

## Scoped verification

`make -C games/fft/campaign core-area-ui-check` runs the controller check.
Evidence: x/core-area-ui. Native builds serial; no geometry census/Jump sweep.

- 499 PC/ASan/UBSan/68000 assertions67EED097: both confirm keys, empty/occupied/
 no-follow centres, allies/absorption/zero victims, cancellation, browsing purity,
 actual Unit/Panel arming, late MP rejection, invalidation, rotation, APPS return,
 thirteen-row scrolling, two floors and valid node255.
- Eight equal HUD-only plane hashes come from the compact checker. A separate
 actual-menu replay compares four full tactical framebuffers and battle states
 at frames111/131/171/191: range, inspection, confirmation, armed return. A private
 learned carrier152 uses Fire16 data; this is synthetic, not a retail unlock.
- Relocated camera:1321 PC/sanitizer/native checks4CC4400E; all ordered Gariland
 actor/target pairs in four views. Hide results/selected views agree, unit records
 are restored, and the228-byte cache size is checked.
- Bank parser130; context/retry bindings146/12CE8E3A, thirty cold/thirty warm
 bindings and seven rejection startups including old ABI pass.
- Regressions: forecast359/7E9BBEF7, CT353/11D6FCB4 plus five Orbonne restarts
 00000230, magic1352/B5895FED, area AI676/D04607FA, elapsed pause171/58CBDE4E.
 Twelve earlier action/reaction frame hashes remain equal. Pilot victory/save
 flow is recorded separately in the same proof directory.

Timing remains incomplete: compact input/backend zone54 peaks279866 cycles;
HUD-only zone55 peaks375252 on the first synthetic13-unit whole-map preparation.
These are separate zones, not a360K full-frame pass. Cold card/forecast work needs
scheduling against the full interactive budget. Full frames, stack, total AI
latency and combined residency remain C07/L4 work. The user's accepted roughly
one-second AI thinking allowance applies inside its frozen1.5-second elapsed
minimum, not to responsive player menus.

## Review preservation and next work

The approved prototype remains immutable in x/core-area-ui-review. Its169
PC/sanitizer checksD7BAD3B5 and42 equal frames describe the historical candidate.
approval.json records the verdict/artifact hashes. review_area_target.py returns
accepted paths read-only. Decision badges were cleared after approval. Existing
gallery bytes/dates, standard playtest group, user saves and live TiEmu remain
untouched; no image date was refreshed to simulate priority.

Next: reviewed common casting/impact presentation for every victim and empty
centres, hit/miss/HP/status feedback and complete response/reward draining.
Broader status/Reflect/multi-action families, campaign/formats/tooling and C07/L4
still prevent mass content production. Do not report the full core complete.
