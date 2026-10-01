#!/usr/bin/env python3
"""Extract a PICO-8 cart into files a TI-89 port can use and read.

usage: p8extract.py CART OUTDIR

CART: a .p8 text cart, or anything shrinko8 reads (.p8.png, .lua, .rom, html .js export,
.pod, a BBS id with --bbs is done by hand: shrinko8 --bbs ID out.p8). Non-.p8 inputs are
converted with tools/shrinko8 first (unminified with -U when the code looks minified).

OUTDIR gets:
  code.lua         the Lua code (PICO-8 dialect), one tab per "-->8" marker kept inline
  sheet.png        sprite sheet 128x128 in the PICO-8 palette, x4, grid and sprite numbers
  sheet_grey.png   the same quantised to 4 greys by luminance (a first look, not final art)
  map.png          the whole map 128x64 cells (rooms of 16x16 outlined), sprites drawn
  sprites.bin      128*128 bytes, one colour index (0..15) per pixel, row-major
  map.bin          128*64 bytes, one sprite index per cell (rows 32..63 = shared gfx memory)
  flags.bin        256 bytes, sprite flags (__gff__)
  mem.bin          PICO-8 memory 0x0000..0x30ff as the cart loads it (read by p8trace.py)
  info.json        sizes, callbacks, frame rate, API usage census (what the port must cover)
"""
import json, os, re, subprocess, sys
from collections import Counter

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../..'))
SHRINKO8 = os.path.join(ROOT, 'tools/shrinko8/shrinko8.py')

PALETTE = [  # PICO-8 default palette 0..15
    0x000000, 0x1d2b53, 0x7e2553, 0x008751, 0xab5236, 0x5f574f, 0xc2c3c7, 0xfff1e8,
    0xff004d, 0xffa300, 0xffec27, 0x00e436, 0x29adff, 0x83769c, 0xff77a8, 0xffccaa]

API = ['_init', '_update', '_update60', '_draw', 'spr', 'sspr', 'map', 'mapdraw', 'mget',
       'mset', 'fget', 'fset', 'sget', 'sset', 'pget', 'pset', 'pal', 'palt', 'fillp',
       'camera', 'clip', 'cls', 'rect', 'rectfill', 'circ', 'circfill', 'oval', 'ovalfill',
       'line', 'print', 'cursor', 'color', 'tline', 'flip', 'btn', 'btnp', 'rnd', 'srand',
       'sin', 'cos', 'atan2', 'sqrt', 'flr', 'ceil', 'abs', 'sgn', 'mid', 'min', 'max',
       'shl', 'shr', 'lshr', 'rotl', 'rotr', 'band', 'bor', 'bxor', 'bnot', 'peek', 'poke',
       'peek2', 'poke2', 'peek4', 'poke4', 'memcpy', 'memset', 'reload', 'cstore',
       'cartdata', 'dget', 'dset', 'sfx', 'music', 'stat', 'time', 't', 'menuitem',
       'cocreate', 'coresume', 'costatus', 'yield', 'add', 'del', 'deli', 'all', 'foreach',
       'pairs', 'count', 'sub', 'split', 'ord', 'chr', 'tostr', 'tonum', 'setmetatable',
       'load', 'run', 'extcmd', 'printh']


def to_p8(cart, outdir):
    if cart.endswith('.p8'):
        return cart
    out = os.path.join(outdir, 'cart.p8')
    subprocess.run([sys.executable, SHRINKO8, cart, out], check=True)
    code = open(out, encoding='utf-8', errors='replace').read()
    lines = code.split('__lua__', 1)[-1].split('\n__', 1)[0].split('\n')
    if lines and max(len(l) for l in lines) > 300:          # minified: one huge line
        subprocess.run([sys.executable, SHRINKO8, cart, out, '-U'], check=True)
    return out


def sections(text):
    secs, name = {}, None
    for line in text.split('\n'):
        m = re.match(r'^__(\w+)__\s*$', line)
        if m:
            name = m.group(1)
            secs[name] = []
        elif name:
            secs[name].append(line)
    return secs


