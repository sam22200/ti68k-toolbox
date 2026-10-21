#!/usr/bin/env python3
"""Native Orbonne/Gariland: original fills, grouped fills, exterior contour.

Uses identical logical reach, camera and key replays. Retail fixtures and all
captures remain local. Native builds are deliberately serialized.
"""
from pathlib import Path
import argparse
import json
import re
import statistics
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
GAME = ROOT/'games/fft'
OUT = GAME/'campaign/x/contour'


def run(args, cwd=ROOT):
    p = subprocess.run(list(map(str, args)), cwd=cwd, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode:
        print(p.stdout)
        p.check_returncode()
    return p.stdout


def fixture(folder, source, banks, contour):
    folder.mkdir(parents=True, exist_ok=True)
    raw = (source/(banks[0]+'.bin')).read_bytes()
    if raw[-10:-6] in (b'FC01',b'FC02'):raw=raw[:int.from_bytes(raw[-6:-4],'big')]
    padding = b'\0' if len(raw)&1 else b''
    offset = len(raw)+len(padding)
    for name in banks:
        for suffix in ('.bin', '.89y'):
            target = folder/(name+suffix)
            if target.exists() or target.is_symlink():
                target.unlink()
            if contour and name == banks[0]:
                if suffix == '.bin':
                    target.write_bytes(raw+padding+(GAME/'fftcont.bin').read_bytes())
            else:
                target.symlink_to(source/target.name)
    if contour:
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat',
             banks[0]+'.bin', banks[0]], cwd=folder)
        p = folder/(banks[0]+'.89y')
        data = bytearray(p.read_bytes()); data[0x49] = 3; p.write_bytes(data)
    return offset


def native_values(log, opening):
    vals = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
    if opening:
        at = vals.index(0x0B0E0001)
        return vals[:at-1], vals[at+3:], vals[at+2]
    return vals[:-3], [vals[-1]], None


def reach_counts(pc):
    found = re.search(r'REACH (\d+) (\d+) (\d+) ([0-9A-F]+)', pc)
    width, height, rot = map(int, found.groups()[:3])
    data = bytes.fromhex(found[4]); boundary = edges = 0
    for i, cost in enumerate(data):
        if cost == 255:
            continue
        x, z = i % width, i // width
        n = sum((z == 0 or data[i-width] == 255,
                 x == width-1 or data[i+1] == 255,
                 z == height-1 or data[i+width] == 255,
                 x == 0 or data[i-1] == 255))
        boundary += n != 0; edges += n
    return dict(reachable=sum(v != 255 for v in data), boundary_tiles=boundary,
                exterior_edges=edges, rotation=rot), found[4]


