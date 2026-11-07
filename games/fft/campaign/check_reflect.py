#!/usr/bin/env python3
"""Targeted Reflect batch and source endpoint integration, PC/sanitizers and actual 68000 modules."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-reflect/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    from check_offensive_magic import catalogue_report
    (out/'coverage.json').write_text(json.dumps(catalogue_report(),indent=2)+'\n')
    source=[HERE/'check_reflect.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=out/'fixture';fixture.mkdir(exist_ok=True);(fixture/'ffgd.bin').write_bytes(descriptor13())
    with tempfile.TemporaryDirectory(prefix='fft-reflect-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        pc=run([out/'check'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'reflchk',*source,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],fixture)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
        cmd+=['--file',(fixture if bank=='ffgd'else HERE/'data')/(bank+'.89y')]
    native=run([*cmd,out/'reflchk.89z']);(out/'native.log').write_text(native)
    m=re.search(r'reflect (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-3000:]
    result=dict(checks=int(m[2]),hash=m[1],native_equal=True,
        batch_entries=25,production_bank=catalogue_report()['bank_sha256'],
        zones=[line.strip()for line in native.splitlines()if re.match(r'^(?:61\s+|max 61\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
