ROOT := ../..
CORE := $(ROOT)/sources/gba_core
PY := $(ROOT)/tools/pyenv/bin/python
CORE_REV := 26b7884bc25a5933960f3cdcd98bac1ae14d42e2

.PHONY: core check
core:
	@test -d $(CORE)/.git || { mkdir -p $(CORE); git -C $(CORE) init; git -C $(CORE) remote add origin https://github.com/mgba-emu/mgba.git; git -C $(CORE) fetch --depth 1 origin $(CORE_REV); git -C $(CORE) checkout --detach FETCH_HEAD; }
	@test "$$(git -C $(CORE) rev-parse HEAD)" = "$(CORE_REV)" || { echo 'unexpected core revision; use a separate clean checkout'; exit 1; }
	cmake -S $(CORE) -B $(CORE)/build -DBUILD_LIBRETRO=ON -DBUILD_QT=OFF -DBUILD_SDL=OFF -DBUILD_GL=OFF -DBUILD_GLES2=OFF -DBUILD_GLES3=OFF -DM_CORE_GB=OFF -DUSE_FFMPEG=OFF -DUSE_LIBZIP=OFF -DUSE_MINIZIP=OFF -DUSE_PNG=OFF -DUSE_ZLIB=OFF -DUSE_LZMA=OFF -DUSE_SQLITE3=OFF -DUSE_ELF=OFF -DENABLE_SCRIPTING=OFF -DUSE_DEBUGGERS=OFF -DUSE_DISCORD_RPC=OFF
	cmake --build $(CORE)/build --target mgba_libretro -j4

check:
	$(PY) check.py $(if $(ROM),"$(ROM)")
