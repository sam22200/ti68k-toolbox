# Ghidra headless post-script (Jython): write the pseudo-C and the disassembly of every function
# of a Z80 program (TI-83, Game Boy...) into the directory given as the script argument.
# Jython, not Java: Ghidra 11.4's OSGi loader refuses Java scripts under JDK 25.  Run by ti83decomp.sh.
# @runtime Jython
import os
from ghidra.app.decompiler import DecompInterface

out = getScriptArgs()[0]
dec = DecompInterface()
dec.openProgram(currentProgram)
listing = currentProgram.getListing()
c = open(os.path.join(out, "decomp.c"), "w")
for f in listing.getFunctions(True):
    r = dec.decompileFunction(f, 60, monitor)
    c.write("// ---- %s @ %s\n" % (f.getName(), f.getEntryPoint()))
    c.write(r.getDecompiledFunction().getC() if r.decompileCompleted()
            else "// decompile failed: %s\n" % r.getErrorMessage())
c.close()
s = open(os.path.join(out, "disasm.s"), "w")
for i in listing.getInstructions(True):
    a = i.getAddress()
    f = listing.getFunctionAt(a)
    if f is not None:
        s.write("\n%s:\n" % f.getName())
    else:
        sym = getSymbolAt(a)
        if sym is not None:
            s.write("%s:\n" % sym.getName())
    s.write("  %s  %s\n" % (a, i))
s.close()
