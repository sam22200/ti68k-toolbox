#!/usr/bin/env python3
"""Source horizontal-jump oracle, portable C, sanitizers and real68000."""
import json,re
from check import HERE,ROOT,run


def main():
    out=HERE/'x/core-jump';data=out/'oracle';data.mkdir(parents=True,exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python',HERE.parent/'tools/jump_surface_oracle.py','--out',data])
    source=HERE/'check_jump_surface.c';common=[source,ROOT/'runtime/core/rt_core.c']
    host=[*common,ROOT/'runtime/platform-sw/rt_sw.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'jump',*host])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-o',out/'jump-san',*host])
    pc=run([out/'jump'],data)
    assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'jump-san'],data)==pc
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'jumpc',*common,ROOT/'tools/extgraph/lib/tilemap.a'])
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--max',10000]
    for p in sorted(data.glob('fjq*.bin')):
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',p.name,p.stem],data)
        cmd+=['--file',p.with_suffix('.89y')]
    native=run([*cmd,out/'jumpc.89z'])
    match=re.search(r'jump (\w+) failures 0 checks (\d+)',pc);assert match,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[2])],native[-2000:]
    assert '_main returned'in native
    (out/'native.log').write_text(native)
    report=dict(checks=int(match[2]),hash=match[1],sanitizers=True,
                source=json.loads((data/'source.json').read_text()))
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(pc.strip(),flush=True)


if __name__=='__main__':main()
