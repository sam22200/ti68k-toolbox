# Final Fantasy Tactics (PS1), Magic City Gariland: isometric tech demo with four camera
# orientations (README.md). Our own engine and units (tools/art.py -> art.h, committed); the
# battlefield's heights and its four textured views come from the local disc
# (tools/extract.py -> map.h, fftv0..3.bin ZX0-packed; .89y data variables for the TI), never
# committed. The TI unpacks them with lib/unpack68k.s.
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
$(VIEWS:=.bin): map.h
%.89y: %.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< $*
ti cycles: $(VIEWS:=.89y)
TI_FILES := $(VIEWS:=.89y)
TI_FLAGS := ../../lib/unpack68k.s
art.h: tools/art.py
	$(PY) tools/art.py $@
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: map.h art.h fft.h $(VIEWS:=.bin)
XCHECK := 0 1 2 3 4 5 6 7
FRAMES := 40
