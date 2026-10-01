#!/bin/sh
# Check assets/p8num.h against z8lua (PICO-8 numbers): the same operations on the same bit
# patterns must print the same bits. Usage: test_p8num.sh (from anywhere). Exit 1 on a diff.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../../.." && pwd)
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
VALS="0x00000000 0x00000001 0xffffffff 0x00010000 0xffff0000 0x00008000 0xffff8000 \
0x0000b504 0x00035c2a 0xfffca3d6 0x7fffffff 0x80000000 0x80000001 0x00050000 0xfffb0000 \
0x00000003 0xfffffffd 0x12345678 0xedcba988 0x00020000 0x7fff0000 0x0000ffff"
{
  echo '#include <stdio.h>'
  echo '#include "../../runtime/core/rt.h"' | sed "s#../../#$ROOT/#"
  echo "#include \"$ROOT/.claude/skills/ti-port-pico8/assets/p8num.h\""
  echo 'u32 p8_ra, p8_rb;'
  echo 'static void p(fix v) { char b[16]; p8_hex(b, v); puts(b); }'
  echo 'int main(void) { static const u32 v[] = {'
  for x in $VALS; do echo "$x,"; done
  echo '}; int n = sizeof v / sizeof v[0], i, j;'
  echo 'for (i = 0; i < n; i++) { fix a = (fix)v[i];'
  echo '  p(p8_flr(a)); p(p8_ceil(a)); p(p8_sgn(a)); p(p8_abs(a)); p(p8_div2k(a, 1)); p(p8_div2k(a, 3));'
  echo '  p(p8_mod2k(a, 3)); p(p8_shr(a, 4)); p(p8_lshr(a, 4)); p(p8_shl(a, 4));'
  echo '  for (j = 0; j < n; j++) { fix b = (fix)v[j]; p(p8_mul(a, b)); p(p8_div(a, b)); p(p8_mod(a, b)); p(p8_mid(a, b, FIX(1))); } }'
  echo 'p8_srand(0); for (i = 0; i < 50; i++) p(p8_rnd(FIX(128))); for (i = 0; i < 20; i++) p(p8_rnd(FIXB(0x3333))); for (i = 0; i < 20; i++) p(p8_rnd(FIX(1))); for (i = 0; i < 20; i++) p(p8_rnd(FIXB(0x38000)));'
  echo 'p8_srand(FIX(42)); for (i = 0; i < 20; i++) p(p8_rndi(5)); p8_srand(FIXB(0x12345678)); for (i = 0; i < 20; i++) p(p8_rndi(10000)); return 0; }'
} > "$T/t.c"
{
  cat "$HERE/p8shim.lua"
  echo 'local v = {'
  for x in $VALS; do echo "$x,"; done | sed 's/0x\(....\)\(....\),/0x\1.\2,/'
  echo '}'
  echo 'local function p(x) __out(tostr(x, true)) end'
  echo 'for a in all(v) do'
  echo '  p(flr(a)) p(ceil(a)) p(sgn(a)) p(abs(a)) p(a / 2) p(a / 8)'
  echo '  p(a % 8) p(shr(a, 4)) p(lshr(a, 4)) p(shl(a, 4))'
  echo '  for b in all(v) do p(a * b) p(a / b) p(a % b) p(mid(a, b, 1)) end end'
  echo 'srand(0) for i = 1, 50 do p(rnd(128)) end for i = 1, 20 do p(rnd(0x0.3333)) end for i = 1, 20 do p(rnd()) end for i = 1, 20 do p(rnd(3.5)) end'
  echo 'srand(42) for i = 1, 20 do p(rnd(5)) end srand(0x1234.5678) for i = 1, 20 do p(rnd(10000)) end'
} > "$T/t.lua"
cc -O1 -Wall -o "$T/t" "$T/t.c"
"$T/t" > "$T/c.txt"
"$ROOT/tools/z8lua/z8lua" "$T/t.lua" > "$T/lua.txt"
if diff "$T/c.txt" "$T/lua.txt" > "$T/diff.txt"; then
  echo "p8num.h = z8lua: $(wc -l < "$T/c.txt") values"
else
  head -20 "$T/diff.txt"; echo "p8num.h differs from z8lua"; exit 1
fi
