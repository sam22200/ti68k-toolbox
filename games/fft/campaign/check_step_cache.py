#!/usr/bin/env python3
"""Accepted/cached graph equality, scratch ownership and node255 on PC/68000."""
import json,re,tempfile
from pathlib import Path
from check_flow import data_links
from check import HERE,ROOT,run


def main():
    out=HERE/'x/core-steps/cache';out.mkdir(parents=True,exist_ok=True)
    source=HERE/'check_step_cache.c';common=[source,ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'cache',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'cache-san',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'cachec',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    files={p.stem:p for folder in (HERE.parent/'opening/data',HERE/'data')for p in folder.glob('*.89y')}
    results=[]
    for label,scenario in [('gariland',201),('orbonne',101)]:
        with tempfile.TemporaryDirectory(prefix='fft-step-cache-')as name:
            data=Path(name);data_links(data);pc=run([out/'cache',scenario],data)
            assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'cache-san',scenario],data)==pc
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',scenario,'--max',10000]
        for p in files.values():cmd+=['--file',p]
        native=run([*cmd,out/'cachec.89z'])
        match=re.search(r'stepcache (\w+) failures 0 checks (\d+)',pc);assert match,pc
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(match[1],16),0,int(match[2])],native[-2000:]
        assert '_main returned'in native
        (out/(label+'.log')).write_text(native)
        zones=[line.strip()for line in native.splitlines()if re.match(r'^[56]\s+(Reference|Cached) step graph',line)]
        results.append(dict(map=label,checks=int(match[2]),hash=match[1],zones=zones))
        print(label,pc.strip(),zones,flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
