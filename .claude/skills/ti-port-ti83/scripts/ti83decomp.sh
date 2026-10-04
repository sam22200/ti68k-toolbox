#!/bin/sh
# Ghidra headless analysis of a TI-83/83+ program (Ghidra's own Z80 processor, no extension):
#   ti83decomp.sh OUTDIR [ADDR ...]
# OUTDIR is the folder ti83var.py wrote (body.bin, info.json). -> OUTDIR/decomp.c (pseudo-C
# per function), OUTDIR/disasm.s, OUTDIR/ghidra/ (the project: open it in the Ghidra GUI to
# rename, then run this again). ADDR (hex): extra entry points Ghidra cannot reach (the
# interrupt handler, jump-table targets, code called through a pointer).
set -e
OUT=$(realpath "$1"); shift
HERE=$(dirname "$(realpath "$0")")
G=$(dirname "$HERE")/../../../tools/ghidra
BASE=$(sed -n 's/.*"base": "\(.*\)".*/\1/p' "$OUT/info.json")
ENTRY=$(sed -n 's/.*"entry": "\(.*\)".*/\1/p' "$OUT/info.json")
rm -rf "$OUT/ghidra" && mkdir -p "$OUT/ghidra"
"$G/support/analyzeHeadless" "$OUT/ghidra" ti83 -import "$OUT/body.bin" -loader BinaryLoader \
  -loader-baseAddr "0x$BASE" -processor "z80:LE:16:default" -scriptPath "$HERE" \
  -preScript z80_entries.py "$ENTRY" "$@" -postScript z80_export.py "$OUT" > "$OUT/ghidra.log" 2>&1 \
  || { tail -20 "$OUT/ghidra.log"; exit 1; }
grep -c '^// ----' "$OUT/decomp.c" | sed 's/$/ functions/'
