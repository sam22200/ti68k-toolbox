# Battle banners and loot animation — 2026-10-17

The later [storage cleanup](STORAGE_CLEANUP.md) compacts the UI/weapon formats while
preserving this presentation. Storage totals below describe the earlier event
revision; current `ffti` is 16,284 bytes. Current replay peaks remain below 360,000
cycles and are recorded in the [bounded blade cache note](BLADE_CACHE.md).

The roadmap's five presentation requests are implemented in the shared tactical engine.

## Behaviour

- Banner text reveals at 12 pixels/tick instead of 6. The diagonal shade moves at
  8 pixels/tick instead of 16, both entering and leaving.
- Every complete message remains visible for 40 ticks. `VICTORY!` includes its final
  letters and exclamation mark; the message no longer expires during glyph construction.
  ENTER/2nd still skips to the outgoing shade, then to the results.
- A KO unit becoming a crystal or chest flashes white and dissolves into its object
  over 32 ticks. The camera frames that unit first; the effect waits for the pan.
  Multiple conversions from one CT advance are queued and presented individually.
  Tactical actions, AI and further CT advances pause during presentation. The original
  CT selection, loot roll, statistics and RNG calls remain unchanged.
- Crystals cycle through all eight original `OTHER.SPR` images, four ticks each.
  Chests remain stationary after appearing. Both objects use the ordinary sprite depth,
  cover and silhouette path. No alternate occlusion algorithm was added.

The intended tick rate is 32/s: a framed conversion lasts one second; one crystal loop
also lasts one second. These are authored timing values, not measured physical-TI FPS.
The source images are original assets; the dissolve and animation timing are adaptations,
not a reproduction measured on the original PlayStation game.

## Banner fix and costs

The former vector renderer incrementally sampled glyphs in the frame's spare CPU budget.
Its expiry timer advanced independently, so a crowded scene could display only `VIC`
before hiding the banner. Increasing just the wipe speed would not fix this.

`tools/banner.py` now evaluates the same integer rasterizer offline, including DIVS
rounding, four greys and white-outline dilation. Four small, packed text strips are
read directly from archived `ffti`; the diagonal shade and text reveal remain runtime
animations. The strips match all three former C banner-plane buffers byte for byte.
Their SHA256 fixtures are checked by `tools/check_events.py`. No full-screen map or
camera images were added.

| Storage | Before | After | Difference |
|---|---:|---:|---:|
| Banner section in ffti | 696 B outlines | 4,128 B strips | +3,432 B archive |
| ffti raw bank | 46,588 B | 50,020 B | +3,432 B archive |
| fftu raw bank | 55,328 B | 56,784 B | +1,456 B archive |
| Default native Work | 43,346 B | 43,192 B | −154 B RAM |
| Default native State | 1,010 B | 1,014 B | +4 B RAM |

The extra crystal phases are seven ordinary 208-byte frames. Existing actor frames and
the first crystal retain their offsets and bytes; the chest keeps its frame offset and
format through its art revision below. Banners no longer need
the vector edge/coverage work arrays; the replacement includes a shared 208-byte
transformation frame. Net Work + State storage falls by 150 bytes.

Default TI build: `fft.89z` 63,809 file bytes / 63,719 variable payload bytes. This is
the isometric tactical build, not the larger all-features perspective research build.
The compact configuration/camera review still builds separately (65,497 file bytes /
65,407 payload bytes); it is not a complete tactical presentation/release test.

## Verification

Reproduce from `games/fft` with the existing local disc-derived assets:

```sh
make test ti
make events-check
make events-review
make camera-prefetch-check
```

`events-check` validates 101 PC/native TI screen pairs, including the full banner and
transformation timelines, a 16,000-frame battle replay, 6,000 fast-forward frames and
4,000 defeat frames. It excludes only the existing scene-load/turn/reach stalls from
the ordinary 360,000-cycle budget.

