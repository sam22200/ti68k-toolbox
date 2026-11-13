#!/usr/bin/env python3
"""PC-first guarded100-row targets; --native adds two selected backend/image checks."""
import argparse
import json
from pathlib import Path
import re
import tempfile

from check_flow import HERE, ROOT, data_links, run


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/battle-ram-margin')
    ap.add_argument('--native',action='store_true')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_battle_planes.c',ROOT/'runtime/core/rt_core.c']
    results=[]
    for variant,flags in (
        ('reference',[]),('compact',['-DRT_DRAW_ROWS=100']),
        ('san',['-DRT_DRAW_ROWS=100','-fsanitize=address,undefined',
                '-fno-sanitize=shift','-fno-pie','-no-pie'])):
        run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',out/('planes-'+variant),
             *source,ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-planes-')as name:
        data=Path(name);data_links(data)
        for case in (0,61,101):
            logs=[]
            for variant in ('reference','compact','san'):
                log=run(['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',
                         out/('planes-'+variant),case],data)
                (out/f'{case}-{variant}.log').write_text(log);logs.append(log)
            assert len(set(logs))==1,logs
            match=re.search(r'planes (\w+) failures 0',logs[0]);assert match,logs
            results.append(dict(scenario=case,hash=match[1],sanitizers_equal=True))
            print(f'PC/reference/ASan/UBSan {case}: {match[1]}',flush=True)
    if args.native:
        # Reuse unchanged banks; the one affected RESULTS image is compiled privately.
        native=out/'native-data';native.mkdir(exist_ok=True)
        run([ROOT/'tools/pyenv/bin/python',HERE.parent/'tools/module.py',native/'ffbm.bin',
             HERE/'battle.c',28,'--second',native/'ffbn.bin','--third',native/'ffbo.bin',
             '--companion',27])
        assert (native/'modules.h').read_bytes()==(HERE/'data/modules.h').read_bytes()
        files={p.stem:p for folder in (HERE.parent/'opening/data',HERE/'data')for p in folder.glob('*.89y')}
        for bank in ('ffbm','ffbn','ffbo'):
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',bank+'.bin',bank],native)
            files[bank]=native/(bank+'.89y')
        log=run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-DRT_DRAW_ROWS=100',
                 '-o',out/'planesc',*source,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        (out/'native-build.log').write_text(log)
        for case in (0,61):
            command=[ROOT/'tools/bin/ti-cycles','--arg',case,'--frames',0,'--max',3000]
            for file in files.values():command+=['--file',file]
            log=run([*command,out/'planesc.89z'])
            (out/f'{case}-native.log').write_text(log)
            values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
            expected=next(row for row in results if row['scenario']==case)
            assert values[-2:]==[int(expected['hash'],16),0],(case,values,expected)
            expected['native_equal']=True
            print(f'68000 selected {case}: {expected["hash"]}',flush=True)
    (out/'planes-results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
