NAME := ffa
SRC := ffa.c world.c dialog.c story.c battle.c
TESTS := test_ffa.c

include ../../runtime/rt.mk
PC_CFLAGS += -Wno-implicit-fallthrough   # the story coroutines fall through on purpose

PY := ../../tools/pyenv/bin/python
gfx.h: tools/gfx.py
	$(PY) tools/gfx.py > $@
gfx_ids.h: tools/gfx.py
	$(PY) tools/gfx.py --ids > $@
rooms.h: tools/rooms.py tools/murparse.py tools/art.py
	$(PY) tools/rooms.py > $@
$(NAME)_pc $(NAME)_test $(NAME).89z: gfx.h gfx_ids.h rooms.h
