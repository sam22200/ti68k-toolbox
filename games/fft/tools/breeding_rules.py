#!/usr/bin/env python3
"""Extract universal USA PS1 breeding rules; no campaign content or shipping edits.

FWB1 is a byte-only cold bank: header8, month lengths12, inclusive zodiac
limits39, breed rows48*4, monster seeds5/variance5, BE zodiac day limits12*2.
Growth/skillset tables stay in ffcj; do not add a second copy here.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import battle
import extract

SIZE = 285
WORLD_LOAD = 0x800E0000
ZODIAC, MONTHS, BREEDS = 0x8018DE84, 0x8018DEAC, 0x8018DEBC


def validate(data):
    if len(data) != SIZE or data[:4] != b'FWB1' or data[4:8] != struct.pack('>HBB', SIZE, 94, 48):
        raise ValueError('FWB1 header/length')
    months = data[8:20]
    if months != bytes((31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31)):
        raise ValueError('FWB1 non-leap calendar')
    previous = 0
    for month, day, sign in struct.iter_unpack('BBB', data[20:59]):
        if not 1 <= month <= 12 or not 1 <= day <= months[month-1] or sign >= 12:
            raise ValueError('FWB1 zodiac record')
        ordinal = sum(months[:month-1]) + day
        if ordinal <= previous:
            raise ValueError('FWB1 unordered zodiac limits')
        previous = ordinal
    if previous != 365:
        raise ValueError('FWB1 zodiac coverage')
    for days, *children in struct.iter_unpack('BBBB', data[59:251]):
        if not 1 <= days <= 15 or any(not 94 <= job < 142 for job in children):
            raise ValueError('FWB1 breed row')
    # The main day limits must describe the same zodiac as WORLD, including
    # both Capricorn portions. Reject a mismatched generator/calendar bank.
    limits = struct.unpack_from('>12H', data, 261)
    if tuple(sorted(set(limits))) != limits or not 1 <= limits[0] <= limits[-1] <= 365:
        raise ValueError('FWB1 birthday limits')
    for day in range(1, 366):
        main_sign = (sum(day >= limit for limit in limits) + 9) % 12
        world_sign = next(sign for m, d, sign in struct.iter_unpack('BBB', data[20:59])
                          if day <= sum(months[:m-1]) + d)
        if main_sign != world_sign:
            raise ValueError('FWB1 birthday/calendar disagreement')
    return data


def build(cue):
    disc, files = extract.disc_files(str(cue))
    with disc.f:
        world = disc.read(*files['WORLD/WORLD.BIN'])
        exe = disc.read(*files['SCUS_942.21'])
        core = disc.read(*files['WORLD/WLDCORE.BIN'])
    at = lambda addr, n: exe[addr-battle.EXE:addr-battle.EXE+n]
    wat = lambda addr, n: world[addr-WORLD_LOAD:addr-WORLD_LOAD+n]
    months = wat(MONTHS, 13)
    assert months[0] == 0 and months[1:] == core[0x8009E66C-0x80067000:0x8009E66C-0x80067000+12]
    # This equivalence lets the runtime use the bounded job interval, without
    # duplicating the universal job/skillset table just to identify parents.
    monster_jobs = [j for j in range(160) if 176 <= at(battle.JOBS+48*j, 1)[0] < 224]
    assert monster_jobs == list(range(94, 142))
    base = at(battle.GEN_BASE+3*12, 12)
    assert base[5:] == b'\xff'*7, 'monster generator equipment must be empty'
    limits = struct.unpack('<12H', at(battle.ZODIAC, 24))
    bank = validate(b'FWB1'+struct.pack('>HBB', SIZE, 94, 48)+months[1:]+wat(ZODIAC, 39)
                    +wat(BREEDS, 192)+base[:5]+at(battle.GEN_VAR+15, 5)+struct.pack('>12H', *limits))
    inputs = {'WORLD/WORLD.BIN': world, 'SCUS_942.21': exe, 'WORLD/WLDCORE.BIN': core}
    return bank, dict(format='FWB1', bytes=len(bank), sha256=hashlib.sha256(bank).hexdigest(),
                     input_sha256={name: hashlib.sha256(raw).hexdigest() for name, raw in inputs.items()},
                     evidence='disc tables plus local decomp source read; not a live retail gameplay oracle')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('cue', type=Path); ap.add_argument('out', type=Path)
    args = ap.parse_args(); bank, proof = build(args.cue)
    args.out.parent.mkdir(parents=True, exist_ok=True); args.out.write_bytes(bank)
    args.out.with_suffix('.json').write_text(json.dumps(proof, indent=2)+'\n')
    print(json.dumps(proof, indent=2))


if __name__ == '__main__': main()
