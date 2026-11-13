#!/usr/bin/env python3
"""Three selected standalone skip replays with actual normalized archive art."""
import argparse
import json
from pathlib import Path
import re
import tempfile
from check_flow import HERE, ROOT, run, data_links


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--data-dir',type=Path,required=True)
    ap.add_argument('--out',type=Path,default=HERE/'x/campaign-prerequisites/standalone-native')
    args=ap.parse_args();data_dir=args.data_dir.resolve();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    common=[HERE/'check_flow_scene.c',ROOT/'runtime/core/rt_core.c']
    (out/'pc-build.log').write_text(run(['cc','-std=gnu99','-O1','-w','-o',out/'pc',*common,ROOT/'runtime/platform-sw/rt_sw.c']))
    (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'skipc',*common,
        ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a']))
    results=[]
    for label,frames,events in [('prompt',12,[(10,'ESC')]),
                              ('resume',16,[(10,'ESC'),(12,'ENTER')]),
                              ('skip',18,[(10,'ESC'),(12,'DOWN'),(14,'ENTER')])]:
        script=out/(label+'.keys');script.write_text('0\n'+''.join(f'{f} {k}\n{f+1}\n'for f,k in events))
        with tempfile.TemporaryDirectory(prefix='fft-scene-skip-')as folder:
            data=Path(folder);data_links(data,data_dir)
            pc=run([out/'pc',1,frames,script,data/'screen.png'],data)
            # PNG is temporary; exact checksums are sufficient after visual approval.
        (out/(label+'-pc.log')).write_text(pc)
        cmd=[ROOT/'tools/bin/ti-cycles','--arg',1,'--frames',frames,'--keys',script,'--max',10000]
        for path in data_dir.glob('*.89y'):cmd+=['--file',path]
        native=run([*cmd,out/'skipc.89z']);(out/(label+'-native.log')).write_text(native)
        match=re.search(r'executed (\d+) checksum (\w+) state (\w+)',pc);assert match,pc
        actual=re.search(r'checksum\s+(\w+)',native);assert actual,native[-1000:]
        assert actual[1].lower()==match[2].lower(),label
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(match[1]),0xacad0001,int(match[3],16)],(label,values[-3:],pc)
        results.append(dict(case=label,frames=int(match[1]),screen=match[2],state=match[3],native_equal=True))
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(results))


if __name__=='__main__':main()
