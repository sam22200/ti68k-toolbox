#!/usr/bin/env python3
"""Compare archive restoration against a full-row historical PC oracle.

Defaults to PC plus sanitizers. --native adds one selected actual68000 carrier;
all saves, generated reference banks and output live in private directories.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import sys
import tempfile

from check_flow import HERE, ROOT, data_links, run
sys.path.insert(0,str(HERE.parent/'tools'))
import extract


def legacy(bank):
    """Restore only the former storage layout, retaining exact colors/depth/tail."""
    tail=b''
    if bank[-10:-6]==b'FC02':
        original,offset,count=struct.unpack('>3H',bank[-6:]);tail=bank[offset:offset+count];bank=bank[:original]
    assert bank[:2]==b'R4'
    a,b=struct.unpack_from('>2H',bank,2)
    c=extract.zx0pack.pack(bank[8:8+a]);m=extract.zx0pack.pack(bank[8+a:8+a+b])if b else b''
    out=(b'R3'if b else b'R2')+struct.pack('>2H',len(c),len(m))+bank[6:8]+c+m+bank[8+a+b:]
    if tail:
        n=len(out);off=(n+1)&~1;out+=bytes(off-n)+tail+b'FC02'+struct.pack('>3H',n,off,len(tail))
    return out


def masked(bank):
    """Synthetic non-shipping mask exercises the retained width-review path."""
    tail=b''
    if bank[-10:-6]==b'FC02':
        original,offset,count=struct.unpack('>3H',bank[-6:]);tail=bank[offset:offset+count];bank=bank[:original]
    a,b=struct.unpack_from('>2H',bank,2);colors=bank[8:8+a];plane=a//2
    mask=bytes((l^d)&0x81 for l,d in zip(colors[:plane],colors[plane:]));assert any(mask)
    out=b'R4'+struct.pack('>2H',a,plane)+bank[6:8]+colors+mask+bank[8+a+b:]
    if tail:
        n=len(out);off=(n+1)&~1;out+=bytes(off-n)+tail+b'FC02'+struct.pack('>3H',n,off,len(tail))
    return out


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/archive-reach-colours')
    ap.add_argument('--native',action='store_true')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    sources=[HERE/'check_range_cancel.c',ROOT/'runtime/core/rt_core.c']
    flags=['-DFFT_ARCHIVE_REACH_TEST','-DFFT_EDGE_WIDTH_REVIEW']
    for name,extra in (
        ('reference',['-DFFT_LEGACY_REACH_BACKUPS','-DFFT_REFERENCE_BK_ROWS=202']),('archive',[]),
        ('archive-san',['-fsanitize=address,undefined','-fno-sanitize=shift','-fno-pie','-no-pie'])):
        log=run(['cc','-std=gnu99','-O1','-g','-w',*flags,*extra,'-o',out/name,
                 *sources,ROOT/'runtime/platform-sw/rt_sw.c'])
        (out/(name+'-build.log')).write_text(log)
    results=[]
    for profile,scenarios in (('selected',(101,201)),('mask',(201,))):
        with tempfile.TemporaryDirectory(prefix='fft-archive-reference-')as folder, \
             tempfile.TemporaryDirectory(prefix='fft-archive-current-')as name:
            reference=Path(folder);data_links(reference);current=Path(name);data_links(current)
            if profile=='mask':
                fixture=out/'masked';fixture.mkdir(exist_ok=True)
                for rot in range(4):
                    path=current/f'ffgv{rot}.bin';raw=masked(path.read_bytes())
                    path.unlink();path.write_bytes(raw);(fixture/path.name).write_bytes(raw)
            for path in list(current.glob('ff?v[0-3].bin')):
                raw=path.read_bytes();assert extract.archive_view(raw)==raw
                converted=legacy(raw);assert extract.archive_view(converted)==raw
                dest=reference/path.name;dest.unlink();dest.write_bytes(converted)
            for scenario in scenarios:
                records=[];frames=[]
                for program,data in (('reference',reference),('archive',current),('archive-san',current)):
                    log=run(['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',
                             out/program,scenario],data)
                    (out/f'{profile}-{scenario}-{program}.log').write_text(log)
                    match=re.search(r'cancel (\w+) failures 0 checks (\d+)',log);assert match,log
                    records.append((match[1],int(match[2])))
                    frames.append(re.findall(r'^range (.*)$',log,re.M))
                assert len({r[0]for r in records})==1,records
                assert records[1]==records[2],records
                assert frames[0]==frames[1]==frames[2] and len(frames[1])==64,frames
                results.append(dict(profile=profile,scenario=scenario,hash=records[1][0],checks=records[1][1],
                                    reference_checks=records[0][1],sanitizers_equal=True,scene_and_screen_pairs=frames[1]))
                print('PC/reference/sanitizers:',profile,scenario,records[1],flush=True)
    if args.native:
        build=run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES',*flags,'-o',out/'reachc',*sources,
                   ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        (out/'native-build.log').write_text(build)
        files={p.stem:p for origin in(HERE.parent/'opening/data',HERE/'data')for p in origin.glob('*.89y')}
        for rot in range(4):
            stem=f'ffgv{rot}';fixture=out/'masked'
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',stem+'.bin',stem],fixture)
            files[stem]=fixture/(stem+'.89y')
        # One native scenario traverses all four source views, both styles,
        # arbitrary sparse/dense ranges, first-render cancel and forced reload.
        cmd=[ROOT/'tools/bin/ti-cycles','--arg',201,'--frames',0,'--max',10000]
        for path in files.values():cmd+=['--file',path]
        log=run([*cmd,out/'reachc.89z']);(out/'native.log').write_text(log)
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
        row=results[-1];assert values[-3:]==[int(row['hash'],16),0,row['checks']],values[-8:]
        expected=[int(word,16)for frame in row['scene_and_screen_pairs']for word in frame.split()[-2:]]
        assert values[-131:-3]==expected,'Native range scene/screen differs'
        row['native_equal']=True
        print('Selected68000:',row['profile'],row['scenario'],row['hash'],row['checks'],'checks; 128 phase hashes agree',flush=True)
    (out/'reach-results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
