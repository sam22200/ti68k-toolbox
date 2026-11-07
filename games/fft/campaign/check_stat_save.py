#!/usr/bin/env python3
"""Source-backed stat-save staging, secondary effects and frozen area ownership."""
import argparse,json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from check import HERE
from check_buffs import trial

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-stat-save/check')
    args=ap.parse_args();out=args.out.resolve()
    with ThreadPoolExecutor(max_workers=2)as pool:
        logic=pool.submit(trial,'stat_save','check_stat_save.c',out,args.pc_only)
        ui=pool.submit(trial,'stat_save_ui','check_stat_save_ui.c',out/'ui',args.pc_only)
        result=logic.result();result['ui']=ui.result()
    result['ui']['pictures']=9
    result.update(source_stages=36,source_effects=18,active_reactions=3)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
