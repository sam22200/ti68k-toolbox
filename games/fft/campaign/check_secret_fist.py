#!/usr/bin/env python3
"""Retail physical-status family50: Secret Fist, PC/sanitizers/68000."""
import argparse,json,sys
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report
sys.path.insert(0,str(HERE.parent/'tools'))
import battle,core_audit
from common_tables import secret_fist_record


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-secret-fist/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    d=battle.Disc(str(core_audit.DEFAULT_DISC));r=d.at(battle.ABILITY_RANGE+14*104,14)
    assert secret_fist_record(d,104,r)==(10,61,1,0,160)
    for i in range(14):
        bad=bytearray(r);bad[i]^=1;assert secret_fist_record(d,104,bad)is None,i
    assert secret_fist_record(d,0x11b,r)is None  # no implicit weapon/monster producer
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('secret_fist','check_secret_fist.c',out,args.pc_only)
    result.update(added_entries=1,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')


if __name__=='__main__':main()
