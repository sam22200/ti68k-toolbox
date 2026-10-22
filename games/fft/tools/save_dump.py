#!/usr/bin/env python3
"""The campaign save (campaign/save.h), decoded.

usage: save_dump.py ffsave.sav|ffsave.89y
A .89y (the TI's variable, ti-cycles --save-dir) holds the same bytes after its header.
"""
from pathlib import Path
import struct
import sys

PARTY_MAX, CAREER_N, FLAG_N = 16, 20, 256
UNIT = 176                                     # sizeof(SaveUnit)
HEAD = 4 + FLAG_N // 8 + 2 + 256


def payload(path):
    """The save's bytes: a PC .sav as is, a TI variable's after its size word."""
    b = Path(path).read_bytes()
    if b[:8] == b'**TI89**':
        size = struct.unpack_from('>H', b, 0x56)[0]
        return b[0x58:0x58 + size - 6]           # 0, "sav", 0, OTH tag after the data
    return b


def decode(b):
    assert len(b) == HEAD + PARTY_MAX * UNIT and b[:4] == b'FSV2', 'not a campaign save'
    flags = [k for k in range(FLAG_N) if b[4 + (k >> 3)] >> (k & 7) & 1]
    n = b[4 + FLAG_N // 8]
    inv = {i: c for i, c in enumerate(b[4 + FLAG_N // 8 + 2:HEAD]) if c}
    units = []
    for k in range(n):
        u = b[HEAD + k * UNIT:HEAD + (k + 1) * UNIT]
        careers = {}
        for c in range(CAREER_N):
            jp, tjp = struct.unpack_from('>H', u, 36 + 2 * c)[0], struct.unpack_from('>H', u, 76 + 2 * c)[0]
            learned = int.from_bytes(u[116 + 3 * c:119 + 3 * c], 'big')
            if jp or tjp or learned:
                careers[c] = dict(jp=jp, tjp=tjp, learned=[i for i in range(24) if learned & 0x800000 >> i])
        units.append(dict(job=u[0], gfx=u[1], type=u[2], name=u[3], level=u[4], exp=u[5], brave=u[6],
                          faith=u[7], zodiac=u[8], secondary=u[9],
                          raw=[int.from_bytes(u[10 + 3 * i:13 + 3 * i], 'big') for i in range(5)],
                          eq=list(u[25:30]), reaction=struct.unpack_from('>H', u, 30)[0],
                          support=struct.unpack_from('>H', u, 32)[0], movement=struct.unpack_from('>H', u, 34)[0],
                          careers=careers))
    return dict(flags=flags, inventory=inv, party=units)


def main():
    s = decode(payload(sys.argv[1]))
    print('story flags', s['flags'])
    print('inventory', ' '.join('%02X:%d' % kv for kv in s['inventory'].items()))
    for u in s['party']:
        print('job %02X type %d gfx %d name %d Lv %d EXP %d Br %d Fa %d zodiac %d eq %s secondary %02X '
              'reaction %03X support %03X movement %03X' % (
                  u['job'], u['type'], u['gfx'], u['name'], u['level'], u['exp'], u['brave'], u['faith'], u['zodiac'],
                  ' '.join('%02X' % e for e in u['eq']), u['secondary'], u['reaction'], u['support'], u['movement']))
        for c, v in u['careers'].items():
            print('   career %d: JP %d / %d, learned %s' % (c, v['jp'], v['tjp'], v['learned']))


if __name__ == '__main__':
    main()
