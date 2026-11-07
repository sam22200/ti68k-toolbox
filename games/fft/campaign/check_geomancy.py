#!/usr/bin/env python3
"""The twelve retail Geomancy actions and both terrain layers, PC/sanitizers/68000."""
import argparse,json,re,sys
from pathlib import Path
from check import HERE,run
from check_buffs import trial
from check_offensive_magic import catalogue_report
sys.path.insert(0,str(HERE.parent/'tools'))
from common_tables import geomancy_record
import battle,core_audit,surface_data


def data_checks():
    d=battle.Disc(str(core_audit.DEFAULT_DISC))
    for ab in range(126,138):
        r=d.at(battle.ABILITY_RANGE+14*ab,14);assert geomancy_record(d,ab,r)
        for i in range(14):
            bad=bytearray(r);bad[i]^=1;assert geomancy_record(d,ab,bad)is None,(ab,i)
    # Native descriptors carry precisely the source type on both layers; the
    # collision prefix remains byte-identical to its established FGS1 encoding.
    disc,files=surface_data.extract.disc_files(str(core_audit.DEFAULT_DISC))
    for folder,number,name in(('campaign',22,'ffgd'),('opening',56,'ffod')):
        raw,meta,provenance=surface_data.terrain(disc,files,number)
        _,actual=surface_data.detach_descriptor((HERE.parent/folder/'data'/f'{name}.bin').read_bytes())
        expected=surface_data.encode(raw,meta['width'],meta['height'],with_terrain=True)
        assert actual==expected,(folder,provenance)
        old=surface_data.encode(raw,meta['width'],meta['height'])
        assert actual[4:len(old)]==old[4:]


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-geomancy/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True);data_checks()
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    result=trial('geomancy','check_geomancy.c',out,True)
    if not args.pc_only:
        math=trial('geomancy_math','check_geomancy_math.c',out/'math',True)
        first=trial('geomancy_math_first','check_geomancy_math_first.c',out/'math-first',False)
        last=trial('geomancy_math_last','check_geomancy_math_last.c',out/'math-last',False)
        runtime=trial('geomancy_runtime','check_geomancy_runtime.c',out/'runtime',False)
        assert math['checks']+runtime['checks']==result['checks']
        def rotate(v,n):
            n&=31
            return ((v<<n)|(v>>(32-n if n else 32)))&0xffffffff
        assert first['checks']+last['checks']==math['checks']
        joined=rotate(int(first['hash'],16),last['checks'])^int(last['hash'],16)^rotate(17,last['checks'])
        assert '%08X'%joined==math['hash']
        math.update(native_equal=True,native_parts=[first,last],native_recombined_hash='%08X'%joined)
        combined=rotate(int(math['hash'],16),runtime['checks'])^int(runtime['hash'],16)^rotate(17,runtime['checks'])
        assert '%08X'%combined==result['hash']
        runtime['zones'] += [line.strip()for line in (out/'runtime/native.log').read_text().splitlines()
            if re.match(r'^(?:5[89]\s+|max 5[89]\s+)',line)]
        result.update(native_equal=True,native_parts=[math,runtime],native_recombined_hash='%08X'%combined)
    result.update(added_entries=12,production_bank=coverage['bank_sha256'],source_descriptors_equal=True)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')


if __name__=='__main__':main()
