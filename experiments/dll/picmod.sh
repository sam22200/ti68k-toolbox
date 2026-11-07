#!/bin/sh
# Build picmod.89y: the module compiled position-independent, its sections laid end to end
# by the assembler (objcopy -O binary), refused if it needs any symbol from outside.
set -e
cd "$(dirname "$0")"
T=../../tools; export TIGCC=$T/gcc4ti-bin PATH=$T/gcc4ti-bin/bin:$PATH
tigcc -c -Os -mpcrel -mregparm=5 -fomit-frame-pointer -o picmod.o picmod.c
if m68k-coff-tigcc-objdump -t picmod.o | grep -q "(sec  0)"; then echo "undefined symbols:"; m68k-coff-tigcc-objdump -t picmod.o | grep "(sec  0)"; exit 1; fi
m68k-coff-tigcc-objcopy -O binary picmod.o picmod.img
entry=$(m68k-coff-tigcc-objdump -t picmod.o | awk '/ mod_entry$/{print $NF=="mod_entry" ? $(NF-1) : ""}')
python3 -c "import struct,sys; img=open('picmod.img','rb').read(); open('picmod.bin','wb').write(b'FMD1'+struct.pack('>HH',int(sys.argv[1],16),len(img))+img)" "$entry"
ttbin2oth -89 dat picmod.bin picmod >/dev/null
python3 -c "from pathlib import Path; p=Path('picmod.89y'); b=bytearray(p.read_bytes()); b[0x49]=3; p.write_bytes(b)"
rm -f picmod.o picmod.img picmod.bin
echo "picmod.89y: $(wc -c < picmod.89y) bytes, entry $entry"
