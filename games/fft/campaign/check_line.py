#!/usr/bin/env python3
"""Earth Slash and shared cardinal line targeting, PC/sanitizers/68000."""
import argparse,json,re,tempfile,sys
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13
from check_offensive_magic import catalogue_report
sys.path.insert(0,str(HERE.parent/'tools'))
import battle,core_audit
from common_tables import line_record


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-line/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    d=battle.Disc(str(core_audit.DEFAULT_DISC));r=d.at(battle.ABILITY_RANGE+14*103,14)
    assert line_record(103,r)==(0,0,8,255,160)
    for i in range(14):
        bad=bytearray(r);bad[i]^=1;assert line_record(103,bad)is None,i
    assert line_record(102,r)is None
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    source=[HERE/'check_line.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=out/'fixture';fixture.mkdir(exist_ok=True);(fixture/'ffgd.bin').write_bytes(descriptor13())
    (fixture/'flg00.bin').write_bytes((HERE/'x/core-line/oracle/flg00.bin').read_bytes())
    with tempfile.TemporaryDirectory(prefix='fft-line-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        (data/'flg00.bin').symlink_to(fixture/'flg00.bin')
        pc=run([out/'check'],data)
        run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
             '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
        assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True);(out/'pc.log').write_text(pc)
    m=re.search(r'line (\w+) failures 0 checks (\d+)',pc);assert m,pc
    result=dict(checks=int(m[2]),hash=m[1],sanitizers_equal=True)
    if not args.pc_only:
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'linechk',*source,
             ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        for name in ('ffgd','flg00'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
        for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj flg00'.split():
            cmd+=['--file',(fixture if bank in ('ffgd','flg00')else HERE/'data')/(bank+'.89y')]
        native=run([*cmd,out/'linechk.89z']);(out/'native.log').write_text(native)
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(result['hash'],16),0,result['checks']]and '_main returned'in native,native[-3000:]
        result.update(native_equal=True,zones=[line.strip()for line in native.splitlines()
            if re.match(r'^(?:6[1-5]\s+|max 6[1-5]\s+)',line)])
    result.update(added_entries=1,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
