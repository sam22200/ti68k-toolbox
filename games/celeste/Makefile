# Celeste Classic port, slice 1 (room 0). x/ (the cart extracted by p8extract.py), room.h, gfx.h
# and traces/ (the cart run under z8lua) are generated from cart/celeste.p8.png: README.md.
NAME := celeste
CFLAGS_GAME := -DRT_FRAME_TICKS2=17       # frames of 8 and 9 ticks: 30.1 fps (decision Q6)
SRC := celeste.c
TESTS := test_celeste.c
TI := ../..
X := x
P8 := $(TI)/.claude/skills/ti-port-pico8/scripts
PY := $(TI)/tools/pyenv/bin/python
include $(TI)/runtime/rt.mk

# scripts.txt: key script, scenario (its p8trace --pre below), frames
PRE0 := begin_game()
PRE1 := begin_game() local s for o in all(objects) do if o.type==player_spawn then s=o end end destroy_object(s) init_object(player,s.x,s.target.y)
SCRIPTS := $(shell awk 'NF == 3 && $$1 ~ /^[a-z]/ {print $$1 ":" $$2 ":" $$3}' scripts.txt)

celeste.c: room.h gfx.h
$(X)/info.json: cart/celeste.p8.png
	$(PY) $(P8)/p8extract.py $< $(X)

STYLE ?= B
room.h gfx.h: gfx.py $(X)/info.json
	python3 gfx.py $(X) . $(STYLE)
$(NAME)_pc $(NAME)_test $(NAME).89z: room.h gfx.h

$(NAME)_test: traces
.PHONY: traces
traces: trace.lua scripts.txt $(X)/info.json $(wildcard keys/*.txt)
	@mkdir -p traces
	@for s in $(SCRIPTS); do n=$${s%%:*}; r=$${s#*:}; sc=$${r%%:*}; fr=$${r#*:}; \
	  if [ $$sc = 1 ]; then pre='$(PRE1)'; else pre='$(PRE0)'; fi; \
	  if [ ! traces/$$n.txt -nt keys/$$n.txt ] || [ ! traces/$$n.txt -nt trace.lua ]; then \
	    $(PY) $(P8)/p8trace.py $(X) --frames $$fr --pre "$$pre" --lib trace.lua --trace "__st()" --keys keys/$$n.txt > traces/$$n.txt || exit 1; \
	    echo "reference traces/$$n.txt ($$fr frames, scenario $$sc)"; fi; \
	done

# The TI binary's logic against the PC's: the same per-frame state hash for every script
# (celesteh.89z = the cycles build plus -DSTATE_HASH, run under ti-cycles, no emulator)
.PHONY: tihash
tihash: $(NAME)_test
	$(TI)/tools/bin/ti-cc -o celesteh -DRT_CYCLES -DSTATE_HASH $(CFLAGS_GAME) $(SRC) $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(TI)/tools/extgraph/lib/tilemap.a
	@for s in $(SCRIPTS); do n=$${s%%:*}; r=$${s#*:}; sc=$${r%%:*}; fr=$${r#*:}; \
	  $(TI)/tools/bin/ti-cycles --arg $$sc --keys keys/$$n.txt --frames $$fr celesteh.89z | awk '/^value/ {print $$3}' | head -n $$fr > ti_$$n.hash; \
	  ./$(NAME)_test --hash $$n $$sc $$fr > pc_$$n.hash; \
	  if cmp -s ti_$$n.hash pc_$$n.hash && [ $$(wc -l < ti_$$n.hash) = $$fr ]; then echo "tihash $$n: $$fr frames, TI = PC"; \
	  else echo "tihash $$n: TI and PC differ from frame $$(cmp ti_$$n.hash pc_$$n.hash | awk '{print $$NF - 1}')"; fail=1; fi; \
	done; rm -f ti_*.hash pc_*.hash; exit $${fail:-0}
