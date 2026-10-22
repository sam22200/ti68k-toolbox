NAME := fftscn
SRC := scene.c
include ../../../runtime/rt.mk
PY := ../../../tools/pyenv/bin/python
DISC := $(ROOT)/roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue
BANKS := ffs1 ffgm ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb
TI_FILES := $(addprefix data/,$(BANKS:=.89y))
.NOTPARALLEL:
.PHONY: assets bundle
check: pc test $(TI_FILES)
	$(PY) check.py
test: status-test
test: dialogue-test
test: scene-test
.PHONY: scene-test
scene-test: data/ffs1.bin
	$(CC) $(PC_CFLAGS) -o fftscn_test test_scene.c $(RT_CORE) $(RT_SW)
	./fftscn_test
.PHONY: dialogue-test
dialogue-test:
	$(CC) $(PC_CFLAGS) -o fftscn_dialogue_test test_dialogue.c $(RT_CORE) $(RT_SW)
	./fftscn_dialogue_test
.PHONY: status-test
status-test:
	$(CC) $(PC_CFLAGS) -o fftgar_status_test test_status.c $(RT_CORE) $(RT_SW)
	./fftgar_status_test
assets:
	$(PY) ../tools/campaign_assets.py "$(DISC)"
data/%.89y: data/%.bin
	cd data && ../../../../tools/gcc4ti-bin/bin/ttbin2oth -89 dat $*.bin $*
	python3 -c "from pathlib import Path; p=Path('$@'); b=bytearray(p.read_bytes()); b[0x49]=3; p.write_bytes(b)"
fftscn_pc: scene.c pc_entry.c $(RT_CORE) $(RT_SW)
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ scene.c $(RT_CORE) $(RT_SW) pc_entry.c $(SDL_LIBS)
pc: fftintro_pc fftgar_pc
fftintro_pc: ../opening/opening.c ../fft.c ../fft.h ../opening/opening.inc pc_entry.c
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ ../opening/opening.c $(RT_CORE) $(RT_SW) pc_entry.c $(SDL_LIBS)
fftgar_pc: gariland.c ../fft.c ../fft.h
	$(CC) $(PC_CFLAGS) $(SDL_CFLAGS) -o $@ gariland.c $(RT_CORE) $(RT_SW) $(RT)platform-sdl/rt_sdl.c $(SDL_LIBS)
# Chapter programs are archived too: in RAM, a waiting one would take the running one's room.
packed: $(TI_FILES)
	$(MAKE) -C ../opening packed
	$(ROOT)/tools/bin/ti-cc -o fftgar -pack ffgcore gariland.c $(RT_CORE) $(RT)platform-ti68k/rt_ti.c $(ROOT)/lib/unpack68k.s $(ROOT)/tools/extgraph/lib/tilemap.a
	$(ROOT)/tools/bin/ti-cc -o fftscn -pack ffscore native_scene.c $(RT_CORE) $(ROOT)/lib/unpack68k.s $(ROOT)/tools/extgraph/lib/tilemap.a
	$(ROOT)/tools/bin/ti-cc -o fftstory launcher.c
	python3 -c "from pathlib import Path; files=[Path('ffgcore.89y'),Path('ffscore.89y'),Path('fftscn.89z'),Path('fftgar.89z')]; [(lambda p,b: (b.__setitem__(0x49,3),p.write_bytes(b)))(p,bytearray(p.read_bytes())) for p in files]"
bundle: packed
	@mkdir -p x
	$(ROOT)/tools/bin/ti-group x/fftcampaign.89g fftstory.89z ../opening/fftopen.89z ../opening/ffocore.89y $(addprefix ../opening/data/,$(addsuffix .89y,ffov0 ffov1 ffov2 ffov3 ffoa ffoi ffost ffod ffs0)) fftscn.89z ffscore.89y fftgar.89z ffgcore.89y $(TI_FILES)

.PHONY: contour-assets
contour-assets:
	$(PY) ../tools/contour_assets.py --attach data/ffgv0.bin
pc test cycles ti packed regression-check visual-check: | contour-assets
fftgar_pc: ../config.inc ../contour.h ../contour_art.inc

../contour.h ../contour_art.inc: ../tools/contour_assets.py ../art.h ../fft.c ../config.inc
	$(PY) ../tools/contour_assets.py

fftintro_pc fftgar_pc fftgar_status_test: | contour-assets

data/ffgd.bin: ../tools/chapter_desc.py data/battle.h data/interface.h ../map.h data/actors.h
	$(PY) ../tools/chapter_desc.py campaign

data/ffgm.bin: ../tools/module.py gariland.c ../fft.c ../fft.h ../module.h $(wildcard ../*.inc)
	$(PY) ../tools/module.py $@ gariland.c 3

data/ffs1.bin: ../tools/scenes.py data/ffac.bin data/academy.h
	$(PY) ../tools/scenes.py
