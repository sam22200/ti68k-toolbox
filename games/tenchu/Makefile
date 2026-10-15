NAME := tenchu
SRC := game.c world.c render.c ray.c sprites.c tables.c poly.c floor.c training.c ti_entry.c
TESTS := tests/test_engine.c
CFLAGS_GAME := -DRT_MONO -DRT_FRAME_TICKS=20
TI_FLAGS := -D__main=te_runtime_main
TI_FILES := tnspr.89y
XCHECK := 0 1 2 3 4 5 6 7 8 9 10 11 128 129 130 131 256 257 258 259 512
FRAMES := 8
include ../../runtime/rt.mk

tables:
	../../tools/bin/ti-table sin --scale 256 --name te_sin | sed 's/static const/const/' > trig.h
	../../tools/bin/ti-table recip --n 2048 --shift 14 --name te_recip | sed 's/static const/const/' > recip.h
	../../tools/bin/ti-table row --n 100 --stride 20 --name te_row | sed 's/static const/const/' > rows.h

$(NAME)_pc $(NAME)_test $(NAME).89z: engine.h trig.h recip.h rows.h sprites_data.h tnspr.89y

sprites_data.h tnspr.bin &: tools/sprite_tables.py
	python3 tools/sprite_tables.py
tnspr.89y: tnspr.bin
	../../tools/gcc4ti-bin/bin/ttbin2oth -quiet -89 dat tnspr.bin tnspr
extract:
	../../tools/pyenv/bin/python tools/extract.py
