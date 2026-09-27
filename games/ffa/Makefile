NAME := ffa
SRC := ffa.c world.c
TESTS := test_ffa.c
include ../../runtime/rt.mk

PY := ../../tools/pyenv/bin/python
gfx.h: tools/gfx.py
	$(PY) tools/gfx.py > $@
rooms.h: tools/rooms.py tools/murparse.py
	$(PY) tools/rooms.py > $@
ffa.c: gfx.h rooms.h
