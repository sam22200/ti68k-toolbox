#!/usr/bin/env python3
"""The AI planner and production order on the Chapter I techniques, PC/sanitizers/68000."""
import argparse,json
from pathlib import Path
from check import HERE
from check_buffs import trial

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-ai-techniques/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    result=trial("ai_techniques","check_ai_techniques.c",out,args.pc_only)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
