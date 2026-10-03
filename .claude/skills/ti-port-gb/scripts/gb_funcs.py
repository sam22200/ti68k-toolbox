# Ghidra pre-script (Jython): disassemble and create the functions Ghidra's analysis misses,
# the targets of jump tables (`ld hl,table; add a; ... jp hl`) and of addresses found by tracing.
# Arguments: TABLE:N (a table of N little-endian words) or ADDR, hex.  Run by gbdecomp.sh.
# @runtime Jython
mem = currentProgram.getMemory()

def word(a):
    return (mem.getByte(toAddr(a)) & 0xff) | ((mem.getByte(toAddr(a + 1)) & 0xff) << 8)

targets = set()
for arg in getScriptArgs():
    if ':' in arg:
        t, n = arg.split(':')
        targets.update(w for w in (word(int(t, 16) + 2 * i) for i in range(int(n))) if w)
    else:
        targets.add(int(arg, 16))
for t in sorted(targets):
    disassemble(toAddr(t))
    if getFunctionAt(toAddr(t)) is None:
        createFunction(toAddr(t), None)
    print("gb_funcs: function at %04x" % t)
