#!/usr/bin/env python3
"""Actual area planner/empty execution, source accounting, PC/sanitizer/native quanta."""
import argparse,json,re,struct,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-area-ai/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_area_ai.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=HERE/'x/core-area-ai/oracle';(fixture/'ffgd.bin').write_bytes(descriptor13())
    catalogue=bytearray((HERE/'data/ffcj.bin').read_bytes());primary=struct.unpack_from('>H',catalogue,12)[0]
    raw=struct.unpack_from('>H',catalogue,14)[0];norm=struct.unpack_from('>H',catalogue,30)[0]
    catalogue[primary+8*0x93:primary+8*0x94]=catalogue[primary+8*0x10:primary+8*0x11]
    value=bytearray(catalogue[raw+14*0x10:raw+14*0x11]);value[2]=255;value[12]=3;value[13]=7
    catalogue[raw+14*0x93:raw+14*0x94]=value
    catalogue[norm+12*0x93:norm+12*0x94]=struct.pack('>8BH2B',0,14,4,255,128|32|1|2,255,100,0,600,8,128)
    for id in (0x94,0x95):
        for at,step in ((primary,8),(raw,14),(norm,12)):
            catalogue[at+step*id:at+step*(id+1)]=catalogue[at+step*0x93:at+step*0x94]
    catalogue[primary+8*0x94+6]|=64
    catalogue[raw+14*0x95+1]=255
    (fixture/'ffcj.bin').write_bytes(catalogue)
    with tempfile.TemporaryDirectory(prefix='fft-area-ai-')as folder:
        data=Path(folder);data_links(data)
        for name in ('ffgd','ffcj'):(data/(name+'.bin')).unlink();(data/(name+'.bin')).symlink_to(fixture/(name+'.bin'))
        (data/'fai00.bin').symlink_to(fixture/'fai00.bin')
        pc=run([out/'check'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'areaai',*source,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    for name in ('ffgd','ffcj','fai00'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj fai00'.split():
        cmd+=['--file',(fixture if bank in ('ffgd','ffcj','fai00')else HERE/'data')/(bank+'.89y')]
    native=run([*cmd,out/'areaai.89z']);(out/'native.log').write_text(native)
    m=re.search(r'area_ai (\w+) failures 0 checks (\d+) max_steps (\d+)',pc);assert m
    v=[int(x,16)for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert v[-3:]==[int(m[1],16),0,int(m[2])]and '_main returned'in native,native[-3500:]
    calls,origins,steps=v[-6:-3];assert calls<=2*13+origins and steps==int(m[3]),v[-6:]
    zones=[s.strip()for s in native.splitlines()if re.match(r'^(?:(?:52|53)\s+|max (?:52|53)\s+)',s)]
    maximum=int(re.search(r'^max 53\s+-\s+(\d+)',native,re.M)[1]);assert maximum<360000,maximum
    other=int(re.search(r'^max 52\s+-\s+(\d+)',native,re.M)[1])
    assert other<360000,other
    report=dict(checks=int(m[2]),hash=m[1],native_equal=True,max_steps=steps,cache_formula_calls=calls,
        cache_origins=origins,zones=zones,max_area_quantum_cycles=maximum,
        max_general_planner_quantum_cycles=other,general_peak_stage=v[-7],
        full_planner_360k_pass=other<360000,source_profiles=12)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(report,flush=True)


if __name__=='__main__':main()
