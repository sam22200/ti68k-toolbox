#!/usr/bin/env python3
"""The campaign save (campaign/save.h), decoded.

usage: save_dump.py ffsave.sav|ffsave.89y
A .89y (the TI's variable, ti-cycles --save-dir) holds the same bytes after its header.
"""
from pathlib import Path
import struct
import sys
import chapter_packages as packages

PARTY_MAX, CAREER_N, FLAG_N = 16, 20, 256
UNIT = 176                                     # sizeof(SaveUnit)
HEAD = 4 + FLAG_N // 8 + 2 + 256


def payload(path):
    """Raw runtime save bytes (including ffslot), with a checked TI envelope."""
    raw = Path(path).read_bytes()
    if raw[:8] != b'**TI89**':
        return raw
    variables = packages.read_group(path)
    if len(variables) != 1:
        raise ValueError('Expected one runtime save variable')
    var = next(iter(variables.values()))
    if var.kind != 0x1c or not var.payload.endswith(b'\0sav\0\xf8'):
        raise ValueError('Expected an OTH runtime save with the sav trailer')
    return var.payload[:-6]


def unit_decode(u):
    careers = {}
    for c in range(CAREER_N):
        jp, tjp = struct.unpack_from('>H', u, 36 + 2 * c)[0], struct.unpack_from('>H', u, 76 + 2 * c)[0]
        learned = int.from_bytes(u[116 + 3 * c:119 + 3 * c], 'big')
        if jp or tjp or learned:
            careers[c] = dict(jp=jp, tjp=tjp, learned=[i for i in range(24) if learned & 0x800000 >> i])
    return dict(job=u[0], gfx=u[1], type=u[2], name=u[3], level=u[4], exp=u[5], brave=u[6],
                      faith=u[7], zodiac=u[8], secondary=u[9],
                      raw=[int.from_bytes(u[10 + 3 * i:13 + 3 * i], 'big') for i in range(5)],
                      eq=list(u[25:30]), reaction=struct.unpack_from('>H', u, 30)[0],
                      support=struct.unpack_from('>H', u, 32)[0], movement=struct.unpack_from('>H', u, 34)[0],
                      careers=careers)


def decode(b):
    base=HEAD+PARTY_MAX*UNIT
    assert (len(b)==base and b[:4]==b'FSV2') or (len(b)==base+28 and b[:4]==b'FSV3') or (len(b)==base+92 and b[:4]==b'FSV4') or (len(b)==base+94 and b[:4]==b'FSV5') or (len(b)==3918 and b[:4]==b'FSV6') or (len(b)==3992 and b[:4]==b'FSV7') or (len(b)==4024 and b[:4]==b'FSV8') or (len(b)==4684 and b[:4]==b'FSV9'), 'not a campaign save'
    flags = [k for k in range(FLAG_N) if b[4 + (k >> 3)] >> (k & 7) & 1]
    n, loc = b[4 + FLAG_N // 8], b[5 + FLAG_N // 8]
    inv = {i: c for i, c in enumerate(b[4 + FLAG_N // 8 + 2:HEAD]) if c}
    units = []
    for k in range(n):
        u = b[HEAD + k * UNIT:HEAD + (k + 1) * UNIT]
        units.append(unit_decode(u))
    result=dict(flags=flags, location=loc - 1 if loc else None, inventory=inv, party=units)
    if b[:4]in(b'FSV3',b'FSV4',b'FSV5',b'FSV6',b'FSV7',b'FSV8',b'FSV9'):
        t=b[base:];result.update(money=int.from_bytes(t[:4],'big'),flow=t[4],
            deployment_battle=int.from_bytes(t[6:8],'big'),
            deployment=[dict(slot=t[8+4*i],x=t[9+4*i],z=t[10+4*i],face=t[11+4*i]) for i in range(t[5])])
    if b[:4]in(b'FSV4',b'FSV5',b'FSV6',b'FSV7',b'FSV8',b'FSV9'):result['discovered']=list(b[base+28:base+92])
    if b[:4]in(b'FSV5',b'FSV6',b'FSV7',b'FSV8',b'FSV9'):
        for i,u in enumerate(units):
            if b[base+92+(i>>3)]&(1<<(i&7)):u['raw_current']=True
    if b[:4]in(b'FSV6',b'FSV7',b'FSV8',b'FSV9'):
        result['guests']=[]
        for i in range(4):
            identity=int.from_bytes(b[3204+2*i:3206+2*i], 'big')
            if not identity:continue
            u=unit_decode(b[3214+UNIT*i:3214+UNIT*(i+1)])
            u.update(id=identity, raw_current=bool(b[3212]&(1<<i)))
            result['guests'].append(u)
    if b[:4]in(b'FSV7',b'FSV8',b'FSV9'):
        result['calendar']=dict(month=b[3918],day=b[3919],era=b[3920])
        result['world_rng']=int.from_bytes(b[3987:3991],'big')
        for i,u in enumerate(units):
            egg=b[3922+i]
            u.update(egg=bool(egg),hatch_remaining=egg&15,hatch_total=egg>>4,
                     birthday=int.from_bytes(b[3942+2*i:3944+2*i],'big'),egg_color=(b[3982+(i>>2)]>>((i&3)*2))&3)
    if b[:4]in(b'FSV8',b'FSV9'):
        result['profiles']={i:(b[3992+(i>>1)]>>((i&1)*4))&15 for i in range(64) if (b[3992+(i>>1)]>>((i&1)*4))&15}
    if b[:4]==b'FSV9':
        at=4024
        result['missions']=[dict(flags=b[at+1+9*i],id=b[at+2+9*i],elapsed=b[at+3+9*i],
            days=b[at+4+9*i],location=b[at+5+9*i],units=list(b[at+7+9*i:at+7+9*i+b[at+6+9*i]])) for i in range(b[at])]
        result['pending_report']=bool(b[at+378])
        result['poach_stock']={i:n for i,n in enumerate(b[4426:4682]) if n}
        result['generic_identity']=[i for i in range(16) if b[4682+(i>>3)]&(1<<(i&7))]
    return result


def main():
    s = decode(payload(sys.argv[1]))
    print('story flags', s['flags'], 'location', s['location'])
    print('inventory', ' '.join('%02X:%d' % kv for kv in s['inventory'].items()))
    for u in s['party']+s.get('guests',[]):
        if 'id' in u: print('guest identity', u['id'])
        print('job %02X type %d gfx %d name %d Lv %d EXP %d Br %d Fa %d zodiac %d eq %s secondary %02X '
              'reaction %03X support %03X movement %03X' % (
                  u['job'], u['type'], u['gfx'], u['name'], u['level'], u['exp'], u['brave'], u['faith'], u['zodiac'],
                  ' '.join('%02X' % e for e in u['eq']), u['secondary'], u['reaction'], u['support'], u['movement']))
        for c, v in u['careers'].items():
            print('   career %d: JP %d / %d, learned %s' % (c, v['jp'], v['tjp'], v['learned']))


if __name__ == '__main__':
    main()
