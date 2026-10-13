# Final Fantasy Tactics (PS1), Magic City Gariland: isometric tech demo with four camera
# orientations (README.md). Our own engine, cursor and shadow (tools/art.py -> art.h,
# committed); the battlefield's heights, its four textured views and the units' sprites come
# from the local disc (tools/extract.py -> map.h, fftv0..3.bin ZX0-packed, .89y data variables
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
$(VIEWS:=.bin): map.h
%.89y: %.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< $*
fftu.bin: units.h
fftw.bin: weapons.h
ti cycles: $(VIEWS:=.89y) fftu.89y fftw.89y
TI_FILES := $(VIEWS:=.89y) fftu.89y fftw.89y
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
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: map.h units.h weapons.h battle.h art.h fft.h $(VIEWS:=.bin) fftu.bin fftw.bin
XCHECK := 0 1 2 3 4 5 6 7 8 9 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25
FRAMES := 40
.PHONY: review-blows check-blows
review-blows: pc
	$(PY) tools/review_blows.py
check-blows:
	$(PY) tools/check_blows.py
