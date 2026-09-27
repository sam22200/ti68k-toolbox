# Installing the toolchain

What the repository needs beside itself, and how to get each piece on **Ubuntu / Linux** (the
reference setup: Ubuntu 24.04, Docker 29, no passwordless sudo needed after the packages) and on
**macOS** (through a Linux container). Third-party and copyrighted pieces stay out of git
(`.gitignore`); they all live under `tools/`.

The development flow (`CLAUDE.md`) runs almost everything without UI: unit tests and headless PC
runs, then the calculator binaries under `ti-cycles`. The TI emulator comes last, once per
milestone; it is the only part that needs an X11 display.

| Piece | Where | From | Used for |
|---|---|---|---|
| GCC4TI 0.96 beta 11 (GCC 4.1.2) | `tools/gcc4ti-bin/` | built by `tools/build-gcc4ti.sh` in Docker (Debian Jessie) from `tools/gcc4ti/` | `ti-cc`: C and asm for the calculator |
| ExtGraph 2 (LGPL) | `tools/extgraph/` | github.com/debrouxl/ExtGraph (prebuilt `lib/`) | sprites, grayscale, TileMap |
| Python venv | `tools/pyenv/` | `python3 -m venv` + pip | asset pipelines, disassembler |
| SDL2 headers | `tools/sdl2/` | `libsdl2-dev` | PC window of runtime games |
| ZX0 packer | `tools/bin/zx0`, `dzx0` | github.com/einar-saukas/ZX0 | data compression |
| Musashi (MIT) → `ti-cycles` | `tools/musashi/`, `tools/bin/ti-cycles` | github.com/kstenerud/Musashi | cycle counts and screens of TI binaries, headless |
| TI OS images | `tools/rom/` | you: see step 8 | the emulator, the AMS fonts of the PC build |
| TiEmu 3.04 | Docker image `tiemu-jammy` | `tools/Dockerfile.tiemu` (Ubuntu 22.04 package + `tiemu-keyfix.c`) | the emulator, last step |
| TiEmu profiles | `tools/tiemu/{89t,89,89u}/` | created once, step 9 | clean saved state for `ti-run` |

## Ubuntu / Linux

1. **Packages**:
   ```sh
   sudo apt install git curl unzip build-essential python3 python3-venv libsdl2-dev \
                    docker.io xdotool imagemagick
   sudo usermod -aG docker $USER      # then log in again; Docker Engine from docker.com works too
   ```
   `xdotool` and `imagemagick` are only needed by the emulator scripts (`ti-key`, `ti-shot`).
2. **The repository**: `git clone https://github.com/sam22200/ti68k-toolbox ti && cd ti`.
3. **GCC4TI** (a few minutes, in a Debian Jessie container: GCC 4.1.2 does not build with a modern
   host compiler):
   ```sh
   git clone https://github.com/debrouxl/gcc4ti tools/gcc4ti
   mkdir -p tools/tarballs
   cp tools/gcc4ti/pool/b/binutils-2.16.1.tar.bz2 tools/gcc4ti/pool/g/gcc-core-4.1.2.tar.bz2 tools/tarballs/
   tools/build-gcc4ti.sh              # → tools/gcc4ti-bin (tigcc, ttunpack, headers, docs)
   ```
4. **ExtGraph**: `git clone https://github.com/debrouxl/ExtGraph tools/extgraph`.
5. **Python**:
   ```sh
   python3 -m venv tools/pyenv
   tools/pyenv/bin/pip install numpy scipy pillow capstone
   ```
6. **SDL2** for the runtime's PC build (`runtime/rt.mk` reads `tools/sdl2/include` and `lib`):
   ```sh
   mkdir -p tools/sdl2 && ln -s /usr/include tools/sdl2/include && ln -s /usr/lib/x86_64-linux-gnu tools/sdl2/lib
   ```
   Without sudo: `apt-get download libsdl2-dev && dpkg -x libsdl2-dev_*.deb /tmp/sdl`, copy its
   `usr/include` to `tools/sdl2/include`, and link `tools/sdl2/lib/libSDL2.so` to the system's
   `libSDL2-2.0.so.0`.
7. **ZX0, Musashi and `ti-cycles`**:
   ```sh
   git clone https://github.com/einar-saukas/ZX0 /tmp/zx0
   gcc -O2 -o tools/bin/zx0 /tmp/zx0/src/zx0.c /tmp/zx0/src/optimize.c /tmp/zx0/src/compress.c /tmp/zx0/src/memory.c
   gcc -O2 -o tools/bin/dzx0 /tmp/zx0/src/dzx0.c
   git clone https://github.com/kstenerud/Musashi tools/musashi
   make -C tools/m68kbench            # → tools/bin/ti-cycles
   ```
