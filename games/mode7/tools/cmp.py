#!/usr/bin/env python3
"""Compares a reassembled mode7.s (objcopy -O binary) with the original code.bin. In the
original the relocated longs hold 0 and the AMS table gives their target; the rebuild holds the
target: patch it in before comparing."""
import sys
d = bytearray(open('code.bin', 'rb').read()); r = open(sys.argv[1], 'rb').read()
i = len(d) - 1
while True:                                   # relocation table: (target, location) pairs from the end
    i -= 2; loc = (d[i] << 8) | d[i + 1]
    if loc == 0: break
    i -= 2; d[loc:loc + 4] = ((d[i] << 8) | d[i + 1]).to_bytes(4, 'big')
diff = [k for k in range(min(len(d), len(r))) if d[k] != r[k]]
print('mode7.s: %d bytes reassembled, %d bytes original, %d differences' % (len(r), len(d), len(diff)))
sys.exit(1 if diff or len(d) != len(r) else 0)
