#!/usr/bin/env python3
"""Five compatible retail percentage/resource-transfer producers, PC and68000."""
import argparse,json
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-percent/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('percent','check_percent.c',out,args.pc_only)
    result.update(edges=trial('percent_edges','check_percent_edges.c',out/'edges',args.pc_only),runtime=trial('percent_runtime','check_percent_runtime.c',out/'runtime',args.pc_only),added_entries=5,catalogue_casts=len(coverage['batch']),production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
