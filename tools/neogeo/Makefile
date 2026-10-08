ROOT := ../..
PY := $(ROOT)/tools/pyenv/bin/python
CORE := $(ROOT)/sources/neogeo_core
CORE_REV := a49cfac4b97cc62d0196c1d0cde8f5b14fde662c
CORE_FLAGS := SUBSET=neogeo USE_SPEEDHACKS=0 FASTMATH=0
ROM := $(ROOT)/roms/neogeo/wjammers
OUT := $(ROOT)/sources/windjammers_neogeo
BIOS := $(ROOT)/roms/neogeo/neogeo.zip

.PHONY: audit prepare check core core-check test reference gameplay rules
core:
	@test -d $(CORE)/.git || { mkdir -p $(CORE); git -C $(CORE) init; git -C $(CORE) remote add origin https://github.com/libretro/FBNeo.git; }
	@test "$$(git -C $(CORE) rev-parse HEAD 2>/dev/null)" = "$(CORE_REV)" || { test -z "$$(git -C $(CORE) status --porcelain)" || { echo 'local core changes: use a separate clean checkout'; exit 1; }; git -C $(CORE) fetch --depth 1 origin $(CORE_REV); git -C $(CORE) checkout --detach FETCH_HEAD; }
	$(PY) expose_video.py
	@test "$$(cat $(CORE)/.ti-reference-flags 2>/dev/null)" = "$(CORE_FLAGS)" || $(MAKE) -C $(CORE)/src/burner/libretro $(CORE_FLAGS) clean
	$(MAKE) -C $(CORE)/src/burner/libretro $(CORE_FLAGS) generate-files
	$(MAKE) -C $(CORE)/src/burner/libretro $(CORE_FLAGS) -j4
	@echo '$(CORE_FLAGS)' > $(CORE)/.ti-reference-flags

audit:
	$(PY) romset.py "$(ROM)"

prepare:
	$(PY) romset.py "$(ROM)" --manifest "$(OUT)/manifest.json" \
		--program "$(OUT)/program.be.bin" --pack "$(OUT)/wjammers.zip"

test:
	$(PY) -m unittest -v test_runner

check: test
	$(PY) check_windjammers.py

reference: prepare
	$(PY) check_reference.py --bios "$(BIOS)" --rom "$(OUT)/wjammers.zip" \
		--report "$(OUT)/reference.json"

core-check: prepare
	$(PY) check_core.py

gameplay: prepare
	$(PY) check_gameplay.py --bios "$(BIOS)" --rom "$(OUT)/wjammers.zip" --out "$(OUT)/gameplay"

rules: prepare
	$(PY) check_rules.py --bios "$(BIOS)" --rom "$(OUT)/wjammers.zip" \
		--program "$(OUT)/program.be.bin" --out "$(OUT)/rules"
