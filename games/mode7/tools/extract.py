#!/usr/bin/env python3
"""Extracts the data of the Mode 7 demo from the unpacked program (code.bin) into src/data.c, and
PNG previews into preview/ (tiles, sky, ship sprite, track map, collision walls, 3D models).
Offsets are those of mode7.s (code offsets, program base = 0)."""
import struct, os
from PIL import Image, ImageDraw

d = open('code.bin', 'rb').read()
os.makedirs('src', exist_ok=True); os.makedirs('preview', exist_ok=True)
GREY = [255, 170, 85, 0]                       # pixel value 0..3 = white, light, dark, black

def s16(a, n): return list(struct.unpack('>%dh' % n, d[a:a + 2 * n]))
def u8(a, n): return list(d[a:a + n])
def s8(a, n): return [b - 256 if b > 127 else b for b in d[a:a + n]]

def carray(ctype, name, vals, per=16, comment='', fmt=None):
    fmt = fmt or ('0x%02x' if 'char' in ctype and 'signed' not in ctype else '%d')
    out = '/* %s */\n' % comment if comment else ''
    out += '%s %s[%d] = {\n' % (ctype, name, len(vals))
    for i in range(0, len(vals), per):
        out += '    ' + ', '.join(fmt % v for v in vals[i:i + per]) + ',\n'
    return out + '};\n\n'

c = ['/* Mode 7 - Demo 2 (David Coz, 2005): data extracted from the binary by tools/extract.py.\n'
     ' * Do not edit: rerun the tool. Offsets = position in the original program (mode7.s). */\n'
     '#include "mode7.h"\n\n']

# --- trigonometry, 256 steps per turn, amplitude 128 (the Mode 7 asm has its own 32767 ones) ---
c.append(carray('const short', 'sin128', s16(0x4900, 256), 16,
                '0x4900: sin(2*pi*i/256)*128 (the original has 3 identical copies: 0x22b8, 0x4900, 0x4f8a)'))
c.append(carray('const short', 'cos128', s16(0x4b00, 256), 16,
                '0x4b00: cos(2*pi*i/256)*128 (copies at 0x1e02, 0x24b8, 0x4b00)'))

# --- track: 64x64 tile map (one byte per 32x32-unit tile, value = tile number 0..91) ---
MAP = u8(0x596, 64 * 64)
c.append(carray('unsigned char', 'track_map', MAP, 32, '0x0596: 64x64 tile map of the track'))

# --- 92 tiles 16x16 in 4 greys, stored as two 1-bit planes of 16 rows x 2 bytes ---
NT = 92
TILES = u8(0x5ca6, NT * 64)
c.append(carray('const unsigned char', 'tiles_gray', TILES, 16,
                '0x5ca6: 92 tiles 16x16, per tile plane A (32 bytes) then plane B; pixel = 2*A + B'))

def tile_pixels(t):
    b = TILES[t * 64:(t + 1) * 64]; px = []
    for r in range(16):
        for x in range(16):
            byte, bit = r * 2 + (x >> 3), 0x80 >> (x & 7)
            px.append(2 * bool(b[byte] & bit) + bool(b[32 + byte] & bit))
    return px

# --- sky: 128x45, dark plane (rows 0..44) then light plane (rows 45..89), 16 bytes per row ---
c.append(carray('const unsigned char', 'sky_image', u8(0x538a, 90 * 16), 16,
                '0x538a: sky 128x45, dark plane rows then light plane rows'))

# --- ship sprite 32x24: light plane, dark plane, mask (the original arrays are longer than used) ---
c.append(carray('const unsigned char', 'ship_light', u8(0x2a76, 0x80), 16, '0x2a76: ship, light plane (32x24 used)'))
c.append(carray('const unsigned char', 'ship_dark', u8(0x2af6, 0x60), 16, '0x2af6: ship, dark plane'))
c.append(carray('const unsigned char', 'ship_mask', u8(0x1abc, 0x100), 16, '0x1abc: ship mask (0 = opaque), both planes'))

# --- ship physics: acceleration by speed >> 8 ---
c.append(carray('const signed char', 'accel_curve', s8(0x36b4, 12), 12, '0x36b4: throttle gain per speed step (speed >> 8)'))

# --- collision: 75 wall segments (point, outward normal*32767, half length), 8x8 cells of 8 walls ---
PTS = s16(0x4d00, 150); NRM = s16(0x218c, 150)
c.append('/* 0x4d00: wall segments, start point (x, y) in 1/4 world units */\n'
         'const short wall_point[75][2] = {\n' +
         ''.join('    {%d, %d},\n' % (PTS[2 * i], PTS[2 * i + 1]) for i in range(75)) + '};\n\n')
c.append('/* 0x218c: wall normals (nx, ny) * 32767 */\n'
         'const short wall_normal[75][2] = {\n' +
         ''.join('    {%d, %d},\n' % (NRM[2 * i], NRM[2 * i + 1]) for i in range(75)) + '};\n\n')
c.append(carray('const unsigned char', 'wall_length', u8(0x4434, 100), 16, '0x4434: wall half length'))
c.append(carray('const signed char', 'cell_walls_count', s8(0x2f48, 64), 8, '0x2f48: walls per 4096-unit cell (8x8 cells)'))
c.append(carray('const signed char', 'cell_walls', s8(0x518a, 512), 8, '0x518a: up to 8 wall numbers per cell'))

