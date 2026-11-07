#!/bin/bash
# Resident per-function section sizes ("name bytes" lines) -> $1. Run before and after a change
# and diff the two files to see what the resident gained (static locals carry a numeric suffix).
set -e
[ -n "$1" ] || { echo "usage: $0 out.txt"; exit 2; }
OUT=$(realpath -m "$1"); HERE=$(cd "$(dirname "$0")/.." && pwd); ROOT=$(cd "$HERE/../.." && pwd)
T=$(mktemp -d "${TMPDIR:-/tmp}/fftresXXXXXX"); trap 'rm -rf "$T"' EXIT
export TIGCC=$ROOT/tools/gcc4ti-bin PATH=$ROOT/tools/gcc4ti-bin/bin:$PATH
cd "$HERE/campaign"
tigcc -Os -mregparm=5 -mno-bss -ffunction-sections -fdata-sections -fomit-frame-pointer \
    -I"$ROOT/tools/extgraph/lib" -c native_battle.c -o "$T/r.o" 2>/dev/null
m68k-coff-tigcc-objdump -h "$T/r.o" > "$T/rh.txt"; m68k-coff-tigcc-objdump -t "$T/r.o" > "$T/rt.txt"
python3 - "$T" "$OUT" <<'P'
import re,sys
T,out=sys.argv[1],sys.argv[2];secs={};names={}
for l in open(T+'/rh.txt'):
    m=re.match(r'\s*(\d+)\s+(\S+)\s+([0-9a-f]+)\s',l)
    if m:secs[int(m[1])+1]=int(m[3],16)
for l in open(T+'/rt.txt'):
    m=re.search(r'\(sec\s+(\d+)\).* 0x[0-9a-f]+ (\S+)$',l)
    if m and int(m[1]) in secs and not m[2].startswith('.'):names.setdefault(int(m[1]),m[2])
open(out,'w').write(''.join('%s %d\n'%(names.get(k,'?%d'%k),v) for k,v in secs.items()))
P
echo "wrote $OUT ($(wc -l < "$OUT") sections)"
