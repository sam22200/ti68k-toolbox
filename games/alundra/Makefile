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
include ../../runtime/rt.mk

# gfx.h and x/ come from the local disc (tools/extract.py): commercial data, never committed
gfx.h: tools/extract.py
	@test -f $(DISC) || { echo "missing the local disc roms/ps1/Alundra (not in git)"; exit 1; }
	@test -f $(STATE) -a -f $(VILLAGE) || { echo "missing the deck or village state (RE_NOTES.md § Tools)"; exit 1; }
	$(PY) tools/extract.py $(DISC) $(STATE) $(VILLAGE) 2>/dev/null
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: gfx.h
XCHECK := 0 1 2 3 4 5 6
FRAMES := 60
