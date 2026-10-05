"""The GPU commands a frame really executed (pcsx_rearmed patched by scripts/pcsx_vram.patch:
every GP0 word logged in order with the RAM address of its DMA chain packet, and the empty
ordering-table entries), parsed into primitives and drawn again: the game's draw order and,
per pixel, the ordering-table entry (its depth slot) of what is on top.

    psx.lib.retro_tiport_gplog_reset(); psx.run()
    prims = parse(*frame_log(psx))
    col, own = replay(prims, vram, area(prims))
"""
import ctypes as C
import numpy as np


def frame_log(psx):
    """the GP0 words logged since the last reset, and each one's packet address (bit 31: an
    empty ordering-table entry, word 0)"""
    L = psx.lib
    for f in ('words', 'addrs'):
        getattr(L, 'retro_tiport_gplog_' + f).restype = C.POINTER(C.c_uint32)
    n = L.retro_tiport_gplog_count()
    if not n:
        return np.zeros(0, np.uint32), np.zeros(0, np.uint32)
    return (np.ctypeslib.as_array(L.retro_tiport_gplog_words(), (n,)).copy(),
            np.ctypeslib.as_array(L.retro_tiport_gplog_addrs(), (n,)).copy())


def _xy(v):
    x, y = v & 0x7ff, (v >> 16) & 0x7ff
    return (x - 2048 if x >= 1024 else x, y - 2048 if y >= 1024 else y)


def parse(w, a):
    """primitives in draw order, dicts: kind 'ot' (addr), 'rect' (xy, wh, uv, clut, tpage),
    'poly' (verts, uv, clut, tpage), 'area' (cmd 0xe3/0xe4, v), 'copy', 'load', 'fill';
    cmd, semi (semi-transparent), off (the drawing offset) on rects and polys"""
    out, i, n, tpage, off = [], 0, len(w), 0, (0, 0)
    while i < n:
        wi, c = int(w[i]), int(w[i]) >> 24
        if int(a[i]) & 0x80000000 and not wi:
            out.append(dict(kind='ot', addr=int(a[i]) & 0xffffff))
            i += 1
        elif 0x20 <= c < 0x40:                 # polygons
            nv, tex, gour = 4 if c & 8 else 3, bool(c & 4), bool(c & 0x10)
            j, vs, uvs, clut, tp = i + 1, [], [], None, tpage
            for k in range(nv):
                if gour and k:
                    j += 1
                vs.append(_xy(int(w[j])))
                j += 1
                if tex:
                    uvs.append((int(w[j]) & 0xff, (int(w[j]) >> 8) & 0xff))
                    if k == 0:
                        clut = int(w[j]) >> 16
                    if k == 1:
                        tp = int(w[j]) >> 16
                    j += 1
            out.append(dict(kind='poly', cmd=c, semi=bool(c & 2), off=off, verts=vs, uv=uvs,
                            clut=clut, tpage=tp))
            i = j
        elif 0x40 <= c < 0x60:                 # lines
            if c & 8:
                j = i + 1
                while j < n and (int(w[j]) & 0xf000f000) != 0x50005000:
                    j += 1
                i = j + 1
            else:
                i += 3 + (1 if c & 0x10 else 0)
        elif 0x60 <= c < 0x80:                 # rectangles (sprites)
            sz, tex, j = (c >> 3) & 3, bool(c & 4), i + 1
            xy = _xy(int(w[j]))
            j += 1
            uv = clut = None
            if tex:
                uv, clut = (int(w[j]) & 0xff, (int(w[j]) >> 8) & 0xff), int(w[j]) >> 16
                j += 1
            if sz == 0:
                wh = (int(w[j]) & 0xffff, int(w[j]) >> 16)
                j += 1
            else:
                wh = {1: (1, 1), 2: (8, 8), 3: (16, 16)}[sz]
            out.append(dict(kind='rect', cmd=c, semi=bool(c & 2), off=off, xy=xy, wh=wh, uv=uv,
                            clut=clut, tpage=tpage))
            i = j
        elif 0x80 <= c < 0xa0:
            out.append(dict(kind='copy', src=_xy(int(w[i + 1])), dst=_xy(int(w[i + 2]))))
            i += 4
        elif 0xa0 <= c < 0xc0:                 # a load into VRAM: its data words follow
            wh = (int(w[i + 2]) & 0xffff, int(w[i + 2]) >> 16)
            out.append(dict(kind='load', dst=_xy(int(w[i + 1])), wh=wh))
            i += 3 + (wh[0] * wh[1] + 1) // 2
        elif 0xc0 <= c < 0xe0:
            i += 3
        elif c == 0x02:
            out.append(dict(kind='fill', xy=_xy(int(w[i + 1]))))
            i += 3
        elif c == 0xe1:
            tpage = wi & 0xffff
            i += 1
        elif c == 0xe5:
            x, y = wi & 0x7ff, (wi >> 11) & 0x7ff
            off = (x - 2048 if x >= 1024 else x, y - 2048 if y >= 1024 else y)
            i += 1
        elif c in (0xe3, 0xe4):
            out.append(dict(kind='area', cmd=c, v=wi & 0xfffff))
            i += 1
        else:
            i += 1
    return out


