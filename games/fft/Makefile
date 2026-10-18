# Final Fantasy Tactics (PS1), Magic City Gariland: isometric tech demo with four camera
# orientations (README.md). Our own engine, cursor and shadow (tools/art.py -> art.h,
# committed); the battlefield's heights, its four textured views and the units' sprites come
# from the local disc (tools/extract.py -> map.h and tiles.bin, fftv0..3.bin ZX0-packed, .89y data variables
# for the TI; tools/units.py -> units.h, fftu.bin; tools/battle.py -> battle.h, the battle's
# units and FFT's numbers), never committed. The TI unpacks them with lib/unpack68k.s.
NAME := fft
SRC := fft.c
TESTS := test_fft.c
TI := ../..
PY := $(TI)/tools/pyenv/bin/python
DISC := "$(TI)/roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue"
include ../../runtime/rt.mk

VIEWS := fftv0 fftv1 fftv2 fftv3
map.h: tools/extract.py
	@test -f $(DISC) || { echo "missing the local disc roms/ps1/Final Fantasy Tactics (USA) (not in git)"; exit 1; }
	$(PY) tools/extract.py $(DISC)
$(VIEWS:=.bin) tiles.bin: map.h
%.89y: %.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< $*
fftu.bin: units.h
fftw.bin: weapons.h
ffti.bin: interface.h
interface.h: tools/interface.py tools/units.py tools/art.py tools/banner.py map.h
	$(PY) tools/interface.py $(DISC)
ti cycles: $(VIEWS:=.89y) fftu.89y fftw.89y ffti.89y
TI_FILES := $(VIEWS:=.89y) fftu.89y fftw.89y ffti.89y
TI_FLAGS := ../../lib/unpack68k.s
art.h: tools/art.py
	$(PY) tools/art.py $@
units.h: tools/units.py
	@test -f $(DISC) || { echo "missing the local disc roms/ps1/Final Fantasy Tactics (USA) (not in git)"; exit 1; }
	$(PY) tools/units.py $(DISC) $@
weapons.h: tools/weapons.py tools/units.py
	$(PY) tools/weapons.py $(DISC) $@
battle.h: tools/battle.py
	@test -f $(DISC) || { echo "missing the local disc roms/ps1/Final Fantasy Tactics (USA) (not in git)"; exit 1; }
	$(PY) tools/battle.py $(DISC) $@
# the original at Ramza's first turn (tools/oracle.py, 30 s, once), its next 40 turns played by
# its AI (20 s, once), 40 attack previews and 400 attacks (9 min, once), its AI's turns (the
# battle and built situations, 2 min, once), and its units, turns and attacks for the tests
ORACLE := $(TI)/sources/fft_ps1/gariland_t1
TURNS := $(TI)/sources/fft_ps1/gariland_turns.txt
ATTACKS := $(TI)/sources/fft_ps1/gariland_attacks.txt
SKILLS := $(TI)/sources/fft_ps1/gariland_skills.txt
REWARDS := $(TI)/sources/fft_ps1/gariland_rewards.txt
AI := $(TI)/sources/fft_ps1/gariland_ai.txt
$(ORACLE).state:
	$(PY) tools/oracle.py $(DISC) $@ $(ORACLE).txt
$(TURNS): $(ORACLE).state
	$(PY) tools/oracle.py --turns $(DISC) $(ORACLE).state 40 $@
$(ATTACKS): $(ORACLE).state
	$(PY) tools/oracle.py --attack $(DISC) $(ORACLE).state 40 400 $@
$(SKILLS): $(ORACLE).state
	$(PY) tools/oracle.py --skills $(DISC) $(ORACLE).state 300 $@
$(REWARDS): $(ORACLE).state
	$(PY) tools/oracle.py --rewards $(DISC) $(ORACLE).state $@
$(AI): $(ORACLE).state
	$(PY) tools/oracle.py --ai $(DISC) $(ORACLE).state 40 12 $@
oracle.h: tools/battle.py $(ORACLE).state $(TURNS) $(ATTACKS) $(SKILLS) $(REWARDS) $(AI)
	$(PY) tools/battle.py $(DISC) --check $(ORACLE).state $(ORACLE).txt $@ $(TURNS) $(ATTACKS) $(SKILLS) $(REWARDS) $(AI)
