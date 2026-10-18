#!/usr/bin/env python3
"""Original ISO art with perspective contrast and visibility-tested geometric ledges.

Generate colour variants offline; leave the original files, sprites and occlusion data
intact. No texture edge detection or all-tile grid is used for the relief outlines.
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess

import numpy as np
from PIL import Image
from attack_texture import load_scene
from texture_review import HERE, TONES, extract

OUT = HERE/'x/isometric-style'


def pack(grey):
    return b''.join(np.packbits((grey >> bit) & 1,axis=1).tobytes() for bit in [0,1])


def ledges(polys):
    adjacency = {}
    for v,*_ in polys:
        n = np.cross(v[1]-v[0],v[2]-v[0]); length = np.linalg.norm(n)
        if length == 0:
            continue
        top = abs(n[1])/length > .5
        side = abs(n[1])/length < .2
        for a,b in zip(v,np.roll(v,-1,axis=0)):
            edge = tuple(sorted((tuple(a.astype(int)),tuple(b.astype(int)))))
            adjacency.setdefault(edge,[]).append((top,side))
    # A top/sloping walkable surface meeting a vertical wall is a real height boundary.
    # Coplanar seams and triangulation diagonals never satisfy this condition.
    return [np.array(edge,float) for edge,normals in adjacency.items()
            if any(t for t,s in normals) and any(s for t,s in normals)]


def project(v,rot,nx,nz):
    x,z,h = v[:,0]/28,v[:,2]/28,-v[:,1]/12
    u,w = [(x,z),(nz-z,x),(nx-x,nz-z),(z,nx-x)][rot]
    return np.stack([(u-w+(nx if rot & 1 else nz))*12,
                     extract.SC_OY+(u+w)*6-h*6,u+w+2*h],axis=1)


def outline(grey,depth,edges,rot,nx,nz):
    height,width = grey.shape; mask = np.zeros_like(grey,dtype=bool)
    for edge in edges:
        a,b = project(edge,rot,nx,nz)
        steps = max(1,int(np.ceil(np.max(np.abs(b[:2]-a[:2]))*4)))
        for t in np.linspace(0,1,steps+1):
            p = a+(b-a)*t; x,y = np.floor(p[:2]).astype(int)
            if not (0 <= x < width and 0 <= y < height):
                continue
            # Test the projected point against original supersampled mesh depth. Small
            # subpixel slack covers its own face slope, never a house hiding the ledge.
            sx = np.clip(int(p[0]*extract.K),0,width*extract.K-1)
            sy = np.clip(int(p[1]*extract.K),0,height*extract.K-1)
            if depth[sy,sx] > p[2]+.15:
                continue
            if depth[sy,sx] < -1e8:
                continue
            mask[y,x] = True
    result = grey.copy()
    # Extremes chosen from the underlying tone: white on dark, black on light.
    result[mask] = np.where(grey[mask] >= 2,0,3)
    return result,mask


def prepare_iso():
    OUT.mkdir(parents=True,exist_ok=True)
    _,tex,palettes,thresholds,strong,_ = load_scene()
    disc,files = extract.disc_files(str(HERE/'../../roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'))
    _,sector,size = next(r for r in extract.gns_records(disc,files) if r[0] == 0x2e01)
    mesh = disc.read(sector,size); polys = extract.polygons(mesh)
    terrain = struct.unpack_from('<I',mesh,0x68)[0]; nx,nz = mesh[terrain:terrain+2]
    tile_heights = []
    for i in range(nx*nz):
        b = mesh[terrain+2+i*8:terrain+10+i*8]
        tile_heights += extract.corners(b[2],b[3]&31,b[4] if b[3]&31 else 0)
    sc_h = 6*(nx+nz)+6*max(tile_heights)+8; plane = sc_h*40
    edges = ledges(polys); styles = [[],[]]; info = {}
    for rot in range(4):
        lum,_,face,depth = extract.render(polys,tex,palettes,rot,nx,nz,sc_h,return_depth=True)
        small = lum.reshape(sc_h,4,320,4).mean((1,3))
        bg = (lum == extract.BG).reshape(sc_h,4,320,4).mean((1,3)) > .5
        original = np.where(small > thresholds[2],0,np.where(small > thresholds[1],1,np.where(small > thresholds[0],2,3))).astype(np.uint8)
        original[bg] = 0
        # Reuse existing depth/masks byte-for-byte; assert our baseline matches its art.
        raw = OUT/f'original-{rot}.bin'
        subprocess.run([HERE/'../../tools/bin/dzx0','-f',HERE/f'fftv{rot}.bin',raw],check=True,capture_output=True)
        source = raw.read_bytes(); assert len(source) == plane*6
        assert pack(original) == source[:plane*2]
        grey = np.where(small > strong[2],0,np.where(small > strong[1],1,np.where(small > strong[0],2,3))).astype(np.uint8)
        grey[bg] = 0
        # Preserve the established isometric canal tones and existing C water glints.
        tiles = np.frombuffer((HERE/'tiles.bin').read_bytes(),np.uint8).reshape(nz*nx,12)
        water_tiles = np.flatnonzero(tiles[:,11] != 0)
        visible_face = face[2::4,2::4]
        water = np.isin(visible_face,water_tiles*5)
        grey[water] = original[water]
        bordered,mask = outline(grey,depth,edges,rot,nx,nz)
        assert np.array_equal(bordered[~mask],grey[~mask])
        assert np.array_equal(grey[bg],original[bg])
        styles[0].append(pack(grey)); styles[1].append(pack(bordered))
        # Production path can substitute these colour planes into the existing six-plane
        # archive format; no depth or per-frame contour work is required.
        (OUT/f'variant-{rot}.raw').write_bytes(pack(bordered)+source[plane*2:])
        Image.fromarray(TONES[original]).save(OUT/f'original-{rot}.png')
        Image.fromarray(TONES[grey]).save(OUT/f'contrast-{rot}.png')
        Image.fromarray(TONES[bordered]).save(OUT/f'edges-{rot}.png')
        Image.fromarray((mask*255).astype(np.uint8)).save(OUT/f'mask-{rot}.png')
        info[rot] = dict(edge_pixels=int(mask.sum()),black_edge_pixels=int((bordered[mask] == 3).sum()),
                         white_edge_pixels=int((bordered[mask] == 0).sum()),water_pixels=int(water.sum()),
                         source_depth_sha256=hashlib.sha256(source[plane*2:]).hexdigest())
    data = struct.pack('>4H',0x4953,1,40,sc_h)+b''.join(styles[0]+styles[1])
    assert len(data) == 8+16*plane
    (HERE/'fftiso.bin').write_bytes(data)
    manifest = dict(scope='Offline colour-only ISO variants, PC preview loader; original art and all depths retained.',
                    styles=['original','contrast','contrast + adaptive geometric height edges'],
                    edge_candidates=len(edges),bank_bytes=len(data),views=info,
                    bank_sha256=hashlib.sha256(data).hexdigest())
    (OUT/'assets.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest


if __name__ == '__main__':
    print(json.dumps(prepare_iso(),indent=2))
