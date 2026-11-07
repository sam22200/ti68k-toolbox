#!/usr/bin/env python3
"""Counter Tackle/Flood and Absorb Used MP, portable/native owned effects."""
import argparse,json
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from check import HERE
from check_buffs import trial

def rotate(v,n):
    n&=31
    return ((v<<n)|(v>>(32-n if n else 32)))&0xffffffff

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-counter-reactions/check');args=ap.parse_args();out=args.out.resolve()
    with ThreadPoolExecutor(max_workers=4)as pool:
        logic=pool.submit(trial,'counter_reactions','check_counter_reactions.c',out,args.pc_only)
        ui=pool.submit(trial,'counter_reactions_ui','check_counter_reactions_ui.c',out/'ui',True)
        parts=[]
        if not args.pc_only:
            parts=[pool.submit(trial,'counter_reactions_ui',f'check_counter_ui_{label}.c',out/'ui'/label,False)
                   for label in('first','flood','absorb')]
        result=logic.result();result['ui']=ui.result()
        if parts:
            values=[p.result()for p in parts];combined=17
            for part,pictures in zip(values,(3,3,6)):
                part['pictures']=pictures;n=part['checks']+pictures
                combined=rotate(combined,n)^int(part['hash'],16)^rotate(17,n)
            assert sum(p['checks']for p in values)==result['ui']['checks']
            assert '%08X'%combined==result['ui']['hash'],(values,result['ui'])
            result['ui'].update(native_equal=True,native_parts=values,native_recombined_hash='%08X'%combined)
    result['ui']['pictures']=12
    result.update(original_stages=60,original_absorb_effects=9,original_constructors=4)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
