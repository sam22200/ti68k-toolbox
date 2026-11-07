# FFT fixed close/wide perspective experiment

2026-10-23 decision: keep perspective inspection experimental, outside the
FFT campaign. The shared renderer costs code once, but each supported map
also needs its own archived geometry/visual data. The current Gariland
prototype uses16585 bytes of mesh/tables and3902 bytes of close/wide image
fixtures, in addition to its RAM scratch and shared engine code.

No perspective banks belong in `games/fft/campaign/x/fftcampaign.89g`.
The chapter engine rejects `FFT_EXPLORATION` at compile time. Its ordinary
isometric F1/F5 low-poly rotations remain a separate shipping feature.

This directory is the experiment entry point. The established conditional
renderer sources and their historical checks remain under `games/fft/`
because they also contain shared projection and rotation reference code:
`perspective.inc`, `fixed.inc`, `cinema.inc`, `prefetch.inc`, and the existing
camera tooling/notes. Keeping those paths avoids breaking reproducible older
research controls; it does not re-enable the feature in the game.

```sh
make -C experiments/fft-perspective check
make -C experiments/fft-perspective review
```

`review` builds the PC inspection renderer; neither target starts TiEmu or
modifies the user's calculator session. See `games/fft/CAMERA_AUDIT.md`,
`STORAGE_CLEANUP.md` and `campaign/UI_SIZE_STUDY.md` for the native feature
limits, code/data/RAM distinction and historical measurements.
