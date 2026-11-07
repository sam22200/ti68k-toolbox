#!/usr/bin/env python3
"""All ten Draw Out producers, PC/sanitizers/68000."""
import argparse,json,re,tempfile,sys
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13
from check_offensive_magic import catalogue_report
sys.path.insert(0,str(HERE.parent/'tools'))
import battle,core_audit
from common_tables import draw_out_record


def trial(label,source_name,out,pc_only):
    out.mkdir(parents=True,exist_ok=True)
    source=[HERE/source_name,ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=out/'fixture';fixture.mkdir(exist_ok=True);(fixture/'ffgd.bin').write_bytes(descriptor13())
    (fixture/'fdo00.bin').write_bytes((HERE/'x/core-draw-out/oracle/fdo00.bin').read_bytes())
    with tempfile.TemporaryDirectory(prefix='fft-draw-out-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        (data/'fdo00.bin').symlink_to(fixture/'fdo00.bin')
        pc=run([out/'check'],data)
        run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
             '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
        assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True);(out/'pc.log').write_text(pc)
    m=re.search(label+r' (\w+) failures 0 checks (\d+)',pc);assert m,pc
    result=dict(checks=int(m[2]),hash=m[1],sanitizers_equal=True)
    if not pc_only:
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'drawchk',*source,
             ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        for name in ('ffgd','fdo00'):run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
        for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj fdo00'.split():
            cmd+=['--file',(fixture if bank in ('ffgd','fdo00')else HERE/'data')/(bank+'.89y')]
        native=run([*cmd,out/'drawchk.89z']);(out/'native.log').write_text(native)
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(result['hash'],16),0,result['checks']]and '_main returned'in native,native[-3000:]
        result.update(native_equal=True,zones=[line.strip()for line in native.splitlines()
            if re.match(r'^(?:6[1-5]\s+|max 6[1-5]\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-draw-out/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    d=battle.Disc(str(core_audit.DEFAULT_DISC))
    for ab in range(76,86):
        r=d.at(battle.ABILITY_RANGE+14*ab,14);assert draw_out_record(d,ab,r)
        for i in range(14):
            bad=bytearray(r);bad[i]^=1;assert draw_out_record(d,ab,bad)is None,(ab,i)
    assert draw_out_record(d,86,r)is None
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('draw_out','check_draw_out.c',out,True)
    if not args.pc_only:
        math=trial('draw_out_math','check_draw_out_math.c',out/'math',False)
        runtime=trial('draw_out_runtime','check_draw_out_runtime.c',out/'runtime',False)
        assert math['checks']+runtime['checks']==result['checks']
        def rotate(v,n):
            n&=31
            return ((v<<n)|(v>>(32-n if n else 32)))&0xffffffff
        combined=rotate(int(math['hash'],16),runtime['checks'])^int(runtime['hash'],16)^rotate(17,runtime['checks'])
        assert '%08X'%combined==result['hash']
        result.update(native_equal=True,native_parts=[math,runtime],native_recombined_hash='%08X'%combined)
    result.update(added_entries=10,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