# --- 3D: level table, object instances, 5 models ---
c.append('/* 0x36c8: per level: objects, vertices, faces */\n'
         'const short level_info[1][3] = {{%d, %d, %d}};\n\n' % tuple(s16(0x36c8, 3)))
inst = []
for i in range(100):
    a = 0x159a + 8 * i
    t, = struct.unpack('>b', d[a:a + 1]); x, y = struct.unpack('>hh', d[a + 2:a + 6])
    inst.append((t, d[a + 1], x, y, d[a + 6], d[a + 7]))
c.append('/* 0x159a: object instances (model, x, y, angle), 100 per level */\n'
         'const Instance instances[1][100] = {{\n' +
         ''.join('    {%d, %d, %d, %d, %d, %d},\n' % v for v in inst) + '}};\n\n')
models = []
for m in range(5):
    a = 0x592a + 178 * m
    nv, nf = struct.unpack('>bb', d[a:a + 2])
    v = [struct.unpack('>hhh', d[a + 2 + 6 * k:a + 8 + 6 * k]) for k in range(16)]
    f = [struct.unpack('>hhhh', d[a + 0x62 + 8 * k:a + 0x6a + 8 * k]) for k in range(10)]
    models.append((nv, nf, v, f))
c.append('/* 0x592a: 3D models: vertices (x, y, z), triangles (3 vertices, colour 0..3) */\n'
         'const Model models[5] = {\n' + ''.join(
             '    {%d, %d,\n     {%s},\n     {%s}},\n' % (nv, nf,
                 ', '.join('{%d,%d,%d}' % p for p in v), ', '.join('{{%d,%d,%d},%d}' % (f0[0], f0[1], f0[2], f0[3]) for f0 in f))
             for nv, nf, v, f in models) + '};\n')
open('src/data.c', 'w').write(''.join(c))

# ---------------------------------------------------------------- previews
im = Image.new('L', (16 * 16, 16 * 6), 255)
for t in range(NT):
    p = tile_pixels(t)
    for i, v in enumerate(p): im.putpixel(((t % 16) * 16 + i % 16, (t // 16) * 16 + i // 16), GREY[v])
im.resize((im.width * 3, im.height * 3), Image.NEAREST).save('preview/tiles.png')

sky = u8(0x538a, 1440); im = Image.new('L', (128, 45))
for y in range(45):
    for x in range(128):
        dk = sky[y * 16 + x // 8] >> (7 - x % 8) & 1; lt = sky[(45 + y) * 16 + x // 8] >> (7 - x % 8) & 1
        im.putpixel((x, y), GREY[2 * dk + lt])
im.resize((512, 180), Image.NEAREST).save('preview/sky.png')

L, D, M = u8(0x2a76, 96), u8(0x2af6, 96), u8(0x1abc, 96); im = Image.new('L', (32, 24))
for y in range(24):
    for x in range(32):
        b, bit = y * 4 + x // 8, 0x80 >> (x % 8)
        im.putpixel((x, y), 200 if M[b] & bit else GREY[2 * bool(D[b] & bit) + bool(L[b] & bit)])
im.resize((256, 192), Image.NEAREST).save('preview/ship.png')

# the whole track, 64x64 tiles of 16x16 pixels, walls and objects drawn on top
tp = [tile_pixels(t) for t in range(NT)]
im = Image.new('L', (1024, 1024))
for ty in range(64):
    for tx in range(64):
        t = MAP[ty * 64 + tx]
        if t >= NT: continue
        for i, v in enumerate(tp[t]): im.putpixel((tx * 16 + i % 16, ty * 16 + i // 16), GREY[v])
im = im.convert('RGB'); dr = ImageDraw.Draw(im)
# units: a world unit is 1/4 texel of the near texture (16 texels per tile): preview pixel = 4 units.
# walls: point * 8 units, the along-wall coordinate goes up to 2 * length units
for i in range(75):
    x, y = PTS[2 * i] * 2, PTS[2 * i + 1] * 2
    nx, ny = NRM[2 * i] / 32767, NRM[2 * i + 1] / 32767; ln = d[0x4434 + i] / 2
    dr.line([(x, y), (x + ny * ln, y - nx * ln)], fill=(255, 0, 0), width=2)
for t, _, x, y, a, _ in inst[:19]:
    dr.ellipse([x - 6, y - 6, x + 6, y + 6], outline=(0, 0, 255), width=2)
im.save('preview/track.png')
print('data.c and previews written')

# ---------------------------------------------------------------- trig tables of the Mode 7 asm
def words(name, vals, comment):
    out = '| %s\n\t.globl\t%s\n\t.even\n%s:\n' % (comment, name, name)
    for i in range(0, len(vals), 8):
        out += '\t.word\t' + ', '.join('%d' % v for v in vals[i:i + 8]) + '\n'
    return out
open('src/trig.s', 'w').write(
    '| Mode 7 - Demo 2: sine and cosine * 32767, 256 steps per turn, used by Mode7Far/Near (render.s).\n'
    '| Extracted by tools/extract.py (original offsets 0x0394 and 0x18bc).\n\t.text\n' +
    words('sin32k', s16(0x394, 256), 'sin(2*pi*i/256) * 32767') +
    words('cos32k', s16(0x18bc, 256), 'cos(2*pi*i/256) * 32767'))
print('trig.s written')
