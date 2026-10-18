#!/usr/bin/env python3
"""Compare the original and coarse transition paths on identical camera/combat cases."""
import json
import csv
from pathlib import Path
import shutil
import subprocess

import camera_audit as audit
from ultra_assets import prepare

HERE = Path(__file__).resolve().parents[1]
OUT = HERE/'x/ultra-transition'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    report = prepare(OUT/'fftlod.bin')
    data = OUT/'data'; data.mkdir(exist_ok=True)
    for path in (HERE/'x/tiemu-camera/data').glob('*'):
        if path.suffix in ('.bin','.89y'): shutil.copy2(path,data/path.name)
    shutil.copy2(OUT/'fftlod.bin',data/'fftlod.bin')
    subprocess.run([str(audit.ROOT/'tools/gcc4ti-bin/bin/ttbin2oth'),'-89','dat','fftlod.bin','fftlod'],cwd=data,check=True)
    bank = data/'fftlod.89y'; raw = bytearray(bank.read_bytes()); raw[0x3c+13]=3; bank.write_bytes(raw)
    audit.DATA = data
    for name, flags in [('reference',[]),('ultra',['-DFFT_ULTRA_TRANSITION'])]:
        audit.OUT = OUT/name; audit.FLAGS = ['-DFFT_EXPLORATION','-DFFT_FIXED','-DFFT_CINEMA', '-DFFT_CAMERA_MOTION', '-DFFT_TURN_HOLD','-DFFT_CAMERA_AUDIT',*flags]
        audit.main()
    original = json.loads((OUT/'reference/headless.json').read_text())
    coarse = json.loads((OUT/'ultra/headless.json').read_text())
    a=list(csv.reader((OUT/'reference/pc.csv').read_text().splitlines()))
    b=list(csv.reader((OUT/'ultra/pc.csv').read_text().splitlines()))
    assert len(a)==len(b)==468
    same_pixels=0
    for left,right in zip(a,b):
        assert left[:3]==right[:3] and left[4:]==right[4:], (left,right)
        c,frame,phase=map(int,left[:3])
        settled=c<2 or c>=9 or (2<=c<=6 and frame==18) or (7<=c<=8 and phase in [0,2])
        if settled: assert left[3]==right[3], (left,right); same_pixels+=1
    rows = []
    for a,b in zip(original['cases'],coarse['cases']):
        assert a['frames']==b['frames']
        rows.append(dict(name=a['name'],frames=a['frames'],reference_cycles=a['total_cycles'],
            ultra_cycles=b['total_cycles'],speedup=a['total_cycles']/b['total_cycles']))
    for c in [2,3,4,5,6,7,8]: assert rows[c]['speedup']>2, rows[c]
    capture = OUT/'pc-frames'; capture.mkdir(exist_ok=True)
    subprocess.run([str(OUT/'ultra/oracle'),str(capture)],cwd=data,stdout=subprocess.DEVNULL,check=True)
    result = dict(proxy=report,matched_frames_each=468,identical_state_and_rng_frames=468,
        identical_settled_framebuffers=same_pixels,cases=rows,
        scope='Different transient visual geometry, identical trajectory, frame count and combat checks. Native CPU counts; no TI emulator speed claimed here.')
    (OUT/'comparison.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__': main()
