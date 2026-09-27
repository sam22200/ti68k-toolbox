# Build rules shared by every runtime game. In the game's Makefile:
#   NAME := flappy                 # calculator variable name (<= 8 chars, lowercase)
#   SRC  := game.c                 # game sources (portable C, rt.h only)
#   TESTS := tests/test_game.c     # optional unit tests (define main, link rt_sw)
#   include ../../runtime/rt.mk
# Targets: pc (SDL window: ./NAME_pc), test (headless unit tests), ti (NAME.89z),
# bench (NAMEb.89z: RT_BENCH non-UI benchmark, BENCH frames, run as NAMEb(scenario)),
# cycles (NAMEc.89z: RT_CYCLES, the TI binary for tools/bin/ti-cycles, no emulator),
# xcheck (NAMEc.89z under ti-cycles against the PC --headless run, same screen checksum per
# scenario: XCHECK="0 1 2" scenarios, FRAMES=100, KEYS=script, TI_FILES="data.89y"), clean.
# Extra: CFLAGS_GAME (both), TI_FLAGS (ti-cc only, e.g. -DRT_MONO or extra .s/.a files).
RT := $(dir $(lastword $(MAKEFILE_LIST)))
ROOT := $(RT)..
CC ?= gcc
PC_CFLAGS := -std=gnu99 -O1 -g -Wall -Wextra -Wno-unused-parameter $(CFLAGS_GAME)
SDL_CFLAGS := -I$(ROOT)/tools/sdl2/include -I$(ROOT)/tools/sdl2/include/SDL2 -D_REENTRANT
SDL_LIBS := -L$(ROOT)/tools/sdl2/lib -lSDL2
RT_CORE := $(RT)core/rt_core.c
RT_SW := $(RT)platform-sw/rt_sw.c
RT_HDR := $(RT)core/rt.h $(RT)platform-sw/rt_sw.h $(RT)platform-sw/amsfont.h

BENCH ?= 256
.PHONY: pc test ti bench cycles xcheck clean
pc: $(NAME)_pc
test: $(if $(TESTS),$(NAME)_test,)
	$(if $(TESTS),./$(NAME)_test,@echo no tests)
ti: $(NAME).89z

$(NAME)_pc: $(SRC) $(RT_CORE) $(RT_SW) $(RT)platform-sdl/rt_sdl.c $(RT_HDR)
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ $(SRC) $(RT_CORE) $(RT_SW) $(RT)platform-sdl/rt_sdl.c $(SDL_LIBS)

$(NAME)_test: $(SRC) $(TESTS) $(RT_CORE) $(RT_SW) $(RT_HDR)
	$(CC) $(PC_CFLAGS) -o $@ $(SRC) $(TESTS) $(RT_CORE) $(RT_SW)

$(NAME).89z: $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(RT)core/rt.h
	$(ROOT)/tools/bin/ti-cc -o $(NAME) $(CFLAGS_GAME) $(TI_FLAGS) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a

bench: $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(RT)core/rt.h
	$(ROOT)/tools/bin/ti-cc -o $(NAME)b -DRT_BENCH=$(BENCH) $(CFLAGS_GAME) $(TI_FLAGS) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a

cycles: $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(RT)core/rt.h
	$(ROOT)/tools/bin/ti-cc -o $(NAME)c -DRT_CYCLES $(CFLAGS_GAME) $(TI_FLAGS) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(ROOT)/tools/extgraph/lib/tilemap.a

XCHECK ?= 0
FRAMES ?= 100
xcheck: cycles pc
	@for n in $(XCHECK); do \
	  t=$$($(ROOT)/tools/bin/ti-cycles --arg $$n --frames $(FRAMES) $(if $(KEYS),--keys $(KEYS)) $(foreach f,$(TI_FILES),--file $(f)) $(NAME)c.89z | awk '/checksum/ {print $$4}'); \
	  p=$$(./$(NAME)_pc --headless --scenario $$n --frames $(FRAMES) $(if $(KEYS),--keys $(KEYS)) | awk '{print $$4}'); \
	  if [ "$$t" = "$$p" ]; then echo "scenario $$n: $$t ok"; else echo "scenario $$n: TI $$t PC $$p DIFFER"; fail=1; fi; \
	done; exit $${fail:-0}

# AMS fonts for the PC build, extracted from the local TI OS file (not in git)
$(RT)platform-sw/amsfont.h: $(RT)tools/amsfont.py
	python3 $<

clean:
	rm -f $(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)b.89z $(NAME)c.89z *.9xz *.v2z *.o
