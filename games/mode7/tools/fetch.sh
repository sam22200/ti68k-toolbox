#!/bin/sh
# Downloads the original demo from ticalc.org and unpacks its program (ExePack/ttpack PPG):
#   mode7.zip, mode7/ (the original files), modebin.bin (the program variable), code.bin (the
#   program without its size word: input of tools/m7dis.py and tools/extract.py).
set -e
cd "$(dirname "$0")/.."
T=../../tools/gcc4ti-bin/bin
[ -f mode7.zip ] || curl -fsSL -o mode7.zip https://www.ticalc.org/pub/89/asm/games/sports/mode7.zip
unzip -oq mode7.zip
python3 - <<'P'
d = open('mode7/mode7.89y', 'rb').read()             # TI header, then the size word
n = (d[0x56] << 8) | d[0x57]
open('build/modebin.ttp', 'wb').write(d[0x58:0x58 + n - 6])   # without the "ppg" tag
P
"$T/ttunpack" build/modebin.ttp modebin.bin >/dev/null 2>&1 || true   # ttunpack 1.8 double-frees at exit
python3 -c "d = open('modebin.bin', 'rb').read(); assert len(d) == 29727, len(d); open('code.bin', 'wb').write(d[2:])"
echo "code.bin: $(wc -c < code.bin) bytes"
