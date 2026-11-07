#!/usr/bin/env python3
"""Talk Skill as one production batch."""
import argparse,json
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-talk-skill/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('talk','check_talk.c',out,True)
    if not args.pc_only:
        math=trial('talk_math','check_talk_math.c',out/'math',False)
        runtime=trial('talk_runtime','check_talk_runtime.c',out/'runtime',False)
        assert math['checks']+runtime['checks']==result['checks']
        def rotate(v,n):
            n&=31
            return ((v<<n)|(v>>(32-n if n else 32)))&0xffffffff
        combined=rotate(int(math['hash'],16),runtime['checks'])^int(runtime['hash'],16)^rotate(17,runtime['checks'])
        assert '%08X'%combined==result['hash']
        result.update(native_equal=True,native_parts=[math,runtime],native_recombined_hash='%08X'%combined)
    result.update(added_entries=7,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
