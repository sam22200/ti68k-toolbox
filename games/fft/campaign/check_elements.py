#!/usr/bin/env python3
"""Targeted Typed elemental outcomes and signed HP integration, PC/sanitizers and actual 68000 modules."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13
import struct


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-elements/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_elements.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=HERE/'x/core-elements/oracle';(fixture/'ffgd.bin').write_bytes(descriptor13())
    catalogue=bytearray((HERE/'data/ffcj.bin').read_bytes());at=struct.unpack_from('>H',catalogue,14)[0]+0x93*14
    catalogue[at+13]=7;catalogue[at+5]|=0x20
    norm=struct.unpack_from('>H',catalogue,30)[0]
    for ab,power,element in ((0xf,50,2),(0x100,96,2),(0x101,96,0),(0x102,50,128),(0x103,50,0),(0x104,255,2)):
        catalogue[norm+ab*12:norm+ab*12+12]=struct.pack('>8BH2B',0,power,5,255,128,255,100,0,600,8,element)
    (fixture/'ffcj.bin').write_bytes(catalogue)
    with tempfile.TemporaryDirectory(prefix='fft-magic-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        (data/'ffcj.bin').unlink();(data/'ffcj.bin').symlink_to(fixture/'ffcj.bin')
        pc=run([out/'check'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'elechk',*source,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    for name in ('ffgd','ffcj'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
        cmd+=['--file',(fixture if bank in ('ffgd','ffcj')else HERE/'data')/(bank+'.89y')]
    native=run([*cmd,out/'elechk.89z']);(out/'native.log').write_text(native)
    m=re.search(r'elements (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-3000:]
    result=dict(checks=int(m[2]),hash=m[1],native_equal=True,
        zones=[line.strip()for line in native.splitlines()if re.match(r'^(?:(?:46|47|48|49|50)\s+|max (?:46|47|48|49|50)\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
