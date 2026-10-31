#!/usr/bin/env python3
"""Owned area packet through commands, real CT, impact and response drain."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13
import struct


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-area-ct/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_area_ct.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=HERE/'x/core-area-ct/fixture';fixture.mkdir(parents=True,exist_ok=True);(fixture/'ffgd.bin').write_bytes(descriptor13())
    catalogue=bytearray((HERE/'data/ffcj.bin').read_bytes());raw=struct.unpack_from('>H',catalogue,14)[0];norm=struct.unpack_from('>H',catalogue,30)[0]
    for i in range(4):
        r=bytearray(catalogue[raw+14*0xf:raw+14*0x10]);r[0]=4;r[1]=1;r[2]=255;r[4]=0;r[5]|=32;r[7]=128;r[10]=14;r[12]=3;r[13]=7
        if i==1:r[4]|=2
        if i==2:r[5]|=4
        catalogue[raw+14*(0x110+i):raw+14*(0x111+i)]=r
        value=bytearray(12);value[:8]=bytes((0,14,4,255,128|32|1|(16 if i==3 else 0),255,100,0));value[10]=8;value[11]=128
        catalogue[norm+12*(0x110+i):norm+12*(0x111+i)]=value
    (fixture/'ffcj.bin').write_bytes(catalogue)
    with tempfile.TemporaryDirectory(prefix='fft-area-ct-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        (data/'ffcj.bin').unlink();(data/'ffcj.bin').symlink_to(fixture/'ffcj.bin')
        pc=run([out/'check'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
        restart=run([out/'check','orbonne'],data);assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san','orbonne'],data)==restart
    print(pc.strip(),flush=True);print(restart.strip(),flush=True)
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'areact',*source,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    for name in ('ffgd','ffcj'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj ffov0 ffov1 ffov2 ffov3 ffoa ffoi ffost ffod ffs0'.split():
        cmd+=['--file',(fixture if bank in ('ffgd','ffcj')else HERE.parent/'opening/data' if bank in ('ffov0','ffov1','ffov2','ffov3','ffoa','ffoi','ffost','ffod','ffs0')else HERE/'data')/(bank+'.89y')]
    native=run([*cmd,out/'areact.89z']);(out/'native.log').write_text(native)
    m=re.search(r'area_ct (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-3000:]
    result=dict(checks=int(m[2]),hash=m[1],native_equal=True,
        zones=[line.strip()for line in native.splitlines()if re.match(r'^(?:(?:61|62|63)\s+|max (?:61|62|63)\s+)',line)])
    reboot=run([105 if x==61 else x for x in cmd]+[out/'areact.89z']);(out/'restart-native.log').write_text(reboot)
    r=re.search(r'area_restart (\w+) failures 0 checks (\d+)',restart);assert r
    v=[int(x,16)for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',reboot)]
    assert v[-3:]==[int(r[1],16),0,int(r[2])]and '_main returned'in reboot
    result['orbonne_restart']=dict(hash=r[1],checks=int(r[2]),native_equal=True)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
