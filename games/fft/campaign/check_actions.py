#!/usr/bin/env python3
"""Targeted primary-effect/commit checks on PC, sanitizers and actual68000."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-actions/check')
    args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    common=[HERE/'check_actions.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-actions-')as name:
        data=Path(name);data_links(data)
        pc=run([out/'check'],data)
        assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'actcheck',*common,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
        cmd+=['--file',HERE/'data'/(bank+'.89y')]
    native=run([*cmd,out/'actcheck.89z']);(out/'native.log').write_text(native)
    match=re.search(r'actions (\w+) failures 0 checks (\d+)',pc);assert match,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[2])],native[-3000:]
    assert '_main returned'in native
    result=dict(checks=int(match[2]),hash=match[1],native_equal=True,
                zones=[line.strip()for line in native.splitlines()if re.match(r'^(?:15\s+|max 15\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result,flush=True)

if __name__=='__main__':main()
