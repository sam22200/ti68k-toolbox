#!/usr/bin/env python3
"""Native cache policies, actual archive streams; validate every color/cover plane."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
OUT=HERE/'x'

def checksum(data):
    a=b=0
    for x in data:a=(a+x)%255;b=(a+b)%255
    return b<<8|a

def run(args,cwd=HERE):
    return subprocess.check_output([str(a) for a in args],cwd=cwd,text=True)

def main():
    results=[]
    for map_no,name in enumerate(('gariland','orbonne')):
        size=len((OUT/(name+'0.raw')).read_bytes());plane=size//6
        originals=ROOT/'games/fft'/('campaign/data' if not map_no else 'opening/data')
        files=[OUT/'sbcommon.89y',OUT/('sb'+name[:3]+'.89y')]
        for label,policy,exact,all_original in (
            ('shared_one',0,0,0),('shared_four',1,0,0),('shared_four_colors',2,0,0),
            ('exact_one',0,1,0),('exact_four',1,1,0),('exact_four_colors',2,1,0),
            ('current_one',3,0,0),('current_four',3,0,1)):
            feed=list(files)
            if exact:feed.append(OUT/('sp'+name[:3]+'.89y'))
            if policy==3:
                prefix='ffov' if map_no else 'ffgv'
                feed.extend(originals/(prefix+str(r)+'.89y') for r in range(4))
            cmd=[ROOT/'tools/bin/ti-cycles','--frames',1,'--max',1000,'--arg',policy+4*exact+8*map_no+16*all_original]
            for f in feed:cmd.extend(('--file',f))
            cmd.append(OUT/'sbcache.89z');log=run(cmd)
            (OUT/(name+'-'+label+'.log')).write_text(log)
            assert '_main returned' in log,log
            values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
            expected=[]
            for r in (0,1,2,3,0):
                raw=(OUT/(name+str(r)+('-original' if exact or policy==3 else '')+'.raw')).read_bytes()
                expected.extend((checksum(raw[:2*plane]),checksum(raw[2*plane:])))
            assert values==expected+[1],(name,label,values,expected)
            zones={int(id):int(cycles) for id,cycles in re.findall(r'^\s*(\d+)\s+[^\n]*?\s+\d+\s+(\d+)\s+\d+\s*$',log,re.M)}
            if policy==3:memory=6*plane*(4 if all_original else 1)
            else:memory=plane*({0:7,1:25,2:13}[policy])+(256 if exact else 0)
            entry=dict(map=name,policy=label,cache_and_scratch_ram=memory,
                       cold_cycles=zones[10],cold_ms=zones[10]/12000,
                       rotation_cycles=[zones[i] for i in range(11,15)],
                       rotation_ms=[zones[i]/12000 for i in range(11,15)],
                       warm_copy_cycles=zones[15]/32,warm_copy_ms=zones[15]/32/12000,
                       full_source_fidelity=bool(exact or policy==3),native_checksums_match=True)
            results.append(entry)
            print(name,label,'load %.2fms'%entry['cold_ms'],'turn %.2f..%.2fms'%(min(entry['rotation_ms']),max(entry['rotation_ms'])),flush=True)
    report=dict(method='MC68000 datasheet cycles / 12 MHz; not TiEmu or hardware FPS. Preparation/copy only, no actors, gameplay, turn animation, I/O or gray interrupts. Four-full policies exceed full-game RAM.',results=results,
                executable_bytes=(OUT/'sbcache.89z').stat().st_size,
                production_game_work_bytes=35540,orbonne_scene_cache_bytes=15360,
                runtime_gray_and_saved_lcd_bytes=11528)
    (OUT/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
    snapshot=dict(storage=json.loads((OUT/'storage.json').read_text()),native=report,
                  native_binary_sha256=hashlib.sha256((OUT/'sbcache.89z').read_bytes()).hexdigest(),
                  source_sha256={p:hashlib.sha256((HERE/p).read_bytes()).hexdigest()
                                 for p in ('bank.c','bank.h','bench.c','build.py','measure.py','test.c')})
    if (OUT/'blocks.json').exists():snapshot['lossless_blocks']=json.loads((OUT/'blocks.json').read_text())
    (HERE/'RESULTS.json').write_text(json.dumps(snapshot,indent=2)+'\n')

if __name__=='__main__':main()
