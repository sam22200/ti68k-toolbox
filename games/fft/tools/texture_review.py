#!/usr/bin/env python3
"""PC-only Gariland scenery texture review with the current TI-style actors and cutaway.

Use the same MAP022 texture atlas, UVs and grey thresholds as the isometric asset pipeline.
No image warping of an isometric screenshot; actual winning C polygons select the texels.
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw
import extract
from perspective_demo import gif

HERE = Path(__file__).resolve().parents[1]
OUT = HERE/'x/scenery-textures'
VIEWS = ['close-front', 'close-back', 'wide-front', 'wide-back']
TONES = np.array([255, 170, 85, 0], np.uint8)


def normalize(path):
    tones = {216: 255, 152: 170, 88: 85, 24: 0}
    return Image.open(path).convert('L').point([tones.get(x, x) for x in range(256)]).convert('RGB')


def unpack(data):
    planes = np.frombuffer(data, np.uint8).reshape(2, 100, 30)
    pixels = np.unpackbits(planes, axis=2)[:, :, :160]
    return pixels[0]+pixels[1]*2


def pack(grey):
    return b''.join(np.packbits(np.pad((grey >> bit) & 1, ((0, 0), (0, 80))), axis=1).tobytes()
                    for bit in [0, 1])


def texture_scene(base, labels, edges, camera, faces, tex, palettes, thresholds, filtered, samples=None, water_faces=None):
    lum_pal = ((palettes & 31)*.299+((palettes >> 5) & 31)*.587+((palettes >> 10) & 31)*.114)*255/31
    grey = base.copy()
    if samples is not None:
        samples['luminance'] = np.full(base.shape, np.nan)
    sampled = transparent = 0
    offsets = [-.375, -.125, .125, .375] if filtered else [0.]
    for i, (indices, uv, palette, page) in enumerate(faces):
        is_water = water_faces is not None and water_faces[i]
        if uv is None:
            continue
        yy, xx = np.nonzero(labels == i+1)
        if not len(xx):
            continue
        a, b, c = camera[indices]
        e0, e1 = b-a, c-a
        normal = np.cross(e0, e1)
        d00, d01, d11 = e0@e0, e0@e1, e1@e1
        determinant = d00*d11-d01*d01
        assert abs(determinant) > 1e-9
        uv = np.asarray(uv)
        accum, count = np.zeros(len(xx)), np.zeros(len(xx), int)
        for oy in offsets:
            for ox in offsets:
                ray = np.stack([(xx+.5+ox-80)/96, (48-yy-.5-oy)/96, np.ones(len(xx))], axis=1)
                distance = (normal@a)/(ray@normal)
                assert np.isfinite(distance).all()
                hit = ray*distance[:, None]-a
                d20, d21 = hit@e0, hit@e1
                v = (d11*d20-d01*d21)/determinant
                w = (d00*d21-d01*d20)/determinant
                coord = uv[0]+v[:, None]*(uv[1]-uv[0])+w[:, None]*(uv[2]-uv[0])
                u, t = np.floor(coord).astype(int).T
                index = tex[np.clip(t, 0, 255)+256*page, np.clip(u, 0, 255)]
                opaque = palettes[palette, index] != 0
                accum += np.where(opaque, lum_pal[palette, index], 0)
                count += opaque
        valid = count > 0
        luminosity = accum/np.maximum(count, 1)
        levels = np.where(luminosity > thresholds[2], 0, np.where(luminosity > thresholds[1], 1,
                          np.where(luminosity > thresholds[0], 2, 3)))
        if is_water:
            levels = np.clip(levels, 1, 2) # texture survives; white is reserved for animated glints
        grey[yy[valid], xx[valid]] = levels[valid]
        if samples is not None and not is_water:
            samples['luminance'][yy[valid], xx[valid]] = luminosity[valid]
        sampled += int(valid.sum())
        transparent += int((~valid).sum())
    grey[edges != 0] = 3
    # Sky/cutaway gaps remain intact; selected dotted architecture never acquires a fill.
    assert np.array_equal(grey[(labels == 0) & (edges == 0)], base[(labels == 0) & (edges == 0)])
    assert np.array_equal(unpack(pack(grey)), grey)
    return grey, dict(textured_pixels=sampled, transparent_flat_fallback_pixels=transparent)


def main():
    disc_path = sys.argv[1] if len(sys.argv) > 1 else str(HERE/'../../roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue')
    OUT.mkdir(parents=True, exist_ok=True)
    disc, files = extract.disc_files(disc_path)
    records = extract.gns_records(disc, files)
    _, sector, size = next(r for r in records if r[0] == 0x2e01)
    mesh = disc.read(sector, size)
    _, sector, size = next(r for r in records if r[0] == 0x1701)
    texture_data = disc.read(sector, size)
    raw = np.frombuffer(texture_data, np.uint8).reshape(1024, 128)
    tex = np.empty((1024, 256), np.uint8)
    tex[:, ::2], tex[:, 1::2] = raw & 15, raw >> 4
    palettes = np.array(struct.unpack_from('<256H', mesh, struct.unpack_from('<I', mesh, 0x44)[0])).reshape(16, 16)
    bank = (HERE/'fftmesh.bin').read_bytes()
    nv, nf = struct.unpack_from('>2H', bank)
    extra = 2120+nv*6+nf*8+struct.unpack_from('>H', bank, 6)[0]*8
    edge_count = struct.unpack_from('>H', bank, extra)[0]
    materials = np.frombuffer(bank, np.uint8, nf, extra+2+edge_count*4)
    water_faces = (materials & 4) != 0
    vertices = np.frombuffer(bank, '>i2', nv*3, 2120).reshape(nv, 3)
    faces = []
    for world, uv, palette, page in extract.polygons(mesh):
        world = world*np.array([1, -1, 1])
        if not np.any(np.cross(world[1]-world[0], world[2]-world[0])):
            continue
        indices = list(struct.unpack_from('>3H', bank, 2120+nv*6+len(faces)*8))
        assert np.array_equal(vertices[indices], world), 'UV/mesh face correspondence'
        faces.append((indices, uv, palette, page))
    assert len(faces) == nf
    # Recompute exactly the contrast thresholds used by the existing isometric extraction.
    terrain = struct.unpack_from('<I', mesh, 0x68)[0]
    nx, nz = mesh[terrain:terrain+2]
    maxh = 0
    for i in range(nx*nz):
        b = mesh[terrain+2+i*8:terrain+10+i*8]
        maxh = max(maxh, *extract.corners(b[2], b[3] & 31, b[4] if b[3] & 31 else 0))
    sc_h = 6*(nx+nz)+6*maxh+8
    luminosity, _, _ = extract.render(extract.polygons(mesh), tex, palettes, 0, nx, nz, sc_h)
    small = luminosity.reshape(sc_h, extract.K, extract.SC_W, extract.K).mean((1, 3))
    background = (luminosity == extract.BG).reshape(sc_h, extract.K, extract.SC_W, extract.K).mean((1, 3)) > .5
    thresholds = np.percentile(small[~background], [25, 55, 82])
    del luminosity
    binary = OUT/'capture-pc'
    subprocess.run(['cc', '-std=gnu99', '-O1', '-DFFT_EXPLORATION', '-DFFT_FIXED', '-DFFT_TEXTURE_REVIEW',
                    '-o', str(binary), 'tools/texture_review_capture.c', '../../runtime/core/rt_core.c',
                    '../../runtime/platform-sw/rt_sw.c'], cwd=HERE, check=True)
    images, stats = {}, {}
    checked = 0
    for view, name in enumerate(VIEWS):
        folder = OUT/name/'flat'
        folder.mkdir(parents=True, exist_ok=True)
        log = subprocess.check_output([str(binary), str(view), str(folder)], cwd=HERE, text=True)
        metadata = json.loads(log)
        (folder/'metadata.json').write_text(log)
        camera = np.array(metadata['vertices'], float)
        labels = np.frombuffer((folder/'faces.bin').read_bytes(), '>u2').reshape(100, 160)
        edges = np.frombuffer((folder/'edges.bin').read_bytes(), np.uint8).reshape(100, 160)
        base = unpack((folder/'background.bin').read_bytes())
        images[name, 'flat'] = [normalize(folder/f'{n:03d}.png') for n in range(32)]
        for n, image in enumerate(images[name, 'flat']):
            reference = HERE/f'x/fixed-review/{name}-auto/{n:03d}.png'
            if reference.exists():
                assert image.tobytes() == normalize(reference).tobytes()
                checked += 1
        for style, filtered in [('textures', True), ('nearest', False)]:
            samples = {} if filtered else None
            grey, counts = texture_scene(base, labels, edges, camera, faces, tex, palettes, thresholds, filtered, samples, water_faces)
            water = np.isin(labels, np.flatnonzero(water_faces)+1)
            assert set(np.unique(grey[water & (edges == 0)])).issubset({1, 2})
            stats[name+'-'+style] = counts
            output = OUT/name/style
            output.mkdir(parents=True, exist_ok=True)
            if filtered:
                np.save(output/'luminance.npy', samples['luminance'], allow_pickle=False)
            planes = output/'textured.bin'
            planes.write_bytes(pack(grey))
            Image.fromarray(TONES[grey]).save(output/'background.png')
            replay = subprocess.check_output([str(binary), str(view), str(output), str(planes)], cwd=HERE, text=True)
            (output/'metadata.json').write_text(replay)
            assert json.loads(replay)['vertices'] == metadata['vertices']
            images[name, style] = [normalize(output/f'{n:03d}.png') for n in range(32)]
            # Remapping is confined to water: changed dry pixels still come only from texturing.
            if filtered:
                assert not np.any(np.isfinite(samples['luminance'])[water])
            for old, new in zip(images[name, 'flat'], images[name, style]):
                assert np.array_equal(np.asarray(old)[:8], np.asarray(new)[:8])
            gif(OUT/f'{name}-{style}.gif', images[name, style], [100]*32)
    for file, styles in [('flat-textured', ['flat', 'textures']), ('nearest-filtered', ['nearest', 'textures'])]:
        frames = []
        for n in range(32):
            image = Image.new('RGB', (len(styles)*320, 880), 'white')
            draw = ImageDraw.Draw(image)
            for row, name in enumerate(VIEWS):
                for col, style in enumerate(styles):
                    x, y = col*320, row*220
                    draw.text((x+4, y+4), name.upper()+' / '+style.upper(), fill='black')
                    image.paste(images[name, style][n].resize((320, 200), Image.Resampling.NEAREST), (x, y+20))
            frames.append(image)
        gif(OUT/(file+'.gif'), frames, [100]*32)
        frames[8].save(OUT/(file+'.png'))
    manifest = dict(platform='PC-only offline texture mapping, replayed by the C renderer; no TI test.',
                    timing='10 FPS review playback, not a performance measurement.',
                    source_frames=128, replay_frames=256, unchanged_flat_reference_frames=checked,
                    thresholds=thresholds.tolist(), stats=stats,
                    water='Original canal texture clamped to light/dark grey; original C glints; excluded from dry-surface contrast conversion.',
                    water_face_ids=(np.flatnonzero(water_faces)+1).tolist(),
                    mesh_sha256=hashlib.sha256(mesh).hexdigest(), texture_sha256=hashlib.sha256(texture_data).hexdigest(),
                    sources={f:hashlib.sha256((HERE/f).read_bytes()).hexdigest() for f in
                             ['fft.c', 'fft.h', 'fixed.inc', 'perspective.inc', 'tools/texture_review.py',
                              'tools/texture_review_capture.c', 'tools/extract.py']})
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(f'{nf} UV faces verified; {checked} unchanged flat frames; 256 textured PC replay frames. No TI validation.')


if __name__ == '__main__':
    main()
