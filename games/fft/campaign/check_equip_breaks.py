#!/usr/bin/env python3
"""Equipment breaks as one production batch."""
import argparse,json
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-equip-breaks/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial("equip_breaks","check_equip_breaks.c",out,args.pc_only)
    result.update(added_entries=4,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