def area(prims):
    """the draw area's top-left in VRAM (the last 0xe3)"""
    v = [p['v'] for p in prims if p['kind'] == 'area' and p['cmd'] == 0xe3]
    return (v[-1] & 0x3ff, v[-1] >> 10) if v else (0, 0)


def texel(vram, tpage, clut, u, v):
    """15-bit colours of the texels (arrays u, v) of a texture page and CLUT; 0 = transparent"""
    tx, ty, mode = (tpage & 15) * 64, ((tpage >> 4) & 1) * 256, (tpage >> 7) & 3
    cx, cy = (clut & 63) * 16, clut >> 6
    if mode == 0:
        return vram[cy, cx + ((vram[ty + v, tx + u // 4] >> ((u & 3) * 4)) & 15)]
    if mode == 1:
        return vram[cy, cx + ((vram[ty + v, tx + u // 2] >> ((u & 1) * 8)) & 255)]
    return vram[ty + v, tx + u]


def replay(prims, vram, org, skip=lambda p: False, size=(240, 320)):
    """textured sprites and axis-aligned textured quads drawn again in order (org = the draw
    area's top-left in VRAM; skip(p) leaves a primitive out): the colours (15-bit) and the
    ordering-table entry address of the last opaque texel per pixel (-1 where nothing was)"""
    H, W = size
    col, own = np.zeros(size, np.uint32), np.full(size, -1, np.int64)
    slot = -1
    for p in prims:
        if p['kind'] == 'ot':
            slot = p['addr']
            continue
        if p['kind'] not in ('rect', 'poly') or skip(p):
            continue
        if p['kind'] == 'rect':
            if p['uv'] is None:
                continue
            (x, y), (w, h), (u0, v0) = p['xy'], p['wh'], p['uv']
            yy, xx = np.mgrid[0:h, 0:w]
            c = texel(vram, p['tpage'], p['clut'], (u0 + xx) & 255, (v0 + yy) & 255)
        else:
            vs, uv = p['verts'], p['uv']
            if len(vs) != 4 or len(uv) != 4:
                continue
            (x0, y0), (x1, _), (_, y1) = vs[0], vs[1], vs[2]
            if vs[3] != (x1, y1) or x0 == x1 or y1 <= y0:
                continue
            (u0, v0), (u1, _), (_, v1) = uv[0], uv[1], uv[2]
            w, h = abs(x1 - x0), y1 - y0
            yy, xx = np.mgrid[0:h, 0:w]
            fx = xx if x1 > x0 else w - 1 - xx           # mirrored when x runs backwards
            c = texel(vram, p['tpage'], p['clut'], (u0 + fx * (u1 - u0) // w) & 255,
                      (v0 + yy * (v1 - v0) // h) & 255)
            x, y = min(x0, x1), y0
        x += p['off'][0] - org[0]
        y += p['off'][1] - org[1]
        sy0, sx0 = max(0, -y), max(0, -x)
        sy1, sx1 = min(c.shape[0], H - y), min(c.shape[1], W - x)
        if sy1 <= sy0 or sx1 <= sx0:
            continue
        cc = c[sy0:sy1, sx0:sx1]
        m = cc != 0
        dst = (slice(y + sy0, y + sy1), slice(x + sx0, x + sx1))
        col[dst][m] = cc[m]
        own[dst][m] = slot
    return col, own
