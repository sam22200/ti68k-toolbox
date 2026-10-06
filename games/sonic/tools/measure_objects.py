#!/usr/bin/env python3
"""Probe REV00 actors and collision reactions in the original, without a UI.

Normalized object RAM is injected into a controlled flat-ground state. The
JSON and PC-test header contain local ROM measurements and remain ignored.
"""
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
GAME = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/md'))
from mdrun import MD

ROM = ROOT / 'roms/md/Sonic_1.md'
START = ROOT / 'sources/sonic1_md/start.state'


def actor(md, address):
    return dict(id=md.read(address), x=md.read(address + 8, 4),
                y=md.read(address + 12, 4), vx=md.read(address + 16, 2, True),
                vy=md.read(address + 18, 2, True), routine=md.read(address + 36),
                secondary=md.read(address + 37), flash=md.read(address + 48, 2),
                timer=md.read(address + 50, 2), state=md.read(address + 52))


def setup(md, kind, x, y):
    md.load(START)
    # Removing the ID alone leaves collision flags and stale object data.
    for address in range(0xffd800, 0xfff000, 4):
        md.write(address, 0, 4)
    md.write(0xffd008, 192 << 16, 4)
    md.write(0xffd00c, 940 << 16, 4)
    md.write(0xffd022, 0)
    for address in (0xffd010, 0xffd012, 0xffd014):
        md.write(address, 0, 2)
    md.pressed = 0
    for _ in range(30):
        md.lib.retro_run()
    md.write(0xffd800, kind)
    md.write(0xffd804, 4)
    md.write(0xffd808, x << 16, 4)
    md.write(0xffd80c, y << 16, 4)


def main():
    md = MD(ROM)
    result = {'provenance': {**md.identity,
              'rom_sha256': hashlib.sha256(ROM.read_bytes()).hexdigest(),
              'start_sha256': hashlib.sha256(START.read_bytes()).hexdigest(),
              'setup': 'clear 96 full object slots; Sonic x192/y940, flat ground'},
              'actors': {}}
    try:
        for name, kind, x, y, count in [('moto', 0x40, 256, 940, 40),
                                        ('buzz', 0x22, 288, 864, 110),
                                        ('chop', 0x2b, 256, 1120, 160)]:
            setup(md, kind, x, y)
            # Keep the stationary observer alive during long projectile traces.
            md.write(0xffd030, 255, 2)
            trace = []
            for f in range(count):
                md.lib.retro_run()
                missiles = [actor(md, a) for a in range(0xffd800, 0xfff000, 64)
                            if md.read(a) == 0x23]
                trace.append({'frame': f, **actor(md, 0xffd800), 'missiles': missiles})
            result['actors'][name] = {'kind': kind, 'x': x, 'y': y, 'trace': trace}
        setup(md, 0x25, 192, 940)
        ring = []
        for f in range(5):
            md.lib.retro_run()
            ring.append({'frame': f, 'rings': md.read(0xfffe20, 2), **actor(md, 0xffd800)})
        assert [r['rings'] for r in ring] == [0, 1, 1, 1, 1]
        result['ring'] = ring
        setup(md, 0x40, 256, 940)
        for _ in range(20):
            md.lib.retro_run()
        x = md.read(0xffd808, 2)
        md.write(0xffd008, (x - 10) << 16, 4)
        md.write(0xfffe20, 10, 2)
        hurt = []
        for f in range(80):
            md.lib.retro_run()
            hurt.append({'frame': f, **actor(md, 0xffd000),
                         'rings': md.read(0xfffe20, 2),
                         'lost': sum(md.read(a) == 0x37 for a in range(0xffd800, 0xfff000, 64))})
        assert hurt[0]['routine'] == 4 and hurt[0]['vx'] == -512 and hurt[0]['vy'] == -1024
        assert hurt[0]['flash'] == 120 and hurt[0]['rings'] == 0 and hurt[0]['lost'] == 10
        result['hurt'] = hurt
        # Same impact, this time as a descending attack ball.
        setup(md, 0x40, 256, 940)
        for _ in range(20):
            md.lib.retro_run()
        x = md.read(0xffd808, 2)
        md.write(0xffd008, x << 16, 4); md.write(0xffd00c, 920 << 16, 4)
        md.write(0xffd012, 768, 2); md.write(0xffd022, 6)
        md.write(0xffd016, 14); md.write(0xffd017, 7)
        md.write(0xffd01c, 2)  # id_Roll animation, collision attack discriminator
        md.lib.retro_run()
        attack = {'sonic': actor(md, 0xffd000), 'enemy': actor(md, 0xffd800)}
        assert attack['enemy']['id'] != 0x40 and attack['sonic']['vy'] < 0
        result['attack'] = attack
    finally:
        md.close()
    (ROOT / 'sources/sonic1_md/objects.json').write_text(json.dumps(result, indent=2) + '\n')
    lines = ['/* Local ROM samples, PC tests only. */']
    for name, action in result['actors'].items():
        lines.append(f'static const ObjectRef objref_{name}[] = {{')
        for row in action['trace']:
            lines.append('    {%d,%d,%d,%d,%d,%d},' % (row['x'] >> 16, row['y'] >> 16,
                         (row['x'] >> 8) & 255, (row['y'] >> 8) & 255, row['vx'], row['vy']))
        lines.append('};')
    (GAME / 'generated/objects_ref.h').write_text('\n'.join(lines) + '\n')
    buzz = result['actors']['buzz']['trace']
    first = next(r for r in buzz if r['missiles'])
    moving = next(r for r in buzz if any(m['routine'] == 4 for m in r['missiles']))
    landed = next(r for r in hurt if r['routine'] == 2)
    print('original: ring collected once; damage -512/-1024, gravity48, 10 lost rings')
    print('hurt landing frame', landed['frame'], 'flash', landed['flash'])
    print('buzz missile spawn', first['frame'], 'active routine', moving['frame'])
    print('attack:', attack)
    print('saved local objects.json and generated/objects_ref.h')


if __name__ == '__main__':
    main()
