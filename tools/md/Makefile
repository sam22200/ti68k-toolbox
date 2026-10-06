# Third-party core and all generated reference data stay under ignored sources/.
ROOT := ../..
CORE := $(ROOT)/sources/md_core
PY := $(ROOT)/tools/pyenv/bin/python
CORE_REV := 58c341487e5bfcf979ea68413c7987633adb0c56

.PHONY: core check
core:
	@test -d $(CORE)/.git || { mkdir -p $(CORE); git -C $(CORE) init; git -C $(CORE) remote add origin https://github.com/libretro/Genesis-Plus-GX.git; git -C $(CORE) fetch --depth 1 origin $(CORE_REV); git -C $(CORE) checkout --detach FETCH_HEAD; }
	@test "$$(git -C $(CORE) rev-parse HEAD)" = "$(CORE_REV)" || { echo 'unexpected core revision; use the pinned revision in a separate clean checkout'; exit 1; }
	$(PY) expose_vdp.py
	$(MAKE) -C $(CORE) -f Makefile.libretro -j4

check:
	$(PY) check_sonic.py
