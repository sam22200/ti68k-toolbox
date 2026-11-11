#!/usr/bin/env python3
"""Check the catalogue against the disc, then sweep it on the PC and actual 68000."""
from pathlib import Path
import re
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE.parent / 'tools'))
import battle
import common_tables
import management_data


def run(args):
    p = subprocess.run(list(map(str, args)), cwd=HERE / 'data', text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode:
        print(p.stdout); p.check_returncode()
    return p.stdout


def main():
    disc = battle.Disc(str(ROOT / 'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'))
    bank = (HERE / 'data/ffcj.bin').read_bytes()
    assert bank == (HERE.parent / 'opening/data/ffcj.bin').read_bytes()
    offsets = struct.unpack_from('>15H', bank, 4)
    assert offsets[-1] == len(bank)
    assert bank[:4] == b'FCJI'
    item_base=offsets[3]
    maximum=0
    for i in range(256):
        primary=disc.at(battle.ITEMS+12*i,12)
        base=common_tables.SHIELD_EVADE if primary[3]&0x40 else common_tables.ACCESSORY_EVADE if primary[3]&8 else None
        ev=disc.at(base+2*primary[4]+1,1)[0] if base else 0
        packed=bank[item_base+30*i+29]
        assert ((packed>>1)&63)==ev and bool(packed&128)==bool(primary[3]&0x40)
        maximum=max(maximum,ev)
    assert maximum==50 and bank==common_tables.build(disc)
    for k, name in enumerate(management_data.names(disc.bat, management_data.ITEM_NAMES, 256)):
        at = offsets[3] + 30 * k + 10
        assert bank[at:bank.index(0, at)].decode('ascii') == name
    for section, address, count in ((5, battle.ABILITY_RANGE, 0x170 * 14),
                                    (6, battle.ITEM_ABILITY, 14 * 3),
                                    (7, common_tables.SPECIAL, 168),
                                    (8, common_tables.HUMAN, 0xB0 * 25),
                                    (9, common_tables.MONSTER, 48 * 5)):
        assert bank[offsets[section]:offsets[section] + count] == disc.at(address, count)
    assert bank[offsets[12]:offsets[12] + 128 * 6] == common_tables.status_rows(disc)   # + Mbarrier's row
    assert bank[offsets[7]+168:offsets[7]+168+224] == disc.at(common_tables.ACTION_MENUS,224)
    assert bank[offsets[12]+128*6:offsets[12]+128*6+650] == common_tables.status_policy_bytes(disc)
    discovery=len(bank)-(96+2048+64+512)-128*8
    assert bank[discovery:discovery+96]==disc.at(0x80066064,96)
    assert bank[discovery+96:discovery+96+2048]==disc.bat[0x800f5e74-battle.BAT:0x800f6674-battle.BAT]
    aliases=bank[discovery+96+2048:discovery+96+2048+64]
    assert aliases[7]==22 and aliases[19]==56
    for item in range(256):
        price=int.from_bytes(disc.at(battle.ITEMS+12*item+8,2),'little')
        assert struct.unpack_from('>H',bank,discovery+96+2048+64+2*item)[0]==price
    assert bank[-128*8:]==disc.at(battle.WEAPONS,128*8)
    for ability in range(512):
        at = offsets[4] + ability * 8
        source = disc.at(battle.ABILITY + ability * 8, 8)
        assert struct.unpack_from('>H', bank, at)[0] == int.from_bytes(source[:2], 'little')
        assert bank[at + 2:at + 8] == source[2:]
    for section, address, count in ((10, management_data.ABILITY_NAMES, 512),
                                    (11, management_data.SET_NAMES, 64)):
        for k, name in enumerate(management_data.names(disc.bat, address, count)):
            at = struct.unpack_from('>H', bank, offsets[section] + 2 * k)[0]
            actual = bank[at:bank.index(0, at)].decode('ascii') if at else ''
            assert actual == (name if name.strip() else '')

    out = HERE / 'x/common'; out.mkdir(parents=True, exist_ok=True)
    core = ROOT / 'runtime/core/rt_core.c'
    run(['cc', '-std=gnu99', '-O1', '-Wall', '-Wextra', '-o', out / 'reader',
         HERE / 'check_common.c', core, ROOT / 'runtime/platform-sw/rt_sw.c'])
    run([ROOT / 'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out / 'readerc',
         HERE / 'check_common.c', core, ROOT / 'tools/extgraph/lib/tilemap.a'])
    pc = run([out / 'reader'])
    ti = run([ROOT / 'tools/bin/ti-cycles', '--frames', 1, '--max', 100,
              '--file', HERE / 'data/ffcj.89y', out / 'readerc.89z'])
    (out / 'cycles.log').write_text(ti)
    match = re.search(r'catalogue ([0-9A-F]+) failure 0', pc); assert match, pc
    values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', ti)]
    assert values[-2:] == [int(match[1], 16), 0], (pc, ti)
    print('Catalogue: 512 primary records, 368 range/formulas, 224 skill sets and names '
          'match the disc; PC/68000 reader hash %s' % match[1])


if __name__ == '__main__':
    main()