$(NAME)_test: oracle.h
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: map.h units.h weapons.h interface.h battle.h art.h fft.h ct_recip.h $(VIEWS:=.bin) fftu.bin fftw.bin ffti.bin
XCHECK := 0 1 2 3 4 5 6 7 8 9 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60
FRAMES := 40
.PHONY: events-test events-review events-check
events-test: map.h units.h weapons.h interface.h battle.h art.h
	@mkdir -p x/events
	$(CC) $(PC_CFLAGS) -o x/events/test-events test_events.c $(RT_CORE) $(RT_SW)
	./x/events/test-events
events-check: events-test
	$(PY) tools/check_events.py
events-review: events-check
	$(PY) tools/review_events.py
.PHONY: chest-review
chest-review: pc
	$(PY) tools/review_chest.py
.PHONY: storage-audit
storage-audit:
	$(PY) tools/storage_audit.py
.PHONY: storage-cleanup-check
storage-cleanup-check: pc
	$(PY) tools/check_storage_cleanup.py
.PHONY: sprite-storage-explore
sprite-storage-explore:
	$(PY) tools/explore_sprite_storage.py
.PHONY: blade-cache-check
blade-cache-check: pc ti cycles
	$(PY) tools/check_blade_cache.py
.PHONY: review-blows check-blows
review-blows: pc
	$(PY) tools/review_blows.py
check-blows:
	$(PY) tools/check_blows.py

.PHONY: review-interface check-interface
review-interface: pc
	$(PY) tools/review_interface.py
check-interface:
	$(PY) tools/check_interface.py

.PHONY: check-water check-water-original review-water
check-water:
	$(PY) tools/check_water.py
check-water-original: $(ORACLE).state
	$(PY) tools/water.py $(DISC) $(ORACLE).state
review-water: pc
	$(PY) tools/review_water.py

# Local-disc mesh, flat material shades and fixed-point tables for perspective inspection.
fftmesh.bin: tools/perspective_assets.py tools/extract.py
	$(PY) tools/perspective_assets.py $(DISC)
ifneq ($(findstring -DFFT_EXPLORATION,$(CFLAGS_GAME)),)
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: perspective.inc fixed.inc fftmesh.bin
ti cycles: fftmesh.89y
TI_FILES += fftmesh.89y
endif
ifneq ($(findstring -DFFT_TURN_TEXTURE,$(CFLAGS_GAME)),)
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: turn_texture.inc
endif

.PHONY: perspective
perspective:
	$(MAKE) NAME=fftp CFLAGS_GAME=-DFFT_EXPLORATION TESTS=test_perspective.c pc ti

.PHONY: perspective-check perspective-gifs perspective-xcheck
perspective-check:
	$(MAKE) NAME=fftp CFLAGS_GAME=-DFFT_EXPLORATION TESTS=test_perspective.c test pc cycles
perspective-gifs:
	$(MAKE) NAME=fftdump CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_CAPTURE' cycles
	$(PY) tools/perspective_demo.py
perspective-xcheck:
	$(MAKE) NAME=fftp CFLAGS_GAME=-DFFT_EXPLORATION TESTS=test_perspective.c xcheck XCHECK='62 63 64' FRAMES=110 KEYS=keys/perspective-return.txt

.PHONY: fixed fixed-cycles fixed-check fixed-gifs fixed-xcheck
fixed:
	$(MAKE) NAME=fftfix CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED' TESTS=test_fixed.c pc ti
fixed-cycles:
	$(MAKE) NAME=fftfix CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED' TESTS=test_fixed.c cycles
fixed-check:
	$(MAKE) NAME=fftfix CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED' TESTS=test_fixed.c test pc cycles
	$(MAKE) fixed-cutaway-check
fixed-gifs:
	$(MAKE) NAME=fftfcap CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED -DFFT_CAPTURE' cycles
	$(PY) tools/fixed_demo.py
fixed-xcheck:
	$(MAKE) NAME=fftfix CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED' TESTS=test_fixed.c xcheck XCHECK=62 FRAMES=110 KEYS=keys/fixed-return.txt

.PHONY: fixed-water-gifs check-placement
fixed-water-gifs:
	$(MAKE) NAME=fftfcap CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED -DFFT_CAPTURE' cycles
	$(PY) tools/fixed_water_demo.py
check-placement:
	$(PY) tools/check_placement.py $(DISC)

.PHONY: fixed-camera-gifs
fixed-camera-gifs:
	$(MAKE) NAME=fftfcap CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED -DFFT_CAPTURE' cycles
	$(PY) tools/fixed_camera_demo.py

