#!/usr/bin/env python3
"""Read-only radial target sets against original PS1 footprints, PC/sanitizers and actual 68000 modules."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13
import struct


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-targets/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_targets.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=HERE/'x/core-targets/oracle';(fixture/'ffgd.bin').write_bytes(descriptor13())
    catalogue=bytearray((HERE/'data/ffcj.bin').read_bytes());raw=struct.unpack_from('>H',catalogue,14)[0]
    fixture_rows=json.loads((fixture/'source.json').read_text())['cases']
    fire=bytes(catalogue[raw+14*0x10:raw+14*0x10+14])
    for i,row in enumerate(fixture_rows):
        r=bytearray(fire);r[1]=row['aoe'];r[2]=row['vert'];r[3]=row['f1'];r[4]=row['f2'];r[6]=row['f4']
        catalogue[raw+(0x100+i)*14:raw+(0x100+i)*14+14]=r
    # Formula 0x24 (Geomancy) stays outside the radial families; 0x0a became the status family.
    for i,(field,value)in enumerate(((8,0x24),(3,32),(4,2),(4,8),(6,32))):
        r=bytearray(fire);r[field]=value;catalogue[raw+(0x150+i)*14:raw+(0x150+i)*14+14]=r
    (fixture/'ffcj.bin').write_bytes(catalogue)
    with tempfile.TemporaryDirectory(prefix='fft-targets-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        (data/'ffcj.bin').unlink();(data/'ffcj.bin').symlink_to(fixture/'ffcj.bin')
        (data/'fat00.bin').symlink_to(fixture/'fat00.bin')
        (data/'fal00.bin').symlink_to(fixture/'fal00.bin')
        pc=run([out/'check'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'tgtchk',*source,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    for name in ('ffgd','ffcj','fat00','fal00'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj fat00 fal00'.split():
        cmd+=['--file',(fixture if bank in ('ffgd','ffcj','fat00','fal00')else HERE/'data')/(bank+'.89y')]
    native=run([*cmd,out/'tgtchk.89z']);(out/'native.log').write_text(native)
    m=re.search(r'targets (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-3000:]
    result=dict(checks=int(m[2]),hash=m[1],native_equal=True,
        zones=[line.strip()for line in native.splitlines()if re.match(r'^(?:(?:61|62|63)\s+|max (?:61|62|63)\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
