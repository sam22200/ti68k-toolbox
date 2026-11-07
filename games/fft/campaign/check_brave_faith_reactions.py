#!/usr/bin/env python3
"""Brave Up/Faith Up/Sunken State, PC/sanitizers and actual68000."""
import argparse,json
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from check import HERE
from check_buffs import trial

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-brave-faith-reactions/check');args=ap.parse_args();out=args.out.resolve()
    with ThreadPoolExecutor(max_workers=2)as pool:
        logic=pool.submit(trial,'brave_faith_reactions','check_brave_faith_reactions.c',out,args.pc_only)
        ui=pool.submit(trial,'brave_faith_reactions_ui','check_brave_faith_reactions_ui.c',out/'ui',args.pc_only)
        result=logic.result();result['ui']=ui.result()
    result['ui']['pictures']=14
    result.update(original_stages=69,original_effects=37,self_target_lists=3)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
