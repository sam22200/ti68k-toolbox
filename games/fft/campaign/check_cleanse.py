#!/usr/bin/env python3
"""Production cleansing family, supplied original profiles and native overlays."""
import argparse,json
from pathlib import Path
from check import HERE
from check_buffs import trial
from check_offensive_magic import catalogue_report
from common_tables import status_magic_record

def compiler_bounds():
    class Rows:
        def __init__(self,row):self.row=row
        def at(self,where,size):return self.row
    raw=bytearray(14);raw[8]=11
    assert status_magic_record(Rows(bytes((16,255,255,0,0,0))),raw)[0]==13
    for row in [(16,255,255,128,0,0),(16,0,0,0,0,0),(64,1,0,0,0,0)]:
        assert status_magic_record(Rows(bytes(row)),raw)is None

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-cleanse/check')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    compiler_bounds();coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('cleanse','check_cleanse.c',out,args.pc_only)
    result['edges']=trial('cleanse_edges','check_cleanse_edges.c',out/'edges',args.pc_only)
    result['render']=trial('cleanse_render','check_cleanse_render.c',out/'render',args.pc_only)
    result.update(added_entries=6,production_bank=coverage['bank_sha256'])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
