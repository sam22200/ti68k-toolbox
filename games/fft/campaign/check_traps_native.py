#!/usr/bin/env python3
"""One opt-in actual-overlay trap/save carrier on private normalized data."""
import argparse
import json
from pathlib import Path
import re
import tempfile
import struct
from check_flow import HERE,ROOT,data_links,run
from check_traps import synthetic_common


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--data-dir',type=Path,required=True,help='normalized shipping .bin/.89y banks')
    ap.add_argument('--out',type=Path,default=HERE/'x/tile-traps/native')
    args=ap.parse_args();source_data=args.data_dir.resolve();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_traps_native.c',ROOT/'runtime/core/rt_core.c']
    (out/'pc-build.log').write_text(run(['cc','-std=gnu99','-O1','-w','-o',out/'pc',*source,ROOT/'runtime/platform-sw/rt_sw.c']))
    with tempfile.TemporaryDirectory(prefix='fft-traps-native-')as name:
        data=Path(name);data_links(data,source_data)
        common=synthetic_common((data/'ffcj.bin').read_bytes())
        desc=bytearray((data/'ffgd.bin').read_bytes());assert desc[:4]==b'FCD5';struct.pack_into('>H',desc,104,0)
        # Only this synthetic trap table/source-map identity differ from the
        # real shipping data. Programs and every other resource remain exact.
        for bank,raw in [('ffcj',common),('ffgd',desc)]:
            (data/(bank+'.bin')).unlink();(data/(bank+'.bin')).write_bytes(raw)
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',bank+'.bin',bank],data)
        pc=run([out/'pc'],data);(out/'pc.log').write_text(pc)
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'trapc',*source,
                              ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a']))
        command=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',200,'--max',10000]
        for f in source_data.glob('*.89y'):
            if not f.stem.startswith('ffsave')and f.stem!='ffslot':command+=['--file',data/f.name if (data/f.name).exists()else f]
        native=run([*command,out/'trapc.89z']);(out/'native.log').write_text(native)
    match=re.search(r'trap-native (\w+) failures 0 checks (\d+)',pc);assert match,pc
    screens=[int(x,16)for x in re.findall(r'trap-screen (\w+)',pc)]
    expected=[*screens,int(match[1],16),0,int(match[2])]
    values=[int(x,16)for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-len(expected):]==expected,(values[-15:],expected)
    report=dict(hash=match[1],checks=int(match[2]),screens=[f'{x:08X}'for x in screens],
                native_equal=True,shipping_data=True,synthetic_tables=['ffcj Move-Find map0..3','ffgd SOURCE_MAP0'],
                virtual_vat_fsv5=True,no_emulator=True)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
