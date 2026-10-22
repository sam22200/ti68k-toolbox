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
assets:
	$(PY) ../tools/opening_assets.py "$(DISC)"
data/%.89y: data/%.bin
	cd data && ../../../../tools/gcc4ti-bin/bin/ttbin2oth -89 dat $*.bin $*
	python3 -c "from pathlib import Path; p=Path('$@'); b=bytearray(p.read_bytes()); b[0x49]=3; p.write_bytes(b)"
# The battle program's modules (one bank for every chapter): built by ../campaign
MODULES := ../campaign/data/ffbm.89y
$(MODULES): ../fft.c ../fft.h ../module.h opening.inc $(wildcard ../*.inc)
	$(MAKE) -C ../campaign data/ffbm.89y
ti cycles: $(TI_FILES) $(MODULES)
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: ../fft.c ../fft.h opening.inc $(wildcard data/*.h)
check: test pc cycles $(MODULES)
	$(PY) check.py
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
banks: $(TI_FILES)

.PHONY: contour-assets
contour-assets:
	$(PY) ../tools/contour_assets.py --attach data/ffov0.bin
pc test cycles ti packed regression-check visual-check: | contour-assets
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: ../config.inc ../contour.h ../contour_art.inc

../contour.h ../contour_art.inc: ../tools/contour_assets.py ../art.h ../fft.c ../config.inc
	$(PY) ../tools/contour_assets.py

$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: | contour-assets

data/ffod.bin: ../tools/chapter_desc.py data/battle.h data/interface.h data/map.h data/units.h
	$(PY) ../tools/chapter_desc.py opening

data/ffs0.bin: ../tools/scenes.py data/ffost.bin data/story.h
	$(PY) ../tools/scenes.py
