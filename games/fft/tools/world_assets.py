#!/usr/bin/env python3
"""Reviewed 67% source world texture, locations/routes and reduced Ramza idle poses.

usage: world_assets.py DISC.cue OUT.bin [preview.png]
FWM3: magic; five big-endian u16 offsets (compressed strips, compressed idle
poses, locations, routes, story); width/height in16px cells. Section0 has u16
row stride/count, then a u16 offset per independent8-row ZX0 stream. Each raw
strip is light rows followed by dark rows. Sixteen cached strips cover any
100-row viewport; the complete map is never allocated. Section1 has eight pose
indices then ZX0 for five unique16x26 sprites. Locations/routes/story keep the
FWM1 metadata layouts. Default scale is2/3 (the approved67% candidate), prepared
offline from the existing full-size four-grey artwork and idle poses. Source
contrast is retained. --scale 100 and --scale 75 are historical review controls.

Source texture: WORLD/WLDTEX.TM2's four CLUT rectangles. Source position and
route tables: WORLD/WLDCORE.BIN,0x2ddfc/0x2e008; texture origin(263,186).
Names: WORLD/WORLD.BIN,0x8e1a0. Ramza: TYPE1.SHP11,10,9,10,11,12,13,12.
The older greys()/tiles_of() helpers below are historical controls only;
they no longer provide active map artwork or defaults.
"""
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

import extract
import management_data
import interface
import units
import zx0pack

WLDCORE = 0x80067000
DOTS, ROUTES, NAMES = 0x2ddfc, 0x2e008, 0x8e1a0
ORIGIN = (263, 186)
TILES_MAX = 255
# Chapter I as far as it is ripped: after Gariland's victory (flag 1) its town and Mandalia
# Plains, the next battle, show, as on the original's map then. Per location: the story flag + 1
# that shows it, the battle program's door it enters (fftbat; 0 none: not ripped yet) and the
# flag + 1 that battle's victory sets (then the location is a town or a field again).
STORY = [(6, 2, 200, 2), (24, 2, 0, 0)]
SCALES = {'100': (1, 1), '75': (3, 4), '67': (2, 3)}
MAP_SCALE = SCALES['67']


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


def texture(disc, files):
    b = disc.read(*files['WORLD/WLDTEX.TM2'])
    # Each sector starts with a RECTANGLE COUNT, followed by that many
    # (x,y,w,h,pixels) uploads. There are three internal8-byte headers in an
    # ordinary sector, not one header plus1010 contiguous pixel words. Treating
    # those headers as pixels produced repeated vertical corruption at seams.
    groups,cur=[],[]
    for sector in range(0,len(b),2048):
        count=struct.unpack_from('<I',b,sector)[0];assert 1<=count<=3
        at=sector+4
        for _ in range(count):
            x,y,w,h=struct.unpack_from('<4H',b,at);at+=8
            end=at+2*w*h;assert end<=sector+2048
            cur.append((x,y,w,h,b[at:end]));at=end
        if count<3:groups.append(cur);cur=[]
    assert not cur and len(groups)>=4
    pieces = []
    for g in groups[:4]:
        assert g[0][:4]==(0,480,256,1)
        clut=np.frombuffer(g[0][4],'<u2').astype(int)
        x0=min(r[0] for r in g[1:]);y0=min(r[1] for r in g[1:])
        width=max(x+w for x,y,w,h,p in g[1:])-x0
        height=max(y+h for x,y,w,h,p in g[1:])-y0
        idx=np.zeros((height,width*2),np.uint8);covered=np.zeros_like(idx,bool)
        for x,y,w,h,pixels in g[1:]:
            target=np.s_[y-y0:y-y0+h,2*(x-x0):2*(x-x0+w)]
            assert not covered[target].any()
            idx[target]=np.frombuffer(pixels,np.uint8).reshape(h,w*2);covered[target]=True
        assert covered.all(),('source image holes',x0,y0,width,height)
        rgb = np.stack([(clut & 31) << 3, (clut >> 5 & 31) << 3, (clut >> 10 & 31) << 3], 1)
        pieces.append(rgb[idx].astype(float))
    top = np.concatenate([pieces[0][:241], pieces[1][:241]], 1)
    bottom = np.concatenate([pieces[2][:132], pieces[3][:132]], 1)
    m = np.concatenate([top, bottom], 0)
    return m


def land_mask(m):
    q = ndimage.median_filter(m[..., 2] / np.maximum(m[..., 0], 1), size=9)
    land = ndimage.binary_closing(ndimage.binary_opening(q < 0.47, iterations=2), iterations=2)
    for mask, keep in ((land, True), (~land, False)):
        lab, n = ndimage.label(mask)
        sizes = ndimage.sum(mask, lab, range(1, n + 1))
        small = np.isin(lab, 1 + np.where(sizes < 60)[0])
        land = land & ~small if keep else land | small
    return land


