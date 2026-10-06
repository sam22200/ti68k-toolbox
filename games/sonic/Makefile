# Sonic traversal prototype. Commercial terrain/reference data are generated locally.
.DEFAULT_GOAL := pc
NAME := sonic
SRC := sonic.c objects.c art.c
TESTS := test_sonic.c
CFLAGS_GAME := -DRT_FRAME_TICKS2=17
TI := ../..
PY := $(TI)/tools/pyenv/bin/python
RUN := $(TI)/tools/md/mdrun.py
ROM := $(TI)/roms/md/Sonic_1.md
OUT := $(TI)/sources/sonic1_md
FIELDS := ffd008:2,ffd00c:2,ffd010:s2,ffd012:s2,ffd014:s2,ffd022:1
include ../../runtime/rt.mk

$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: sonic.h art.h generated/trig.h generated/art_ids.h sonterr.bin sonart.bin
$(NAME)_test: generated/physics_ref.h generated/objects_ref.h

generated/trig.h: $(TI)/tools/bin/ti-table
	@mkdir -p generated
	$(TI)/tools/bin/ti-table sin --scale 256 > $@

sonterr.bin sonterr.be.bin &: tools/extract.py $(OUT)/start.state
	$(PY) tools/extract.py

sonterr.89y: sonterr.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< sonterr

sonart.bin sonart.be.bin generated/art_ids.h &: tools/art.py tools/extract.py $(OUT)/start.state
	$(PY) tools/art.py

sonart.89y: sonart.be.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< sonart

generated/physics_ref.h: tools/trace_header.py $(OUT)/physics.json
	$(PY) tools/trace_header.py

$(OUT)/physics.json: tools/measure.py $(OUT)/start.state
	$(PY) tools/measure.py

generated/objects_ref.h: tools/measure_objects.py $(OUT)/start.state
	@mkdir -p generated
	$(PY) tools/measure_objects.py

$(OUT)/start.state: keys/ref_boot.txt $(RUN)
	@mkdir -p $(OUT)/x
	$(PY) $(RUN) $(ROM) --frames 1000 --keys keys/ref_boot.txt --save $@ --shot 999:$(OUT)/x/start.png --metadata $(OUT)/boot.json

ti cycles: sonterr.89y sonart.89y
TI_FILES := sonterr.89y sonart.89y
XCHECK := 0 1 2 3 4 5 6 7 8 9 10 11 12
FRAMES := 220

.PHONY: reference reference-check measure profile
test: artcheck
.PHONY: artcheck rendercheck
artcheck: sonart.bin sonart.be.bin
	$(PY) tools/check_art.py
rendercheck: $(NAME)_test
	@mkdir -p x
	$(CC) $(PC_CFLAGS) -DSON_NO_PARTICLE_CULL -o x/artfull_test $(SRC) $(TESTS) $(RT_CORE) $(RT_SW)
	@set -e; for case in play:0 damage:10 burst:12; do \
	  script=$${case%:*}; scenario=$${case#*:}; \
	  ./$(NAME)_test --planes keys/$$script.txt 260 $$scenario > x/$$script.culled.planes; \
	  ./x/artfull_test --planes keys/$$script.txt 260 $$scenario > x/$$script.full.planes; \
	  cmp x/$$script.culled.planes x/$$script.full.planes; echo "render $$script: 260 frames culling = full draw"; \
	done
profile: cycles
	$(PY) tools/profile.py
reference:
	@mkdir -p $(OUT)/x
	$(PY) $(RUN) $(ROM) --frames 1000 --keys keys/ref_boot.txt --save $(OUT)/start.state --shot 999:$(OUT)/x/start.png --metadata $(OUT)/boot.json
	$(PY) $(RUN) $(ROM) --load $(OUT)/start.state --frames 60 --keys keys/ref_move.txt --trace $(FIELDS) > $(OUT)/x/move.trace
	$(PY) $(RUN) $(ROM) --load $(OUT)/start.state --frames 70 --keys keys/ref_jump.txt --trace $(FIELDS) > $(OUT)/x/jump.trace

reference-check:
	$(MAKE) -C $(ROOT)/tools/md check

measure:
	$(PY) tools/measure.py

.PHONY: tihash
tihash: $(NAME)_test sonterr.89y sonart.89y generated/trig.h
	$(TI)/tools/bin/ti-cc -o sonich -DRT_CYCLES -DSTATE_HASH $(CFLAGS_GAME) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(TI)/tools/extgraph/lib/tilemap.a
	@mkdir -p x
	@set -e; for case in walk:0 jump:0 roll:0 play:0 damage:10 burst:12; do \
	  script=$${case%:*}; scenario=$${case#*:}; \
	  $(TI)/tools/bin/ti-cycles --arg $$scenario --file sonterr.89y --file sonart.89y --keys keys/$$script.txt --frames 260 sonich.89z > x/$$script.ti.log; \
	  awk '/^value/ {print $$2}' x/$$script.ti.log | head -n 260 > x/$$script.ti.hash; \
	  ./$(NAME)_test --hash keys/$$script.txt 260 $$scenario > x/$$script.pc.hash; \
	  test "$$(wc -l < x/$$script.ti.hash)" = 260; \
	  cmp x/$$script.ti.hash x/$$script.pc.hash; echo "state $$script: 260 frames TI = PC"; \
	done
