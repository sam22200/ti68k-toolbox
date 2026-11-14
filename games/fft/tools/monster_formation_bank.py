#!/usr/bin/env python3
"""Losslessly serialize the approved FMR1 pictures as shared FMA1 formation art.

One32x26 body and32x29 portrait per selected family. All48 source jobs keep
their identities and alias these16 records. Names use the existing source
monster-name list; pool equal strings and borrow all bytes from the archive.
"""
import argparse
from pathlib import Path
import struct

HEADER, ALIASES, NAMES, FRAMES, RECORD = 16, 48, 255, 16, 660
NAME_DIR = HEADER+ALIASES
FRAME_BASE = NAME_DIR+NAMES*2
POOL_BASE = FRAME_BASE+FRAMES*RECORD


def pack(records, names):
    if len(records)!=FRAMES or any(len(r)!=RECORD for r in records) or len(names)!=NAMES:
        raise ValueError('FMA1 requires16 pictures and255 names')
    pool, offsets, found = bytearray(), [], {}
    for name in names:
        name=bytes(name)
        if not 1<=len(name)<=15 or any(c<32 or c>126 for c in name):
            raise ValueError('Invalid FMA1 name')
        if name not in found:
            found[name]=POOL_BASE+len(pool);pool+=name+b'\0'
        offsets.append(found[name])
    raw=b'FMA1'+bytes((16,32,26,0))+struct.pack('>4H',NAMES,FRAME_BASE,NAME_DIR,POOL_BASE)
    raw+=bytes(i//3 for i in range(ALIASES))+struct.pack('>255H',*offsets)+b''.join(records)+pool
    if len(raw)&1:raw+=b'\0'
    validate(raw)
    return raw


def from_review(raw):
    if len(raw)!=14696 or raw[:8]!=b'FMR1\0\x10\0\xff' or raw[8:56]!=bytes(i//3 for i in range(48)):
        raise ValueError('Expected approved FMR1 review bank')
    names=[]
    for i in range(255):
        record=raw[56+16*i:72+16*i]
        end=record.find(b'\0')
        if end<0 or any(record[end:]):raise ValueError('Invalid FMR1 name')
        names.append(record[:end])
    return pack([raw[4136+660*i:4136+660*(i+1)]for i in range(16)],names)


def validate(raw):
    def reject():raise ValueError('Invalid FMA1 formation bank')
    if len(raw)<POOL_BASE+2 or len(raw)>65512 or len(raw)&1 or raw[:8]!=b'FMA1\x10\x20\x1a\0':reject()
    if struct.unpack_from('>4H',raw,8)!=(NAMES,FRAME_BASE,NAME_DIR,POOL_BASE):reject()
    if raw[16:64]!=bytes(i//3 for i in range(48)):reject()
    last=POOL_BASE
    for offset in struct.unpack_from('>255H',raw,NAME_DIR):
        if not POOL_BASE<=offset<len(raw):reject()
        end=raw.find(b'\0',offset,min(len(raw),offset+16))
        if end<=offset or any(c<32 or c>126 for c in raw[offset:end]):reject()
        last=max(last,end)
    if len(raw)!=((last+2)&~1) or any(raw[last+1:]):reject()
    for i in range(FRAMES):
        start=FRAME_BASE+RECORD*i
        for size in (104,116):
            a,b,mask=[raw[start+size*p:start+size*(p+1)]for p in range(3)]
            if any((x|y)&m for x,y,m in zip(a,b,mask)):reject()
            start+=size*3
    return True


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('review',type=Path);ap.add_argument('output',type=Path);args=ap.parse_args()
    raw=from_review(args.review.read_bytes());args.output.parent.mkdir(parents=True,exist_ok=True)
    if not args.output.exists()or args.output.read_bytes()!=raw:args.output.write_bytes(raw)
    print(f'FMA1: {len(raw)} bytes,16 bodies/portraits,48 job aliases,255 names')


if __name__=='__main__':main()
