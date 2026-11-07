#!/usr/bin/env python3
"""Pure compound forecasts: original preview groups, PC/sanitizers and native images."""
import argparse,json,re,struct,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-area-forecast/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_area_forecast.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=HERE/'x/core-area-forecast/oracle';fixture.mkdir(parents=True,exist_ok=True)
    (fixture/'ffgd.bin').write_bytes(descriptor13())
    rows=json.loads((fixture/'source.json').read_text())['cases']
    catalogue=bytearray((HERE/'data/ffcj.bin').read_bytes());raw=struct.unpack_from('>H',catalogue,14)[0]
    norm=struct.unpack_from('>H',catalogue,30)[0]
    for i,row in enumerate([*rows,*([rows[1]]*4)]):
        parent=0x10 if row['evade']else 0xf
        r=bytearray(catalogue[raw+14*parent:raw+14*(parent+1)])
        r[1]=0 if i==4 else 255;r[2]=255;r[4]=1 if i==5 else 0
        r[7]=row['element'];r[10]=row['power'];r[13]=7
        if i==7:r[5]|=4
        catalogue[raw+14*(0x100+i):raw+14*(0x101+i)]=r
        value=bytearray(12);value[:8]=bytes((0,row['power'],5,255,128|32|1|(2 if row['evade']else 0)|(16 if i==6 else 0),255,100,0))
        value[10]=8;value[11]=row['element'];catalogue[norm+12*(0x100+i):norm+12*(0x101+i)]=value
    (fixture/'ffcj.bin').write_bytes(catalogue)
    with tempfile.TemporaryDirectory(prefix='fft-area-forecast-')as folder:
        data=Path(folder);data_links(data)
        for name in ('ffgd','ffcj'):(data/(name+'.bin')).unlink();(data/(name+'.bin')).symlink_to(fixture/(name+'.bin'))
        for name,origin in [('fbf00',fixture),('fat00',HERE/'x/core-targets/oracle')]:
            (data/(name+'.bin')).symlink_to(origin/(name+'.bin'))
        pc=run([out/'check'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'forecst',*source,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    for name in ('ffgd','ffcj','fbf00'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
    target=HERE/'x/core-targets/oracle'
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','fat00.bin','fat00'],target)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj fbf00 fat00'.split():
        origin=fixture if bank in ('ffgd','ffcj','fbf00')else target if bank=='fat00'else HERE/'data'
        cmd+=['--file',origin/(bank+'.89y')]
    native=run([*cmd,out/'forecst.89z']);(out/'native.log').write_text(native)
    m=re.search(r'forecast (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])]and '_main returned'in native,native[-3000:]
    report=dict(checks=int(m[2]),hash=m[1],native_equal=True,source_groups=len(rows),
        zones=[s.strip()for s in native.splitlines()if re.match(r'^(?:(?:58|59)\s+|max (?:58|59)\s+)',s)])
    assert any(s.startswith('58 ')for s in report['zones'])and any(s.startswith('59 ')for s in report['zones'])
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(report,flush=True)


if __name__=='__main__':main()