.PHONY: fixed-cutaway-check
fixed-cutaway-check:
	@mkdir -p x
	$(CC) -std=gnu99 -O1 -g -DFFT_EXPLORATION -DFFT_FIXED -o x/test-cutaway test_cutaway.c ../../runtime/core/rt_core.c ../../runtime/platform-sw/rt_sw.c
	./x/test-cutaway

.PHONY: fixed-dashed-gifs
fixed-dashed-gifs:
	$(MAKE) NAME=fftfcap CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED -DFFT_CAPTURE' cycles
	$(PY) tools/fixed_dashed_demo.py

# Visibility strategy is awaiting user approval: intentionally PC-only.
.PHONY: fixed-review
fixed-review:
	$(MAKE) NAME=fftfix CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED' TESTS=test_fixed.c test pc fixed-cutaway-check
	$(PY) tools/fixed_review.py

# Source-resolution sprite experiment: portable captures and an offline PC compositor.
.PHONY: fixed-original-sprites
fixed-original-sprites:
	$(PY) tools/original_sprite_review.py $(DISC)

# Actual isometric scenery textures on perspective geometry, PC visual review only.
.PHONY: fixed-scenery-textures
fixed-scenery-textures:
	$(PY) tools/texture_review.py $(DISC)

.PHONY: fixed-texture-contrast
fixed-texture-contrast:
	$(PY) tools/texture_contrast_review.py $(DISC)

.PHONY: fixed-motion-review
fixed-motion-review:
	$(PY) tools/perspective_motion.py $(DISC)

.PHONY: fixed-modes-review
fixed-modes-review:
	$(PY) tools/perspective_modes.py $(DISC)

.PHONY: fixed-sprite-bench-prepare
fixed-sprite-bench-prepare: fixed-scenery-bench-prepare

.PHONY: fixed-scenery-bench-prepare
fixed-scenery-bench-prepare:
	$(PY) tools/prepare_scenery_bench.py

# Explicit TI measurements, authorized after the PC visual review.
.PHONY: fixed-scenery-ti-benchmark
fixed-scenery-ti-benchmark:
	$(PY) tools/scenery_ti_benchmark.py

# Attack camera review: selection remains isometric; perspective is a noninteractive shot.
.PHONY: attack-camera attack-camera-test attack-camera-review attack-camera-check
attack-camera:
	$(PY) tools/preview_textures.py
	$(MAKE) NAME=fftcam CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED -DFFT_CINEMA -DFFT_TEXTURE_REVIEW -DFFT_TEXTURE_PREVIEW' pc
attack-camera-test:
	@mkdir -p x
	$(CC) -std=gnu99 -O1 -g -DFFT_EXPLORATION -DFFT_FIXED -DFFT_CINEMA -DFFT_CAMERA_MOTION -o x/test-cinema test_cinema.c ../../runtime/core/rt_core.c ../../runtime/platform-sw/rt_sw.c
	./x/test-cinema
attack-camera-review:
	$(PY) tools/textured_transitions.py
attack-camera-check: attack-camera-test
	$(PY) tools/attack_camera.py --cycles
fftcam_pc fftcam.89z fftcamc.89z: cinema.inc
fftcam_pc: texture_preview.inc

# Offline F1/F5 texture feasibility review; no rotation bitmap bank in the game.
.PHONY: rotation-texture-review
rotation-texture-review:
	$(PY) tools/rotation_texture_review.py

# PC-only general texture sampler: all camera motion, no per-camera bitmap cache.
.PHONY: textured-transitions textured-transitions-review
textured-transitions: attack-camera
textured-transitions-review:
	$(PY) tools/textured_transitions.py

.PHONY: textured-transitions-test
textured-transitions-test:
	$(PY) tools/preview_textures.py
	$(CC) -std=gnu99 -O1 -g -DFFT_EXPLORATION -DFFT_FIXED -DFFT_CINEMA -DFFT_CAMERA_MOTION -DFFT_TEXTURE_REVIEW -DFFT_TEXTURE_PREVIEW -o x/test-textured-cinema test_cinema.c ../../runtime/core/rt_core.c ../../runtime/platform-sw/rt_sw.c
	./x/test-textured-cinema

# Local offline colour-only ISO variants; runtime contours are never drawn over actors.
fftiso.bin: tools/isometric_style.py tools/extract.py tools/attack_texture.py tiles.bin
	$(PY) tools/isometric_style.py
