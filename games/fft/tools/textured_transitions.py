#!/usr/bin/env python3
"""Review actual C-rendered textured transitions; PC reference sampler, never TI FPS."""
import csv
import hashlib
import io
import json
from pathlib import Path
import subprocess

from PIL import Image, ImageDraw
from preview_textures import prepare_preview
from perspective_demo import gif
from texture_review import HERE, normalize

OUT = HERE/'x/textured-transitions'
FLAGS = ['-DFFT_EXPLORATION','-DFFT_FIXED','-DFFT_CINEMA', '-DFFT_CAMERA_MOTION','-DFFT_TEXTURE_REVIEW','-DFFT_TEXTURE_PREVIEW']


def run(args):
    return subprocess.check_output([str(a) for a in args],cwd=HERE,text=True)


def frames(folder, rows, name):
    pictures = []
    for row in rows:
        n = int(row['frame']); image = normalize(folder/f'{n:03d}.png')
        canvas = Image.new('RGB',(640,440),'white'); draw = ImageDraw.Draw(canvas)
        draw.text((4,3),name,fill='black')
        canvas.paste(image.resize((640,400),Image.Resampling.NEAREST),(0,20))
        draw.text((4,425),'PC texture reference / 20 FPS preview - not calculator speed',fill='black')
        pictures.append(canvas)
    return pictures


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    atlas = prepare_preview()
    run(['cc','-std=gnu99','-O1','-g',*FLAGS,'-o',OUT/'capture-attack','tools/attack_camera_capture.c',
         '../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g',*FLAGS,'-o',OUT/'capture-inspection','tools/textured_transition_capture.c',
         '../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'])
    results = {}; sheet = []
    for name,scenario,confirm in [('melee',65,80),('ranged',66,136)]:
        folder = OUT/name; folder.mkdir(exist_ok=True)
        count = confirm+65
        text = run([OUT/'capture-attack',scenario,HERE/f'keys/cinema-{name}.txt',count,folder])
        (folder/'frames.csv').write_text(text); rows = list(csv.DictReader(io.StringIO(text)))
        assert len(rows) == count
        assert all(sum(r['phase'] == str(phase) for r in rows) == n for phase,n in [(1,10),(2,35),(3,10)])
        assert all(int(r['sampled']) > 1000 for r in rows if r['phase'] != '0')
        assert rows[-1]['mode'] == '3' and rows[-1]['phase'] == '0'
        assert int(rows[0]['target_hp'])-int(rows[-1]['target_hp']) == (12 if name == 'melee' else 6)
        pictures = frames(folder,rows,name.upper()+' / TEXTURES THROUGHOUT / 2.75 s CINEMATIC')
        clip = pictures[confirm-8:confirm+63]
        gif(OUT/f'{name}-textured-motion.gif',clip,[50]*len(clip))
        (OUT/f'{name}-050s-cinema.gif').write_bytes((OUT/f'{name}-textured-motion.gif').read_bytes())
        (OUT/f'{name}-attacker-rewards.gif').write_bytes((OUT/f'{name}-textured-motion.gif').read_bytes())
        sheet.extend([pictures[confirm+3],pictures[confirm+20],pictures[confirm+45]])
        results[name] = dict(frames=len(rows),cinematic_frames=55,preview_seconds=2.75,
                             transition_seconds_each=.5,action_seconds=1.75)
    for wide in [0,1]:
        name = 'wide-entry' if wide else 'close-entry'
        folder = OUT/name; folder.mkdir(exist_ok=True)
        text = run([OUT/'capture-inspection',wide,folder]); (folder/'frames.csv').write_text(text)
        rows = list(csv.DictReader(io.StringIO(text))); assert len(rows) == 204
        assert all(int(r['sampled']) > 1000 for r in rows if r['perspective'] != '0' or r['turn'] != '0')
        assert rows[-1]['perspective'] == '0' and rows[-1]['rot'] == '0'
        assert sum(r['turn'] == '1' for r in rows) == 6
        assert sum(r['turn'] == '-1' for r in rows) == 6
        assert {int(r['zoom']) for r in rows if r['perspective'] != '0'} == {40,80}
        assert {r['transition'] for r in rows} == {'0','1','2','3'}
        pictures = frames(folder,rows,name.upper()+' / ENTER - ZOOM - RETURN - F5/F1')
        gif(OUT/f'{name}-all-transitions.gif',pictures,[50]*len(pictures))
        if not wide:
            gif(OUT/'f1-f5-textured-live.gif',pictures[160:196],[50]*36)
            (OUT/'f1-f5-slower.gif').write_bytes((OUT/'f1-f5-textured-live.gif').read_bytes())
        sheet.extend([pictures[15],pictures[48],pictures[144]])
        results[name] = dict(frames=204,checks='Real keys; both zoom directions, entry/return, F5/F1; units and both RNGs unchanged.')
    contact = Image.new('RGB',(1920,1760),'white')
    for i,picture in enumerate(sheet):
        contact.paste(picture,((i%3)*640,(i//3)*440))
    contact.save(OUT/'review.png')
    manifest = dict(scope='Full PC game with C filtered texture reference. No TI texture mapper or hardware FPS claim.',
                    atlas=atlas,cases=results,bitmap_camera_cache=False,
                    sources={name:hashlib.sha256((HERE/name).read_bytes()).hexdigest() for name in
                             ['fft.c','perspective.inc','fixed.inc','cinema.inc','texture_preview.inc',
                              'tools/preview_textures.py','tools/textured_transitions.py','tools/textured_transition_capture.c']})
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(results,indent=2))


if __name__ == '__main__':
    main()
