# Writes data.h (not kept: regenerate) and prints the expected sum. Build: python3 gen.py 30000; ti-cc -o bigkb big.c
# Data file for bigdat.c: see ti68k-c-patterns.md §1 (ttbin2oth -89 dat bigdt.bin bigdt).
# Writes data.h: N pseudo-random bytes (N = argv[1]) and prints their 16-bit sum.
import sys
n=int(sys.argv[1]); x=12345; b=[]
for i in range(n):
    x=(x*1103515245+12345)&0x7fffffff; b.append((x>>16)&255)
open('data.h','w').write('#define DATA_N %d\n#define DATA_SUM %du\nconst unsigned char data[DATA_N]={%s};\n'%(n,sum(b)&0xffff,','.join(map(str,b))))
print(sum(b)&0xffff)