def comparison():
    from PIL import Image, ImageDraw
    sheet=Image.new('RGB',(640,460),'#eeeeee');draw=ImageDraw.Draw(sheet)
    for row,scene in enumerate(('orbonne','gariland')):
        for col,variant in enumerate(('filled','contour')):
            with Image.open(OUT/(scene+'-'+variant)/'view0-ti.png') as source:
                picture=source.crop((0,0,160,100)).convert('RGB').resize((320,200),Image.Resampling.NEAREST)
            x,y=col*320,row*230;sheet.paste(picture,(x,y+24))
            draw.text((x+6,y+6),scene.title()+' - '+('full tiles' if col==0 else 'outside contour'),fill='black')
    sheet.save(OUT/'comparison.png')


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('--baseline-ref', default='5cbffb5',
                        help='commit before grouped reach patches')
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python', GAME/'tools/contour_assets.py'])
    original = run(['git', 'show', args.baseline_ref+':games/fft/fft.c'])
    rows = []; checks = {}; temporary = []
    try:
        f = tempfile.NamedTemporaryFile(mode='w', dir=GAME, prefix='.ring-original-', suffix='.c', delete=False)
        f.write(original); f.close(); old_source = Path(f.name); temporary.append(old_source)
        for scene, opening, scenario, banks in (
            ('orbonne', True, 1, ['ffov0','ffov1','ffov2','ffov3','ffoa','ffoi','ffost','ffcu','ffcw','ffca','ffcb']),
            ('gariland', False, 61, ['ffgv0','ffgv1','ffgv2','ffgv3','ffga','ffgi','ffcu','ffcw','ffca','ffcb'])):
            source = GAME/('opening' if opening else 'campaign')
            template = (source/'check_native.c').read_text()
            # Host-only instrumentation records the actual reach independently
            # of the renderer and of the tactical state hash.
            counts = ('printf("REACH %u %u %u ",MAP_W,MAP_H,st.rot);'
                      'for(unsigned k=0;k<MAP_W*MAP_H;k++)printf("%02X",st.reach[k]);puts("");\n')
            needle = 'if(strcmp(argv[4],"-"))' if opening else 'return sw_write_png(argv[4],1);'
            template = template.replace(needle, counts+needle)
            for variant in ('original', 'filled', 'contour'):
                folder = OUT/(scene+'-'+variant)
                offset = fixture(folder, source/'data', banks, variant == 'contour')
                code = template
                if variant == 'original':
                    include = '#include "opening.c"' if opening else '#include "gariland.c"'
                    defines = '#define FFT_OPENING\n' if opening else '#define FFT_STORY_GAR\n'
                    code = code.replace(include, defines+'#define FFT_SOURCE_CAMERA\n#include "'+str(old_source)+'"')
                f = tempfile.NamedTemporaryFile(mode='w', dir=source, prefix='.ring-check-', suffix='.c', delete=False)
                f.write(code); f.close(); wrapper = Path(f.name); temporary.append(wrapper)
                flags = [] if opening else ['-DCHECK_GAR']
                flags += ['-DFFT_NO_REACH_CONTOUR']
                if variant == 'contour':
                    flags += ['-DFFT_REACH_CONTOUR','-DFFT_CONTOUR_DEFAULT', '-DFFT_CONTOUR_OFFSET='+str(offset)]
                binary = folder/'ringchk'
                run(['cc', '-std=gnu99', '-O1', *flags, '-o', binary, wrapper,
                     ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c'])
                run([ROOT/'tools/bin/ti-cc', '-o', folder/'ringnat', '-DRT_CYCLES',
                     '-DFFT_MEASURE', *flags, wrapper, ROOT/'runtime/core/rt_core.c',
                     ROOT/'lib/unpack68k.s', ROOT/'tools/extgraph/lib/tilemap.a'])
                for turns in range(4):
                    keys = folder/('view%d.keys'%turns)
                    keys.write_text('0\n'+''.join('%d F5\n%d\n'%(10+30*k,11+30*k) for k in range(turns))+'120 ENTER\n121\n')
                    png = folder/('view%d'%turns)
                    pc = run([binary, scenario, 145, keys, str(png)+'-pc.png'], cwd=folder)
                    command = [ROOT/'tools/bin/ti-cycles', '--max', 1000, '--arg', scenario,
                               '--frames', 145, '--keys', keys, '--png', str(png)+'-ti.png']
                    for name in banks:
                        command += ['--file', folder/(name+'.89y')]
                    ti = run([*command, folder/'ringnat.89z'], cwd=folder)
                    png.with_suffix('.log').write_text(ti)
                    values, state, work = native_values(ti, opening)
                    assert len(values) == 290, (scene, variant, turns)
                    checksum = re.search(r'checksum\s+(\w+)', ti)[1]
                    assert checksum.lower() == re.search(r'checksum\s+(\w+)', pc)[1].lower()
                    expected = ([int(v,16) for v in re.findall(r'STATE,(\w+)',pc)] if opening
                                else [int(re.search(r'state (\w+)',pc)[1],16)])
                    assert state == expected
                    count, reach = reach_counts(pc)
                    key = (scene, turns)
                    if variant == 'original': checks[key] = (state, reach, checksum)
                    assert checks[key][:2] == (state, reach), 'renderer changed gameplay'
                    if variant == 'filled': assert checks[key][2] == checksum
                    frames = {m&65535:c for m,c in zip(values[::2],values[1::2])}
                    ring = int(re.search(r'^10\s+-\s+1\s+(\d+)', ti, re.M)[1])
                    row = dict(scene=scene, variant=variant, turns=turns, **count,
                               frame_cycles=frames[120], ring_cycles=ring,
                               warm_average_cycles=round(statistics.mean(frames[i] for i in range(125,145))),
                               native_work_bytes=work, executable_bytes=(folder/'ringnat.89z').stat().st_size,
                               checksum=checksum)
                    rows.append(row); print(json.dumps(row), flush=True)
                if variant == 'contour':
                    test = folder/'cache-test'
                    run(['cc', '-std=gnu99', '-O1', *flags,'-UFFT_CONTOUR_DEFAULT',
                         '-DTEST_OPEN' if opening else '-DTEST_CAMPAIGN',
                         '-DTEST_CONTOUR_DATA="'+str(folder)+'"', '-o', test,
                         GAME/'test_rings.c', ROOT/'runtime/core/rt_core.c',
                         ROOT/'runtime/platform-sw/rt_sw.c'])
                    print(run([test], cwd=GAME).strip(), flush=True)
        (OUT/'benchmark.json').write_text(json.dumps(dict(
            measurement='MC68000 datasheet CPU cycles, nominal 12MHz; no grayscale/wait/hardware FPS claim',
            baseline_ref=args.baseline_ref, contour_payload_bytes=(GAME/'fftcont.bin').stat().st_size,
            cases=rows), indent=2)+'\n')
        comparison()
        print('24 native/PC views checked; gameplay and reach identical across strategies')
    finally:
        for p in temporary:
            p.unlink(missing_ok=True)


if __name__ == '__main__':
    main()