def hexrows(lines, width):
    rows = [l.strip() for l in lines if l.strip()]
    return [r.ljust(width, '0')[:width] for r in rows]


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    cart, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    p8 = to_p8(cart, outdir)
    secs = sections(open(p8, encoding='utf-8', errors='replace').read())
    code = '\n'.join(secs.get('lua', []))
    open(os.path.join(outdir, 'code.lua'), 'w').write(code)

    # sprite sheet: 128 rows of 128 hex digits, one digit per pixel, left to right
    gfx = hexrows(secs.get('gfx', []), 128) + ['0' * 128] * 128
    pix = [[int(c, 16) for c in row] for row in gfx[:128]]
    # map: 32 rows of 128 bytes; rows 32..63 live in gfx memory 0x1000.. (sheet rows 64..127),
    # a byte there = pixel 2i in the low nibble, pixel 2i+1 in the high nibble
    mrows = hexrows(secs.get('map', []), 256) + ['0' * 256] * 32
    cells = [[int(r[2 * i:2 * i + 2], 16) for i in range(128)] for r in mrows[:32]]
    for y in range(32):
        row = []
        for k in range(128):
            off = y * 128 + k                       # byte offset in 0x1000..0x1fff
            sy, sx = 64 + off // 64, (off % 64) * 2
            row.append(pix[sy][sx] | (pix[sy][sx + 1] << 4))
        cells.append(row)
    gff = ''.join(l.strip() for l in secs.get('gff', [])).ljust(512, '0')
    flags = [int(gff[2 * i:2 * i + 2], 16) for i in range(256)]

    open(os.path.join(outdir, 'sprites.bin'), 'wb').write(bytes(v for r in pix for v in r))
    open(os.path.join(outdir, 'map.bin'), 'wb').write(bytes(v for r in cells for v in r))
    open(os.path.join(outdir, 'flags.bin'), 'wb').write(bytes(flags))
    # the cart ROM as PICO-8 memory 0x0000..0x30ff (sheet, shared, map, flags), for p8trace.py
    mem = bytearray(0x3100)
    for y in range(128):
        for x in range(0, 128, 2):
            mem[y * 64 + x // 2] = pix[y][x] | (pix[y][x + 1] << 4)
    for y in range(32):
        mem[0x2000 + y * 128:0x2000 + y * 128 + 128] = bytes(cells[y])
    mem[0x3000:0x3100] = bytes(flags)
    open(os.path.join(outdir, 'mem.bin'), 'wb').write(bytes(mem))

    # census of the code: what the port has to provide
    body = re.sub(r'--\[\[.*?\]\]|--[^\n]*', '', code, flags=re.S)
    body = re.sub(r'"(\\.|[^"\\\n])*"|\'(\\.|[^\'\\\n])*\'', '""', body)
    calls = Counter(m.group(1) for m in re.finditer(r'\b([a-z_][a-z0-9_]*)\s*\(', body))
    used = {k: calls[k] for k in API if calls[k]}
    used_cells = sorted({v for r in cells for v in r if v})
    info = {
        'cart': os.path.abspath(cart),
        'lua_lines': code.count('\n') + 1,
        'frame_rate': 60 if '_update60' in calls else 30,
        'callbacks': [c for c in ('_init', '_update', '_update60', '_draw') if
                      re.search(r'function\s+' + c + r'\s*\(', body)],
        'api_calls': used,
        'functions_defined': len(re.findall(r'\bfunction\b', body)),
        'uses': {
            'map_layers (map/mapdraw with a flag mask)': bool(re.search(r'\bmap\s*\([^)]*,[^)]*,[^)]*,[^)]*,[^)]*,[^)]*,', body)),
            'palette_swaps (pal/palt)': 'pal' in used or 'palt' in used,
            'fill_patterns (fillp)': 'fillp' in used,
            'memory (peek/poke/memcpy/memset/reload)': any(k in used for k in ('peek', 'poke', 'peek2', 'poke2', 'peek4', 'poke4', 'memcpy', 'memset', 'reload')),
            'coroutines': 'cocreate' in used,
            'save (cartdata/dget/dset)': 'cartdata' in used,
            'map_writes (mset)': 'mset' in used,
            'screen_reads (pget)': 'pget' in used,
            'scaled_sprites (sspr)': 'sspr' in used,
            'textured_lines (tline)': 'tline' in used,
            'sound (sfx/music)': 'sfx' in used or 'music' in used,
            'metatables (setmetatable)': 'setmetatable' in used,
        },
        'sprites_used_on_map': len(used_cells),
        'flags_nonzero': {i: f for i, f in enumerate(flags) if f},
    }
    json.dump(info, open(os.path.join(outdir, 'info.json'), 'w'), indent=1)

    try:
        from PIL import Image, ImageDraw
    except ImportError:
        print('PIL missing (tools/pyenv/bin/python): PNGs skipped')
        return
    rgb = lambda c: ((PALETTE[c] >> 16) & 255, (PALETTE[c] >> 8) & 255, PALETTE[c] & 255)
    lum = [0.299 * rgb(c)[0] + 0.587 * rgb(c)[1] + 0.114 * rgb(c)[2] for c in range(16)]
    grey = [255, 170, 85, 0]
    order = sorted(range(16), key=lambda c: -lum[c])     # 4 bins of 4 colours by luminance
    tog = {c: grey[order.index(c) // 4] for c in range(16)}
    for name, colour in (('sheet.png', rgb), ('sheet_grey.png', lambda c: (tog[c],) * 3)):
        im = Image.new('RGB', (128, 128))
        im.putdata([colour(v) for r in pix for v in r])
        im = im.resize((512, 512), Image.NEAREST)
        d = ImageDraw.Draw(im)
        for i in range(17):
            d.line([(i * 32, 0), (i * 32, 512)], fill=(60, 60, 60))
            d.line([(0, i * 32), (512, i * 32)], fill=(60, 60, 60))
        for n in range(256):
            d.text((n % 16 * 32 + 1, n // 16 * 32), str(n), fill=(255, 0, 255))
        im.save(os.path.join(outdir, name))
    sheet = Image.new('RGB', (128, 128))
    sheet.putdata([rgb(v) for r in pix for v in r])
    tiles = [sheet.crop((n % 16 * 8, n // 16 * 8, n % 16 * 8 + 8, n // 16 * 8 + 8)) for n in range(256)]
    im = Image.new('RGB', (1024, 512))
    for y in range(64):
        for x in range(128):
            if cells[y][x]:
                im.paste(tiles[cells[y][x]], (x * 8, y * 8))
    d = ImageDraw.Draw(im)
    for ry in range(4):
        for rx in range(8):
            d.rectangle([rx * 128, ry * 128, rx * 128 + 127, ry * 128 + 127], outline=(255, 0, 255))
            d.text((rx * 128 + 2, ry * 128 + 2), f'{rx},{ry}', fill=(255, 0, 255))
    im.save(os.path.join(outdir, 'map.png'))
    print(f"{outdir}: {info['lua_lines']} lines of Lua, {info['frame_rate']} fps, "
          f"{len(used)} API functions, {len(used_cells)} sprites on the map")


if __name__ == '__main__':
    main()
