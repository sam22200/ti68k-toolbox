ROOT := ../..
PY := $(ROOT)/tools/pyenv/bin/python
NAME := yjprobe
SRC := yoshi.c terrain.c actors.c art.c damage.c items.c
TESTS := test_yoshi.c
# Diagnostic only: one logic step per frame at 51.2 Hz; PAL is 50.007 Hz.
CFLAGS_GAME := -DRT_FRAME_TICKS=5
XCHECK := 0 1 2 3 10 12 13 14 15
TI_FILES := yjterr.89y yjart.89y

.PHONY: reference check measure movement tihash survey
reference:
	$(PY) tools/reference.py

check:
	$(MAKE) -C $(ROOT)/tools/snes check

measure: reference
	$(PY) tools/measure.py
	$(PY) tools/trace_header.py

movement:
	@test -f $(ROOT)/sources/yoshi_snes/start.state || $(MAKE) reference
	$(PY) tools/movement_reference.py
	$(PY) tools/movement_header.py

survey: reference
	$(PY) tools/survey.py

generated/jump_ref.h: tools/measure.py tools/trace_header.py tools/reference.py keys/ref_boot.txt keys/ref_enter.txt
	$(MAKE) measure

generated/movement_ref.h: tools/movement_reference.py tools/movement_header.py
	$(MAKE) movement

$(NAME)_test: generated/jump_ref.h generated/movement_ref.h generated/terrain_ref.h generated/actors_ref.h generated/actors_map.h yoshi.h terrain.h actors.h yjterr.bin
$(NAME)_test: generated/damage_ref.h
$(NAME)_pc $(NAME).89z: yoshi.h terrain.h actors.h generated/actors_map.h yjterr.bin
$(NAME)_test $(NAME)_pc $(NAME).89z: art.h damage.h items.h generated/art_ids.h generated/coins.h yjart.bin
$(NAME)_test $(NAME)_pc $(NAME).89z: generated/egg_angles.h
$(NAME)_test $(NAME)_pc $(NAME).89z: generated/screen_rows.h
$(NAME)_test $(NAME)_pc $(NAME).89z: generated/egg_cursor.h

generated/egg_angles.h: $(ROOT)/tools/bin/ti-table
	$(ROOT)/tools/bin/ti-table sin --n 64 --scale 2272 --name egg_sin > $@

generated/screen_rows.h: $(ROOT)/tools/bin/ti-table
	$(ROOT)/tools/bin/ti-table row --n 100 --stride 30 --name screen_rows > $@

generated/egg_cursor.h: $(ROOT)/tools/bin/ti-table
	$(ROOT)/tools/bin/ti-table sin --n 64 --scale 64 --name egg_cursor > $@

.PHONY: art
art: generated/scene.bin generated/art_ids.h yjart.89y yjterr.89y

.PHONY: preview damage-preview
preview: pc
	$(PY) tools/preview.py

damage-preview: pc
	@mkdir -p x
	@printf '0\n' > x/idle.txt
	$(PY) tools/preview.py --damage

generated/scene.bin generated/art_ids.h generated/coins.h yjart.bin yjart.be.bin &: tools/art.py tools/terrain.py
	@test -f $(ROOT)/sources/yoshi_snes/start.state || $(MAKE) reference
	@test -f $(ROOT)/sources/yoshi_snes/survey/view4.vram || $(MAKE) survey
	@test -f $(ROOT)/sources/yoshi_snes/damage/contact.state || $(MAKE) damage-reference
	$(PY) tools/art.py

yjart.89y: yjart.be.bin
	$(ROOT)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< yjart

.PHONY: actors-reference
actors-reference:
	@test -f $(ROOT)/sources/yoshi_snes/start.state || $(MAKE) reference
	$(PY) tools/actors_reference.py

generated/actors_ref.h generated/actors_map.h &: tools/actors_reference.py
	$(MAKE) actors-reference

