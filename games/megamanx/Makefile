NAME := mmx
SRC := mmx.c terrain.c art.c
TESTS := test_mmx.c
CFLAGS_GAME := -DRT_FRAME_TICKS2=17
TI_FILES := mmxmap.89y mmxart.89y
XCHECK := 0 1 2 3 4 5 6 7 8 9
PY := ../../tools/pyenv/bin/python
REFOUT := ../../sources/megamanx_snes
.DEFAULT_GOAL := test

.PHONY: reference measure terrain art combat-reference culling-reference damage-reference capacity-reference wall-charge-reference traversal-reference
reference:
	$(PY) tools/reference.py
measure: reference
	$(PY) tools/measure.py
combat-reference: $(REFOUT)/start.state
	$(PY) tools/combat_reference.py
culling-reference: $(REFOUT)/start.state
	$(PY) tools/culling_reference.py
damage-reference: $(REFOUT)/damage/reference.json
capacity-reference: $(REFOUT)/capacity/reference.json
wall-charge-reference: $(REFOUT)/wall_charge/reference.json
traversal-reference: $(REFOUT)/traverse/reference.json
art: mmxmap.89y mmxart.89y
terrain:
	$(PY) tools/terrain.py
$(REFOUT)/start.state: tools/reference.py keys/ref_boot.txt
	$(PY) tools/reference.py
generated/motion_ref.h $(REFOUT)/wall.state $(REFOUT)/hit.state &: tools/measure.py $(REFOUT)/start.state
	$(PY) tools/measure.py
$(REFOUT)/combat/natural-260.state $(REFOUT)/combat/shoot-290.state &: tools/combat_reference.py $(REFOUT)/start.state
	$(PY) tools/combat_reference.py
mmxmap.bin mmxmap.be.bin mmxart.bin mmxart.be.bin generated/art_ids.h &: tools/art.py tools/rom.py tools/terrain.py ../../tools/snes/ppu.py generated/motion_ref.h $(REFOUT)/combat/natural-260.state $(REFOUT)/combat/shoot-290.state $(REFOUT)/enemy/brake.state $(REFOUT)/wall_charge/ready-0.state
	$(PY) tools/art.py
mmxmap.89y: mmxmap.be.bin
	../../tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< mmxmap
mmxart.89y: mmxart.be.bin
	../../tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< mmxart
generated/combat_ref.h: tools/combat_header.py generated/motion_ref.h
	$(PY) tools/combat_header.py
$(REFOUT)/combined/reference.json: tools/combined_reference.py $(REFOUT)/start.state $(REFOUT)/wall.state $(REFOUT)/traverse/reference.json $(REFOUT)/traverse/ledge.state
	$(PY) tools/combined_reference.py
generated/combined_ref.h: tools/combined_header.py $(REFOUT)/combined/reference.json
	$(PY) tools/combined_header.py
$(REFOUT)/enemy/reference.json $(REFOUT)/enemy/brake.state $(REFOUT)/enemy/collider-pellet.state $(REFOUT)/enemy/collider-medium.state $(REFOUT)/enemy/collider-large.state $(REFOUT)/enemy/charged-260.state $(REFOUT)/enemy/medium-ready.state &: tools/enemy_reference.py $(REFOUT)/combat/natural-260.state $(REFOUT)/start.state
	$(PY) tools/enemy_reference.py
$(REFOUT)/collision/reference.json: tools/collision_reference.py $(REFOUT)/enemy/reference.json
	$(PY) tools/collision_reference.py
generated/combat_boxes.h: tools/combat_boxes.py $(REFOUT)/collision/reference.json
	$(PY) tools/combat_boxes.py
generated/enemy_ref.h: tools/enemy_header.py $(REFOUT)/enemy/reference.json
	$(PY) tools/enemy_header.py
$(REFOUT)/damage/reference.json: tools/damage_reference.py $(REFOUT)/combat/natural-260.state $(REFOUT)/hit.state $(REFOUT)/wall.state $(REFOUT)/enemy/charged-260.state
	$(PY) tools/damage_reference.py
generated/damage_ref.h: tools/damage_header.py $(REFOUT)/damage/reference.json
	$(PY) tools/damage_header.py
$(NAME)_test: generated/damage_ref.h
$(REFOUT)/capacity/reference.json: tools/capacity_reference.py $(REFOUT)/start.state
	$(PY) tools/capacity_reference.py
generated/capacity_ref.h: tools/capacity_header.py tools/combined_header.py $(REFOUT)/capacity/reference.json
	$(PY) tools/capacity_header.py
$(NAME)_test: generated/capacity_ref.h
$(REFOUT)/wall_charge/reference.json $(REFOUT)/wall_charge/ready-0.state $(REFOUT)/wall_charge/ready-1.state $(REFOUT)/wall_charge/ready-3.state $(REFOUT)/wall_charge/grip.state &: tools/wall_charge_reference.py $(REFOUT)/start.state
	$(PY) tools/wall_charge_reference.py
generated/wall_ref.h: tools/wall_header.py $(REFOUT)/wall_charge/reference.json
	$(PY) tools/wall_header.py
$(NAME)_test: generated/wall_ref.h
$(REFOUT)/traverse/reference.json $(REFOUT)/traverse/ledge.state $(REFOUT)/traverse/pre-clamp.state &: tools/traversal_reference.py $(REFOUT)/start.state
	$(PY) tools/traversal_reference.py
generated/traversal_ref.h: tools/traversal_header.py $(REFOUT)/traverse/reference.json
	$(PY) tools/traversal_header.py
$(NAME)_test: generated/traversal_ref.h
$(NAME)_test: generated/enemy_ref.h
$(NAME)_test $(NAME)_pc $(NAME).89z: generated/combat_boxes.h
$(NAME)_test: generated/motion_ref.h generated/combat_ref.h generated/combined_ref.h mmxmap.bin mmx.h terrain.h art.h generated/art_ids.h
$(NAME)_pc $(NAME).89z: mmx.h terrain.h art.h generated/art_ids.h mmxmap.bin mmxart.bin
ti cycles: $(TI_FILES)
include ../../runtime/rt.mk
.PHONY: trace check preview original-ti package
trace: $(TI_FILES) generated/art_ids.h generated/combat_boxes.h
	../../tools/bin/ti-cc -o mmxtrace -DRT_CYCLES -DMX_TRACE $(CFLAGS_GAME) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c ../../tools/extgraph/lib/tilemap.a
original-ti: $(TI_FILES) generated/art_ids.h generated/combat_boxes.h $(REFOUT)/damage/reference.json $(REFOUT)/capacity/reference.json $(REFOUT)/wall_charge/reference.json $(REFOUT)/traverse/reference.json
	../../tools/bin/ti-cc -o mxref -DRT_CYCLES -DMX_TRACE -DMX_REFERENCE $(CFLAGS_GAME) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c ../../tools/extgraph/lib/tilemap.a
	$(PY) tools/check_original_ti.py
	$(PY) tools/check_combined_ti.py
	$(PY) tools/check_enemy_ti.py
	$(PY) tools/check_damage_ti.py
	$(PY) tools/check_capacity_ti.py
	$(PY) tools/check_wall_ti.py
	$(PY) tools/check_traversal_ti.py
check: test pc ti cycles trace
	$(PY) tools/check.py
	$(PY) tools/check_art.py
preview: pc
	$(PY) tools/preview.py
package: ti
	$(PY) tools/package.py
