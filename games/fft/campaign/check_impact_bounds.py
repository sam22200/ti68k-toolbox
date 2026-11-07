#!/usr/bin/env python3
"""Approved area effects through the actual shared renderer and68000 images."""
import argparse,hashlib,json,re,struct,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-area-impact/bounds');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_impact_bounds.c',ROOT/'runtime/core/rt_core.c']
    for name,flags in [('check',[]),('check-san',['-fsanitize=address,undefined','-fno-sanitize=shift'])]:
        run(['cc','-std=gnu99','-O1','-g','-w','-o',out/name,*flags,*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-area-impact-')as name:
        data=Path(name);data_links(data)
        fixture=HERE/'x/core-area-ui/fixture'
        if not (fixture/'source.sha256').exists() or (fixture/'source.sha256').read_text()!=hashlib.sha256((HERE/'data/ffcj.bin').read_bytes()).hexdigest():run([ROOT/'tools/pyenv/bin/python',HERE/'check_area_ui.py','--pc-only'])
        for bank in ('ffgd','ffcj'):
            (data/(bank+'.bin')).unlink();(data/(bank+'.bin')).write_bytes((fixture/(bank+'.bin')).read_bytes())
        pc=run([out/'check'],data);(out/'pc.log').write_text(pc);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
        print(pc.strip(),flush=True)
        m=re.search(r'impact_bounds (\w+) picture (\w+) failures 0 checks (\d+)',pc);assert m
        if args.pc_only:return
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'areahit',*source,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        for bank in ('ffgd','ffcj'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',bank+'.bin',bank],data)
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
        for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
            cmd+=['--file',(data if bank in ('ffgd','ffcj')else HERE/'data')/(bank+'.89y')]
        native=run([*cmd,out/'areahit.89z']);(out/'native.log').write_text(native)
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-4:]==[int(m[1],16),int(m[2],16),0,int(m[3])]and '_main returned'in native,native[-3000:]
    zones=[s.strip()for s in native.splitlines()if re.match(r'^(?:(?:59|60)\s+|max (?:59|60)\s+)',s)]
    report=dict(checks=int(m[3]),hash=m[1],pictures=m[2],backend_frame_only=True,native_equal=True,zones=zones)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(report,flush=True)


if __name__=='__main__':main()
