# Metal Gear Solid (GBC) port, milestone 1: VR Sneaking Practice Lv.01 (README.md, ROADMAP.md).
# level.h, gfx.h, mgsdat.bin / mgsdat.be.bin and x/ are generated from the local ROM
# (roms/gb/Metal_Gear_Solid_.gbc) by tools/extract.py: commercial data, never committed.
NAME := mgs
CFLAGS_GAME := -DRT_FRAME_TICKS2=17       # 30.1 fps, two GB logic frames per TI frame
SRC := mgs.c
TESTS := test_mgs.c
TI := ../..
PY := $(TI)/tools/pyenv/bin/python
ROM := $(TI)/roms/gb/Metal_Gear_Solid_.gbc
include $(TI)/runtime/rt.mk

level.h gfx.h mgsdat.bin mgsdat.be.bin: tools/extract.py
	@test -f $(ROM) || { echo "missing $(ROM) (local ROM, not in git)"; exit 1; }
	$(PY) tools/extract.py $(ROM) | head -3
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: level.h gfx.h mgsdat.bin

mgsdat.89y: mgsdat.be.bin                 # the TI data variable: send it with mgs.89z, archive it
	$(ROOT)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat mgsdat.be.bin mgsdat
ti: mgsdat.89y
cycles: mgsdat.89y
TI_FILES := mgsdat.89y
XCHECK := 0 1 2 3
FRAMES := 200

# The TI binary's logic against the PC's: the same per-frame state hash for each key script
SCRIPTS := win spotted
.PHONY: tihash
tihash: $(NAME)_test mgsdat.89y
	$(TI)/tools/bin/ti-cc -o mgsh -DRT_CYCLES -DSTATE_HASH $(CFLAGS_GAME) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(TI)/tools/extgraph/lib/tilemap.a
	@for n in $(SCRIPTS); do \
	  $(TI)/tools/bin/ti-cycles --file mgsdat.89y --keys keys/$$n.txt --frames 400 mgsh.89z | awk '/^value/ {print $$3}' | head -n 400 > ti_$$n.hash; \
	  ./$(NAME)_test --hash keys/$$n.txt 400 > pc_$$n.hash; \
	  if cmp -s ti_$$n.hash pc_$$n.hash && [ $$(wc -l < ti_$$n.hash) = 400 ]; then echo "tihash $$n: 400 frames, TI = PC"; \
	  else echo "tihash $$n: TI and PC differ"; fail=1; fi; \
	done; rm -f ti_*.hash pc_*.hash; exit $${fail:-0}
