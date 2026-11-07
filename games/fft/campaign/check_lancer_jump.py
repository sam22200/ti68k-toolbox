#!/usr/bin/env python3
"""Lancer Jump: local source evidence, native lifecycle and bounded layer policy."""
import argparse,json
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-lancer-jump/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    report=catalogue_report();assert report['generic_actions']==dict(total=201,enabled=201,remaining=0)
    (out/'coverage.json').write_text(json.dumps(report,indent=2)+'\n')
    result=trial('lancer_jump','check_lancer_jump.c',out,args.pc_only)
    result['ui']=trial('lancer_jump_ui','check_lancer_jump_ui.c',out/'ui',args.pc_only)
    result['ui']['pictures']=16
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
