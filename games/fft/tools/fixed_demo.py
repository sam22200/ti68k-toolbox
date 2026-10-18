#!/usr/bin/env python3
"""Native close shoulder vs preserved prototype: TI frames, PC oracle, cycle-paced GIF."""
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess
from PIL import Image, ImageDraw
from perspective_demo import gif, delays

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE/'x/fixed-shoulder'
BANKS = ['fftmesh', 'fftu', 'fftw', 'ffti', 'fftv0', 'fftv1', 'fftv2', 'fftv3']


def run(args):
    return subprocess.check_output([str(a) for a in args], cwd=HERE, text=True)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    keys = OUT/'camera.txt'
    keys.write_text('0\n64 F5\n65\n128 F5\n129\n192 F5\n193\n256 F5\n257\n320\n')
    manifest = {'clock_hz': 12000000, 'grayscale_reserve': .09, 'frame_cap': 32,
                'timing': 'MC68000 datasheet emulation, estimated AMS calls; dt=max(1/32,(render+600)/(12MHz*.91)). Not hardware FPS.',
                'comparison': 'First 64 frames, synchronized 8 FPS for visual comparison only.',
                'cases': {}}
    cases = [('close', HERE/'fftfcapc.89z', True, 320)]
    baseline = HERE/'x/fixed-before/fftdumpc.89z'
    if baseline.exists() and (baseline.parent/'fftmesh.89y').exists():
        cases.insert(0, ('before', baseline, False, 64))
    images_by_case, rows = {}, []
    for name, binary, native, frames in cases:
        folder = OUT/name; folder.mkdir(exist_ok=True)
        oracle = OUT/('pc-'+name)
        run(['cc', '-std=gnu99', '-O1', '-DFFT_EXPLORATION',
             *(['-DFFT_FIXED'] if native else []), '-o', oracle, 'fft.c',
             'tools/perspective_capture.c', '../../runtime/core/rt_core.c',
             '../../runtime/platform-sw/rt_sw.c'])
        log = run([ROOT/'tools/bin/ti-cycles', '--arg', 62, '--frames', frames,
                   '--max', 20000, '--keys', keys, '--png', folder/'%03d.png',
                   *[v for bank in BANKS for v in ['--file',
                       HERE/('x/fixed-before/fftmesh.89y' if not native and bank == 'fftmesh' else bank+'.89y')]], binary])
        (folder/'cycles.log').write_text(log)
        assert '_main returned' in log
        pc = run([oracle, 62, keys, frames]); (folder/'pc.log').write_text(pc)
        hashes = re.findall(r'shot \d+ checksum (\w+)', log)
        expected = re.findall(r'^\d+ (\w+)$', pc, re.M)
        assert hashes[:-1] == expected and len(expected) == frames
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
        metrics = [values[i+1:i+5] for i,v in enumerate(values) if v >> 24 == 0x72]
        assert len(metrics) == frames
        images, seconds = [], []
        for n,(geometry,sprites,present,total) in enumerate(metrics):
            images.append(Image.open(folder/f'{n:03d}.png').convert('RGB').crop((0,0,160,100)))
            dt = max(1/32, (total+600)/(12000000*.91)); seconds.append(dt)
            rows.append(dict(case=name,frame=n,geometry=geometry,sprites=sprites,present=present,
                             render=total,seconds=dt,checksum=hashes[n]))
        gif(OUT/(name+'.gif'), [im.resize((640,400),Image.Resampling.NEAREST) for im in images], delays(seconds))
        images_by_case[name] = images
        m = dict(binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(), frames=frames,
                 resolution='160x100' if native else '80x50', all_fps=frames/sum(seconds),
                 first_view_ms=seconds[0]*1000,
                 steady_fps=(frames-32)/sum(seconds[32:]),
                 warmed_last_64_fps=min(64,frames)/sum(seconds[-64:]))
        if native:
            m['switch_ms'] = {str(n):seconds[n]*1000 for n in [64,128,192,256]}
            m['pose_pool_peak'] = max(v & 0xffffff for v in values if v >> 24 == 0x74)
            m['mask_pool_peak'] = max(v & 0xffffff for v in values if v >> 24 == 0x75)
            m['pose_misses'] = max(v & 0xffffff for v in values if v >> 24 == 0x76)
            m['pose_pool_resets'] = max(v & 0xffffff for v in values if v >> 24 == 0x77)
        manifest['cases'][name] = m
        print(name, json.dumps(m), flush=True)
    if 'before' in images_by_case:
        comparison = []
        for n in range(64):
            im = Image.new('RGB',(640,220),'white'); draw=ImageDraw.Draw(im)
            for k,name in enumerate(['before','close']):
                im.paste(images_by_case[name][n].resize((320,200),Image.Resampling.NEAREST),(k*320,20))
                draw.text((k*320+4,4), ['BEFORE - 80x50','CLOSE - 160x100'][k], fill='black')
            comparison.append(im)
        gif(OUT/'before-after.gif', comparison, [130 if n & 1 else 120 for n in range(64)])
        comparison[24].save(OUT/'before-after.png')
    with (OUT/'frames.csv').open('w') as f:
        writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')


if __name__ == '__main__':
    main()
