# Mode 7 - Demo 2 (David Coz, 2005), decompiled and optimised (README.md, OPTIMISATIONS.md).
#   make             mode7.89z (Titanium: ti-run mode7.89z); the first make downloads the original
#                    from ticalc.org and extracts its data (src/data.c, src/trig.s, not in git)
#   make ORIGINAL=1  the faithful decompilation, without the optimisations
#   make bench       build/m7bench.89z for ti-cycles; tools/bench.py runs the scenarios
#   make disasm      mode7.s, the complete disassembly, checked to reassemble byte for byte
# Stack parameters as the original (TI_CC_PLAIN: no -mregparm), asm without % register prefixes.
T := ../../tools
PY := $(T)/pyenv/bin/python
SRC := src/main.c src/ship.c src/floor.c src/world3d.c src/data.c src/render.s src/trig.s
CC := TI_CC_PLAIN=1 ../$(T)/bin/ti-cc
FLAGS := -Wa,--register-prefix-optional $(if $(ORIGINAL),-DORIGINAL)

mode7.89z: $(SRC) src/mode7.h
	cd build && $(CC) -o mode7 $(addprefix ../,$(SRC)) $(FLAGS)
	mv build/mode7.89z .

bench: $(SRC) src/mode7.h
	cd build && $(CC) -o m7bench $(addprefix ../,$(SRC)) $(FLAGS) -DBENCH $(BENCH_FLAGS)

code.bin:
	mkdir -p build && tools/fetch.sh

src/data.c src/trig.s: code.bin tools/extract.py
	$(PY) tools/extract.py

romcalls.json: tools/romcalls.py
	$(PY) tools/romcalls.py

disasm: code.bin romcalls.json tools/m7dis.py tools/hints.json
	$(PY) tools/m7dis.py
	$(T)/gcc4ti-bin/bin/m68k-coff-tigcc-as -m68000 --register-prefix-optional -o build/mode7s.o mode7.s
	$(T)/gcc4ti-bin/bin/m68k-coff-tigcc-objcopy -O binary build/mode7s.o build/mode7s.bin
	$(PY) tools/cmp.py build/mode7s.bin

clean:
	rm -rf build/* mode7.89z

distclean: clean
	rm -rf mode7.zip mode7 modebin.bin code.bin romcalls.json mode7.s src/data.c src/trig.s preview

.PHONY: bench disasm clean distclean
