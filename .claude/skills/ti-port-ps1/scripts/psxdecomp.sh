#!/bin/sh
# Ghidra headless analysis of a PlayStation executable (MIPS R3000 little endian, built in):
#   psxdecomp.sh INFO.json OUTDIR [ADDR ...]     the boot executable (psxiso.py's info.json:
#                                                text file, load address t_addr, entry pc0)
#   psxdecomp.sh RAM.bin OUTDIR ADDR [ADDR ...]   a 2 MB main-RAM dump (psxrun.py --dump) at
#                                                80000000: code loaded at run time (overlays)
#                                                included; ADDR = function starts to begin from,
#                                                or LO-HI: every prologue (addiu sp,sp,-N) in
#                                                that range (find the code range by prologue
#                                                density per 64 KB first)
# -> OUTDIR/decomp.c (pseudo-C per function), OUTDIR/disasm.s, OUTDIR/ghidra/ (the project:
# open it in the Ghidra GUI to rename, then run this again: no reanalysis, just the export).
# ADDR (hex): extra function starts Ghidra missed (jump-table targets, callbacks).
set -e
INFO=$(realpath "$1"); OUT=$(realpath -m "$2"); shift 2
HERE=$(dirname "$(realpath "$0")")
G=$(dirname "$HERE")/../../../tools/ghidra
PY=$(dirname "$HERE")/../../../tools/pyenv/bin/python
case "$INFO" in
  *.json) eval "$("$PY" -c "import json,os;e=json.load(open('$INFO'))['exe'];print('BIN=%s BASE=%s PC=%s'%(os.path.realpath(e['text_file']),e['t_addr'],e['pc0']))")" ;;
  *) BIN=$INFO BASE=80000000
     ARGS=$("$PY" - "$INFO" "$@" <<'PYEOF'
import struct, sys
ram = open(sys.argv[1], 'rb').read()
out = []
for a in sys.argv[2:]:
    if '-' in a:
        lo, hi = (int(x, 16) & 0x1fffff for x in a.split('-'))
        for i in range(lo & ~3, hi, 4):
            w = struct.unpack_from('<I', ram, i)[0]
            if w >> 16 == 0x27bd and w & 0x8000:
                out.append('%08x' % (0x80000000 + i))
    else:
        out.append(a)
print(' '.join(out))
PYEOF
)
     set -- $ARGS; PC=$1; shift ;;
esac
mkdir -p "$OUT/ghidra"
if [ -d "$OUT/ghidra/psx.rep" ]; then
  MODE="-process $(basename "$BIN")"
  [ $# -gt 0 ] || MODE="$MODE -noanalysis"
else
  MODE="-import $BIN -loader BinaryLoader -loader-baseAddr 0x$BASE -processor MIPS:LE:32:default"
fi
# shellcheck disable=SC2086
"$G/support/analyzeHeadless" "$OUT/ghidra" psx $MODE -scriptPath "$HERE" \
  -preScript psx_funcs.py "$PC" "$@" -postScript psx_export.py "$OUT" \
  > "$OUT/ghidra.log" 2>&1 || { tail -20 "$OUT/ghidra.log"; exit 1; }
grep -c '^// ----' "$OUT/decomp.c" | sed 's/$/ functions/'
grep -c 'decompile failed' "$OUT/decomp.c" | sed 's/$/ failed/'
