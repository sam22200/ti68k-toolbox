#!/usr/bin/env python3
"""The jobs and items every chapter shares (campaign/ENGINE.md core milestone) -> ffcj.bin
(never committed): fft.h's Job and Item records indexed by FFT's own ids, so a battle can field
any job and equipment the campaign save holds (a unit's job and equipment are FFT ids).

usage: common_tables.py DISC.cue      (writes opening/data and campaign/data/ffcj.bin)
Layout: "FCJ1", u16 offsets: jobs (Job, 14 bytes, 160 of them: growth and multiplier per raw
stat, Move, Jump, class evade, career row), job names (14 bytes each), items (Item, 26 bytes,
256: id, HP, MP, PA, MA, Speed, Move, Jump, WP, knife (1 a knife, 2 a bow: XA (PA + Speed) / 2),
name, reach (the weapon's range), shield evade, 0). Read from SCUS_942.21 as tools/battle.py
reads them; the names from BATTLE.BIN.
"""
from pathlib import Path
import struct
import sys

import battle
import management_data

HERE = Path(__file__).resolve().parents[1]
JOBS, ITEMS = 0xA0, 256
SHIELD_EVADE = 0x80063eb8                       # 2 bytes a shield (ids 0x80..0x8F): physical first
KNIFE, BOW = 1, 12                              # item types


def main():
    d = battle.Disc(sys.argv[1])
    job_names = management_data.names(d.bat, management_data.JOB_NAMES, JOBS)
    item_names = management_data.names(d.bat, management_data.ITEM_NAMES, ITEMS)
    body = bytearray(b'FCJ1' + bytes(6))
    offs = [len(body)]
    for j in range(JOBS):
        e = d.job(j)
        row = j - 0x4A if 0x4A <= j < 0x4A + 20 else 0
        body += bytes(e['growth'] + e['mult'] + [e['move'], e['jump'], e['cev'], row])
    offs.append(len(body))
    for j in range(JOBS):
        body += job_names[j][:13].encode('ascii').ljust(14, b'\0')
    offs.append(len(body))
    for i in range(ITEMS):
        it = d.item(i)
        w = d.at(battle.WEAPONS + 8 * i, 8) if it['flags'] & 0x80 else bytes(8)
        knife = 1 if it['type'] == KNIFE else 2 if it['type'] == BOW else 0
        sev = d.at(SHIELD_EVADE + 2 * (i - 0x80), 1)[0] if 0x80 <= i < 0x90 else 0
        rec = bytes([i, it['hp'], it['mp'], it['pa'], it['ma'], it['sp'], it['move'], it['jump'], w[4], knife])
        rec += item_names[i][:12].encode('ascii').ljust(13, b'\0') + bytes([w[0], sev, 0])
        assert len(rec) == 26
        body += rec
    struct.pack_into('>3H', body, 4, *offs)
    for chapter in ('opening', 'campaign'):
        (HERE / chapter / 'data/ffcj.bin').write_bytes(bytes(body))
    print('ffcj: %d bytes (%d jobs, %d items)' % (len(body), JOBS, ITEMS))


if __name__ == '__main__':
    main()
