#!/usr/bin/env python3
"""Throw selected-item commands, PC/sanitizer/native equality."""
import argparse,json
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-throw/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('throwing','check_throw.c',out,args.pc_only)
    result['ui']=trial('throw_ui','check_throw_ui.c',out/'ui',args.pc_only)
    result.update(production_bank=coverage['bank_sha256'],added_selectors=12)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
