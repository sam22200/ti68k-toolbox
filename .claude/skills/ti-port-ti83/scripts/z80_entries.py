# Ghidra pre-script (Jython): disassemble and make functions at the given hex addresses (the
# program entry, then the entries Ghidra cannot reach by itself).  Run by ti83decomp.sh.
# @runtime Jython
for a in getScriptArgs():
    disassemble(toAddr(int(a, 16)))
    if getFunctionAt(toAddr(int(a, 16))) is None:
        createFunction(toAddr(int(a, 16)), None)
