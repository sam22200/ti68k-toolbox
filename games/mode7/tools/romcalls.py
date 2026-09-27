#!/usr/bin/env python3
"""ROM call number -> name, from GCC4TI's headers (romcalls.json, used by tools/m7dis.py)."""
import re, glob, json, os
inc = os.path.join(os.path.dirname(__file__), '../../../tools/gcc4ti-bin/include')
names = {}
for f in glob.glob(inc + '/c/*.h'):
    for m in re.finditer(r'#define\s+(\w+)\s+_rom_call(?:_addr)?(?:_hack)?(?:_attr)?\s*\((?:[^()]|\([^()]*\))*?,\s*([0-9A-Fa-f]+)\s*\)',
                         open(f, errors='ignore').read()):
        names.setdefault(int(m.group(2), 16), m.group(1))
for l in open(inc + '/asm/os.h', errors='ignore'):
    m = re.match(r'(\w+)\s+equ\s+\$([0-9a-fA-F]+)', l)
    if m: names.setdefault(int(m.group(2), 16), m.group(1))
json.dump({str(k): v for k, v in names.items()}, open('romcalls.json', 'w'))
print(len(names), 'ROM calls')
