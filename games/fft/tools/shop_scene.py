#!/usr/bin/env python3
"""Shop scene art candidates (FSK1): the shopkeeper's portrait and body, Ramza's body.

usage: shop_scene.py DISC.cue OUT.bin

Review art for the town/shop layout proposals (campaign/SHOP_SCENE.md). The
shopkeeper is the disc's 40M townsperson sheet, the same one the source shows
behind a store counter; his portrait comes from that sheet's portrait block
exactly as common_portraits.py reads a speaker's, and his body from TYPE1 frame
9 (idle, facing the camera) exactly as units.py reads a unit's. Ramza is RAMUZA
frame 14 (idle, facing away), so the two face each other across the counter.
Both bodies stay at the battle size, 16x26: the map's 67% poses are too small to
read against a room.

FSK1: magic, u16 offsets (portrait 32x29/348 bytes, Ramza 16x26/156, shopkeeper
16x26/156), then those records.
"""
from pathlib import Path
import struct
import sys

import extract
import common_portraits
import interface
import units

KEEPER = '40M'
RAMZA = 'RAMUZA'


def build(cue):
    disc, files = extract.disc_files(cue)
    shp = units.shp_frames(disc.read(*files['BATTLE/TYPE1.SHP']))
    portrait = common_portraits.portrait(disc, files, KEEPER)
    bodies = []
    for name, frame in ((RAMZA, 14), (KEEPER, 9)):
        spr = disc.read(*files['BATTLE/%s.SPR' % name])
        bodies.append(interface.planes(units.sprite(spr, shp[frame]), 16))
    at = 10
    head = b'FSK1'
    for record in (portrait,) + tuple(bodies):
        head += struct.pack('>H', at)
        at += len(record)
    out = head + portrait + b''.join(bodies)
    assert len(out) == at
    return out, dict(bytes=len(out), keeper=KEEPER)


def main():
    cue, out = sys.argv[1], Path(sys.argv[2])
    raw, info = build(cue)
    out.write_bytes(raw)
    print('%s: %d bytes, shopkeeper %s' % (out.name, info['bytes'], info['keeper']))


if __name__ == '__main__':
    main()
