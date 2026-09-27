# ti-cycles: Musashi (tools/musashi, MIT, not in git: git clone https://github.com/kstenerud/Musashi tools/musashi)
# built with our m68kconf.h (plain 68000 + instruction hook).
M := ../musashi
B := build
../bin/ti-cycles: ti-cycles.c m68kconf.h $(B)/m68kops.c
	cc -O2 -Wall -I$(B) -o $@ ti-cycles.c $(B)/m68kcpu.c $(B)/m68kops.c $(B)/m68kdasm.c $(B)/softfloat/softfloat.c -lm
$(B)/m68kops.c: m68kconf.h
	mkdir -p $(B) && cp -r $(M)/*.c $(M)/*.h $(M)/softfloat $(B)/ && cp m68kconf.h $(B)/
	cd $(B) && cc -o m68kmake m68kmake.c && ./m68kmake
clean:
	rm -rf $(B) ../bin/ti-cycles
