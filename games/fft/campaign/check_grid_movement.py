#!/usr/bin/env python3
"""Compare native paths/costs with the pre-migration search on both real maps."""
import json
import re
from pathlib import Path
import tempfile
from check_flow import HERE,ROOT,data_links
from check import run
from check_core_boundaries import descriptor13


def main():
    out=HERE/'x/core-grid/movement';out.mkdir(parents=True,exist_ok=True)
    source=HERE/'check_grid_movement.c';core=ROOT/'runtime/core/rt_core.c'
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',source,core,
         ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'movec',source,core,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    files={p.stem:p for folder in (HERE.parent/'opening/data',HERE/'data')for p in folder.glob('*.89y')}
    results=[]
    for label,scenario in (('orbonne',101),('gariland',201),('gariland13',201)):
        with tempfile.TemporaryDirectory(prefix='fft-grid-movement-')as name:
            folder=Path(name);data_links(folder);banks=dict(files)
            if label=='gariland13':
                (folder/'ffgd.bin').unlink();(folder/'ffgd.bin').write_bytes(descriptor13())
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],folder)
                banks['ffgd']=folder/'ffgd.89y'
            pc=run([out/'check',scenario],folder)
            command=[ROOT/'tools/bin/ti-cycles','--arg',scenario,'--frames',0,'--max',20000]
            for p in banks.values():command+=['--file',p]
            native=run([*command,out/'movec.89z'])
        match=re.search(r'movement (\w+) failures 0 checks (\d+)',pc);assert match,pc
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(match[1],16),0,int(match[2])],(pc,values)
        samples=values[:-3];assert len(samples)%3==0
        costs=[]
        for limit in (4,12,124):
            pairs=[samples[i+1:i+3]for i in range(0,len(samples),3)if(samples[i]&255)==limit]
            costs.append(dict(limit=limit,samples=len(pairs),legacy_average=sum(x[0]for x in pairs)//len(pairs),
                              compact_average=sum(x[1]for x in pairs)//len(pairs),
                              compact_peak=max(x[1]for x in pairs)))
        zones=[line.strip()for line in native.splitlines()if re.match(r'^[45]\s+(Legacy|Compact) map search',line)]
        (out/(label+'.log')).write_text(native)
        results.append(dict(case=label,checks=int(match[2]),hash=match[1],native_zones=zones,costs_by_limit=costs))
        print(label+': '+pc.strip(),flush=True)
        for zone in zones:print(zone,flush=True)
        print(json.dumps(costs),flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
