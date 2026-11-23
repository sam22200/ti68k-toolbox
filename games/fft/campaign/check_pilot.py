#!/usr/bin/env python3
"""One-command actual pilot route on PC/ASan, using matching installation groups.

No rewritten campaign scripts, live saves or emulator. Real fp_story and cold
title/scene/deployment/battle/map processes; --combat auto adds real fights,
town services and Formation. Default skip retains the fast lifecycle control.
The defeat fixture starts at the real pre-Gariland snapshot from the first run.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import shutil
import re
import struct

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(HERE.parent/'tools'))
import chapter_packages as packages
import save_dump


def run(command,cwd,log):
    p=subprocess.run(list(map(str,command)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                     env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','UBSAN_OPTIONS':'halt_on_error=1'})
    log.write_text(p.stdout)
    if p.returncode:print(p.stdout);p.check_returncode()
    return p.stdout


def installed(folder):
    manifest=json.loads((folder/'packages.json').read_text());source={}
    for row in [manifest['core'],manifest['chapters'][0]]:
        path=folder/row['file'];assert path.stat().st_size==row['bytes']
        assert hashlib.sha256(path.read_bytes()).hexdigest()==row['sha256']
        group=packages.read_group(path);assert not set(group)&set(source);source.update(group)
    return manifest,source


def native_fight(out,source,pc_case):
    """One affected68000 replay, not a TI sweep or a hardware RAM measurement."""
    dest=out/'native';dest.mkdir(exist_ok=True);records=[];splits=[]
    for name in ('ffbm','ffbn','ffbo','ffbp'):
        raw=source[name].raw;first,count,total,_=struct.unpack_from('>3HI',raw,4)
        assert raw[:4]==b'FMB3' and first==len(records) and total==29
        offsets=list(struct.unpack_from('>'+str(count)+'H',raw,14))+[len(raw)]
        records.extend(raw[a:b] for a,b in zip(offsets,offsets[1:]));splits.append(first+count)
    zone=max(struct.unpack_from('>H',r,6)[0] for i,r in enumerate(records) if i!=27)
    companion=max(struct.unpack_from('>H',records[i],6)[0] for i in (27,28))
    header=dest/'modules.h';header.write_text('#define MOD_ZONE %d\n#define MOD_BANK0_COUNT %d\n#define MOD_BANK1_END %d\n#define MOD_BANK2_END %d\n#define MOD_COMPANION_BYTES %d\n'%(zone,*splits[:3],companion))
    wrapper=dest/'bridge.c';wrapper.write_text('#include "'+str(HERE/'check_pilot_native.c')+'"\n')
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-DFFT_MODULE_HEADER="'+str(header)+'"','-o','ffpe2e',wrapper,
         ROOT/'runtime/core/rt_core.c',ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'],dest,dest/'build.log')
    files=[]
    for v in source.values():
        if v.kind!=0x1c:continue
        path=dest/(v.name+'.89y');path.write_bytes(packages.group_bytes([v],single=True));files.extend(['--file',path])
    seed=(pc_case/'before-gariland.sav').read_bytes()
    var=packages.Variable('ffsave',0x1c,3,seed+b'\0sav\0\xf8');path=dest/'ffsave.89y';path.write_bytes(packages.group_bytes([var],single=True));files.extend(['--file',path])
    # A previous successful export must never hide a missing write on this run.
    with tempfile.TemporaryDirectory(prefix='native-save-',dir=dest) as temporary:
        saves=Path(temporary)
        log=run([ROOT/'tools/bin/ti-cycles','--arg','200','--frames','50000','--max','10000',
                 '--save-dir',saves,*files,dest/'ffpe2e.89z'],dest,dest/'replay.log')
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
        assert values[-4:]==[1,0,1,1],values
        actual=packages.read_group(saves/'ffsave.89y')['ffsave'].payload[:-6]
        expected=(pc_case/'battle-200-win.sav').read_bytes();assert actual==expected,'Native battle settlement differs from PC'
        assert sorted(p.name for p in saves.iterdir())==['ffsave.89y'],'Unexpected export of unchanged input banks'
        saved=dest/'saved';saved.mkdir(exist_ok=True);shutil.copy2(saves/'ffsave.89y',saved/'ffsave.89y')
    return dict(ordinary_fight=True,no_god_or_skip=True,save_bytes=len(actual),save_sha256=hashlib.sha256(actual).hexdigest(),
                matching_images=len(records),frames=values[-5],teardown_buffers_released=True,hardware_ram_measured=False,
                input_policy='State-driven actual menu input, not fixed PC frame numbers')


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--packages',type=Path,required=True)
    ap.add_argument('--out',type=Path,default=HERE/'x/chapter1-ready/pilot')
    ap.add_argument('--combat',choices=['skip','auto'],default='skip',help='auto plays ordinary combat through All allies, without God/skip')
    ap.add_argument('--pc-only',action='store_true',help='bounded development run; final acceptance also uses sanitizers')
    ap.add_argument('--native',action='store_true',help='one full Gariland68000 replay after PC acceptance, no TiEmu/screenshots')
    args=ap.parse_args()
    if args.native and args.combat!='auto':ap.error('--native requires --combat auto')
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    manifest,source=installed(args.packages.resolve());results={};reference=None
    for mode in (('pc',) if args.pc_only else ('pc','san')):
        dest=out/mode;dest.mkdir(exist_ok=True);prefix=dest/'pilot'
        flags=['-fsanitize=address,undefined','-fno-sanitize=shift','-fno-omit-frame-pointer','-no-pie'] if mode=='san' else []
        if args.combat=='auto':flags.append('-DPILOT_AUTOPLAY')
        common=[ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
        programs=('title','scene','deploy','battle','map')+(('manage',) if args.combat=='auto' else ())
        for k,name in enumerate(programs,1):
            run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-DPILOT_KIND='+str(k),'-o',str(prefix)+'-'+name,HERE/'check_pilot_child.c',*common],ROOT,dest/(name+'-build.log'))
        exe=dest/'supervisor';run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',exe,HERE/'check_pilot_supervisor.c'],ROOT,dest/'supervisor-build.log')
        cases=[];checkpoints={}
        for variant,name in enumerate(('new-game-to-world-save','continue-completed-pilot','defeat-continue-retry')):
            with tempfile.TemporaryDirectory(prefix='fft-pilot-') as temp:
                data=Path(temp)
                for v in source.values():
                    (data/(v.name+'.payload')).write_bytes(v.payload)
                    if v.kind==0x1c and v.payload.endswith(packages.resources.TRAILER):(data/(v.name+'.bin')).write_bytes(v.raw)
                if variant:
                    initial=checkpoints['complete' if variant==1 else 'before-battle']
                    for n in ('ffsave','ffsave1'):(data/(n+'.sav')).write_bytes(initial)
                    (data/'ffslot.sav').write_bytes(b'FSL1\x01\0')
                backup=(HERE/'x/check/ffsave_won.sav').read_bytes();(data/'ffsave2.sav').write_bytes(backup)
                case_out=dest/name;case_out.mkdir(exist_ok=True)
                try:log=run([exe,prefix,variant],data,dest/(name+'.log'))
                finally:
                    for pattern in ('battle-*.csv','battle-*.state','battle-*.keys','battle-*.sav','before-*.sav','after-*.sav'):
                        for path in data.glob(pattern):shutil.copy2(path,case_out/path.name)
                saved=(data/'ffsave.sav').read_bytes();decoded=save_dump.decode(saved)
                assert saved[:4]==b'FSV9' and len(saved)==4684 and 1 in decoded['flags']
                assert (data/'ffsave2.sav').read_bytes()==backup
                assert (data/'ffsave1.sav').read_bytes()==saved
                assert not decoded['deployment'] and any(g['id']==4 for g in decoded['guests'])
                if not variant:
                    checkpoints['before-battle']=(data/'before-gariland.sav').read_bytes();checkpoints['complete']=saved
                    before=save_dump.decode(checkpoints['before-battle'])
                    assert before['flow']==3 and len(before['deployment'])==5
                    (out/'before-gariland.sav').write_bytes(checkpoints['before-battle'])
                trace_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in case_out.iterdir() if p.suffix in ('.csv','.keys','.sav')}
                cases.append(dict(case=name,save_sha256=hashlib.sha256(saved).hexdigest(),trace_sha256=trace_hashes,route=log.strip().splitlines()))
                (dest/(name+'.sav')).write_bytes(saved)
                print(mode,name,'passed',flush=True)
        if reference is None:reference=cases
        else:assert reference==cases,'PC/sanitizer route or save diverged'
        results[mode]=cases
    native=native_fight(out,source,out/'pc/new-game-to-world-save') if args.native else None
    hashes={n:hashlib.sha256((HERE/n).read_bytes()).hexdigest() for n in ('check_pilot.py','check_pilot_child.c','check_pilot_supervisor.c','check_pilot_native.c','pilot_services.inc','pack_story.h','deployment.inc','save_types.h')}
    (out/'results.json').write_text(json.dumps(dict(core_key=manifest['core_key'],results=results,pc_sanitizers_equal=not args.pc_only,
        battle_completion='Real All allies input, ordinary AI/actions, no God/skip; defeat remains an explicit HP fixture' if args.combat=='auto' else 'Shipping DEBUG skip; defeat is an explicit HP fixture, not ordinary combat certification',
        native=native,input_sha256=hashes),indent=2)+'\n')
    print('Actual pilot:',args.combat,'New Game/scenes/deployment/results/world Save/Continue/defeat/retry passed',','.join(results))


if __name__=='__main__':main()
