NAME ?= minish
# GCC4TI emits shared source-name assembly temporaries. Serialize recipes;
# invoke the two scale builds sequentially as well.
.NOTPARALLEL:
SRC := minish.c actions.c combat.c effects.c $(if $(ZOOM),render_zoom.c)
TESTS := test_minish.c
CFLAGS_GAME := -DRT_FRAME_TICKS2=17 $(if $(ZOOM),-DMINISH_ZOOM)
TI := ../..
PY := $(TI)/tools/pyenv/bin/python
include $(TI)/runtime/rt.mk

mindat.bin mindat.be.bin &: tools/traversal.py tools/reference.py
	$(PY) tools/traversal.py

mindat.89y: mindat.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mindat.be.bin mindat

REFERENCE_STATE := $(TI)/sources/minish_gba/woods.state
$(REFERENCE_STATE): tools/reference.py $(TI)/tools/gba/gbarun.py
	$(PY) tools/reference.py

fixtures/traversal.txt fixtures/traversal.json &: tools/traversal.py tools/reference.py $(REFERENCE_STATE)
	$(PY) tools/traversal.py --measure

generated.h miscen.bin miscen.be.bin michar.bin michar.be.bin fixtures/art.json fixtures/animation.txt fixtures/occlusion.txt fixtures/depth.txt fixtures/source_art.npz &: tools/art.py fixtures/traversal.json $(REFERENCE_STATE)
	$(PY) tools/art.py

miscen.89y: miscen.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat miscen.be.bin miscen
michar.89y: michar.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat michar.be.bin michar

actions_generated.h miact.bin miact.be.bin mizact.bin mizact.be.bin fixtures/actions.txt &: tools/actions.py tools/ground.py fixtures/source_art.npz $(REFERENCE_STATE)
	$(PY) tools/actions.py
miact.89y: miact.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat miact.be.bin miact
mizact.89y: mizact.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mizact.be.bin mizact

$(NAME)_pc $(NAME)_test $(NAME).89z: minish.h mindat.bin generated.h miscen.bin michar.bin actions_generated.h $(if $(ZOOM),mizact.bin,miact.bin)
fixtures/action_pixels.bin fixtures/action_pixels_zoom.bin &: tools/action_oracle.py tools/ground.py actions_generated.h fixtures/source_art.npz
	$(PY) tools/action_oracle.py
test: fixtures/traversal.txt fixtures/animation.txt fixtures/occlusion.txt fixtures/depth.txt fixtures/actions.txt fixtures/action_pixels.bin fixtures/action_pixels_zoom.bin
ti cycles: mindat.89y miscen.89y michar.89y generated.h actions_generated.h
bench: generated.h
TI_FILES := mindat.89y $(if $(ZOOM),mizscene.89y mizactor.89y mizact.89y mizfight.89y mizfx.89y,miscen.89y michar.89y miact.89y mifight.89y mifx.89y)
ti cycles bench: $(TI_FILES) actions_generated.h
ifneq ($(ZOOM),)
$(NAME)_pc $(NAME)_test $(NAME).89z: zoom_generated.h mizscene.bin mizactor.bin
test: fixtures/zoom_pixels.bin
ti cycles bench: zoom_generated.h $(TI_FILES)
endif

zoom_generated.h mizscene.bin mizactor.bin mizactor.be.bin fixtures/zoom_pixels.bin fixtures/zoom.json &: tools/zoom.py fixtures/source_art.npz
	$(PY) tools/zoom.py

mizscene.89y: mizscene.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mizscene.bin mizscene
mizactor.89y: mizactor.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mizactor.be.bin mizactor
XCHECK := 0 1 2 3 4 5 6
FRAMES := 400

.PHONY: reference extract measure art tihash zoom preview
preview: $(NAME)_pc
	$(PY) tools/preview.py $(if $(ZOOM),--zoom)
zoom:
	$(MAKE) ZOOM=1 NAME=minishz test pc ti cycles tihash
reference:
	$(PY) tools/reference.py

extract:
	$(PY) tools/reference.py --extract-only

measure:
	$(PY) tools/traversal.py --measure

art:
	$(PY) tools/art.py

tihash: $(NAME)_test $(TI_FILES) generated.h
	$(TI)/tools/bin/ti-cc -o $(NAME)h -DRT_CYCLES -DSTATE_HASH $(CFLAGS_GAME) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(TI)/tools/extgraph/lib/tilemap.a
	$(PY) tools/check_native.py --name $(NAME) $(if $(ZOOM),--zoom)

combat_generated.h mifight.bin mifight.be.bin mizfight.bin mizfight.be.bin fixtures/combat.json fixtures/combat.txt fixtures/source_combat.npz &: tools/combat.py $(REFERENCE_STATE) tools/effects.py tools/hud.py
	$(PY) tools/combat.py --door --measure
mifight.89y: mifight.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mifight.be.bin mifight
mizfight.89y: mizfight.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mizfight.be.bin mizfight
$(NAME)_pc $(NAME)_test $(NAME).89z: combat_generated.h $(if $(ZOOM),mizfight.bin,mifight.bin)
hud_generated.h: tools/hud.py $(RT)platform-sw/amsfont.h
	$(PY) tools/hud.py
$(NAME)_pc $(NAME)_test $(NAME).89z: hud_generated.h
ti cycles bench: combat_generated.h hud_generated.h
test: fixtures/combat.json fixtures/combat.txt
fixtures/combat_pixels.bin fixtures/combat_pixels_zoom.bin &: tools/combat_oracle.py fixtures/source_combat.npz fixtures/source_art.npz
	$(PY) tools/combat_oracle.py
test: fixtures/combat_pixels.bin fixtures/combat_pixels_zoom.bin

effects_generated.h mifx.bin mifx.be.bin mizfx.bin mizfx.be.bin fixtures/roll.txt &: tools/effects.py $(REFERENCE_STATE) fixtures/combat.json
	$(PY) tools/effects.py
mifx.89y: mifx.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mifx.be.bin mifx
mizfx.89y: mizfx.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mizfx.be.bin mizfx
$(NAME)_pc $(NAME)_test $(NAME).89z: effects_generated.h $(if $(ZOOM),mizfx.bin,mifx.bin)
ti cycles bench: effects_generated.h
test: fixtures/roll.txt
fixtures/effects_pixels.bin fixtures/effects_pixels_zoom.bin &: tools/effects_oracle.py fixtures/source_effects.npz fixtures/source_art.npz
	$(PY) tools/effects_oracle.py
test: fixtures/effects_pixels.bin fixtures/effects_pixels_zoom.bin
