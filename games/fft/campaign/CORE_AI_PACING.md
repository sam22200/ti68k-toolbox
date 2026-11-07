# AI turn pause includes computation

2026-10-28. User clarification: a roughly one-second AI calculation is acceptable
inside the requested one-to-two-second frozen pause before an AI unit acts. Do
not prioritize splitting that calculation solely to meet the interactive frame
budget. This supersedes the setup-latency priority in CORE_AREA_AI.md.

The previous code finished thinking and then added48 presentation frames. A slow
plan therefore paid both delays. ai_step now starts a256Hz timestamp on the first
AI_THINK step and waits only until384 ticks (1.5 seconds) have elapsed. The first
AI_GO step after both the plan and deadline are ready presents the action. If
planning exceeds the deadline, no further full pause is added. Time spent
settling the initial camera and thinking is included. This is a minimum pause,
not a promise that every stress plan finishes in1.5 seconds.

The retained frozen picture stays frozen during the remainder. Repeated update
calls, held confirm/fast-forward and low frame rates do not grant elapsed-time
credit. Unsigned16-bit subtraction handles one clock wrap. A second plan after
acting carries AI_HOLD_SKIP; weather/late rejection inside a turn retains its
original timestamp. A new turn or manual-to-Auto transition resets ai_t and arms
a fresh timestamp. Cancellation remains checked before AI work in update().

The timestamp is one resident two-byte static, armed before use. State, Work,
Unit, FSV3 and all five heap requests are unchanged. No timestamp crosses an
overlay boundary; all fourteen code images and both FMB3 shards are byte-identical
to CORE_AREA_AI (ABI32/signature63878/ModCtx364/zone6218). No battle rules,
ranking, CT scheduling, MP or RNG policy changes.

## Scoped verification and budget

`make core-ai-pause-check` runs171 assertions (58CBDE4E) on PC, ASan/UBSan and
the actual68000 binary. An injected256Hz clock covers0/100/256/383/384/512-tick
planning durations, wrap, repeated same-clock fast updates, new turns, replans,
cancellation/re-enable and unchanged frozen planes. The completed-plan fixture
uses the real AI image. The unchanged runtime clock is tested separately by a
short ordinary-clock PC/native replay; no timer/interrupt driver is rewritten.
There is no new live TiEmu or physical-calculator timing claim.

Private group509418 bytes (-24 compressed archive bytes); only ffbcore changes,
with all42 other variables and28 non-code banks byte-identical. Resident TTPack
input64467 (+20) leaves469 bytes. Static RAM grows2 bytes; no heap growth.
The standard466045-byte playtest group, user saves, live TiEmu and every prior
gallery image's bytes and modification date are preserved.

The prior12059828-cycle setup peak is retained as a CPU measurement; it is now
an accepted frozen-AI workload, not a standalone30fps blocker. End-to-end worst
plan latency, RAM/stack/residency and interactive-frame budgets remain open.
Do not infer total planning time from the longest single quantum. Next priority
returns to reviewed area targeting/forecast/per-target and empty-impact
presentation, then the remaining source-backed AI/action/status/campaign gates.
Jump refinements remain deferred. Full core/content production is not complete.

Artifacts: x/core-ai-pause/{check/results.json,replay.json,budget.json,
verification.json}; preceding rules/effect proof is reused from CORE_AREA_AI.md.
