# Alundra-style traversal (RE_NOTES.md): our own engine, the behaviour studied in the PS1 game
# with ti-port-ps1. No code or data from the disc; the art (gfx.h) is generated from the local
# disc by tools/extract.py and never committed.
NAME := alundra
SRC := alundra.c
TESTS := test_alundra.c
TI := ../..
PY := $(TI)/tools/pyenv/bin/python
DISC := "$(TI)/roms/ps1/Alundra (USA) (Rev 1)/Alundra (USA) (Rev 1).cue"
STATE := $(TI)/sources/alundra_ps1/deck.state
VILLAGE := $(TI)/sources/alundra_ps1/inoa.state
SCALE := 22/42                   # the game's scale on the TI (Alundra 42 px tall -> 22): sprite, speeds, levels
include ../../runtime/rt.mk

# gfx.h and x/ come from the local disc (tools/extract.py): commercial data, never committed
gfx.h: tools/extract.py Makefile
	@test -f $(DISC) || { echo "missing the local disc roms/ps1/Alundra (not in git)"; exit 1; }
	@test -f $(STATE) -a -f $(VILLAGE) || { echo "missing the deck or village state (RE_NOTES.md § Tools)"; exit 1; }
	$(PY) tools/extract.py $(DISC) $(STATE) $(VILLAGE) $(strip $(SCALE)) 2>/dev/null
# world.h: the test room's image, drawn once by the game's own code (PC renderer, tools/bake.c);
# from gfx.h, so local too. The village's image (alvil0/1: light, dark) and what hides the
# player (alvil2/3: depth threshold, mask): .bin (PC) and .89y (TI data variables: send them
# with alundra.89z, archived), made by tools/extract.py
VIL := alvil0 alvil1 alvil2 alvil3
$(VIL:=.bin): gfx.h
%.89y: %.bin
	$(TI)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< $*
ti cycles: $(VIL:=.89y)
TI_FILES := $(VIL:=.89y)
world.h: gfx.h alundra.c alundra.h tools/bake.c
	@mkdir -p x
	$(CC) $(PC_CFLAGS) -DBAKE -o x/bake alundra.c tools/bake.c $(RT_CORE) $(RT_SW)
	x/bake > $@
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: gfx.h world.h
XCHECK := 0 1 2 3 4 5 6 7
FRAMES := 60
