#!/usr/bin/env python3
"""Original MAP022 scenery on the two reviewed shot endpoints, using exact C face coverage."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

import numpy as np
from PIL import Image
from texture_review import HERE, TONES, extract, pack, texture_scene, unpack

OUT = HERE/'x/attack-camera/textures'


def load_scene():
    disc, files = extract.disc_files(str(HERE/'../../roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'))
    records = extract.gns_records(disc, files)
    _, sector, size = next(r for r in records if r[0] == 0x2e01)
    mesh = disc.read(sector, size)
    _, sector, size = next(r for r in records if r[0] == 0x1701)
    raw_texture = disc.read(sector, size)
    source = HERE/'x/scenery-textures/manifest-pc.json'
    if not source.exists():
        raise RuntimeError('Run make fixed-scenery-textures once to establish the existing isometric grey thresholds.')
    original = json.loads(source.read_text())
    assert original['mesh_sha256'] == hashlib.sha256(mesh).hexdigest()
    assert original['texture_sha256'] == hashlib.sha256(raw_texture).hexdigest()
    thresholds = np.array(original['thresholds'])
    strong = thresholds[1]+(thresholds-thresholds[1])*.65
    raw = np.frombuffer(raw_texture,np.uint8).reshape(1024,128)
    tex = np.empty((1024,256),np.uint8); tex[:,::2],tex[:,1::2] = raw & 15,raw >> 4
    palettes = np.array(struct.unpack_from('<256H',mesh,struct.unpack_from('<I',mesh,0x44)[0])).reshape(16,16)
    bank = (HERE/'fftmesh.bin').read_bytes(); nv,nf,_,np_ = struct.unpack_from('>4H',bank)
    extra = 2120+nv*6+nf*8+np_*8; ne = struct.unpack_from('>H',bank,extra)[0]
    materials = np.frombuffer(bank,np.uint8,nf,extra+2+ne*4); water_faces = (materials & 4) != 0
    vertices = np.frombuffer(bank,'>i2',nv*3,2120).reshape(nv,3)
    faces = []
    for world,uv,palette,page in extract.polygons(mesh):
        world = world*np.array([1,-1,1])
        if not np.any(np.cross(world[1]-world[0],world[2]-world[0])):
            continue
        indices = list(struct.unpack_from('>3H',bank,2120+nv*6+len(faces)*8))
        assert np.array_equal(vertices[indices],world)
        faces.append((indices,uv,palette,page))
    assert len(faces) == nf
    return faces, tex, palettes, thresholds, strong, water_faces


def prepare():
    OUT.mkdir(parents=True, exist_ok=True)
    faces, tex, palettes, thresholds, strong, water_faces = load_scene()
    binary = OUT/'capture-flat'
    subprocess.run(['cc','-std=gnu99','-O1','-g','-DFFT_EXPLORATION','-DFFT_FIXED','-DFFT_CINEMA','-DFFT_TEXTURE_REVIEW',
                    '-o',str(binary),'tools/attack_texture_capture.c','../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'],cwd=HERE,check=True)
    result = bytearray(struct.pack('>2H',0x4353,2)); manifest = {}
    for name,scenario in [('melee',65),('ranged',66)]:
        folder = OUT/name; folder.mkdir(exist_ok=True)
        text = subprocess.check_output([str(binary),str(scenario),str(folder)],cwd=HERE,text=True)
        metadata = json.loads(text); (folder/'metadata.json').write_text(text)
        labels = np.frombuffer((folder/'faces.bin').read_bytes(),'>u2').reshape(100,160)
        edges = np.frombuffer((folder/'edges.bin').read_bytes(),np.uint8).reshape(100,160)
        base = unpack((folder/'background.bin').read_bytes()); samples = {}
        assert np.count_nonzero(labels) > 3000
        grey, counts = texture_scene(base,labels,edges,np.array(metadata['vertices'],float),faces,tex,palettes,thresholds,True,samples,water_faces)
        valid = np.isfinite(samples['luminance']) & (edges == 0)
        lum = samples['luminance'][valid]
        grey[valid] = np.where(lum > strong[2],0,np.where(lum > strong[1],1,np.where(lum > strong[0],2,3)))
        water = np.isin(labels,np.flatnonzero(water_faces)+1) & (edges == 0)
        assert set(np.unique(grey[water])).issubset({1,2})
        assert np.array_equal(grey[edges != 0],base[edges != 0])
        data = pack(grey); assert len(data) == 6000 and np.array_equal(unpack(data),grey)
        (folder/'textured.bin').write_bytes(data); Image.fromarray(TONES[grey]).save(folder/'background.png')
        result.extend(bytes(metadata['camera'])); result.extend(data)
        manifest[name] = dict(camera=metadata['camera'],**counts,water_pixels=int(water.sum()))
    assert len(result) == 12028
    (HERE/'fftshot.bin').write_bytes(result)
    (OUT/'manifest.json').write_text(json.dumps(dict(cameras=manifest,scope='Two exact reviewed endpoint cameras; projection offline on PC.',
                    contrast='Existing contrast-plus; original grey water and C glints.',bank_sha256=hashlib.sha256(result).hexdigest()),indent=2)+'\n')
    return manifest


if __name__ == '__main__':
    print(json.dumps(prepare(),indent=2))
