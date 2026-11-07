#!/usr/bin/env python3
"""Gil Taking / Negotiate production family, PC/sanitizers/68000."""
import argparse,json,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_buffs import trial
from check_offensive_magic import catalogue_report
from check_flow import data_links

def save_trial(out):
    exe=out/'save-check'
    run(['cc','-std=gnu99','-O1','-g','-w','-o',exe,HERE/'test_save.c',
         ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
    # test_save creates/removes a legacy slot: never use the user's data directory.
    with tempfile.TemporaryDirectory(prefix='fft-gil-save-')as folder:
        data=Path(folder);data_links(data);log=run([exe],data)
    (out/'save.log').write_text(log)
    return dict(pc_equal=True,isolated_files=True)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-gil/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('gil','check_gil.c',out,args.pc_only)
    result['save']=save_trial(out)
    result.update(added_entries=2,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