def motif(shape, points):
    """A 16-pixel periodic motif: aligned on the tiles, the open sea's and land's tiles repeat."""
    p = np.zeros((16, 16), bool)
    for x, y in points:
        p[y % 16, x % 16] = True
    yy, xx = np.mgrid[:shape[0], :shape[1]]
    return p[yy % 16, xx % 16]


WAVE = [(0, 0), (1, -1), (2, -1), (3, 0), (4, 0), (5, -1)]     # a small '~'


def greys(land):
    """Light-grey land with a sparse dark stipple, dark-grey sea with light waves, shallows
    checkered light along the coast (3 pixels), a black coast (the user's simplified geography,
    then "un peu de motif")."""
    h, w = land.shape
    pad = np.zeros(((h + 15) // 16 * 16, (w + 15) // 16 * 16), bool)
    pad[:h, :w] = land
    yy, xx = np.mgrid[:pad.shape[0], :pad.shape[1]]
    coast = pad & ~ndimage.binary_erosion(pad, border_value=1)
    near = ndimage.distance_transform_edt(~pad) <= 3
    g = np.where(pad, 1, 2)
    g[~pad & motif(pad.shape, [(2 + x, 3 + y) for x, y in WAVE] + [(10 + x, 11 + y) for x, y in WAVE])] = 1
    g[~pad & near & ((xx + yy) % 2 == 0)] = 1
    g[pad & motif(pad.shape, [(5, 6), (13, 14), (1, 12), (9, 2)])] = 2
    g[coast] = 3
    return g


def tiles_of(g):
    ty, tx = g.shape[0] // 16, g.shape[1] // 16
    cells = [g[y * 16:y * 16 + 16, x * 16:x * 16 + 16] for y in range(ty) for x in range(tx)]
    uniq, index = {}, []
    for c in cells:
        k = c.tobytes()
        if k not in uniq:
            uniq[k] = len(uniq)
        index.append(uniq[k])
    tiles = [np.frombuffer(k, g.dtype).reshape(16, 16) for k in uniq]
    count = np.bincount(index, minlength=len(tiles))
    alias = list(range(len(tiles)))
    live = set(range(len(tiles)))
    while len(live) > TILES_MAX:               # the rarest tile into its nearest: a coast pixel
        a = min(live, key=lambda t: count[t])
        b = min((t for t in live if t != a), key=lambda t: int((tiles[t] != tiles[a]).sum()))
        live.discard(a); count[b] += count[a]
        alias = [b if v == a else v for v in alias]
    order = sorted(live)
    renum = {t: i for i, t in enumerate(order)}
    return [tiles[t] for t in order], bytes(renum[alias[i]] for i in index), tx, ty


def tile_bytes(t):
    out = bytearray()
    for row in t:
        light = sum(1 << (15 - x) for x in range(16) if row[x] & 1)
        dark = sum(1 << (15 - x) for x in range(16) if row[x] & 2)
        out += struct.pack('>HH', dark, light)
    return bytes(out)


def shades(m, contrast='source'):
    lum = m @ np.array([.299, .587, .114])
    levels = {'source': [20,50,80], 'strong': [35,50,65], 'light': [12,38,62]}
    q = np.percentile(lum, levels[contrast])
    g = (3-np.digitize(lum,q)).astype(np.uint8)
    tw,th=(g.shape[1]+15)//16,(g.shape[0]+15)//16
    return np.pad(g,((0,th*16-g.shape[0]),(0,tw*16-g.shape[1])),mode='edge')


def scale_coord(v, n=MAP_SCALE[0], d=MAP_SCALE[1]):
    return (v*n+d//2)//d


def scaled_texture(g, n=MAP_SCALE[0], d=MAP_SCALE[1]):
    if n==d:return g.copy()
    h,w=g.shape
    g=np.asarray(Image.fromarray(g).resize((scale_coord(w,n,d),scale_coord(h,n,d)),Image.Resampling.NEAREST))
    return np.pad(g,((0,(-g.shape[0])%16),(0,(-g.shape[1])%16)),mode='edge')


def scaled_sprite(frame, n=MAP_SCALE[0], d=MAP_SCALE[1]):
    if n==d:return frame.copy()
    # Same approved preview: reduce an existing pose around its feet anchor(8,22),
    # then restore a one-pixel white rim. No scaling or new sprite rip at runtime.
    w,h=scale_coord(16,n,d),scale_coord(26,n,d)
    small=np.asarray(Image.fromarray((frame+1).astype(np.uint8)).resize((w,h),Image.Resampling.NEAREST)).astype(np.int8)-1
    out=np.full((26,16),-1,np.int8)
    x,y=8-scale_coord(8,n,d),22-scale_coord(22,n,d)
    out[y:y+h,x:x+w]=small
    mask=out>=0;p=np.pad(mask,1)
    ring=(p[:-2,1:-1]|p[2:,1:-1]|p[1:-1,:-2]|p[1:-1,2:])&~mask
    out[ring]=0
    assert (out[0]<0).all() and (out[:,0]<0).all() and (out[:,-1]<0).all()
    return out


def texture_block(g):
    # Eight-row independent streams, sixteen cached strips cover any100-row view.
    # A stream contains light rows then dark rows; no whole-map decoded buffer.
    streams=[]
    for y in range(0,g.shape[0],8):
        raw=b''.join(np.packbits((g[y:y+8]&bit)!=0,axis=1).tobytes() for bit in (1,2))
        streams.append(zx0pack.pack(raw))
    body=bytearray(struct.pack('>HH',g.shape[1]//8,len(streams))+bytes(2*len(streams)))
    for i,stream in enumerate(streams):
        struct.pack_into('>H',body,4+2*i,len(body));body+=stream
    return bytes(body)


def main():
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cue');parser.add_argument('out',type=Path)
    parser.add_argument('preview',nargs='?',type=Path)
    parser.add_argument('--contrast',choices=('source','strong','light'),default='source')
    parser.add_argument('--scale',choices=tuple(SCALES),default='67')
    args=parser.parse_args();cue,out=args.cue,args.out
    n,d=SCALES[args.scale]
    disc, files = extract.disc_files(cue)
    core = disc.read(*files['WORLD/WLDCORE.BIN'])
    world = disc.read(*files['WORLD/WORLD.BIN'])
    m = texture(disc, files)
    # Reviewed source texture, including its original relief and lettering.
    # Keep the exact two planes: merging to 255 tiles would damage unique lettering.
    g=scaled_texture(shades(m,args.contrast),n,d);th,tw=g.shape[0]//16,g.shape[1]//16
    packed_texture=texture_block(g)
    shp = units.shp_frames(disc.read(*files['BATTLE/TYPE1.SHP']))
    spr = disc.read(*files['BATTLE/RAMUZA.SPR'])
    animation=b''.join(interface.planes(scaled_sprite(units.sprite(spr,shp[k]),n,d),16) for k in (9,10,11,12,13))
    packed_animation=bytes((2,1,0,1,2,3,4,3))+zx0pack.pack(animation)
    names = management_data.names(world, NAMES, 43)
    assert (names[6], names[24]) == ('Magic City Gariland', 'Mandalia Plains'), names[:8]
    locs = []
    for i in range(43):
        a, y = struct.unpack_from('<II', core, DOTS + 8 * i)
        locs.append((s16(a & 0xFFFF) + ORIGIN[0], s16(y & 0xFFFF) + ORIGIN[1], (a >> 16) & 255))
    routes = []
    for r in range(48):
        p = struct.unpack_from('<I', core, ROUTES + 4 * r)[0] - WLDCORE
        count, s, e, cost = core[p:p + 4]
        pts = []
        for k in range(count):
            v = struct.unpack_from('<I', core, p + 4 + 12 * k)[0]
            pts.append((s16(v & 0xFFFF) + ORIGIN[0], s16(v >> 16) + ORIGIN[1]))
        near = lambda a, b: abs(a[0] - b[0]) <= 3 and abs(a[1] - b[1]) <= 3   # (Bariaus Hill's: 2 off)
        assert near(pts[0], locs[s]) and near(pts[-1], locs[e]), ('route ends', r)
        routes.append((s, e, cost, pts))
    body = bytearray(b'FWM3' + bytes(10) + struct.pack('>BB', tw, th))
    offs = []

    def section():
        if len(body) & 1: body.append(0)
        offs.append(len(body))

    section()
    body += packed_texture
    section(); body += packed_animation
    section(); loc_at = len(body); body += bytes(8 * 43)
    section(); route_at = len(body); body += bytes(6 * 48)
    section(); body += bytes([len(STORY)]) + b''.join(struct.pack('>BBHBx', *e) for e in STORY)
    for r, (s, e, cost, pts) in enumerate(routes):
        if len(body) & 1: body.append(0)
        struct.pack_into('>4BH', body, route_at + 6 * r, s, e, cost, len(pts), len(body))
        for x, y in pts: body += struct.pack('>hh', scale_coord(x,n,d), scale_coord(y,n,d))
    for i, (x, y, kind) in enumerate(locs):
        struct.pack_into('>hhBBH', body, loc_at + 8 * i, scale_coord(x,n,d), scale_coord(y,n,d), kind, 0, len(body))
        body += names[i].encode('ascii') + b'\0'
    struct.pack_into('>5H', body, 4, *offs)
    assert len(body) < 65000
    out.write_bytes(bytes(body))
    if args.preview:
        Image.fromarray(np.array([255,170,85,0],np.uint8)[g]).save(args.preview)
    print('%s: %d bytes (%d lossless texture bytes, %d compressed idle bytes, %s contrast)' %
          (out.name,len(body),len(packed_texture),len(packed_animation),args.contrast))
    print('Offline scale %s; %dx%d texture, all43 locations/48 routes and eight idle steps retained' %
          (args.scale,g.shape[1],g.shape[0]))


if __name__ == '__main__':
    main()
