#!/usr/bin/env python3
"""Actual battle-cell transitions and module-bank validation, PC/sanitizers/68000."""
import json
import re
import tempfile
from pathlib import Path
from check_flow import HERE,ROOT,data_links
from check import run


def main():
    out=HERE/'x/core-floors';out.mkdir(parents=True,exist_ok=True)
    results=[]
    for label in ('cell_state','module_bank'):
        source=HERE/('check_'+label+'.c');name='cellc'if label=='cell_state'else'modc'
        common=[source]
        if label=='cell_state':common+=[ROOT/'runtime/core/rt_core.c']
        host=common+([ROOT/'runtime/platform-sw/rt_sw.c']if label=='cell_state'else[])
        run(['cc','-std=gnu99','-O1','-g','-w','-o',out/label,*host])
        run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined',
             '-fno-omit-frame-pointer',*(['-fno-sanitize=shift']if label=='cell_state'else[]),
             '-o',out/(label+'-san'),*host])
        with tempfile.TemporaryDirectory(prefix='fft-cells-')as folder:
            data=Path(folder);data_links(data)
            pc=run([out/label],data)
            # Battle fixtures retain backend file-cache/process allocations.
            sanitized=run(['env','ASAN_OPTIONS=detect_leaks=0',out/(label+'-san')],data)
            assert sanitized==pc,(pc,sanitized)
        native_sources=common+([ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a']if label=='cell_state'else[])
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/name,*native_sources])
        command=[ROOT/'tools/bin/ti-cycles','--frames',0,'--max',10000]
        if label=='cell_state':
            for directory in (HERE.parent/'opening/data',HERE/'data'):
                for p in directory.glob('*.89y'):command+=['--file',p]
        native=run([*command,out/(name+'.89z')])
        match=re.search(r'(cells|modules) (\w+) failures 0 checks (\d+)',pc);assert match,pc
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(match[2],16),0,int(match[3])],(pc,values)
        assert '_main returned'in native
        (out/(label+'.log')).write_text(native)
        results.append(dict(case=label,checks=int(match[3]),hash=match[2],sanitizers=True))
        print(pc.strip(),flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