8. **TI OS images**, into `tools/rom/` with these names (they are TI's software, not in the
   repository: download them from education.ti.com, TI's OS pages, or the TI-Planet archive listed
   in `docs/resources.md`):
   - `TI89Titanium_OS.89u`: TI-89 Titanium AMS 3.10 (the default profile);
   - `TI89_AMS209.89u`: TI-89 AMS 2.09, and `TI89_AMS209_amspatch.89u`: the same patched with
     AMSpatch (github.com/debrouxl/tiosmod), for the HW2 profiles;
   - optional: HW3Patch into `tools/patches/` (tigen.org/kevin.kofler/ti89prog.htm).
   `make` in a runtime game extracts the AMS fonts from the Titanium OS (`runtime/platform-sw/amsfont.h`).
9. **TiEmu profile**, once per model (the saved state that every `ti-run` restarts from):
   ```sh
   mkdir -p tools/tiemu/89t
   export PATH=$PWD/tools/bin:$PATH
   TI_CALC=89t ti-emu start           # builds the Docker image, opens TiEmu with a new profile
   ```
   In the window TiEmu asks for a ROM: give it `tools/rom/TI89Titanium_OS.89u` (it converts it to
   an image inside the profile). Let the calculator boot, turn the Apps Desktop off (MODE, F3,
   Apps Desktop OFF), leave an empty HOME screen, then `ti-emu save`. The profile stores absolute
   paths: do not move `tools/tiemu/` afterwards. `TI_CALC=89` (AMSpatch image) and `89u` (the
   official one, no saved state) are made the same way.
10. **Check**:
    ```sh
    export PATH=$PWD/tools/bin:$PATH
    ti-cycles tools/m68kbench/test/cyctest.89z   # after: TI_CC_PLAIN=1 ti-cc -DUSE_TI89 -o tools/m68kbench/test/cyctest tools/m68kbench/test/cyctest.c
    make -C runtime/demo test                     # unit tests, no window
    make -C games/mode7 && make -C games/mode7 bench && (cd games/mode7 && ../../tools/pyenv/bin/python tools/bench.py --ref base)
    ti-run runtime/demo/demo.89z                  # the emulator, last (make -C runtime/demo ti first)
    ```

## macOS

The toolchain is Linux software: GCC4TI is an x86-64 Linux build, and TiEmu comes from Ubuntu.
On a Mac, everything runs in Docker. The development shell `tools/bin/ti-shell`
(`tools/Dockerfile.dev`: Ubuntu 22.04 with the build tools, Python and SDL2) mounts the
repository at the same path. That shell was tested on Linux, not on a Mac.

1. **Docker Desktop**. On Apple Silicon, enable "Use Rosetta for x86_64/amd64 emulation" in the
   settings, then `export DOCKER_DEFAULT_PLATFORM=linux/amd64` (the GCC4TI build image is
   amd64-only). Also `git`, from Xcode's command line tools.
2. **The repository and the sources**: steps 2, 3 (the `git clone` and `cp` lines), 4, and the two
   `git clone` of step 7, as on Linux. Then build GCC4TI from the Mac: `tools/build-gcc4ti.sh`.
3. **Everything else, inside the Linux shell** (`tools/bin/ti-shell`, or
   `tools/bin/ti-shell <command>` for one command):
   ```sh
   python3 -m venv tools/pyenv && tools/pyenv/bin/pip install numpy scipy pillow capstone
   mkdir -p tools/sdl2 && ln -s /usr/include tools/sdl2/include && ln -s /usr/lib/x86_64-linux-gnu tools/sdl2/lib
   gcc -O2 -o tools/bin/zx0 ...        # step 7, same commands (clone ZX0 inside the repo, e.g. tools/zx0-src)
   make -C tools/m68kbench
   make -C runtime/demo test
   ```
   The venv, `ti-cycles` and `zx0` are Linux binaries: use them from `ti-shell`, not from the Mac.
4. **TI OS images**: step 8, the same files.
5. **Without the emulator**, the whole flow works: unit tests, headless PC runs (`--headless ... --shot`), and `ti-cycles` for the calculator binaries.
6. **The emulator on macOS is not ready**. `ti-emu` starts TiEmu with Linux assumptions:
   - `--network host`, `DISPLAY` and `XAUTHORITY` for the host's X server;
   - `tiemu-keyfix.c`, which translates Linux evdev keycodes;
   - `xdotool` / `import` on the host for keys and screenshots.

   A port would need XQuartz (with "Allow connections from network clients"), `xhost +localhost`, `DISPLAY=host.docker.internal:0` instead of the host network, and `brew install xdotool imagemagick`; the keycode table of `tiemu-keyfix.c` would also have to be checked under XQuartz. Until then, run the final TI check on a Linux machine, or on a real calculator.