.PHONY: actors-check
actors-check: $(NAME)_test cycles
	./$(NAME)_test --play-actors x/actors-replay.txt
	cmp keys/actors.txt x/actors-replay.txt
	$(ROOT)/tools/bin/ti-cc -o yjactorh -DRT_CYCLES -DYJ_TRACE -DYJ_TERR_TRACE -DYJ_ACTOR_TRACE $(CFLAGS_GAME) $(SRC) $(ROOT)/runtime/core/rt_core.c $(ROOT)/runtime/platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a
	$(PY) tools/check_actors.py

.PHONY: state-check
state-check: $(NAME)_test yjterr.89y yjart.89y
	$(ROOT)/tools/bin/ti-cc -o yjtrace -DRT_CYCLES -DYJ_TRACE $(CFLAGS_GAME) $(SRC) $(ROOT)/runtime/core/rt_core.c $(ROOT)/runtime/platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a
	$(PY) tools/check_ti.py --states-only
	$(ROOT)/tools/bin/ti-cc -o yjterrh -DRT_CYCLES -DYJ_TRACE -DYJ_TERR_TRACE $(CFLAGS_GAME) $(SRC) $(ROOT)/runtime/core/rt_core.c $(ROOT)/runtime/platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a
	$(PY) tools/check_terrain.py --states-only

yjterr.bin yjterr.be.bin &: tools/terrain.py generated/scene.bin
	@test -f $(ROOT)/sources/yoshi_snes/start.state || $(MAKE) reference
	$(PY) tools/terrain.py

yjterr.89y: yjterr.be.bin
	$(ROOT)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< yjterr

ti cycles: yjterr.89y yjart.89y generated/art_ids.h

.PHONY: terrain-reference
terrain-reference:
	$(PY) tools/terrain_reference.py
	$(PY) tools/terrain_header.py

generated/terrain_ref.h: tools/terrain_reference.py tools/terrain_header.py
	$(MAKE) terrain-reference

.PHONY: terrain-check
terrain-check: $(NAME)_test cycles
	$(ROOT)/tools/bin/ti-cc -o yjterrh -DRT_CYCLES -DYJ_TRACE -DYJ_TERR_TRACE $(CFLAGS_GAME) $(SRC) $(ROOT)/runtime/core/rt_core.c $(ROOT)/runtime/platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a
	$(PY) tools/check_terrain.py

tihash: generated/jump_ref.h generated/movement_ref.h cycles
	$(ROOT)/tools/bin/ti-cc -o yjtrace -DRT_CYCLES -DYJ_TRACE $(CFLAGS_GAME) $(SRC) $(ROOT)/runtime/core/rt_core.c $(ROOT)/runtime/platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a
	$(PY) tools/check_ti.py

include ../../runtime/rt.mk

.PHONY: damage-reference
damage-reference:
	$(PY) tools/damage_reference.py

generated/damage_ref.h: tools/damage_reference.py
	$(MAKE) damage-reference

.PHONY: damage-check
damage-check: test pc cycles
	./$(NAME)_test --play-actors x/actors-replay.txt
	cmp keys/actors.txt x/actors-replay.txt
	$(ROOT)/tools/bin/ti-cc -o yjdamageh -DRT_CYCLES -DYJ_TRACE -DYJ_TERR_TRACE -DYJ_ACTOR_TRACE -DYJ_DAMAGE_TRACE $(CFLAGS_GAME) $(SRC) $(ROOT)/runtime/core/rt_core.c $(ROOT)/runtime/platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a
	$(PY) tools/check_damage.py

.PHONY: items-reference aim-reference items-check items-preview
items-reference:
	$(PY) tools/items_reference.py

aim-reference:
	$(PY) tools/aim_reference.py

items-check: test pc cycles
	./$(NAME)_test --play-stomp keys/jump-stomp.txt
	$(ROOT)/tools/bin/ti-cc -o yjitemh -DRT_CYCLES -DYJ_TRACE -DYJ_TERR_TRACE -DYJ_ACTOR_TRACE -DYJ_DAMAGE_TRACE -DYJ_ITEM_TRACE $(CFLAGS_GAME) $(SRC) $(ROOT)/runtime/core/rt_core.c $(ROOT)/runtime/platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a
	$(PY) tools/check_items.py

items-preview: pc
	$(PY) tools/preview.py --items
