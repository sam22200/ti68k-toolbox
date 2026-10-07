ROOT := ../..
CORE := $(ROOT)/sources/snes_core
PY := $(ROOT)/tools/pyenv/bin/python
CORE_REV := fae2fea08f74180759ef540ee94259213f503480

.PHONY: core check
core:
	@test -d $(CORE)/.git || { mkdir -p $(CORE); git -C $(CORE) init; git -C $(CORE) remote add origin https://github.com/libretro/snes9x.git; git -C $(CORE) fetch --depth 1 origin $(CORE_REV); git -C $(CORE) checkout --detach FETCH_HEAD; }
	@test "$$(git -C $(CORE) rev-parse HEAD)" = "$(CORE_REV)" || { echo 'unexpected core revision; use a separate clean checkout'; exit 1; }
	$(PY) expose_ppu.py
	$(MAKE) -C $(CORE)/libretro -j4

check:
	$(PY) check_yoshi.py
