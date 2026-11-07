#!/usr/bin/env python3
"""Source critical HP/MP/CT reactions through the shared action pipeline."""
import argparse,json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from check import HERE
from check_buffs import trial

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-critical-reactions/check');args=ap.parse_args();out=args.out.resolve()
    with ThreadPoolExecutor(max_workers=2)as pool:
        logic=pool.submit(trial,'critical_reactions','check_critical_reactions.c',out,args.pc_only)
        ui=pool.submit(trial,'critical_reactions_ui','check_critical_reactions_ui.c',out/'ui',args.pc_only)
        result=logic.result();result['ui']=ui.result()
    result['ui']['pictures']=18
    result.update(original_stages=87,original_effects=24,self_target_lists=3)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
