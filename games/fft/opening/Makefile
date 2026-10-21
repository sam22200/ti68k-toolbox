NAME := fftopen
SRC := opening.c
TESTS := test_opening.c
include ../../../runtime/rt.mk
PY := ../../../tools/pyenv/bin/python
DISC := $(ROOT)/roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue
BANKS := ffov0 ffov1 ffov2 ffov3 ffoa ffoi ffost ffcu ffcw ffca ffcb
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
ti cycles: $(TI_FILES)
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: ../fft.c ../fft.h opening.inc $(wildcard data/*.h)
check: test pc cycles
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

# Stock AMS 2.xx accepts the small launcher; the 64K native program is a packed
# archive data variable. This does not stand in for a real HW2 launch test.
.PHONY: packed
packed: $(TI_FILES)
	$(ROOT)/tools/bin/ti-cc -o fftopen -pack ffocore $(CFLAGS_GAME) $(TI_FLAGS) ../campaign/native_opening.c $(RT_CORE) $(ROOT)/tools/extgraph/lib/tilemap.a
	python3 -c "from pathlib import Path; files=[Path('ffocore.89y')]+[Path(p) for p in '$(TI_FILES)'.split()]; [(lambda p,b: (b.__setitem__(0x49,3),p.write_bytes(b)))(p,bytearray(p.read_bytes())) for p in files]"

.PHONY: bundle
bundle: packed
	@mkdir -p x
	$(ROOT)/tools/bin/ti-group x/fftopening.89g fftopen.89z ffocore.89y $(TI_FILES)

.PHONY: contour-assets
contour-assets:
	$(PY) ../tools/contour_assets.py --attach data/ffov0.bin
pc test cycles ti packed regression-check visual-check: | contour-assets
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: ../config.inc ../contour.h ../contour_art.inc

../contour.h ../contour_art.inc: ../tools/contour_assets.py ../art.h ../fft.c ../config.inc
	$(PY) ../tools/contour_assets.py

$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z fftopen_regression_test fftopen_visual_test: | contour-assets
