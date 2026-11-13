#!/usr/bin/env python3
"""Private13-actor board/rotation/last-actor Status planes on both CPUs."""
import json
import argparse
import re
from pathlib import Path
import tempfile
from check_flow import HERE,ROOT,data_links
from check import run
from check_core_boundaries import descriptor13


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/core-capacity/render')
    ap.add_argument('--case',action='append',choices=('board','rotations','status-last','equipment-last'),
                    help='select affected cases; default is the original four-case certification')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    common=[HERE/'check_capacity_render.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-w','-o',out/'render',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'renderc',*common,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    results=[]
    files={p.stem:p for folder in (HERE.parent/'opening/data',HERE/'data')for p in folder.glob('*.89y')}
    with tempfile.TemporaryDirectory(prefix='fft-render13-')as name:
        data=Path(name);data_links(data);(data/'ffgd.bin').unlink()
        (data/'ffgd.bin').write_bytes(descriptor13())
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],data)
        files['ffgd']=data/'ffgd.89y'
        empty=out/'none.keys';empty.write_text('0\n')
        rotations=out/'rotate.keys';rotations.write_text('12 F5\n13\n50 F1\n51\n')
        for label,scenario,frames,keys in (('board',0,90,empty),('rotations',0,100,rotations),
                                         ('status-last',1,100,empty),('equipment-last',2,100,empty)):
            if args.case and label not in args.case:continue
            pc=run([out/'render',scenario,frames,keys,out/(label+'-pc.png')],data)
            command=[ROOT/'tools/bin/ti-cycles','--arg',scenario,'--frames',frames,'--max',3000,
                     '--keys',keys,'--png',out/(label+'-native.png')]
            for p in files.values():command+=['--file',p]
            native=run([*command,out/'renderc.89z'])
            match=re.search(r'capacity checksum (\w+) identity (\w+)',pc);assert match,pc
            assert re.search(r'checksum (\w+)',native)[1].lower()==match[1].lower()
            values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
            assert values[-1]==int(match[2],16)and values[-1]>>24==13,(label,pc,native[-1000:])
            assert values[-2]==frames and len(values[:-2])==2*frames,(label,len(values))
            frame_costs=values[:-2]
            ordinary=[cost for mode,cost in zip(frame_costs[::2],frame_costs[1::2])if not mode>>31]
            cold=[cost for mode,cost in zip(frame_costs[::2],frame_costs[1::2])if mode>>31]
            assert max(ordinary)<360000,(label,max(ordinary))
            if scenario:assert(values[-1]>>16)&255==12
            (out/(label+'.log')).write_text(native)
            results.append(dict(case=label,checksum=match[1],identity=match[2],actors=13,
                                ordinary_average_cycles=sum(ordinary)//len(ordinary),
                                ordinary_peak_cycles=max(ordinary),cold_or_turn_peak_cycles=max(cold,default=0)))
            print(label+': '+pc.strip(),flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
