#!/usr/bin/env python3
"""Full PC gameplay/GIF review; optional isolated TI renderer/logic diagnostic (no TiEmu)."""
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import statistics
import subprocess
import sys

from PIL import Image, ImageDraw
from perspective_demo import gif, BANKS
from texture_review import normalize
from attack_texture import prepare

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE/'x/attack-camera'
FLAGS = ['-DFFT_EXPLORATION', '-DFFT_FIXED', '-DFFT_CINEMA', '-DFFT_CAMERA_MOTION']
CASES = [('melee', 65, 250, 80), ('ranged', 66, 310, 136)]
LABELS = ['ISO: SELECT / CONFIRM TARGET', 'TRANSITION TO TARGET', 'ATTACK / DAMAGE', 'RETURN TO ISOMETRIC']


def run(args):
    return subprocess.check_output([str(a) for a in args], cwd=HERE, text=True)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    texture_manifest = prepare()
    run(['cc', '-std=gnu99', '-O1', '-g', *FLAGS, '-o', OUT/'capture-pc', 'tools/attack_camera_capture.c',
         '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'])
    cycles = '--cycles' in sys.argv
    if cycles:
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat', 'fftshot.bin', 'fftshot'])
        run(['cc', '-std=gnu99', '-O1', *FLAGS, '-o', OUT/'bench-pc', 'tools/attack_camera_bench.c',
             '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'])
        run([ROOT/'tools/bin/ti-cc', '-o', OUT/'fftshotc', '-DRT_CYCLES', '-DFFT_MEASURE', *FLAGS,
             'tools/attack_camera_bench.c', '../../runtime/core/rt_core.c', '../../runtime/platform-ti68k/rt_ti.c',
             '../../lib/unpack68k.s', '../../tools/extgraph/lib/tilemap.a'])
    manifest = dict(platform='Complete PC game; optional headless TI camera/logic diagnostic with ISO UI omitted.',
                    gif_timing='Uniform 20 FPS visual preview; 55 cinematic frames = 2.75 s, previously 49 = 2.45 s; not TI cadence.',
                    textures=texture_manifest,
                    camera_motion='Direct geometric zoom; preserve isometric yaw unless head-on (maximum 45 degree turn); close melee, wide ranged.',
                    fixture='Original enemies. Ramza starts at (5,5) for melee or (4,6) for ranged; the latter fixture grants Throw Stone.',
                    cases={})
    clips, clip_times = [], []
    for name, scenario, frames, confirm in CASES:
        folder = OUT/name; folder.mkdir(exist_ok=True)
        keys = HERE/f'keys/cinema-{name}.txt'
        raw = run([OUT/'capture-pc', scenario, keys, frames, folder]); (folder/'frames.csv').write_text(raw)
        rows = list(csv.DictReader(io.StringIO(raw)))
        assert len(rows) == frames and rows[confirm-1]['phase'] == '0' and rows[confirm]['phase'] == '1'
        assert all(sum(r['phase'] == str(phase) for r in rows) == count for phase,count in [(1,10),(2,35),(3,10)])
        assert rows[-1]['phase'] == '0' and rows[-1]['mode'] == '3'
        assert int(rows[-1]['target_hp']) < int(rows[0]['target_hp'])
        assert all((r['anchor_x'],r['anchor_z']) == ('6','5') for r in rows if r['phase'] != '0')
        assert all(r['textured'] == '1' for r in rows if r['phase'] == '2')
        assert all((int(r['distance']) < 80) == (name == 'melee') for r in rows if r['phase'] != '0')
        pictures = []
        for n, r in enumerate(rows):
            canvas = Image.new('RGB', (640, 430), 'white'); draw = ImageDraw.Draw(canvas)
            draw.text((4, 3), name.upper()+' / '+LABELS[int(r['phase'])], fill='black')
            lcd = normalize(folder/f'{n:03d}.png').crop((0,0,160,100))
            if r['phase'] in ('1','3'):
                # Keep the moving map on screen: the rejected white title card had no
                # scenery pixels in this viewport region.
                assert sum(p[0] < 255 for p in lcd.crop((0,8,160,93)).get_flattened_data()) > 100
            canvas.paste(lcd.resize((640,400), Image.Resampling.NEAREST), (0,18))
            draw.text((4, 419), 'PC visual review - 20 FPS preview, not TI performance', fill='black')
            pictures.append(canvas)
        times = [50]*frames
        pictures[confirm+15].save(OUT/f'{name}.png')
        info = dict(frames=frames, phases=dict(enter=10, action=35, leave=10), cinematic_seconds_preview=2.75, previous_seconds_preview=2.45, damage=int(rows[0]['target_hp'])-int(rows[-1]['target_hp']))
        if cycles:
            oracle = run([OUT/'bench-pc', scenario, keys, frames]); (folder/'bench-pc.log').write_text(oracle)
            args = [ROOT/'tools/bin/ti-cycles', '--arg', scenario, '--frames', frames, '--max', 100000,
                    '--keys', keys, '--png', folder/'ti-final.png']
            for bank in BANKS+['fftshot']:
                args += ['--file', HERE/(bank+'.89y')]
            log = run([*args, OUT/'fftshotc.89z']); (folder/'cycles.log').write_text(log)
            assert '_main returned' in log
            hashes = re.findall(r'shot \d+ checksum (\w+)', log)
            expected = [line.split()[1] for line in oracle.splitlines()]
            assert len(hashes) == frames+1
            assert [x.lower() for x in hashes[:-1]] == [x.lower() for x in expected]
            active = [n for n,r in enumerate(rows) if r['phase'] != '0']
            assert all(hashes[n].lower() == rows[n]['checksum'].lower() for n in active)
            values = [int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
            assert len(values) == frames*2+1
            costs = [dict(frame=n, phase=int(rows[n]['phase']), cycles=values[n*2+1]) for n in range(frames)]
            with (folder/'cycles.csv').open('w') as f:
                w = csv.DictWriter(f,fieldnames=costs[0].keys()); w.writeheader(); w.writerows(costs)
            groups = {}
            for phase,label in [(1,'enter'),(2,'action'),(3,'return')]:
                sample = [c['cycles'] for c in costs if c['phase'] == phase]
                warm = sample[1:] if phase == 2 else sample
                groups[label] = dict(mean=statistics.mean(sample), maximum=max(sample),
                                     warm_mean=statistics.mean(warm), cold=sample[0],
                                     warm_cpu_only_fps=min(32,10920000/statistics.mean(warm)))
            info.update(diagnostic_matched_frames=frames, full_game_native_perspective_matched_frames=len(active), cycles=groups)
            estimated_times = [max(50,round(c['cycles']/109200)*10) if c['phase'] else 50 for c in costs]
            gif(OUT/f'{name}-cpu-estimate.gif',pictures,estimated_times)
        gif(OUT/f'{name}.gif', pictures, times)
        focused = pictures[confirm-8:confirm+63]
        gif(OUT/f'{name}-fast-cinema.gif', focused, [50]*len(focused))
        clips.append(focused); clip_times.append([50]*len(focused))
        manifest['cases'][name] = info
    together = []
    for a,b in zip(*clips):
        canvas = Image.new('RGB',(1280,430),'white'); canvas.paste(a,(0,0)); canvas.paste(b,(640,0)); together.append(canvas)
    gif(OUT/'both-attacks.gif', together, [max(a,b) for a,b in zip(*clip_times)])
    for source,target in [('melee.gif','melee-close-textured.gif'),('ranged.gif','ranged-wide-textured.gif'),
                          ('both-attacks.gif','both-textured-lowpoly.gif'),
                          ('melee.gif','melee-direct-zoom.gif'),('ranged.gif','ranged-direct-zoom.gif')]:
        (OUT/target).write_bytes((OUT/source).read_bytes())
    manifest['source_sha256'] = {p:hashlib.sha256((HERE/p).read_bytes()).hexdigest() for p in
                                 ['fft.c','fft.h','perspective.inc','fixed.inc','cinema.inc','test_cinema.c',
                                  'tools/attack_camera.py','tools/attack_camera_capture.c','tools/attack_camera_bench.c']}
    manifest['source_sha256'].update({p:hashlib.sha256((HERE/p).read_bytes()).hexdigest() for p in
                                    ['tools/attack_texture.py','tools/attack_texture_capture.c']})
    if cycles:
        manifest['diagnostic_binary_bytes'] = (OUT/'fftshotc.89z').stat().st_size
        manifest['diagnostic_binary_sha256'] = hashlib.sha256((OUT/'fftshotc.89z').read_bytes()).hexdigest()
    (OUT/('validated-ti.json' if cycles else 'manifest-pc.json')).write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest['cases'],indent=2))


if __name__ == '__main__':
    main()
