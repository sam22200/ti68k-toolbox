#!/bin/bash
# Production headroom of the campaign battle core: TTPack input (the .89z's varsize) against the
# 64936-byte limit. The program is the resident only: the module zone (data/modules.h MOD_ZONE,
# the largest image, printed for reference) is a heap block since HW3Patch is required. Rebuild
# the banks first: (cd campaign && make data/ffcj.89y data/ffbm.89y data/ffbn.89y).
# Keep about 250 bytes: some 68000 test binaries (area-impact) are larger than production.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd); ROOT=$(cd "$HERE/../.." && pwd)
T=$(mktemp -d "${TMPDIR:-/tmp}/fftheadroomXXXXXX")   # no dot: tigcc cuts -o at the first one
trap 'rm -rf "$T"' EXIT

cd "$HERE/campaign"
"$ROOT/tools/bin/ti-cc" -o "$T/fftbat" native_battle.c "$ROOT/runtime/core/rt_core.c" "$ROOT/lib/unpack68k.s" \
    "$ROOT/tools/extgraph/lib/tilemap.a" >/dev/null 2>&1 || { echo "BUILD FAIL"; exit 1; }
python3 -c "
import struct;b=open('$T/fftbat.89z','rb').read();n=struct.unpack('>H',b[0x56:0x58])[0]
print('input',n,'headroom',64936-n,'zone',open('data/modules.h').read().split('MOD_ZONE')[1].split()[0])"
