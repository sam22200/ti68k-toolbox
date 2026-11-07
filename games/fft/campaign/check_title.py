#!/usr/bin/env python3
"""Independent title slots on PC and actual68000, including both post-teardown writes."""
from pathlib import Path
from PIL import Image
import argparse,json,re,tempfile
from check_flow import HERE,ROOT,run,data_links,fixture
from check import keys
from review_files import write_changed
import save_dump

OUT=HERE/'x/qa/title/check'
def main():
    global OUT
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--variant',choices=['current','a','b'],default='current')
    ap.add_argument('--bank',type=Path);ap.add_argument('--out',type=Path)
    args=ap.parse_args();OUT=(args.out or OUT).resolve()
    bank=(args.bank or HERE/'data/fftitle.bin').resolve()
    OUT.mkdir(parents=True,exist_ok=True)
    flags=['-DFFT_TITLE_VARIANT='+str({'current':0,'a':1,'b':2}[args.variant])]
    common=[HERE/'check_title.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-w',*flags,'-o',OUT/'title',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES',*flags,'-o',OUT/'titlec',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    (OUT/'fftitle.bin').write_bytes(bank.read_bytes())
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','fftitle.bin','fftitle'],OUT)
    base=bytearray(fixture());base[4]=3;base[37]=7;base[298]=8
    two=bytearray(base);two[298]=22
    legacy=bytearray(base[:3110]);legacy[:4]=b'FSV2'
    bad=bytearray(base);bad[:4]=b'BAD!'
    populated={'ffsave1':bytes(base),'ffsave3':bytes(two)}
    selected=b'FSL1'+bytes((1,0))
    new=[(2,'ENTER'),(4,'DOWN'),(6,'ENTER')]
    load=[(2,'DOWN'),(4,'ENTER'),(6,'UP'),(8,'ENTER')]
    overwrite=[(2,'ENTER'),(4,'ENTER'),(6,'UP'),(8,'ENTER')]
    cases=[('empty',{},0,8,[],0),('empty-continue',{},2,8,[(2,'ENTER')],0),
           ('new-slot2',{},0,10,new,1),('populated',populated,2,8,[],0),
           ('load-slot3',populated,0,12,load,2),('overwrite-default',populated,0,10,[(2,'ENTER'),(4,'ENTER')],0),
           ('overwrite-return',populated,0,12,[(2,'ENTER'),(4,'ENTER'),(6,'ENTER')],0),
           ('overwrite-confirm',populated,0,12,overwrite,1),
           ('legacy-migrate',{'ffsave':bytes(legacy)},2,8,[(2,'ENTER')],2),
           ('damaged',{'ffsave3':bytes(bad)},2,10,[(2,'UP'),(4,'ENTER')],0),
           ('save-slots',{**populated,'ffslot':selected},3,8,[],0),
           ('save-cancel',{**populated,'ffslot':selected},3,8,[(2,'B')],0),
           ('save-slot2',{**populated,'ffslot':selected},3,10,[(2,'DOWN'),(4,'ENTER')],3),
           ('save-overwrite-default',{**populated,'ffslot':selected},3,10,[(2,'UP'),(4,'ENTER')],0),
           ('save-overwrite-return',{**populated,'ffslot':selected},3,12,[(2,'UP'),(4,'ENTER'),(6,'ENTER')],0),
           ('save-overwrite',{**populated,'ffslot':selected},3,14,[(2,'UP'),(4,'ENTER'),(6,'UP'),(8,'A')],3)]
    cases += [(f'glove-{i}',populated,0,4+4*i,[],0) for i in range(8)]
    if args.variant=='b':
        cases=[(name,initial,door,frames+(180 if not door else 0),
                [(f+180,k) for f,k in events] if not door else events,expected)
               for name,initial,door,frames,events,expected in cases]
        cases += [(f'reveal-{f}',{},0,f,[],0)for f in (1,30,60,86,100,114,145,179)]
        cases += [('skip-enter',{},0,8,[(2,'ENTER')],0),
                  ('skip-escape',{},0,8,[(2,'ESC')],0),
                  ('skip-new-slot2',{},0,12,[(2,'ENTER'),(4,'ENTER'),(6,'DOWN'),(8,'ENTER')],1)]
    results=[]
    for name,initial,door,frames,events,expected in cases:
        out=OUT/name;out.mkdir(exist_ok=True);script=keys(out/'input.keys',events)
        save_dir=out/'written';save_dir.mkdir(exist_ok=True)
        for old in save_dir.glob('*.89y'):old.unlink()
        with tempfile.TemporaryDirectory(prefix='fft-title-check-') as folder:
            data=Path(folder);data_links(data);ti_files=[]
            (data/'fftitle.bin').unlink();(data/'fftitle.bin').symlink_to(bank)
            for var,raw in initial.items():
                (data/(var+'.sav')).write_bytes(raw)
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav',var+'.sav',var],data)
                ti_files.append(data/(var+'.89y'))
            pc=run([OUT/'title',door,frames,script,out/'pc.png'],data)
            after={p.stem:p.read_bytes() for p in data.glob('*.sav')}
            command=[ROOT/'tools/bin/ti-cycles','--max',2000,'--arg',door,'--frames',frames,
                     '--keys',script,'--png',out/'ti.png','--save-dir',save_dir]
            command+=['--file',OUT/'fftitle.89y','--file',HERE/'data/ffmg.89y']
            if args.variant=='b':command+=['--file',HERE/'data/ffflow.89y']
            for path in ti_files:command+=['--file',path]
            ti=run([*command,OUT/'titlec.89z']);write_changed(out/'native.log',ti.encode())
            values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
            checksum=re.search(r'checksum (\w+)',pc)[1];state=int(re.search(r'state (\w+)',pc)[1],16)
            assert int(re.search(r'result (\d+)',pc)[1])==expected,(name,pc)
            assert checksum.lower()==re.search(r'checksum\s+(\w+)',ti)[1].lower(),(name,'pixels')
            # Normalize the backend preview palettes; compare every visible pixel,
            # since a short plane checksum can miss black/white changes.
            a=Image.open(out/'pc.png').convert('L').point(lambda v:(216-v)//64)
            b=Image.open(out/'ti.png').convert('L').crop((0,0,160,100)).point(lambda v:(255-v)//85)
            assert a.tobytes()==b.tobytes(),(name,'complete four-gray image')
            assert values[-3:]==[0x54495431,state,expected],(name,'state',values[-3:],pc)
            for var,raw in after.items():
                if initial.get(var)!=raw:
                    assert save_dump.payload(save_dir/(var+'.89y'))==raw,(name,var,'saved bytes')
                elif (save_dir/(var+'.89y')).exists():
                    assert save_dump.payload(save_dir/(var+'.89y'))==raw,(name,var,'untouched bytes')
            if expected:
                slot=after['ffslot'][4];saved=save_dump.decode(after['ffsave'+str(slot)])
                if expected==1:assert saved['party']==[] and saved['money']==2000
                elif expected==3:
                    assert after['ffsave'+str(slot)]==bytes(base),(name,'complete campaign snapshot')
                    for keep in (1,3):
                        if keep!=slot:assert after['ffsave'+str(keep)]==populated['ffsave'+str(keep)]
                else:assert saved['party'][0]['level']==(22 if slot==3 else 8)
                if name=='legacy-migrate':assert after['ffsave']==bytes(legacy)
            results.append(dict(case=name,checksum=checksum,state=state,result=expected));print(name,checksum,'PC/68000 ok',flush=True)
    write_changed(OUT/'results.json',(json.dumps(results,indent=2)+'\n').encode())
    print('Title:',len(results),'PC/68000 pixel/state/write cases passed')

if __name__=='__main__':main()
