#!/usr/bin/env python3
"""Source recoil in actual battle adapters, PC/sanitizers/actual68000."""
import json,re,tempfile
from pathlib import Path
from check_flow import HERE,ROOT,data_links
from check import run
from check_core_boundaries import descriptor13


def main():
    out=HERE/'x/core-knockback/battle';out.mkdir(parents=True,exist_ok=True)
    source=HERE/'check_knockback_battle.c';common=[source,ROOT/'runtime/core/rt_core.c']
    host=common+[ROOT/'runtime/platform-sw/rt_sw.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'bridge',*host])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined',
         '-fno-omit-frame-pointer','-fno-sanitize=shift','-o',out/'bridge-san',*host])
    bank=out/'data13';bank.mkdir(exist_ok=True);(bank/'ffgd.bin').write_bytes(descriptor13())
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],bank)
    with tempfile.TemporaryDirectory(prefix='fft-full-')as name:
        data=Path(name);data_links(data);(data/'ffgd.bin').unlink()
        (data/'ffgd.bin').symlink_to(bank/'ffgd.bin')
        pc=run([out/'bridge'],data)
        sanitized=run(['env','ASAN_OPTIONS=detect_leaks=0',out/'bridge-san'],data)
        assert sanitized==pc,(pc,sanitized)
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'bridgec',*common,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    files={p.stem:p for d in (HERE.parent/'opening/data',HERE/'data')for p in d.glob('*.89y')}
    files['ffgd']=bank/'ffgd.89y'
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--max',10000]
    for p in files.values():cmd+=['--file',p]
    native=run([*cmd,out/'bridgec.89z'])
    match=re.search(r'recoilbattle (\w+) failures 0 checks (\d+)',pc);assert match,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[2])],(pc,values)
    assert '_main returned'in native
    (out/'native.log').write_text(native)
    (out/'results.json').write_text(json.dumps(dict(checks=int(match[2]),hash=match[1],
        actors=13,nodes=[3,6],sanitizers=True,
        native_zones=[line.strip()for line in native.splitlines()if re.match(r'^[5-7]\s+(Legacy|Source|Classified) terrain edges',line)],scope='Source-bound recoil floors/headroom, chosen-floor occupancy/targetability, water, fall damage and feet invalidation; no raster claim.'),indent=2)+'\n')
    print(pc.strip(),flush=True)


if __name__=='__main__':main()
