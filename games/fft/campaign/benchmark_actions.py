#!/usr/bin/env python3
"""Compare captured pre-L2 code/bank with the current impact and action pictures.

Snapshots are explicit local evidence, never a runtime fallback. Serial native
builds; no saves or emulator input. No image files/gallery timestamps are touched.
"""
import json,os,re,shutil,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links


def variants(source,out,data,native_data):
    common=[source,ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-w','-o',out,*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    pc=run([out],data)
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out,*common,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    files={p.stem:p for p in (HERE/'data').glob('*.89y')}
    files['ffbm']=native_data/'ffbm.89y'
    if (native_data/'ffbn.89y').exists():files['ffbn']=native_data/'ffbn.89y'
    if (native_data/'ffbo.89y').exists():files['ffbo']=native_data/'ffbo.89y'
    for p in files.values():cmd+=['--file',p]
    log=run([*cmd,out.with_suffix('.89z')])
    assert '_main returned'in log,log[-2000:]
    out.with_suffix('.log').write_text(log)
    out.with_suffix('.txt').write_text(pc)
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
    frames=[int(v,16)for v in re.findall(r'(?:action|reaction)frame \d+ (\w+)',pc)]
    if frames:assert values[-len(frames):]==frames,(pc,values)
    else:
        match=re.search(r'actions (\w+) failures 0 checks (\d+)',pc);assert match,pc
        assert values[-3:]==[int(match[1],16),0,int(match[2])],values[-3:]
    return dict(pc=pc,zones=[s.strip()for s in log.splitlines()if re.match(r'^(?:15\s+|max 15\s+)',s)])


def main():
    base=HERE/'x/core-actions';out=base/'baseline';out.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='fft-action-baseline-')as name:
        stage=Path(name)
        for folder in ('runtime','lib','tools','roms','sources'):(stage/folder).symlink_to(ROOT/folder,target_is_directory=True)
        target=stage/'games/fft';target.mkdir(parents=True)
        for folder,dirs,files in os.walk(HERE.parent):
            dirs[:]=[d for d in dirs if d not in ('x','__pycache__','.git')]
            dest=target/Path(folder).relative_to(HERE.parent);dest.mkdir(exist_ok=True)
            for filename in files:
                src=Path(folder)/filename
                if src.suffix in ('.c','.h','.inc','.bin','.89y'):(dest/filename).symlink_to(src)
        for src in (base/'before-src/games/fft').rglob('*'):
            if src.is_file():
                dest=target/src.relative_to(base/'before-src/games/fft');dest.unlink(missing_ok=True);shutil.copyfile(src,dest)
        data=target/'campaign/data'
        for old,new in [('before-modules.bin','ffbm.bin'),('before-modules.89y','ffbm.89y'),('before-modules.h','modules.h')]:
            (data/new).unlink();shutil.copyfile(base/old,data/new)
        # Fully private data directory prevents accidental campaign save reuse.
        work=stage/'work';work.mkdir();data_links(work)
        for p in work.glob('*'):
            if p.is_symlink() and (data/p.name).exists():p.unlink();p.symlink_to(data/p.name)
        before=variants(target/'campaign/check_actions.c',out/'old',work,data)
        before_frames=variants(target/'campaign/check_action_frames.c',out/'oldpics',work,data)
    with tempfile.TemporaryDirectory(prefix='fft-action-current-')as name:
        data=Path(name);data_links(data)
        after_frames=variants(HERE/'check_action_frames.c',out/'newpics',data,HERE/'data')
    assert before_frames['pc']==after_frames['pc'],(before_frames,after_frames)
    after=json.loads((base/'check/results.json').read_text())
    assert f"actions {after['hash']} failures 0 checks {after['checks']}"in before['pc']
    result=dict(before=before,after=after,frames=after_frames,all_six_pictures_unchanged=True)
    (base/'comparison.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__=='__main__':main()
