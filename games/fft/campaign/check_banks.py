#!/usr/bin/env python3
"""FMB3 parser, exact context binding and failure-before-execution on actual68000."""
import argparse,json,re,struct,tempfile
from pathlib import Path
from check import HERE,ROOT,run

def values(log):return [int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,default=HERE/'x/core-banks/check')
    ap.add_argument('--previous-bank-dir',type=Path,help='also reject a real previous two-shard export')
    args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    run(['cc','-std=gnu99','-O1','-g','-fsanitize=address,undefined','-o',out/'parser',HERE/'check_module_bank.c'])
    pc=run([out/'parser']);(out/'parser-pc.log').write_text(pc);print(pc.strip(),flush=True)
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'mbparse',HERE/'check_module_bank.c'])
    native=run([ROOT/'tools/bin/ti-cycles','--frames',0,'--max',100,out/'mbparse.89z']);(out/'parser-native.log').write_text(native)
    m=re.search(r'modules (\w+) failures 0 checks (\d+)',pc);assert m
    assert values(native)[-3:]==[int(m[1],16),0,int(m[2])]
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'mbcalls',HERE/'check_banks.c',ROOT/'runtime/core/rt_core.c',ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    files={p.stem:p for origin in (HERE.parent/'opening/data',HERE/'data')for p in origin.glob('*.89y')if not p.stem.startswith('ffsave')and p.stem!='ffslot'}
    def execute(label,overrides):
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
        for name,path in {**files,**overrides}.items():
            if path:cmd+=['--file',path]
        log=run([*cmd,out/'mbcalls.89z']);(out/(label+'.log')).write_text(log);assert '_main returned'in log
        return log
    normal=execute('bindings',{});v=values(normal);assert v[-4]==1 and v[-2]==0 and v[-1]>100,v
    zones=[s.strip()for s in normal.splitlines()if re.match(r'^(?:(?:61|62|63)\s+|max (?:61|62|63)\s+)',s)]
    rejected={}
    with tempfile.TemporaryDirectory(prefix='fft-banks-bad-')as folder:
        temp=Path(folder)
        def bank(label,name,data):
            dest=temp/label;dest.mkdir();(dest/(name+'.bin')).write_bytes(data)
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],dest)
            return dest/(name+'.89y')
        a=(HERE/'data/ffbm.bin').read_bytes();b=(HERE/'data/ffbn.bin').read_bytes()
        corrupt=bytearray(b);corrupt[12]^=1
        first=struct.unpack_from('>H',b,14)[0];context=struct.unpack_from('>H',b,first+8)[0]
        badsig=bytearray(b);badsig[first+10+context:first+12+context]=b'\0\0'
        previous=bytearray(b)
        signature=struct.unpack_from('>H',b,first+10+context)[0]
        struct.pack_into('>H',previous,first+10+context,(signature-257)&65535)
        # A valid one-image historical envelope suffices for magic rejection.
        # Recombining every current image would exceed the AMS variable limit.
        at=struct.unpack_from('>H',a,14)[0];n=struct.unpack_from('>H',a,at+6)[0]
        end=struct.unpack_from('>H',a,16)[0] if struct.unpack_from('>H',a,6)[0]>1 else len(a)
        records=[a[at:end]]
        legacy=bytearray(b'FMB2')+struct.pack('>H',len(records))+bytes(2*len(records))
        for i,record in enumerate(records):struct.pack_into('>H',legacy,6+2*i,len(legacy));legacy+=record
        cases=[('missing',{'ffbn':None}),('missing-third',{'ffbo':None}),('third-truncated',{'ffbo':bank('third-short','ffbo',b'FMB3')}),('token',{'ffbn':bank('token','ffbn',corrupt)}),
            ('truncated',{'ffbn':bank('short','ffbn',b[:16])}),('signature',{'ffbn':bank('signature','ffbn',badsig)}),
            ('legacy',{'ffbm':bank('legacy','ffbm',legacy)}),
            ('swapped',{'ffbm':bank('swapa','ffbm',b),'ffbn':bank('swapb','ffbn',a)}),
            ('previous-abi',{'ffbn':bank('previous-abi','ffbn',previous)})]
        if args.previous_bank_dir:
            cases.append(('previous-export',{
                name:bank('previous-export-'+name,name,(args.previous_bank_dir/(name+'.bin')).read_bytes())
                for name in('ffbm','ffbn')}))
        for label,override in cases:
            log=execute(label,override);assert values(log)[-4:]==[0,17,0,0],(label,values(log))
            rejected[label]=dict(accepted=False,bindings_or_module_calls=0)
    count=struct.unpack_from('>H',(HERE/'data/ffbm.bin').read_bytes(),8)[0]
    report=dict(parser=dict(hash=m[1],checks=int(m[2]),native_equal=True),bindings=dict(hash=f'{v[-3]:08X}',checks=v[-1],exact_old_context_equal=True,cold_loads=2*count,warm_preloads=2*count,stale_chapter_rebindings=3,zones=zones),rejected=rejected,live_tiemu_touched=False)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':main()
