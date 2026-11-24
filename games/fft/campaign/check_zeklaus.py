#!/usr/bin/env python3
"""Approved C2 through the shared variable-width engine; no installed files changed.

Private source copies only resolve include paths; no renderer/layout substitutions.
The arena uses source terrain and a labelled borrowed Gariland roster, not an
authored Chapter I encounter. No shipping groups, saves or emulator are changed.
"""
import argparse
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
FFT = HERE.parent
ROOT = HERE.parents[2]
sys.path.insert(0,str(FFT/'tools'))
import chapter_desc
import chapter_resources
import extract
import map_art_review as art
import map_material_review as materials
import surface_data


def run(command,cwd,log):
    p = subprocess.run(list(map(str,command)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                       env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','UBSAN_OPTIONS':'halt_on_error=1'})
    log.write_text(p.stdout)
    if p.returncode:
        print(p.stdout)
        p.check_returncode()
    return p.stdout


def overlays(out):
    """Use the actual shared source with isolated compiler intermediates."""
    folder = out/'source'
    folder.mkdir(exist_ok=True)
    paths = {FFT/name:folder/Path(name).name for name in
             ('fft.c','fft.h','view_format.h','chapter_files.inc','campaign/native_battle.c','campaign/battle.c')}
    for source,target in paths.items():
        text = source.read_text()
        def include(match):
            original = (source.parent/match[1]).resolve()
            return '#include "'+str(paths.get(original,original))+'"'
        text = re.sub(r'#include "([^"]+)"',include,text)
        target.write_text(text)
    return folder


def arena(data,source_dir,approved):
    # Reuse Gariland as a second202-row arena to exercise width transitions in
    # one process. Route0 is explicitly a control, not the source Orbonne event.
    for suffix in ('d','i','a','v0','v1','v2','v3'):
        target=data/('ffo'+suffix+'.bin');target.unlink()
        target.symlink_to((data/('ffg'+suffix+'.bin')).resolve())
    # Legacy extraction controls keep these shared tables on view0. Hoist them
    # once to ffcu, as the shipping packager does, before replacing the views.
    common,tail = chapter_resources.contour_tail((data/'ffcu.bin').read_bytes())
    if tail is None:
        _,tail = chapter_resources.contour_tail((data/'ffgv0.bin').read_bytes())
        assert tail is not None
        offset = (len(common)+1)&~1
        common += bytes(offset-len(common))+tail+b'FC02'+struct.pack('>3H',len(common),offset,len(tail))
        (data/'ffcu.bin').unlink()
        (data/'ffcu.bin').write_bytes(common)
    info = json.loads((source_dir/'results.json').read_text())
    terrain = (source_dir/'terrain.bin').read_bytes()
    nx,nz = info['source']['nx'],info['source']['nz']
    assert (nx,nz,info['scene_rows'],info['source']['canvas_width']) == (10,18,188,352)
    shades = [[[] for _ in range(5)] for _ in range(nx*nz)]
    import numpy as np
    images = []
    for rot in range(4):
        grey = materials.read_grey(approved/'C2'/f'view-{rot}.png')
        assert art.style.pack(grey) == (approved/'C2'/f'view-{rot}.planes').read_bytes()
        images.append(grey)
        with np.load(source_dir/f'raster-{rot}.npz',allow_pickle=False) as cache:
            faces = cache['face'][info['scene_top']:info['scene_top']+info['scene_rows']]
            for face in np.unique(faces[faces>=0]):
                cell,side = divmod(int(face),5)
                if cell<nx*nz:
                    shades[cell][side].extend(grey[faces==face].tolist())
    tiles = bytearray()
    for cell in range(nx*nz):
        b = terrain[2+cell*8:10+cell*8]
        corners = extract.corners(b[2],b[3]&31,b[4] if b[3]&31 else 0)
        look = [int(round(sum(v)/len(v))) if v else 2 for v in shades[cell]]
        walk = 0 if b[6]&1 else 2 if b[6]&2 else 1
        tiles.extend([*corners,*look,walk,2*b[2]+((b[3]&31) if b[4] else 0),0])
    interface = bytearray((data/'ffgi.bin').read_bytes())
    tiles_at = len(interface)
    interface.extend(tiles)
    (data/'ffgi.bin').unlink()
    (data/'ffgi.bin').write_bytes(interface)
    descriptor,_ = surface_data.detach_descriptor((data/'ffgd.bin').read_bytes())
    descriptor = bytearray(descriptor)
    fields = {name:4+2*i for i,name in enumerate(chapter_desc.FIELDS)}
    def put(name,value):struct.pack_into('>H',descriptor,fields[name],value)
    for name,value in dict(MAP_W=nx,MAP_H=nz,SC_H=188,SC_TOP=info['scene_top'],TILES=tiles_at,
                           SOURCE_MAP=76,START=0,PARTY=0,WON=0,LOCATION=0,WATER=0,MAP_SCRIPT=0,
                           UNIT_RAW=0).items():
        put(name,value)
    count = struct.unpack_from('>H',descriptor,fields['UNIT_COUNT'])[0]
    definitions = struct.unpack_from('>H',descriptor,fields['UNIT_DEFS'])[0]
    free = [(x,z) for z in range(2,nz-2) for x in range(1,nx-1) if not terrain[2+8*(z*nx+x)+6]&3]
    # Deliberately a diagnostic roster; source encounter extraction is separate.
    positions = [(4,8),(6,8),(5,10)]+[p for p in free if p not in ((4,8),(6,8),(5,10))]
    for i in range(count):
        at = definitions+28*i
        descriptor[at+8:at+10] = bytes(positions[i])
    # WATER is a required offset even for a dry arena: four view offsets, each
    # pointing to four zero phase counts. Zero itself means descriptor header.
    descriptor.extend(bytes(len(descriptor)&1))
    put('WATER',len(descriptor))
    descriptor.extend(struct.pack('>4H',8,8,8,8)+bytes(8))
    descriptor = surface_data.attach_descriptor(descriptor,surface_data.encode(terrain[2:],nx,nz,True))
    (data/'ffgd.bin').unlink()
    (data/'ffgd.bin').write_bytes(descriptor)
    sizes = []
    for rot,grey in enumerate(images):
        with np.load(source_dir/f'raster-{rot}.npz',allow_pickle=False) as cache:
            diagonal = cache['diagonal'][info['scene_top']:info['scene_top']+188]
        depth = art.depth_planes(diagonal)
        colors = art.style.pack(grey)
        ragged = extract.ragged_depth(colors+depth,row_bytes=44)
        assert len(ragged)<=18488
        bank = extract.view_bank(b'R4',colors,b'',ragged,row_bytes=44)
        tag,actual,mask,unpacked = extract.read_view(bank,row_bytes=44)
        assert (tag,actual,mask,unpacked)==(b'R5',colors,b'',depth)
        path = data/f'ffgv{rot}.bin'
        path.unlink()
        path.write_bytes(bank)
        (data/f'expected-{rot}.bin').write_bytes(colors+depth)
        sizes.append(dict(view=rot+1,bank_bytes=len(bank),depth_bytes=len(ragged),colors_sha256=art.sha(colors)))
    return dict(width=352,rows=188,nodes=180,source_map=76,source=info['source'],
                roster='Borrowed Gariland diagnostic actors, not source Zeklaus encounter',views=sizes)


def measure_layout(source,out):
    sdk = ROOT/'tools/gcc4ti-bin'
    os.environ['TIGCC'] = str(sdk)
    carrier = source/'native-layout.c'
    carrier.write_text('#define RT_CYCLES\n#include "'+str(source/'native_battle.c')+'"\n'+
                       ''.join('const unsigned short wide_'+n+'='+v+';\n' for n,v in
                               [('State','sizeof(State)'),('Save','sizeof(Save)'),('Work','sizeof(Work)'),
                                ('scene','SC_BUFFER_BYTES'),('stride','SC_BYTES_MAX'),('rows','SC_HMAX')]))
    run([sdk/'bin/tigcc','-S','-Os','-mno-bss','-mregparm=5','-I'+str(ROOT/'tools/extgraph/lib'),'-I'+str(FFT),
         '-o',source/'native-layout.s',carrier],source,out/'native-layout-build.log')
    text = (source/'native-layout.s').read_text()
    layout = {n:int(v)&65535 for n,v in re.findall(r'^wide_(\w+):\s*\n\s*\.word\s+(-?\d+)',text,re.M)}
    assert layout == dict(State=2278,Save=4684,Work=37790,scene=16544,stride=44,rows=202)
    return layout


def render_sheet(out):
    materials.preferred.full_sheet(out,[art.Image.open(out/'pc'/f'view-{r}.png') for r in range(4)],
                                   dict(source=dict(canvas_width=160),scene_rows=100),
                                   filename='C2-engine-four-views.png',
                                   heading='C2 / actual PC battle / borrowed diagnostic actors')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path,default=HERE/'x/wide-core/latest')
    parser.add_argument('--native-layout',action='store_true',help='Compile only one68000 layout carrier, no emulator')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True,exist_ok=True)
    source = overlays(out)
    wrapper = source/'bridge.c'
    wrapper.write_text('#include <stdlib.h>\nstatic size_t fail_scene_bytes;\n'
                      'static void *checked_scene_malloc(size_t n){if(n==fail_scene_bytes)return NULL;return malloc(n);}\n'
                      '#define malloc checked_scene_malloc\n#define FFT_BATTLE\n#define FFT_SOURCE_CAMERA\n#define FFT_DEFAULT_CHAPTER 1\n#include "'+str(source/'fft.c')+'"\n#include "'+str(HERE/'check_zeklaus.c')+'"\n')
    from check_flow import data_links
    results = []
    with tempfile.TemporaryDirectory(prefix='data-',dir=out) as name:
        data = Path(name)
        data_links(data)
        # This control intentionally uses old route-letter fixtures without an
        # installed core manifest; never mix a wide arena with production ABI97.
        for n in ('ffcore.bin','ffpack.bin','ffsave.sav'):
            (data/n).unlink(missing_ok=True)
        metadata = arena(data,FFT/'campaign/x/zeklaus-review',FFT/'campaign/x/zeklaus-materials')
        for mode in ('pc','san'):
            flags = ['-fsanitize=address,undefined','-fno-sanitize=shift','-fno-omit-frame-pointer','-no-pie'] if mode=='san' else []
            binary = out/('zeklaus-'+mode)
            run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',binary,wrapper,
                 ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'],ROOT,out/(mode+'-build.log'))
            log = run([binary,out/mode],data,out/(mode+'.log'))
            results.append(log)
        assert results[0] == results[1], 'PC/sanitizer state or framebuffer differs'
        render_sheet(out)
        # Keep the private arena for further bounded runtime/native work.
        snapshots = out/'arena'
        snapshots.mkdir(exist_ok=True)
        for n in ('ffcu','ffgd','ffgi','ffgv0','ffgv1','ffgv2','ffgv3'):
            (snapshots/(n+'.bin')).write_bytes((data/(n+'.bin')).read_bytes())
    report = dict(arena=metadata,pc_sanitizers_equal=True,logs=results[0].splitlines(),
                  production_abi_changed=True,shipping_groups_changed=False,native_runtime_tested=False,
                  scope='Actual shared variable-width battle engine on PC; private source copies do not change layouts; not installed native compatibility',
                  skill='.claude/skills/fft-map-extraction/SKILL.md')
    if args.native_layout:
        report['native_layout'] = measure_layout(source,out)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    main()
