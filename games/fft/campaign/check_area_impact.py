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
        # Move transport-envelope checks off the size-bound native renderer.
        # Every gameplay assertion and all64 exact pixel samples remain native.
        fixture=(data/'fip00.bin').read_bytes();stride=struct.unpack_from('>H',fixture,6)[0]
        units=struct.unpack_from('>H',(data/'ffgd.bin').read_bytes(),100)[0]
        assert fixture[:4]==b'FIP1' and struct.unpack_from('>H',fixture,4)[0]==8
        assert fixture[8:10]==bytes((units,14)) and stride==26+38*units
        assert len(fixture)==10+8*stride
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','fip00.bin','fip00'],data)
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffcj.bin','ffcj'],data)
        parts=[]
        for first,last in [(0,4),(4,6),(6,8)]:
            part=out/('part%d'%first);part.mkdir(exist_ok=True)
            flags=['-DAREA_IMPACT_FIRST=%d'%first,'-DAREA_IMPACT_LAST=%d'%last]
            for name,extra in [('check',[]),('check-san',['-fsanitize=address,undefined','-fno-sanitize=shift'])]:
                run(['cc','-std=gnu99','-O1','-g','-w','-o',part/name,*flags,*extra,*source,ROOT/'runtime/platform-sw/rt_sw.c'])
            partial=run([part/'check'],data)
            assert run(['env','ASAN_OPTIONS=detect_leaks=0',part/'check-san'],data)==partial
            pm=re.search(r'area_impact (\w+) pictures (\w+) failures 0 checks (\d+)',partial);assert pm
            run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES',*flags,'-o',part/'areahit',*source,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
            cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000,'--file',data/'fip00.89y']
            for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj ffflow'.split():
                cmd+=['--file',(data if bank=='ffcj'else HERE/'data')/(bank+'.89y')]
            native=run([*cmd,part/'areahit.89z']);(part/'native.log').write_text(native)
            values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
            frames=[int(v,16)for v in re.findall(r'frame \d+ \d+ (\w+)',partial)]
            assert len(frames)==8*(last-first) and values[-len(frames)-4:-4]==frames
            assert values[-4:]==[int(pm[1],16),int(pm[2],16),0,int(pm[3])]and '_main returned'in native,native[-3000:]
            zones=[s.strip()for s in native.splitlines()if re.match(r'^(?:(?:57|58)\s+|max (?:57|58)\s+)',s)]
            parts.append(dict(first=first,last=last,checks=int(pm[3]),hash=pm[1],pictures=pm[2],frames=frames,zones=zones))
    def rotate(v,n):
        n&=31
        return ((v<<n)|(v>>(32-n if n else 32)))&0xffffffff
    assert sum(p['checks']for p in parts)==int(m[3])
    combined=17;combined_pictures=17
    for p in parts:
        combined=rotate(combined,p['checks'])^int(p['hash'],16)^rotate(17,p['checks'])
        combined_pictures=rotate(combined_pictures,len(p['frames']))^int(p['pictures'],16)^rotate(17,len(p['frames']))
    assert '%08X'%combined==m[1]
    assert [v for p in parts for v in p['frames']]==[int(v,16)for v in re.findall(r'frame \d+ \d+ (\w+)',pc)]
    assert '%08X'%combined_pictures==m[2]
    report=dict(checks=int(m[3]),hash=m[1],pictures=m[2],sampled_full_frames=64,native_equal=True,
                native_parts=parts,native_recombined_hash='%08X'%combined)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(report,flush=True)


if __name__=='__main__':main()
