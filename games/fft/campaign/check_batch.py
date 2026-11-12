#!/usr/bin/env python3
"""Compound formula08 preparation/payment/commit and response drain on PC/sanitizers/68000."""
import argparse,json,re,tempfile
from fixture_monster import monster_sets
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13
import struct


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-batch/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_batch.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=HERE/'x/core-batch/oracle';(fixture/'ffgd.bin').write_bytes(descriptor13())
    catalogue=bytearray((HERE/'data/ffcj.bin').read_bytes());raw=struct.unpack_from('>H',catalogue,14)[0]
    rows=json.loads((fixture/'source.json').read_text())['cases']
    norm=struct.unpack_from('>H',catalogue,30)[0]
    for i,row in enumerate([*rows,dict(rows[2])]):
        at=raw+14*(0x110+i);r=bytearray(catalogue[raw+14*(0x10 if row['evade']else 0xf):raw+14*(0x10 if row['evade']else 0xf)+14])
        r[1]=255;r[2]=255;r[4]=1 if i==13 else 0;r[7]=row['element'];r[10]=row['power'];r[13]=7
        catalogue[at:at+14]=r
        value=bytearray(12);value[:8]=bytes((0,row['power'],5,255,128|32|1|(2 if row['evade']else 0),255,100,0));value[10]=8;value[11]=row['element']
        catalogue[norm+12*(0x110+i):norm+12*(0x110+i)+12]=value
    for i in (14,15):
        catalogue[raw+14*(0x110+i):raw+14*(0x111+i)]=catalogue[raw+14*0x112:raw+14*0x113]
        catalogue[norm+12*(0x110+i):norm+12*(0x111+i)]=catalogue[norm+12*0x112:norm+12*0x113]
    catalogue[raw+14*0x11e+5]|=4
    catalogue[norm+12*0x11f+4]|=16
    monster_sets(catalogue)
    (fixture/'ffcj.bin').write_bytes(catalogue)
    with tempfile.TemporaryDirectory(prefix='fft-batch-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        (data/'ffcj.bin').unlink();(data/'ffcj.bin').symlink_to(fixture/'ffcj.bin')
        (data/'fba00.bin').symlink_to(fixture/'fba00.bin')
        pc=run([out/'check'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'betchk',*source,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    for name in ('ffgd','ffcj','fba00'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj fba00'.split():
        cmd+=['--file',(fixture if bank in ('ffgd','ffcj','fba00')else HERE/'data')/(bank+'.89y')]
    native=run([*cmd,out/'betchk.89z']);(out/'native.log').write_text(native)
    m=re.search(r'batch (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-3000:]
    result=dict(checks=int(m[2]),hash=m[1],native_equal=True,
        zones=[line.strip()for line in native.splitlines()if re.match(r'^(?:(?:61|62)\s+|max (?:61|62)\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
