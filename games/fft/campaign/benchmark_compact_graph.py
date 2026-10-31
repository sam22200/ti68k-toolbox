#!/usr/bin/env python3
"""Replay a captured pre-change Work/search build in an isolated source stage.

The baseline snapshot is explicit local evidence, never a shipping fallback.
Native compilers are serialized; this script never invokes TiEmu or writes saves.
"""
import argparse,json,os,re,shutil,subprocess,tempfile
from pathlib import Path
from check import HERE,ROOT,run


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--baseline',type=Path,default=HERE/'x/core-compact')
    base=ap.parse_args().baseline.resolve();out=base/'baseline';out.mkdir(exist_ok=True)
    assert (base/'before-src/games/fft/fft.c').exists()
    with tempfile.TemporaryDirectory(prefix='fft-compact-baseline-')as name:
        stage=Path(name)
        for folder in ('runtime','lib','tools','roms','sources'):(stage/folder).symlink_to(ROOT/folder,target_is_directory=True)
        target=stage/'games/fft';target.mkdir(parents=True)
        for folder,dirs,files in os.walk(HERE.parent):
            dirs[:]=[d for d in dirs if d not in ('x','__pycache__','.git')]
            rel=Path(folder).relative_to(HERE.parent);dest=target/rel;dest.mkdir(exist_ok=True)
            for filename in files:
                src=Path(folder)/filename
                if src.suffix in ('.c','.h','.inc','.bin','.89y'):(dest/filename).symlink_to(src)
        for src in (base/'before-src/games/fft').rglob('*'):
            if src.is_file():
                dest=target/src.relative_to(base/'before-src/games/fft');dest.unlink(missing_ok=True);shutil.copyfile(src,dest)
        data=target/'campaign/data'
        for filename in ('ffbm.bin','ffbm.89y','modules.h'):(data/filename).unlink(missing_ok=True)
        shutil.copyfile(base/'before-modules.bin',data/'ffbm.bin')
        # Size comes from the original captured bank, not the new generated header.
        import struct
        blob=(data/'ffbm.bin').read_bytes();count=struct.unpack_from('>H',blob,4)[0]
        sizes=[struct.unpack_from('>H',blob,struct.unpack_from('>H',blob,6+2*i)[0]+6)[0]for i in range(count)]
        (data/'modules.h').write_text('#define MOD_ZONE '+str(max(sizes))+'\n')
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffbm.bin','ffbm'],data)
        payload=bytearray((data/'ffbm.89y').read_bytes());payload[0x49]=3;(data/'ffbm.89y').write_bytes(payload)
        source=target/'campaign/check_span_battle.c'
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'old',source,ROOT/'runtime/core/rt_core.c',ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        files={p.stem:p for folder in (HERE.parent/'opening/data',HERE/'data')for p in folder.glob('*.89y')};files['ffbm']=data/'ffbm.89y'
        results=[]
        for label,scenario in [('gariland',201),('orbonne',101)]:
            cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',scenario,'--max',10000]
            for bank in files.values():cmd+=['--file',bank]
            log=run([*cmd,out/'old.89z']);(out/(label+'.log')).write_text(log)
            values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
            expected=(0xAF1961E0,1219)if label=='gariland'else(0x435FD706,1065)
            assert values[-3:]==[expected[0],0,expected[1]],values[-3:]
            zones=[line.strip()for line in log.splitlines()if re.match(r'^(?:[5678]\s+|max [5678]\s+)',line)]
            results.append(dict(map=label,hash=f'{expected[0]:08X}',checks=expected[1],zones=zones))
            print(label,zones,flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')

if __name__=='__main__':main()
