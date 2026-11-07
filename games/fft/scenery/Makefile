ROOT := ../../..
PY := $(ROOT)/tools/pyenv/bin/python
.NOTPARALLEL:
.PHONY: assets test native benchmark blocks
assets:
	$(PY) build.py
x/test: bank.c bank.h test.c
	mkdir -p x
	$(CC) -std=gnu99 -O1 -g -Wall -Wextra -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer bank.c test.c -o $@
test: x/test
	./x/test
native:
	$(ROOT)/tools/bin/ti-cc -o x/sbcache -DRT_CYCLES bench.c bank.c $(ROOT)/runtime/core/rt_core.c $(ROOT)/runtime/platform-ti68k/rt_ti.c $(ROOT)/lib/unpack68k.s $(ROOT)/tools/extgraph/lib/tilemap.a
benchmark: test native
	$(PY) measure.py
blocks:
	$(PY) blocks.py
.PHONY: faces proj proj-test proj-bench
faces:
	$(PY) faces.py
proj:
	$(PY) proj.py
x/test_proj: proj.c proj.h test_proj.c
	$(CC) -std=gnu99 -O1 -g -Wall -Wextra -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer proj.c test_proj.c -o $@
proj-test: x/test_proj
	./x/test_proj
proj-bench: proj-test
	sh bench_proj.sh
