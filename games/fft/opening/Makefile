NAME := fftopen
SRC := opening.c
TESTS := test_opening.c
include ../../../runtime/rt.mk
PY := ../../../tools/pyenv/bin/python
DISC := $(ROOT)/roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue
BANKS := ffov0 ffov1 ffov2 ffov3 ffoa ffoi ffost ffod ffs0 ffcp ffcu ffcw ffca ffcb ffcj
TI_FLAGS := ../../../lib/unpack68k.s
TI_FILES := $(addprefix data/,$(BANKS:=.89y))

.PHONY: assets reference check review
reference:
	$(PY) ../tools/opening_oracle.py "$(DISC)" --story --frames 90000
assets: data/ffcj.bin
	$(PY) ../tools/opening_assets.py "$(DISC)"
data/%.89y: data/%.bin
	cd data && ../../../../tools/gcc4ti-bin/bin/ttbin2oth -89 dat $*.bin $*
	python3 -c "from pathlib import Path; p=Path('$@'); b=bytearray(p.read_bytes()); b[0x49]=3; p.write_bytes(b)"
# The battle program's shared archive pair: built once by ../campaign
MODULES := ../campaign/data/ffbm.89y ../campaign/data/ffbn.89y ../campaign/data/ffbo.89y
$(MODULES) &: ../fft.c ../fft.h ../module.h opening.inc $(wildcard ../*.inc)
	$(MAKE) -C ../campaign data/ffbm.89y data/ffbn.89y data/ffbo.89y
ti cycles: $(TI_FILES) $(MODULES)
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: ../fft.c ../fft.h opening.inc $(wildcard data/*.h)
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../unit_card.h ../unit_card.inc ../portrait32.inc ../choice_menu.h ../choice_menu.inc
$(MODULES): ../unit_card.h ../choice_menu.h ../campaign/save.h ../campaign/save_slots.h
$(MODULES): ../scene_skip.h ../archive_art.h ../view_format.h ../core_limits.h ../chapter.h
$(MODULES): ../tactical_grid.h
$(MODULES) $(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../raw_stats.h ../trap_rules.h ../traps.inc
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../tactical_grid.h ../tactical_search.inc
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../scene_skip.h ../archive_art.h ../view_format.h ../core_limits.h ../chapter.h ../chapters.h ../module.h
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../campaign/save.h ../campaign/save_slots.h
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test $(MODULES): ../campaign/save_types.h
check: test pc cycles $(MODULES)
	$(PY) check.py
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../stats.inc ../unit_setup.inc ../formula.inc
$(MODULES) $(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../action_effect.h ../action_resolve.inc
$(MODULES) $(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../status_rules.h ../common.h
$(MODULES) $(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: ../status_policy.h ../status_unit.h ../status_policy.inc ../status_lifetime.h ../status_control.h ../status_runtime.inc ../turn_order.inc ../setup_glyphs.inc
check: visual-check
check: regression-check
.PHONY: regression-check
regression-check: fftopen_regression_test
	./fftopen_regression_test
fftopen_regression_test: regressions.c opening.c ../fft.c ../fft.h opening.inc $(RT_CORE) $(RT_SW) $(wildcard data/*.h)
	$(CC) $(PC_CFLAGS) -o $@ regressions.c $(RT_CORE) $(RT_SW)
.PHONY: visual-check
visual-check: fftopen_visual_test
	./fftopen_visual_test
fftopen_visual_test: visual_checks.c opening.c ../fft.c ../fft.h opening.inc $(RT_CORE) $(RT_SW) $(RT_HDR) $(wildcard data/*.h)
	$(CC) $(PC_CFLAGS) -o $@ visual_checks.c $(RT_CORE) $(RT_SW)
review: pc
	$(PY) review.py

# The battle program and the campaign bundle are ../campaign's (make packed, make bundle).
.PHONY: banks
banks:                         # the shared contour tail first: ffov0 must match ffcu's
	$(PY) ../tools/view_split.py opening
	$(PY) ../tools/contour_assets.py --attach data/ffov0.bin
	$(MAKE) $(TI_FILES)

.PHONY: contour-assets
contour-assets:
	$(PY) ../tools/view_split.py opening
	$(PY) ../tools/contour_assets.py --attach data/ffov0.bin
pc test cycles ti packed regression-check visual-check: | contour-assets data/ffcj.bin data/ffod.bin
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: ../config.inc ../contour.h ../contour_art.inc

../contour.h ../contour_art.inc: ../tools/contour_assets.py ../art.h ../setup_glyphs.inc ../fft.c ../config.inc
	$(PY) ../tools/contour_assets.py

$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: | contour-assets

data/ffod.bin: ../tools/chapter_desc.py ../tools/surface_data.py ../core_limits.h data/battle.h data/interface.h data/map.h data/units.h data/ffcj.bin data/ffcp.bin
	$(PY) ../tools/chapter_desc.py opening
data/ffcp.bin: ../tools/common_portraits.py ../tools/identity_assets.py ../tools/management_data.py
	$(MAKE) -C ../campaign ../opening/data/ffcp.bin

data/ffs0.bin: ../tools/scenes.py data/ffost.bin data/story.h
	$(PY) ../tools/scenes.py

data/ffcj.bin: ../tools/common_tables.py ../tools/battle.py ../tools/management_data.py
	$(MAKE) -C ../campaign ../opening/data/ffcj.bin

data/ffod.bin: ../tools/surface_data.py ../surface_geometry.h
$(MODULES) $(NAME)_pc $(NAME)_test fftopen_regression_test fftopen_visual_test: ../surface_geometry.h
$(MODULES) $(NAME)_pc $(NAME)_test fftopen_regression_test fftopen_visual_test: ../knockback_geometry.h
$(MODULES) $(NAME)_pc $(NAME)_test fftopen_regression_test fftopen_visual_test: ../step_geometry.h
$(MODULES) $(NAME)_pc $(NAME)_test fftopen_regression_test fftopen_visual_test: ../action_context.h ../action_start.inc
$(MODULES) $(NAME)_pc $(NAME)_test fftopen_regression_test fftopen_visual_test: ../reaction_policy.h ../reaction_runtime.inc ../action_module.inc ../rewards.inc
