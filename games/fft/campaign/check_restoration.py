#!/usr/bin/env python3
"""Five new restorative items and the whole seven-producer scalar family."""
import argparse,json
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-restoration/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('restoration','check_restoration.c',out,args.pc_only)
    result['edges']=trial('restoration_edges','check_restoration_edges.c',out/'edges',args.pc_only)
    result['input']=trial('restoration_input','check_restoration_input.c',out/'input',args.pc_only)
    result.update(added_entries=5,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
