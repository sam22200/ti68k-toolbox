#!/usr/bin/env python3
"""Float: source formula/movement evidence, owned status and native navigation."""
import argparse,json,sys,re
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report
sys.path.insert(0,str(HERE.parent/'tools'))
import battle,core_audit
from common_tables import float_record

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-float/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    d=battle.Disc(str(core_audit.DEFAULT_DISC));r=d.at(battle.ABILITY_RANGE+14*38,14);assert float_record(d,r)
    for i in range(14):
        bad=bytearray(r);bad[i]^=1;assert float_record(d,bad)is None,i
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('floating','check_float.c',out,args.pc_only)
    if not args.pc_only:
        result['zones'] += [line.strip()for line in (out/'native.log').read_text().splitlines()
            if re.match(r'^(?:59\s+|60\s+|max (?:59|60)\s+)',line)]
    result.update(added_entries=1,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
