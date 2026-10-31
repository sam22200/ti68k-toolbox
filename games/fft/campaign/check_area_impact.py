#!/usr/bin/env python3
"""Approved area effects through the actual shared renderer and68000 images."""
import argparse,json,re,struct,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-area-impact/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_area_impact.c',ROOT/'runtime/core/rt_core.c']
    for name,flags in [('check',[]),('check-san',['-fsanitize=address,undefined','-fno-sanitize=shift'])]:
        run(['cc','-std=gnu99','-O1','-g','-w','-o',out/name,*flags,*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-area-impact-')as name:
        data=Path(name);data_links(data)
        raw=bytearray((HERE/'data/ffcj.bin').read_bytes());norm=struct.unpack_from('>H',raw,30)[0]
        raw[norm+16*12:norm+17*12]=struct.pack('>8BH2B',0,14,4,255,128|32|1|2,255,100,0,50,8,128)
        (data/'ffcj.bin').unlink();(data/'ffcj.bin').write_bytes(raw)
        pc=run([out/'check'],data);(out/'pc.log').write_text(pc);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
        print(pc.strip().splitlines()[-1],flush=True)
        m=re.search(r'area_impact (\w+) pictures (\w+) failures 0 checks (\d+)',pc);assert m
        if args.pc_only:return
        # Export the same constructor to a portable fixture bank: keep the
        # headless full-render checker under the calculator variable limit.
        exporter=HERE/'check_area_impact_export.c'
        run(['cc','-std=gnu99','-O1','-w','-o',out/'export',exporter,ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
        run([out/'export',data/'fip00.bin'],data)
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','fip00.bin','fip00'],data)
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'areahit',*source,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffcj.bin','ffcj'],data)
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000,'--file',data/'fip00.89y']
        for bank in 'ffbm ffbn ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj ffflow'.split():
            cmd+=['--file',(data if bank=='ffcj'else HERE/'data')/(bank+'.89y')]
        native=run([*cmd,out/'areahit.89z']);(out/'native.log').write_text(native)
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    frames=[int(v,16)for v in re.findall(r'frame \d+ \d+ (\w+)',pc)]
    assert values[-68:-4]==frames, [(i,hex(a),hex(b))for i,(a,b)in enumerate(zip(values[-68:-4],frames))if a!=b][:8]
    assert values[-4:]==[int(m[1],16),int(m[2],16),0,int(m[3])]and '_main returned'in native,native[-3000:]
    zones=[s.strip()for s in native.splitlines()if re.match(r'^(?:(?:57|58)\s+|max (?:57|58)\s+)',s)]
    report=dict(checks=int(m[3]),hash=m[1],pictures=m[2],sampled_full_frames=64,native_equal=True,zones=zones)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(report,flush=True)


if __name__=='__main__':main()
