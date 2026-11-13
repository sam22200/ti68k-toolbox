#!/usr/bin/env python3
"""One selected actual-overlay scene/save carrier; no emulator or native sweep."""
import argparse
import json
from pathlib import Path
import re
import tempfile
from check_flow import HERE, ROOT, data_links, run


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path,default=HERE/'x/campaign-prerequisites/native')
    parser.add_argument('--data-dir',type=Path,help='optional normalized shipping banks, .bin and .89y')
    args=parser.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_campaign_native.c',ROOT/'runtime/core/rt_core.c']
    log=run(['cc','-std=gnu99','-O1','-w','-o',out/'pc',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    (out/'pc-build.log').write_text(log)
    with tempfile.TemporaryDirectory(prefix='fft-prerequisite-native-')as name:
        data=Path(name);data_links(data,args.data_dir.resolve()if args.data_dir else None)
        pc=run([out/'pc'],data);(out/'pc.log').write_text(pc)
    log=run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'prereqc',*source,
             ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    (out/'native-build.log').write_text(log)
    folders=[args.data_dir.resolve()]if args.data_dir else [HERE.parent/'opening/data',HERE/'data']
    files={p.stem:p for folder in folders for p in folder.glob('*.89y')
           if not p.stem.startswith('ffsave')and p.stem!='ffslot'}
    command=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',200,'--max',10000]
    for p in files.values():command+=['--file',p]
    native=run([*command,out/'prereqc.89z']);(out/'native.log').write_text(native)
    values=[int(x,16)for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    match=re.search(r'prerequisites (\w+) failures 0 checks (\d+)',pc);assert match,pc
    screens=[int(x,16)for x in re.findall(r'scene-screen (\w+)',pc)]
    expected=[*screens,int(match[1],16),0,int(match[2])]
    assert values[-len(expected):]==expected,(values[-12:],expected)
    report=dict(hash=match[1],checks=int(match[2]),screen_hashes=[f'{x:08X}'for x in screens],
                native_equal=True,save_vat_roundtrips=True,shipping_data=bool(args.data_dir))
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
