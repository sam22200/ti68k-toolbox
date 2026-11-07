#!/usr/bin/env python3
"""Approved identity bytes, malformed FCP2 boundaries and actual68000 equality."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,default=HERE/'x/core-factorization/check');args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    fixture=HERE/'x/core-factorization/oracle';fixture.mkdir(parents=True,exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python',HERE.parent/'tools/identity_reference.py',ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue',fixture/'fidref.bin'])
    common=[HERE/'check_identity.c',ROOT/'runtime/core/rt_core.c']
    for target,flags in [('identity',[]),('identity-san',['-fsanitize=address,undefined'])]:
        run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',out/target,*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-identities-')as name:
        data=Path(name);data_links(data);(data/'fidref.bin').symlink_to(fixture/'fidref.bin')
        pc=run([out/'identity'],data);san=run(['env','ASAN_OPTIONS=detect_leaks=0',out/'identity-san'],data);assert pc==san
    print(pc.strip(),flush=True)
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'identity',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','fidref.bin','fidref'],fixture)
    log=run([ROOT/'tools/bin/ti-cycles','--max',3000,'--frames',0,'--file',fixture/'fidref.89y','--file',HERE/'data/ffcp.89y',out/'identity.89z']);(out/'native.log').write_text(log)
    m=re.search(r'identity (\w+) failures 0 checks (\d+)',pc);assert m,pc
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])]and'_main returned'in log,log[-1800:]
    (out/'results.json').write_text(json.dumps(dict(checks=int(m[2]),hash=m[1],portraits=61,generic_names=510,malformed_cases=12,sanitizers_equal=True,native_equal=True),indent=2)+'\n')
if __name__=='__main__':main()
