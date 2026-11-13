#!/usr/bin/env python3
"""Isolated flow fixtures and actual-renderer review, never the user's live save/TiEmu."""
from pathlib import Path
import argparse,json,re,struct,subprocess,sys,tempfile
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OUT=HERE/'x/qa/flow'
sys.path.insert(0,str(HERE.parent/'tools'))
import save_dump
from review_files import publish_images,write_changed

def run(args,cwd=HERE):
    argv=list(map(str,args))
    with tempfile.TemporaryDirectory(prefix='fft-flow-images-') as name:
        destination=None
        if '--png' in argv:
            i=argv.index('--png')+1;destination=Path(argv[i]);argv[i]=str(Path(name)/destination.name)
        r=subprocess.run(argv,cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        if r.returncode:print(r.stdout);r.check_returncode()
        if destination:publish_images(name,destination.parent)
        return r.stdout

def fixture():
    q=(HERE/'data/ffflow.bin').read_bytes();a,d=struct.unpack_from('>2H',q,4)
    roster=q[a:a+1493];dep=q[d:d+79]
    b=bytearray(b'FSV3'+bytes(32)+bytes((7,7))+roster[5:261]+roster[261:]+bytes(9*176))
    assert len(b)==3110
    cells=[dep[4+3*i:7+3*i] for i in range(25) if dep[4+3*i]]
    placements=b''.join(bytes((slot,c[1],c[2],dep[3])) for slot,c in zip((0,2,3,4,6),cells))
    b+=roster[1:5]+bytes((2,5))+struct.pack('>H',200)+placements
    b[4]=1;assert len(b)==3138
    return bytes(b)

def data_links(folder,source=None):
    for origin in ((source,) if source else (HERE.parent/'opening/data',HERE/'data')):
        for src in origin.glob('*.bin'):
            p=folder/src.name;p.unlink(missing_ok=True);p.symlink_to(src)

def main():
    global OUT
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',action='store_true')
    ap.add_argument('--out',type=Path,default=OUT);ap.add_argument('--slot',type=int,choices=(1,2,3))
    ap.add_argument('--data-dir',type=Path,help='validate actual installed pack data in isolation')
    args=ap.parse_args();OUT=args.out.resolve();save_name='ffsave'   # the session; a --slot is seeded and must stay untouched
    if args.data_dir:args.data_dir=args.data_dir.resolve()
    OUT.mkdir(parents=True,exist_ok=True);initial=b'FSV5'+fixture()[4:]+bytes(66);write_changed(OUT/'initial.sav',initial)
    run(['cc','-std=gnu99','-O1','-g','-w','-o',OUT/'battle',HERE/'check_flow_battle.c',
         ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-flow-') as name:
        temp=Path(name);data_links(temp,args.data_dir);shots=temp/'shots';shots.mkdir()
        if args.slot:
            (temp/'ffslot.sav').write_bytes(b'FSL1'+bytes((args.slot,0)))
            (temp/f'ffsave{args.slot}.sav').write_bytes(initial)
        (temp/(save_name+'.sav')).write_bytes(initial)
        pc=run([OUT/'battle',shots],temp);write_changed(OUT/'pc.log',pc.encode());print(pc.strip())
        saved=(temp/(save_name+'.sav')).read_bytes();write_changed(OUT/'settled.sav',saved)
        if args.slot:assert (temp/f'ffsave{args.slot}.sav').read_bytes()==initial,'a battle never writes a slot'
        result=save_dump.decode(saved);assert result['money']==4500 and len(result['party'])==7,result
        assert result['inventory'][2]==1 and result['inventory'][253]==initial[38+253]+1
        # The ordinary KO and the two unselected reserve records must survive.
        for slot in (1,5):assert saved[294+slot*176:294+(slot+1)*176]==initial[294+slot*176:294+(slot+1)*176]
        orbonne=run([OUT/'battle',shots,'orbonne'],temp);write_changed(OUT/'orbonne-pc.log',orbonne.encode());print(orbonne.strip())
        publish_images(shots,OUT/'new')
        repeat=run([OUT/'battle'],temp);assert save_dump.decode((temp/(save_name+'.sav')).read_bytes())['money']==4500
        write_changed(OUT/'repeat.log',repeat.encode())
        overflow=bytearray(initial);overflow[38+2]=99;(temp/(save_name+'.sav')).write_bytes(overflow)
        run([OUT/'battle'],temp)
        overflow_saved=(temp/(save_name+'.sav')).read_bytes();decoded=save_dump.decode(overflow_saved)
        q=(HERE/'data/ffflow.bin').read_bytes();price=struct.unpack_from('>H',q,struct.unpack_from('>H',q,12)[0]+4)[0]
        assert decoded['money']==4500+price and decoded['inventory'][2]==99
    if args.native:
        # Keep each instrumented fixture below AMS's per-variable limit without
        # dropping any checks or packing a checker the headless runner cannot load.
        pc_variants={}
        for program,flag in (('flowgar','FLOW_GARILAND_RESULTS_ONLY'),('flowpost','FLOW_GARILAND_AFTERMATH_ONLY'),('flowpre','FLOW_GARILAND_EARLY_ONLY'),
                             ('floworb','FLOW_ORBONNE_ONLY'),('flowhit','FLOW_FIRST_FRAME_ONLY')):
            run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-D'+flag,'-o',OUT/program,HERE/'check_flow_battle.c',
                 ROOT/'runtime/core/rt_core.c',ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
            run(['cc','-std=gnu99','-O1','-w','-D'+flag,'-o',OUT/(program+'pc'),HERE/'check_flow_battle.c',
                 ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
            with tempfile.TemporaryDirectory(prefix='fft-flow-split-')as name:
                data=Path(name);data_links(data,args.data_dir);(data/(save_name+'.sav')).write_bytes(initial)
                if args.slot:(data/'ffslot.sav').write_bytes(b'FSL1'+bytes((args.slot,0)))
                argv=[OUT/(program+'pc')]
                if program=='floworb':
                    shots=data/'shots';shots.mkdir();argv+=[shots,'orbonne']
                pc_variants[program]=run(argv,data)
        (OUT/(save_name+'.sav')).write_bytes(initial)
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav',save_name+'.sav',save_name],OUT)
        files={}
        for origin in ((args.data_dir,) if args.data_dir else (HERE.parent/'opening/data',HERE/'data')):
            for p in origin.glob('*.89y'):files[p.stem]=p
        files[save_name]=OUT/(save_name+'.89y')
        if args.slot:
            (OUT/'ffslot.sav').write_bytes(b'FSL1'+bytes((args.slot,0)))
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','ffslot.sav','ffslot'],OUT)
            files['ffslot']=OUT/'ffslot.89y'
        (OUT/'native-save').mkdir(exist_ok=True)
        # Several split fixtures reuse native.png. Publish only the settled
        # final captures, so temporary pictures cannot refresh an unchanged date.
        pending_shots=tempfile.TemporaryDirectory(prefix='fft-flow-native-shots-')
        images=Path(pending_shots.name)
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',261,'--max',10000,'--png',images/'native.png','--save-dir',OUT/'native-save']
        for p in files.values():cmd+=['--file',p]
        ti=run([*cmd,OUT/'flowgar.89z'],OUT);write_changed(OUT/'native.log',ti.encode())
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        m=re.search(r'flow ([0-9A-F]+) failures (\d+) checks (\d+)',pc_variants['flowgar'])
        assert values[-3:]==[int(m[1],16),0,int(m[3])],(values[-5:],pc)
        assert save_dump.payload(OUT/'native-save'/(save_name+'.89y'))==saved
        ti=run([*cmd,OUT/'flowpost.89z'],OUT);write_changed(OUT/'aftermath-native.log',ti.encode())
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        m=re.search(r'flow ([0-9A-F]+) failures (\d+) checks (\d+)',pc_variants['flowpost'])
        assert values[-3:]==[int(m[1],16),0,int(m[3])],('aftermath',values[-3:],pc_variants['flowpost'])
        ti=run([*cmd,OUT/'flowpre.89z'],OUT);write_changed(OUT/'prebattle-native.log',ti.encode())
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        m=re.search(r'flow ([0-9A-F]+) failures (\d+) checks (\d+)',pc_variants['flowpre'])
        assert values[-3:]==[int(m[1],16),0,int(m[3])],('prebattle',values[-3:],pc_variants['flowpre'])
        # A separate original chapter process respects the software backend's
        # bounded file cache, just as the real supervisor releases each chapter.
        command=list(cmd);command[command.index('--arg')+1]=101
        command[command.index('--png')+1]=images/'orbonne-native.png'
        ti=run([*command,OUT/'floworb.89z'],OUT);write_changed(OUT/'orbonne-native.log',ti.encode())
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        m=re.search(r'flow ([0-9A-F]+) failures (\d+) checks (\d+)',orbonne)
        assert values[-3:]==[int(m[1],16),0,int(m[3])]
        ti=run([*cmd,OUT/'flowhit.89z'],OUT);write_changed(OUT/'first-frame-native.log',ti.encode())
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        m=re.search(r'flow ([0-9A-F]+) failures (\d+) checks (\d+)',pc_variants['flowhit'])
        assert values[-3:]==[int(m[1],16),0,int(m[3])],('first victory frame',values[-3:],pc_variants['flowhit'])
        # Repeated receipt and saturated trophy stock exercise native byte order
        # as well as the quarter-price fallback instead of silently dropping loot.
        for label,source,expected in (('repeat',saved,None),('overflow',bytes(overflow),overflow_saved)):
            (OUT/(save_name+'.sav')).write_bytes(source)
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav',save_name+'.sav',save_name],OUT)
            variant=list(cmd);variant[variant.index('--png')+1]=images/(label+'-native.png')
            text=run([*variant,OUT/'flowgar.89z'],OUT);write_changed(OUT/(label+'-native.log'),text.encode())
            actual=save_dump.payload(OUT/'native-save'/(save_name+'.89y'))
            assert save_dump.decode(actual)['money']==(4500 if label=='repeat' else 4500+price)
            if expected:assert actual==expected
        publish_images(images,OUT);pending_shots.cleanup()
        print('Actual68000 Gariland/Orbonne pixels/rules, settled save, no double payment and trophy overflow agree PC')
    write_changed(OUT/'results.json',(json.dumps(result,indent=2)+'\n').encode())

if __name__=='__main__':main()
