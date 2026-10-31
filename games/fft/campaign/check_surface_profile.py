#!/usr/bin/env python3
"""Reduced-layer geometry equivalence, portable C, sanitizers and real68000."""
import argparse,json,re
from pathlib import Path
from check import HERE,ROOT,run


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--deep',action='store_true',help='Opt-in full retail profile/height sweep')
    ap.add_argument('--pc-only',action='store_true',help='Skip native compilation for host-only edits')
    ap.add_argument('--out',type=Path)
    args=ap.parse_args();out=(args.out or HERE/('x/core-profile-deep'if args.deep else'x/core-profile-fast')).resolve()
    data=out/'profiles';data.mkdir(parents=True,exist_ok=True)
    if args.deep:run([ROOT/'tools/pyenv/bin/python',HERE.parent/'tools/map_layers.py','--out',out])
    flags=['-DPROFILE_DEEP']if args.deep else[]
    source=HERE/'check_surface_profile.c';common=[source,ROOT/'runtime/core/rt_core.c']
    host=[*common,ROOT/'runtime/platform-sw/rt_sw.c']
    run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',out/'jump',*host])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined',*flags,'-o',out/'jump-san',*host])
    pc=run([out/'jump'],data)
    assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'jump-san'],data)==pc
    if args.pc_only:
        (out/'pc.log').write_text(pc);print(pc.strip(),flush=True);return
    run([ROOT/'tools/bin/ti-cc',*flags,'-DRT_CYCLES','-o',out/'jumpc',*common,ROOT/'tools/extgraph/lib/tilemap.a'])
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--max',30000]
    for p in sorted(data.glob('fsp*.bin')):
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',p.name,p.stem],data)
        cmd+=['--file',p.with_suffix('.89y')]
    native=run([*cmd,out/'jumpc.89z'])
    match=re.search(r'profile (\w+) failures 0 checks (\d+)',pc);assert match,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[2])],native[-2000:]
    assert '_main returned'in native
    (out/'native.log').write_text(native)
    report=dict(checks=int(match[2]),hash=match[1],sanitizers=True,
                mode='deep'if args.deep else'focused',
                source=json.loads((out/'maps.json').read_text())if args.deep else'Nine curated profiles and22 boundary heights')
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(pc.strip(),flush=True)


if __name__=='__main__':main()
