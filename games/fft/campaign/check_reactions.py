#!/usr/bin/env python3
"""First actual reaction effects and source staging, PC/sanitizers/68000."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links,fixture as save_fixture
from check_core_boundaries import descriptor13


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true');ap.add_argument("--out",type=Path,default=HERE/"x/core-reactions/check");args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_reactions.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=HERE/'x/core-reactions/oracle';(fixture/'ffgd.bin').write_bytes(descriptor13())
    save=bytearray(save_fixture());save[294+30:294+32]=bytes((1,0xba));(fixture/'ffsave.sav').write_bytes(save)
    with tempfile.TemporaryDirectory(prefix='fft-reactions-')as folder:
        data=Path(folder);data_links(data);(data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        (data/'ffsave.sav').write_bytes(save)
        pc=run([out/'check'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'reactchk',*source,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],fixture)
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','ffsave.sav','ffsave'],fixture)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
        cmd+=['--file',(fixture if bank=='ffgd'else HERE/'data')/(bank+'.89y')]
    cmd+=['--file',fixture/'ffsave.89y']
    native=run([*cmd,out/'reactchk.89z']);(out/'native.log').write_text(native)
    m=re.search(r'reactions (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-3000:]
    result=dict(checks=int(m[2]),hash=m[1],native_equal=True,
        zones=[line.strip()for line in native.splitlines()if re.match(r'^(?:27\s+|max 27\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