fftcam_pc: fftiso.bin
attack-camera-review textured-transitions-review textured-transitions-test: fftiso.bin
.PHONY: isometric-style-review
isometric-style-review: fftiso.bin
	$(PY) tools/isometric_review.py

.PHONY: isometric-storage
isometric-storage: fftiso.bin
	$(PY) tools/isometric_storage.py

.PHONY: isometric-decode-cost
isometric-decode-cost:
	$(PY) tools/isometric_decode.py

# Bounded real-TI camera review; prepares archive banks and checks PC/TI frames.
.PHONY: tiemu-camera-demo
tiemu-camera-demo: fftmesh.bin fftiso.bin
	$(PY) tools/camera_tiemu_demo.py

.PHONY: camera-audit
camera-audit:
	$(PY) tools/camera_audit.py

.PHONY: ultra-transition-bench
ultra-transition-bench:
	$(PY) tools/ultra_transition_bench.py

# Accepted camera policy: two paused inspection presets, direct cuts, no attack shots.
.PHONY: camera-cuts-check camera-cuts-demo
camera-cuts-check:
	$(MAKE) NAME=fftcutf CFLAGS_GAME='-DFFT_EXPLORATION -DFFT_FIXED' TESTS=test_fixed.c test
	@mkdir -p x
	$(CC) -std=gnu99 -O1 -g -DFFT_EXPLORATION -DFFT_FIXED -DFFT_CINEMA -o x/test-camera-cuts test_camera_cuts.c ../../runtime/core/rt_core.c ../../runtime/platform-sw/rt_sw.c
	./x/test-camera-cuts
camera-cuts-demo: camera-cuts-check
	$(PY) tools/camera_cuts.py

.PHONY: turn-audit
turn-audit: camera-cuts-check
	$(PY) tools/turn_audit.py

.PHONY: turn-texture-bench
turn-texture-bench:
	$(PY) tools/turn_texture_bench.py

.PHONY: turn-texture-opt-bench
turn-texture-opt-bench:
	$(PY) tools/turn_texture_opt.py

.PHONY: turn-surface-bench
turn-surface-bench:
	$(PY) tools/turn_surface_bench.py

.PHONY: turn-horizontal-bench
turn-horizontal-bench:
	$(PY) tools/turn_horizontal_bench.py

.PHONY: turn-comparison-bench
turn-comparison-bench:
	$(PY) tools/turn_comparison.py

.PHONY: turn-direction-check
turn-direction-check:
	$(PY) tools/turn_direction_check.py

.PHONY: turn-edge-review
turn-edge-review:
	$(PY) tools/turn_edge_review.py

CONFIG_FLAGS := -DFFT_EXPLORATION -DFFT_FIXED -DFFT_CINEMA -DFFT_CONFIG -DFFT_TURN_LOWPOLY_NATIVE
ifneq ($(findstring -DFFT_CONFIG,$(CFLAGS_GAME)),)
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: config.inc prefetch.inc
endif
.PHONY: camera-config-check camera-config camera-config-ti
camera-config-check: camera-config-ti
	@mkdir -p x/config
	$(CC) -std=gnu99 -O1 -g $(CONFIG_FLAGS) -o x/config/test-config test_config.c ../../runtime/core/rt_core.c ../../runtime/platform-sw/rt_sw.c
	cd x/config/data && ../test-config ../menu.png ../filled.png ../wire.png
camera-config: camera-config-check
	$(MAKE) NAME=fftcfg CFLAGS_GAME='$(CONFIG_FLAGS) -DFFT_TEXTURE_REVIEW -DFFT_TEXTURE_PREVIEW' pc
camera-config-ti: fftmesh.89y fftu.89y fftw.89y ffti.89y
	$(PY) tools/config_review.py

.PHONY: camera-prefetch-check
camera-prefetch-check: camera-config-check
	$(PY) tools/prefetch_review.py

.PHONY: camera-prefetch-filter-check
camera-prefetch-filter-check:
	$(CC) -std=gnu99 -O1 -g $(CONFIG_FLAGS) -DFFT_TEXTURE_REVIEW -DFFT_TEXTURE_PREVIEW -o x/config/test-config-filter test_config.c ../../runtime/core/rt_core.c ../../runtime/platform-sw/rt_sw.c
	cd x/config/data && ../test-config-filter

.PHONY: iso-silhouette-check
iso-silhouette-check:
	$(PY) tools/iso_silhouette_review.py
