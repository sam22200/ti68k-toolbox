# Standalone UI laboratory. No tactical executable or battle save changes.
NAME := fftmenu
.NOTPARALLEL:
SRC := menu.c
TESTS := test_menu.c
include ../../../runtime/rt.mk
DATA := ../x/management/data
DISC := ../../../roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue
$(DATA)/fixture.h $(DATA)/fftmfont.bin $(DATA)/fftmicon.bin $(DATA)/fftmvis.bin &: ../tools/management_assets.py ../tools/management_research.py ../fftu.bin
	../../../tools/pyenv/bin/python ../tools/management_assets.py "$(DISC)"
menu.c: $(DATA)/fixture.h
$(NAME)_pc $(NAME)_test $(NAME).89z $(NAME)c.89z: menu.h $(DATA)/fixture.h ../art.h ../units.h ../interface.h
.PHONY: assets review check
assets: $(DATA)/fixture.h $(DATA)/fftmfont.bin $(DATA)/fftmicon.bin $(DATA)/fftmvis.bin
	ln -sf $(abspath $(DATA)/fftmfont.bin) fftmfont.bin
	ln -sf $(abspath $(DATA)/fftmicon.bin) fftmicon.bin
	ln -sf $(abspath $(DATA)/fftmvis.bin) fftmvis.bin
	ln -sf $(abspath ../fftu.bin) fftu.bin
	ln -sf $(abspath ../ffti.bin) ffti.bin
test pc ti cycles: assets
review: test pc
	../../../tools/pyenv/bin/python ../tools/management_review.py
check: test pc ti cycles
	../../../tools/pyenv/bin/python ../tools/management_review.py --check
