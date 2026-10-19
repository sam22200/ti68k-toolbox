#!/usr/bin/env python3
"""PC chapter supervisor. Exit/back never starts another chapter."""
import subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
for program,cwd in ((HERE/'fftintro_pc',HERE/'../opening/data'),
                    (HERE/'fftacad_pc',HERE/'data'),(HERE/'fftgar_pc',HERE/'data')):
    result=subprocess.run([str(program)] ,cwd=cwd).returncode
    if result!=23:break
