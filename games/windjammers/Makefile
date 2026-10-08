.DEFAULT_GOAL := pc
NAME := windjam
SRC := windjam.c render.c
TESTS := test_windjam.c
CFLAGS_GAME := -DRT_FRAME_TICKS2=17
PY := ../../tools/pyenv/bin/python
REF := ../../sources/windjammers_neogeo
include ../../runtime/rt.mk

generated/tables.h generated/reference.h generated/mita_hold.state &: tools/prepare.py $(REF)/gameplay/report.json $(REF)/rules/report.json
	$(PY) tools/prepare.py

$(REF)/gameplay/report.json:
	$(MAKE) -C ../../tools/neogeo gameplay
$(REF)/rules/report.json:
	$(MAKE) -C ../../tools/neogeo rules

$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: windjam.h generated/tables.h
generated/advanced_tables.h: tools/advanced_tables.py
	$(PY) tools/advanced_tables.py
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: generated/advanced_tables.h
$(NAME)_test: generated/reference.h
$(NAME)_test: generated/actions.h generated/holds.h generated/timing.h
$(NAME)_test: generated/advanced.h
$(NAME)_test: generated/curves.h
$(NAME)_test: generated/dash.h
generated/dash.h generated/dash.json &: tools/measure_dash.py
	$(PY) tools/measure_dash.py
generated/curves.h generated/curves.json &: tools/measure_curves.py generated/actions.json generated/advanced_tables.h
	$(PY) tools/measure_curves.py
generated/advanced.h generated/advanced.json &: tools/measure_advanced.py generated/timing.json generated/advanced_tables.h
	$(PY) tools/measure_advanced.py
	$(PY) tools/measure_curves.py
generated/timing.h generated/timing.json generated/timing_doors.h &: tools/measure_timing.py generated/actions.json generated/tables.h
	$(PY) tools/measure_timing.py
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: generated/timing_doors.h
generated/holds.h generated/holds.json &: tools/measure_holds.py generated/actions.json generated/actions_serve.state generated/actions_mita.state
	$(PY) tools/measure_holds.py
generated/actions.h generated/actions.json generated/actions_serve.state generated/actions_mita.state &: tools/measure_actions.py generated/mita_hold.state
	$(PY) tools/measure_actions.py
generated/art.h generated/art.json generated/actors.png &: tools/extract_art.py generated/actions.json
	$(PY) tools/extract_art.py
generated/art_bank.h wjart.bin wjart.be.bin &: tools/pack_art.py generated/art.h
	$(PY) tools/pack_art.py
wjart.89y: wjart.be.bin
	../../tools/gcc4ti-bin/bin/ttbin2oth -89 dat $< wjart
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: generated/art_bank.h wjart.bin
ti cycles: wjart.89y
TI_FILES := wjart.89y
generated/layout.h: tools/layout.py
	$(PY) tools/layout.py
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: generated/layout.h
generated/court.h generated/court.json generated/court.png &: tools/extract_court.py tools/extract_art.py tools/layout.py $(REF)/gameplay/report.json
	$(PY) tools/extract_court.py
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: generated/court.h
XCHECK := 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
FRAMES := 220
KEYS := keys/play.txt

.PHONY: reference profile validate codegen art-check court-check advanced-reference curve-reference
reference:
	$(PY) tools/prepare.py
	$(PY) tools/measure_actions.py
	$(PY) tools/measure_holds.py
	$(PY) tools/measure_timing.py
	$(PY) tools/advanced_tables.py
	$(PY) tools/measure_advanced.py
	$(PY) tools/extract_art.py
	$(PY) tools/pack_art.py
	$(PY) tools/extract_court.py
advanced-reference:
	$(PY) tools/advanced_tables.py
	$(PY) tools/measure_advanced.py
curve-reference:
	$(PY) tools/measure_curves.py
court-check:
	$(PY) tools/extract_court.py
art-check: generated/art.h generated/actions_serve.state generated/actions_mita.state
	$(PY) tools/validate_art.py
profile: cycles
	$(PY) tools/validate.py --profile
validate: test pc cycles
	$(PY) tools/validate.py
codegen: generated/tables.h generated/advanced_tables.h generated/timing_doors.h
	$(PY) tools/validate.py --codegen

.PHONY: showcase dash-reference
showcase: generated/art_bank.h wjart.bin generated/layout.h generated/court.h
	$(PY) tools/showcase.py
dash-reference:
	$(PY) tools/measure_dash.py
