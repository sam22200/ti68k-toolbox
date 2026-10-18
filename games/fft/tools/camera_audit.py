#!/usr/bin/env python3
"""Prepare a real-TiEmu camera audit after matching the same case driver PC/TI."""
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import statistics
import subprocess
import sys

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE/'x/camera-audit'
DATA = HERE/'x/tiemu-camera/data'
FLAGS = ['-DFFT_EXPLORATION', '-DFFT_FIXED', '-DFFT_CINEMA', '-DFFT_CAMERA_MOTION', '-DFFT_TURN_HOLD', '-DFFT_CAMERA_AUDIT']
NAMES = ['F5', 'F1', 'ISO>C', 'ISO>W', 'W>C', 'C>W', 'W>ISO',
         'CINE M', 'CINE R', 'COLD C', 'COLD W', 'WARM C', 'WARM W']


def read_tiemu():
    """Decode the known AMS digits from the recorded result table, without OCR guesses."""
    from PIL import Image
    text = (ROOT/'runtime/platform-sw/amsfont.h').read_text()
    font = list(map(int, re.findall(r'\d+', re.search(
        r'ams_f4x6\[1536\] = \{(.*?)\};', text, re.S).group(1))))

    def trim(rows):
        columns = [x for x in range(len(rows[0])) if any(row[x] for row in rows)]
        return tuple(tuple(row[columns[0]:columns[-1]+1]) for row in rows)

    digits = {str(c):trim([[(font[(48+c)*6+1+y] >> (7-x)) & 1
                            for x in range(8)] for y in range(5)]) for c in range(10)}

    def numbers(image, y, cutoff=0):
        rows = [[int(image.getpixel((x,y+j)) < 128) for x in range(image.width)] for j in range(5)]
        parts = []; x = cutoff; last = -10
        while x < image.width:
            if not any(row[x] for row in rows): x += 1; continue
            left = x
            while x < image.width and any(row[x] for row in rows): x += 1
            piece = tuple(tuple(row[left:x]) for row in rows)
            matches = [c for c, glyph in digits.items() if glyph == piece]
            assert len(matches) == 1, (y,left,matches)
            if left-last > 4: parts.append('')
            parts[-1] += matches[0]; last = left
        return list(map(int, parts))

    shot = Image.open(OUT/'timer-results.png').convert('L')
    assert shot.size == (220,492), 'This decoder expects the recorded default Titanium skin.'
    lcd = shot.crop((30,46,192,148)); headless = json.loads((OUT/'headless.json').read_text())
    results = []
    for c, name in enumerate(NAMES):
        cutoff = sum(font[ord(ch)*6] for ch in name)+1
        total, frames, worst = numbers(lcd,15+c*6,cutoff)
        assert frames == headless['cases'][c]['frames'] and total >= worst
        results.append(dict(case=c,name=name,total_ms=total,frames=frames,
                            worst_frame_ms=worst,mean_displayed_fps=frames*1000/total))
    stage = Image.open(OUT/'stage-page.gif').convert('L')
    breakdown = {name:dict(zip(['entry_ms','action_ms','return_ms'],numbers(stage,y)))
                 for name,y in [('CINE M',24),('CINE R',56)]}
    for name, values in breakdown.items():
        assert len(values) == 3
        total = next(row['total_ms'] for row in results if row['name'] == name)
        assert 0 <= total-sum(values.values()) < 1000  # final ISO response, not a missing stage
    report = dict(platform='Docker TiEmu 3.04, Titanium HW3 AMS 3.10, configured 12 MHz',
        timer='256 Hz; grayscale and buffer swaps active; eight-tick/32 FPS limiter',
        scope='Native bounded camera demo. Flat travelling geometry, prepared textures at exact fixed endpoints. Compact ISO UI; scripted input.',
        precision='Integer ms printed from 256 Hz ticks; digits decoded exactly against the local AMS font. Physical hardware not measured.',
        matched_headless_frames=headless['matched_frames'],results=results,cinematic_stages=breakdown,
        binary=headless['binaries']['fftaudit'],
        evidence_sha256={name:hashlib.sha256((OUT/name).read_bytes()).hexdigest() for name in
                         ['timer-results.png','stage-page.gif']})
    (OUT/'tiemu.json').write_text(json.dumps(report,indent=2)+'\n')
    with (OUT/'tiemu.csv').open('w') as f:
        writer = csv.DictWriter(f,fieldnames=list(results[0])); writer.writeheader(); writer.writerows(results)
    print(json.dumps(report,indent=2))


def run(args, cwd=HERE):
    return subprocess.check_output([str(a) for a in args], cwd=cwd, text=True)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    assert (DATA/'fftlook.bin').exists(), 'Run make tiemu-camera-demo first.'
    run(['cc', '-std=gnu99', '-O1', '-g', *FLAGS, '-o', OUT/'oracle',
         'tools/camera_audit_cycles.c', '../../runtime/core/rt_core.c',
         '../../runtime/platform-sw/rt_sw.c'])
    pc = run([OUT/'oracle'], DATA); (OUT/'pc.csv').write_text(pc)
    oracle = list(csv.reader(io.StringIO(pc)))
    common = ['../../runtime/core/rt_core.c', '../../lib/unpack68k.s',
              '../../tools/extgraph/lib/tilemap.a']
    for name, src, extra in [('fftaudit', 'camera_audit_tiemu.c', []),
                             ('fftaudc', 'camera_audit_cycles.c', ['-DRT_CYCLES'])]:
        run([ROOT/'tools/bin/ti-cc', '-o', OUT/name, *FLAGS, *extra, 'tools/'+src, *common])
    args = [ROOT/'tools/bin/ti-cycles', '--max', 100000]
    for path in sorted(DATA.glob('*.89y')): args += ['--file', path]
    log = run([*args, OUT/'fftaudc.89z'], DATA); (OUT/'cycles.log').write_text(log)
    hashes = re.findall(r'shot \d+ checksum (\w+)', log)
    assert [h.lower() for h in hashes] == [row[3].lower() for row in oracle]
    values = [int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
    costs = [[] for _ in NAMES]; phases = [[] for _ in NAMES]; case = None; verified = 0
    for n, value in enumerate(values):
        if value >> 24 == 0x61: case = value & 255
        elif value >> 24 == 0x62:
            costs[case].append(values[n+1]); phases[case].append(value & 255)
        elif value >> 24 == 0x63:
            assert value & 255; verified += 1
    assert verified == len(NAMES)
    result = []
    for c, name in enumerate(NAMES):
        sample = costs[c]
        result.append(dict(case=c, name=name, frames=len(sample), total_cycles=sum(sample),
            mean_cycles=statistics.mean(sample), max_cycles=max(sample),
            cpu_only_ms_at_12mhz=sum(sample)/12000,
            phase_cycles={str(phase):sum(v for v,p in zip(sample,phases[c]) if p == phase) for phase in range(4)},
            scenery='hold/swap; no travelling texture mapper' if c<2 else
                    'offline texture on fixed endpoint; flat moving geometry'))
    report = dict(scope='Bounded native TI camera/combat renderer with compact ISO UI. Datasheet CPU counts exclude emulator display/timer/limiter; no hardware FPS claim.',
        matched_frames=len(hashes), cases=result,
        binaries={name:dict(bytes=(OUT/(name+'.89z')).stat().st_size,
                           sha256=hashlib.sha256((OUT/(name+'.89z')).read_bytes()).hexdigest()) for name in ['fftaudit','fftaudc']})
    (OUT/'headless.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    if '--read-tiemu' in sys.argv: read_tiemu()
    else: main()
