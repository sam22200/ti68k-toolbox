#!/usr/bin/env python3
"""Boot the unchanged tactical rules with real installed and remapped pack data."""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import re
import sys
import tempfile

from check_chapter_packages import run, HERE, ROOT
sys.path.insert(0, str(HERE.parent/'tools'))
import chapter_packages as packages


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input',type=Path,required=True)
    ap.add_argument('--out',type=Path,default=HERE/'x/chapter-packages/routing')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=packages.read_group(args.input)
    core=json.loads((HERE/'packages/core.json').read_text())
    chapter=json.loads((HERE/'packages/chapter-01.json').read_text())
    src=HERE/'check_package_routing.c';rt=ROOT/'runtime/core/rt_core.c'
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'pc',src,rt,ROOT/'runtime/platform-sw/rt_sw.c'],ROOT,out/'pc-build.log')
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-pie','-no-pie',
         '-o',out/'san',src,rt,ROOT/'runtime/platform-sw/rt_sw.c'],ROOT,out/'san-build.log')
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'pkroute',src,rt,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'],ROOT,out/'native-build.log')
    report=[]
    for label,scenario,remap in [('orbonne',101,False),('gariland',201,False),
                                  ('remapped-gariland',301,True),('continuation-only',301,True),
                                  ('absent-route',401,True),('missing-pack',299,False)]:
        spec=deepcopy(chapter)
        if remap:
            spec['routes'][1]['band']=3
            for step in spec['steps']:
                if step['program']!='scene' and step['argument']==200:step['argument']=300
        if label=='continuation-only':
            spec.update(id=2,requires_flag=1,previous=1,routes=[{'band':3,'prefix':'g'}])
            spec['steps']=spec['steps'][2:]
            spec['variables']=[n for n in spec['variables'] if not n.startswith('ffo') and n!='ffs0']
        with tempfile.TemporaryDirectory(prefix='fft-installed-pack-')as folder:
            data=Path(folder)
            _,shared,packs=packages.build(source,core,[spec],data/'delivery',allow_unused=label=='continuation-only')
            variables=shared+packs[0][1]
            if label=='missing-pack':variables=[v for v in variables if v.name!='ffpack']
            for v in variables:
                # PC rt_file opens raw data, while native reads AMS's dat trailer.
                if v.kind==0x1c and v.payload.endswith(b'\0dat\0\xf8'):(data/(v.name+'.bin')).write_bytes(v.raw)
                (data/(v.name+('.89z' if v.kind==0x21 else '.89y'))).write_bytes(packages.group_bytes([v],single=True))
            san=run([out/'san',scenario],data,out/(label+'-san.log'))
            pc=run([out/'pc',scenario],data,out/(label+'-pc.log'))
            m=re.search(r'routing ([0-9A-F]+) failures 0 checks (\d+)',pc)
            assert m and re.search(r'routing ([0-9A-F]+) failures 0 checks (\d+)',san).groups()==m.groups(),(pc,san)
            cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',scenario,'--max',10000]
            for v in variables:
                cmd+=['--file',data/(v.name+('.89z' if v.kind==0x21 else '.89y'))]
            native=run([*cmd,out/'pkroute.89z'],ROOT,out/(label+'-native.log'))
            values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
            assert values[-3:]==[int(m[1],16),0,int(m[2])],(label,pc,values)
        report.append(dict(case=label,checks=int(m[2]),hash=m[1],native_equal=True))
        print(label+': '+pc.strip(),flush=True)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
