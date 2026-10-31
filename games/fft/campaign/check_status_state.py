#!/usr/bin/env python3
"""Canonical bitmap storage/filtering and actual cure/Charge/AI adapters."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--integration-only',action='store_true',help='check public adapters, reuse unchanged bitmap oracle')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-status-state/check')
    args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    common=[HERE/'check_status_state.c',ROOT/'runtime/core/rt_core.c']
    flags=['-DSTATUS_STATE_INTEGRATION_ONLY']if args.integration_only else[]
    run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',out/'check',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         *flags,'-o',out/'check-san',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-status-state-')as name:
        data=Path(name);data_links(data)
        if not args.integration_only:(data/'fsp00.bin').symlink_to(HERE/'x/core-status-state/oracle/fsp00.bin')
        pc=run([out/'check'],data)
        assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES',*flags,'-o',out/'stcheck',*common,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
        cmd+=['--file',HERE/'data'/(bank+'.89y')]
    fixture=HERE/'x/core-status-state/oracle'
    if not args.integration_only:
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','fsp00.bin','fsp00'],fixture)
        cmd+=['--file',fixture/'fsp00.89y']
    native=run([*cmd,out/'stcheck.89z']);(out/'native.log').write_text(native)
    match=re.search(r'statusstate (\w+) failures 0 checks (\d+)',pc);assert match,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[2])],native[-3000:]
    assert '_main returned'in native
    result=dict(checks=int(match[2]),hash=match[1],native_equal=True,integration_only=args.integration_only,
                zones=[line.strip()for line in native.splitlines()if re.match(r'^(?:1[67]\s+|max 1[67]\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result,flush=True)

if __name__=='__main__':main()
