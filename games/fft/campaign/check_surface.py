#!/usr/bin/env python3
"""Original MIPS oracle -> shared C on PC, sanitizers and real68000, no UI."""
import json,re
from pathlib import Path
from check import HERE,ROOT,run

def main():
    out=HERE/'x/core-terrain';data=out/'oracle';data.mkdir(parents=True,exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python',HERE.parent/'tools/surface_data.py','--oracle',data])
    source=HERE/'check_surface.c'
    common=[source,ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'surface',*common])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-o',out/'surface-san',*common])
    pc=run([out/'surface'],data)
    sanitized=run(['env','ASAN_OPTIONS=detect_leaks=0',out/'surface-san'],data);assert sanitized==pc
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'surfc',source,ROOT/'runtime/core/rt_core.c',ROOT/'tools/extgraph/lib/tilemap.a'])
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--max',10000]
    for p in sorted(data.glob('fgq*.bin')):
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',p.name,p.stem],data)
        cmd+=['--file',p.with_suffix('.89y')]
    native=run([*cmd,out/'surfc.89z']);match=re.search(r'surface (\w+) failures 0 checks (\d+)',pc);assert match,pc
    values=[int(x,16)for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[2])],native[-2000:]
    assert '_main returned'in native
    (out/'native.log').write_text(native)
    (out/'results.json').write_text(json.dumps(dict(checks=int(match[2]),hash=match[1],original=json.loads((data/'source.json').read_text()),sanitizers=True),indent=2)+'\n')
    print(pc.strip(),flush=True)

if __name__=='__main__':main()
