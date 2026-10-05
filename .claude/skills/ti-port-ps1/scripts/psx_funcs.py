# Ghidra headless pre-script (Jython): disassemble and make functions at the given hex addresses
# (the PS-X EXE entry point, plus any address Ghidra missed: jump-table targets, callbacks).
# A raw binary has no symbols, so without this the analysis has no starting point.  Run by psxdecomp.sh.
# @runtime Jython
from ghidra.app.cmd.disassemble import DisassembleCommand

for arg in getScriptArgs():
    a = toAddr(int(arg, 16))
    DisassembleCommand(a, None, True).applyTo(currentProgram, monitor)
    if getFunctionAt(a) is None:
        createFunction(a, None)
