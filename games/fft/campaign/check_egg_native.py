#!/usr/bin/env python3
"""Five selected native archive/render/RNG cases, not a TI battle sweep."""
import json
import re
import tempfile
from pathlib import Path
from check_flow import HERE,ROOT,data_links,fixture,run


def main():
    out=HERE/'x/eggs/native-render';out.mkdir(parents=True,exist_ok=True)
    source=HERE/'x/eggs/shipping';rows=[]
    initial=bytearray(b'FSV6'+fixture()[4:]+bytes(3918-len(fixture())))
    initial[4]|=2;initial[36]=8;initial[37]=7
    parent=bytearray(initial[470:646]);parent[0:5]=bytes((95,255,3,250,42));initial[294+7*176:294+8*176]=parent
    initial=b'FSV7'+bytes(initial[4:])+bytes((1,1,0,0))+bytes(65)+bytes((0,0,0,1,0))
    for label,flags,cases in [('manage',[],[(0,0x9c07),(1,0xaa1b),(2,0xe3df)]),('map',['-DEGG_MAP'],[(0,0xeaaa),(1,None)])]:
        common=[*flags,HERE/'check_egg_render.c',ROOT/'runtime/core/rt_core.c']
        (out/(label+'-build.log')).write_text(run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/label,*common,
            ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a']))
        run(['cc','-std=gnu99','-O1','-w','-o',out/(label+'-pc'),*common,ROOT/'runtime/platform-sw/rt_sw.c'])
        with tempfile.TemporaryDirectory(prefix='fft-egg-native-') as folder:
            data=Path(folder);data_links(data,source);(data/'ffsave.sav').write_bytes(initial)
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','ffsave.sav','ffsave'],data)
            for scenario,approved in cases:
                pc=run([out/(label+'-pc'),scenario],data);match=re.search(r'egg-render (\w+) state(\w+)',pc);assert match
                if approved is not None:assert int(match[1],16)==approved,pc
                command=[ROOT/'tools/bin/ti-cycles','--frames',1,'--arg',scenario,'--max',5000]
                for name in ('ffmg','ffcj','ffcp','ffwm','ffeg','ffwb'):command += ['--file',source/(name+'.89y')]
                log=run([*command,'--file',data/'ffsave.89y',out/(label+'.89z')])
                (out/f'{label}-{scenario}.log').write_text(log)
                assert re.search(r'checksum (\w+)',log)[1].lower()==match[1].lower(),log
                values=[int(x,16) for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
                assert values[-2:]==[int(match[2],16),3992],values
                if scenario==1 and label=='map':assert int(match[2],16)!=17
                rows.append(dict(case=f'{label}-{scenario}',checksum=match[1],state_hash=match[2]))
    (out/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows,indent=2))


if __name__=='__main__':main()
