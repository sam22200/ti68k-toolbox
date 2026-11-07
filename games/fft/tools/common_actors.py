#!/usr/bin/env python3
"""Common 2D actor and blade banks of the chapter packages (run by both chapter generators).

ffca, ffcb: every distinct 208-byte actor frame of every chapter, stored once as its own
ZX0 stream; a frame that is another one mirrored is not stored (bit 15 of a mapping entry:
fft.c actor_frame() flips it after decoding). Frames are numbered across the two
variables (one variable holds at most 65,512 bytes). Layout of each: u16 count, u16
stream offsets (from the end of that table), the streams.
Per chapter, only the logical frame table (u16 per logical frame) remains:
- Orbonne, opening/data/ffoa.bin (its records move out; ACTOR_DIR/ACTOR_DATA dropped);
- Gariland, campaign/data/ffga.bin + actors.h: root fftu's 6 x 44 frames, its objects at
  ACTOR_OBJECTS, then fftw's 96 armed bodies at ACTOR_ARMED.
ffcw: the blades alone (root fftw after its bodies) for both chapters, with their
weapons.h offsets shifted (opening/data/weapons.h, campaign/data/weapons.h).
Converted Orbonne frames are recovered from ffca, so either chapter may be regenerated
alone. Writes identical ffca/ffcw into both data folders.
"""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import extract  # noqa: E402

FB = 208
BANKS = ('ffca', 'ffcb')
BODY = 19968                     # fftw: 6 actors x 16 armed bodies, then the blades
MARK = '// Actor frames live in ffca (tools/common_actors.py).'
REV = bytes(int('{:08b}'.format(i)[::-1], 2) for i in range(256))


def mirror(r):
    return b''.join(bytes((REV[r[i + 1]], REV[r[i]])) for i in range(0, len(r), 2))


def unzx0(stream):
    with tempfile.TemporaryDirectory() as d:
        src = Path(d) / 'i'; dst = Path(d) / 'o'; src.write_bytes(stream)
        subprocess.run([HERE.parents[1] / 'tools/bin/dzx0', '-f', src, dst], check=True, capture_output=True)
        return dst.read_bytes()


def defines(text):
    return {k: int(v) for k, v in re.findall(r'#define (ACTOR_\w+) (\d+)', text)}


def opening_frames():
    """Orbonne's logical frames, raw, from its original bank or the current ffca."""
    bank = (HERE / 'opening/data/ffoa.bin').read_bytes(); header = (HERE / 'opening/data/units.h').read_text()
    if MARK in header:
        mapping = struct.unpack('>%dH' % (len(bank) // 2), bank)
        records = read_common(HERE / 'opening/data')
    else:
        d = defines(header); count = (d['ACTOR_DATA'] - d['ACTOR_DIR']) // 2
        mapping = struct.unpack_from('>%dH' % (d['ACTOR_DIR'] // 2), bank)
        offsets = struct.unpack_from('>%dH' % count, bank, d['ACTOR_DIR'])
        ends = list(offsets[1:]) + [len(bank) - d['ACTOR_DATA']]
        records = [unzx0(bank[d['ACTOR_DATA'] + a:d['ACTOR_DATA'] + b])[:FB] for a, b in zip(offsets, ends)]
    return [mirror(records[k & 0x7FFF]) if k & 0x8000 else records[k] for k in mapping]


def read_common(folder):
    out = []
    for name in BANKS:
        data = (folder / (name + '.bin')).read_bytes(); count = struct.unpack_from('>H', data)[0]
        offsets = struct.unpack_from('>%dH' % count, data, 2); base = 2 + 2 * count
        ends = list(offsets[1:]) + [len(data) - base]
        out += [unzx0(data[base + a:base + b])[:FB] for a, b in zip(offsets, ends)]
    return out


def split(streams):
    """Consecutive frames into BANKS variables, each within one archive variable."""
    banks = [[]]
    for s in streams:
        size = 2 + sum(2 + len(x) for x in banks[-1]) + 2 + len(s)
        if size > 65000:
            banks.append([])
        banks[-1].append(s)
    assert len(banks) <= len(BANKS), ('more actor variables needed', len(banks))
    banks += [[] for _ in range(len(BANKS) - len(banks))]
    out = []
    for frames in banks:
        table = bytearray(struct.pack('>H', len(frames))); data = bytearray()
        for s in frames:
            table += struct.pack('>H', len(data)); data += s
        out.append(bytes(table + data))
    return out


def gariland_frames():
    units = (HERE / 'fftu.bin').read_bytes(); weapons = (HERE / 'fftw.bin').read_bytes()
    frames = [units[i:i + FB] for i in range(0, len(units), FB)]
    bodies = [weapons[i:i + FB] for i in range(0, BODY, FB)]
    return frames + bodies, len(frames)   # objects end fftu; bodies follow


def main():
    orbonne = opening_frames(); gariland, armed = gariland_frames()
    records = []; canonical = {}
    def store(raw):
        if raw in canonical: return canonical[raw]
        flipped = mirror(raw)
        if flipped in canonical: return canonical[flipped] | 0x8000
        canonical[raw] = len(records); records.append(raw); return canonical[raw]
    omap = [store(f) for f in orbonne]; gmap = [store(f) for f in gariland]
    assert len(records) < 0x8000
    streams = [extract.zx0pack.pack(r) for r in records]
    common = split(streams)
    blades = (HERE / 'fftw.bin').read_bytes()[BODY:]
    wh = (HERE / 'weapons.h').read_text()
    for name in ('WEAPON_INDEX', 'WEAPON_ZOFF', 'WEAPON_OFF'):
        wh = re.sub(r'(#define ' + name + r' )(\d+)UL', lambda m: m[1] + str(int(m[2]) - BODY) + 'UL', wh)
    for chapter in ('opening', 'campaign'):
        folder = HERE / chapter / 'data'
        for name, data in zip(BANKS, common): (folder / (name + '.bin')).write_bytes(data)
        (folder / 'ffcw.bin').write_bytes(blades)
        (folder / 'weapons.h').write_text(wh)
    (HERE / 'opening/data/ffoa.bin').write_bytes(struct.pack('>%dH' % len(omap), *omap))
    header = (HERE / 'opening/data/units.h').read_text()
    header = re.sub(r'#define ACTOR_(DIR|DATA) \d+\n', '', header)
    if MARK not in header: header = header.rstrip('\n') + '\n' + MARK + '\n'
    (HERE / 'opening/data/units.h').write_text(header)
    (HERE / 'campaign/data/ffga.bin').write_bytes(struct.pack('>%dH' % len(gmap), *gmap))
    (HERE / 'campaign/data/actors.h').write_text(
        '// Generated locally by tools/common_actors.py; never committed.\n'
        '#define ACTOR_OBJECTS %d\n#define ACTOR_ARMED %d\n%s\n' % (6 * 44, armed, MARK))
    shared = len({k & 0x7FFF for k in omap} & {k & 0x7FFF for k in gmap})
    print('ffca+ffcb: %d frames (%d used by both chapters), %s bytes; Orbonne %d / Gariland %d logical'
          % (len(records), shared, '+'.join(str(len(c)) for c in common), len(omap), len(gmap)))


if __name__ == '__main__':
    main()
