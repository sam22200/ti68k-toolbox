#!/usr/bin/env python3
"""Audit exploration placements against the disc and a fresh original PS1 run.

Run from games/fft: tools/pyenv/bin/python tools/check_placement.py DISC.cue.
No combat position pokes. The existing oracle's documented boot/deployment door is reused.
ROM-derived JSON, CSV, save state and original screenshots remain in ignored x/.
"""
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import battle
import oracle

HERE = Path(__file__).resolve().parents[1]
OUT = HERE/'x/placement'


def positions(psx):
    return [(psx.read(oracle.UNITS+i*oracle.UNIT_SIZE+0x47),
             psx.read(oracle.UNITS+i*oracle.UNIT_SIZE+0x48)) for i in oracle.IDS]


def main(cue):
    OUT.mkdir(parents=True, exist_ok=True)
    binary = OUT/'native-placements'
    subprocess.run(['cc', '-std=gnu99', '-O1', '-DFFT_EXPLORATION', '-DFFT_FIXED',
                    '-o', str(binary), 'fft.c', 'tools/placement_capture.c',
                    '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'],
                   cwd=HERE, check=True)
    native = [tuple(map(int, ln.split())) for ln in subprocess.check_output([binary], cwd=HERE, text=True).splitlines()]
    defs = battle.unit_defs(battle.Disc(cue))
    expected = [(u['x'], u['z']) for u in defs]
    assert [(x, z) for _, x, z, _ in native] == expected
    print('Native exploration matches all 11 disc/deployment positions.', flush=True)
    psx = oracle.PSX(cue)
    oracle.boot(psx)
    oracle.deploy(psx)
    # Deployment finishes during the introduction, before any CT/AI turn.
    initial = positions(psx)
    assert initial == expected, (initial, expected)
    assert all(psx.read(oracle.UNITS+i*oracle.UNIT_SIZE+0x39) == 0 for i in oracle.IDS)
    psx.save(str(OUT/'initial.state'))
    psx.image().save(OUT/'original-initial.png')
    print('Fresh original PS1 run: all 11 initial positions match; every CT is zero.', flush=True)
    trace = []
    oracle.first_turn(psx, trace)
    later = positions(psx)
    assert psx.read(oracle.TURN_UNIT) == 16
    psx.save(str(OUT/'ramza.state'))
    psx.image().save(OUT/'original-ramza.png')
    rows = []
    for i, (u, start, end) in enumerate(zip(defs, initial, later)):
        # World conversion used by the shared renderer: x is unchanged; PS1 y -> z.
        rows.append(dict(unit=i, ps1_slot=oracle.IDS[i], kind=u['gfx'],
                         start_x=start[0], start_z=start[1], native_x=native[i][1], native_z=native[i][2],
                         ramza_turn_x=end[0], ramza_turn_z=end[1],
                         mesh_x=28*start[0]+14, mesh_z=28*start[1]+14))
    with (OUT/'placements.csv').open('w') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)
    (OUT/'trace.txt').write_text('\n'.join(trace)+'\n')
    report = dict(source='Fresh PCSX-ReARMed original PS1 run, HLE BIOS; documented oracle boot/deployment.',
                  disc_matches=11, original_initial_matches=11, x_mirrored=False, x_z_transposed=False,
                  ramza_turn_changed_units=[i for i, (a, b) in enumerate(zip(initial, later)) if a != b],
                  native_binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(), rows=rows)
    (OUT/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2), flush=True)


if __name__ == '__main__':
    main(sys.argv[1])
