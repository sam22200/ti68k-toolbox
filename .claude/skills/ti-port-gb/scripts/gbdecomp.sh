#!/bin/sh
# Ghidra (GhidraBoy) headless analysis of a Game Boy ROM:
#   gbdecomp.sh ROM.gb OUTDIR [TABLE:N | ADDR ...]
# -> OUTDIR/decomp.c (pseudo-C per function), OUTDIR/disasm.s, OUTDIR/ghidra/ (the project:
# open it in the Ghidra GUI to rename functions and variables, then run this again).
# TABLE:N / ADDR (hex): jump tables of N words and addresses whose targets Ghidra missed; they
# become functions (gb_funcs.py) before the export.
set -e
ROM=$(realpath "$1"); OUT=$(realpath -m "$2"); shift 2
HERE=$(dirname "$(realpath "$0")")
G=$(dirname "$HERE")/../../../tools/ghidra
mkdir -p "$OUT/ghidra"
PRE=""
[ $# -gt 0 ] && PRE="-preScript gb_funcs.py $*"
if [ -d "$OUT/ghidra/gb.rep" ]; then
  MODE="-process $(basename "$ROM")"
  [ -n "$PRE" ] || MODE="$MODE -noanalysis"
else
  MODE="-import $ROM -loader GameBoyLoader"
fi
# shellcheck disable=SC2086
"$G/support/analyzeHeadless" "$OUT/ghidra" gb $MODE -scriptPath "$HERE" $PRE \
  -postScript gb_export.py "$OUT" > "$OUT/ghidra.log" 2>&1 \
  || { tail -20 "$OUT/ghidra.log"; exit 1; }
grep -c '^// ----' "$OUT/decomp.c" | sed 's/$/ functions/'
