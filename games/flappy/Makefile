NAME := flappy
SRC := flappy.c
TESTS := test_flappy.c
include ../../runtime/rt.mk

flappy.c: gfx.h
gfx.h: gfx.py
	python3 gfx.py > gfx.h