| Scenario | Peak ordinary cycles |
|---|---:|
| Battle-start presentation | 318,842 |
| Victory | 249,672 |
| Defeat fixture | 247,734 |
| Crystal conversion fixture | 306,976 |
| Chest conversion fixture | 307,586 |
| Two queued conversions | 289,232 |
| Covered conversion fixture | 304,334 |
| Full battle replay | 350,610 |
| Held fast-forward replay | 356,180 |
| Defeat replay | 357,096 |

These are MC68000 datasheet CPU counts from `ti-cycles`, not TiEmu or physical-hardware
measurements; grayscale/presentation/waits are excluded. The default unit suite, an
AddressSanitizer run, 5,754 covered-interior pixel assertions, and unchanged stats/RNG
through presentation pass. Configuration/idle preparation additionally passes 488
interactive PC/TI frames and 34 cold/prepared camera images. Current UI/unit banks
are refreshed in the camera-review staging pipeline so generated offsets match.

No emulator was started for this change. The earlier real-TiEmu perspective latency
figures in `PREFETCH.md` belong to their recorded pre-event binaries; they have not been
retimed on this revision.

Local review artifacts:

- `x/events/banners-pc.gif`: start, victory and defeat side by side.
- `x/events/loot-pc.gif`: crystal, chest and queued conversions.
- `x/events/review.png`: complete words and conversion phases.
- `x/events/checked.json`, `cycles-*.log`, `replay-*.log`: native screen/cycle evidence.

The GIFs are PC art previews at the intended 32 ticks/s, sampled every two ticks;
their playback cadence is not a TI performance measurement. Injection doors are 69
(crystal), 70 (chest), 71 (two conversions), 72 (covered crystal), plus existing 0/59/60
for start/victory/defeat. Rebuild and resend **both fftu and ffti** with the executable.

## Chest source-art revision

After the user's visual review, the chest uses the complete closed-chest drawing from
`BATTLE/OTHER.SPR`, palette 0, source box `(225, 6, 23, 22)`. The earlier crop omitted
the bottom front corner; majority scaling and a forced black inner contour also erased
the lid/straps. `chest_sprite()` now resamples the source to 14x14, preserves bright
metal when it occupies the majority of a source cell, and adds only the external white
outline. The wood, lock and metal colours are mapped to four greys offline. Its lower
outline meets the shared feet/shadow anchor.

Only the chest's existing 208-byte frame changes; other unit/crystal bytes and total
bank size are identical. No C renderer, RAM layout or sprite routine changes. The
101 PC/native event/replay comparisons pass with the new art, as do the default unit
and presentation suites. At this art-only revision the chest fixture peaked at 307,514 ordinary cycles.
These remain headless 68000 checks; TiEmu was not started.

`make chest-review` creates `x/chest/comparison.png` (original / previous TI / current
TI when the local before-bank snapshot exists), `scene.png` and `chest-pc.gif`. The
images decode the actual packed frame used by the game. The GIF is a PC preview,
not a TI performance capture. Resend the rebuilt `fftu.89y` to update the chest art.

## Subtle chest float

After conversion, the original chest sprite gently rises by at most two source
pixels and returns: heights 0, 0, 1, 1, 2, 2, 1, 1, each held for eight game ticks
(64 ticks per loop, two seconds at the intended 32 ticks/s). The conversion itself
uses the steady base frame. Two lifted frames are generated offline before the
ground shadow is added, so the shadow stays anchored. Existing masks and covered
silhouettes follow each lifted frame through the shared sprite renderer, including
perspective views. This adds 416 archived asset bytes and an eight-byte selection
table, with no additional working buffer or runtime image shifting. Rebuild the
executable and resend `fftu.89y` together because the generated object layout changed.
The unit bank is now 57,200 bytes (the event-only storage table above predates this
float). The default tests, fixed-shadow/two-pixel/covered-interior checks and all 101
PC/native event/replay comparisons pass. The chest fixture peaks at 307,586 ordinary
cycles; replay peaks remain unchanged. TiEmu was not started. `make chest-review`
now exports five seconds, including complete floating cycles after conversion.
