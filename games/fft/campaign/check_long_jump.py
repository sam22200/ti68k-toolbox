#!/usr/bin/env python3
"""22 original full horizontal-spread fixtures, focused PC/sanitized/native comparison."""
import json,re
from check import HERE,ROOT,run


def main():
    out=HERE/'x/core-long-jump';data=out/'oracle';data.mkdir(parents=True,exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python',HERE.parent/'tools/long_jump_oracle.py','--out',data])
    source=HERE/'check_long_jump.c';common=[source,ROOT/'runtime/core/rt_core.c']
    host=[*common,ROOT/'runtime/platform-sw/rt_sw.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'recoil',*host])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-o',out/'recoil-san',*host])
    pc=run([out/'recoil'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'recoil-san'],data)==pc
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'recoilc',*common,ROOT/'tools/extgraph/lib/tilemap.a'])
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','flj00.bin','flj00'],data)
    native=run([ROOT/'tools/bin/ti-cycles','--frames',0,'--max',1000,'--file',data/'flj00.89y',out/'recoilc.89z'])
    m=re.search(r'longjump (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])],native[-2000:]
    assert '_main returned'in native;(out/'native.log').write_text(native)
    (out/'results.json').write_text(json.dumps(dict(checks=int(m[2]),hash=m[1],sanitizers=True,source=json.loads((data/'source.json').read_text())),indent=2)+'\n')
    print(pc.strip(),flush=True)


if __name__=='__main__':main()
