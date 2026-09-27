NAME := ffa
SRC := ffa.c world.c dialog.c story.c battle.c menu.c title.c
TESTS := test_ffa.c

include ../../runtime/rt.mk
PC_CFLAGS += -Wno-implicit-fallthrough   # the story coroutines fall through on purpose

PY := ../../tools/pyenv/bin/python
gfx.h gfx.bin gfx.be.bin: tools/gfx.py         # sprites: macros + the block appended to ffadat
	$(PY) tools/gfx.py > gfx.h
gfx_ids.h: tools/gfx.py
	$(PY) tools/gfx.py --ids > $@
rooms.h ffadat.bin ffadat.be.bin: tools/rooms.py tools/murparse.py tools/art.py gfx.bin
	$(PY) tools/rooms.py > rooms.h
ffadat.89y: ffadat.be.bin                 # the TI data variable (send it with ffa.89z, archive it)
	$(ROOT)/tools/gcc4ti-bin/bin/ttbin2oth -89 dat ffadat.be.bin ffadat
ti: ffadat.89y
$(NAME)_pc $(NAME)_test $(NAME).89z: gfx.h gfx_ids.h rooms.h

TI_FILES := ffadat.89y
XCHECK := 0 10 1 2 3 4 5 6 7 8 9 50 51 52 53 105 106 110
cycles: ffadat.89y
