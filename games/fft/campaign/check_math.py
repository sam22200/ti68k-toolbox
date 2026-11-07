#!/usr/bin/env python3
"""Math Skill original targeting and production PC/sanitizer/68000 checks."""
import argparse,json,re
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-math/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    names=('menu','preview','confirm','status')
    with ThreadPoolExecutor(max_workers=6)as pool:
        logic=pool.submit(trial,'math_skill','check_math.c',out,args.pc_only)
        ui=pool.submit(trial,'math_ui','check_math_ui.c',out/'ui',True)
        pending=[]if args.pc_only else [pool.submit(trial,'math_'+part,'check_math_ui_'+part+'.c',out/'ui'/part,False)for part in names]
        result=logic.result();result['ui']=ui.result();parts=[f.result()for f in pending]
    if not args.pc_only:
        native=(out/'native.log').read_text()
        result['ai_cycles']=int(re.search(r'^60\s+-\s+\d+\s+(\d+)',native,re.M)[1])
    if parts:
        def rotate(v,n):
            n&=31
            return ((v<<n)|(v>>(32-n if n else 32)))&0xffffffff
        assert sum(p['checks']for p in parts)==result['ui']['checks']
        combined=17
        for p in parts:
            p['frames']=int(re.search(r'^61\s+-\s+(\d+)',next(z for z in p['zones']if z.startswith('61 ')))[1])
            steps=p['checks']+p['frames']
            combined=rotate(combined,steps)^int(p['hash'],16)^rotate(17,steps)
        assert combined==int(result['ui']['hash'],16)
        result['ui'].update(native_equal=True,native_parts=parts,native_recombined_hash='%08X'%combined)
    result.update(production_bank=coverage['bank_sha256'],selectors=8)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
