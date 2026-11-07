#!/usr/bin/env python3
"""All fourteen persistent performances, PC/sanitizers/68000."""
import argparse,json,sys
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report
sys.path.insert(0,str(HERE.parent/'tools'))
import battle,core_audit
from common_tables import performance_record


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-performance/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    d=battle.Disc(str(core_audit.DEFAULT_DISC))
    for ab in range(86,100):
        r=d.at(battle.ABILITY_RANGE+14*ab,14);assert performance_record(d,ab,r)
        for i in range(14):
            bad=bytearray(r);bad[i]^=1;assert performance_record(d,ab,bad)is None,(ab,i)
    assert performance_record(d,100,r)is None
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('performance','check_performance.c',out,True)
    if not args.pc_only:
        math=trial('performance_math','check_performance_math.c',out/'math',False)
        runtime=trial('performance_runtime','check_performance_runtime.c',out/'runtime',False)
        assert math['checks']+runtime['checks']==result['checks']
        def rotate(v,n):
            n&=31
            return ((v<<n)|(v>>(32-n if n else 32)))&0xffffffff
        combined=rotate(int(math['hash'],16),runtime['checks'])^int(runtime['hash'],16)^rotate(17,runtime['checks'])
        assert '%08X'%combined==result['hash']
        result.update(native_equal=True,native_parts=[math,runtime],native_recombined_hash='%08X'%combined)
    result.update(added_entries=14,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')


if __name__=='__main__':main()
