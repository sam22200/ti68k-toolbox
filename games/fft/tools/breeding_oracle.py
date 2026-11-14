#!/usr/bin/env python3
"""Execute one original USA MIPS daily-breeding routine in bounded PC fixtures.

The original scan/timer/rarity/parent/offspring/day/level policy runs unchanged.
External roster lookup, RNG, script variables, creation and leveling are explicit
fixtures. This does not certify full retail generation, travel or save/UI flows.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

import battle
import breeding_rules
import extract


def check(cue, cases):
    import unicorn
    from unicorn import mips_const as reg
    if unicorn.__version__ != '2.1.4':
        raise ValueError('Use unicorn==2.1.4 in tools/pyenv')
    disc, files = extract.disc_files(str(cue))
    with disc.f:
        exe, world = [disc.read(*files[name]) for name in ('SCUS_942.21', 'WORLD/WORLD.BIN')]
    root = Path(__file__).resolve().parents[3]
    uc = unicorn.Uc(unicorn.UC_ARCH_MIPS, unicorn.UC_MODE_MIPS32 | unicorn.UC_MODE_LITTLE_ENDIAN)
    uc.mem_map(0, 0x200000); uc.mem_write(0x10000, exe[0x800:]); uc.mem_write(0xE0000, world)
    symbols = dict(daily=('world', 'world_unit_update_monster_breeding'),
                   zodiac=('world', 'world_unit_get_zodiac_for_date'),
                   get=('main', 'main_party_get_data_pointer'), rng=('main', 'rand'), seed=('main', 'srand'),
                   variable=('world', 'world_script_get_variable'), skillset=('world', 'world_job_get_skillset'),
                   create=('main', 'main_party_create_monster_egg'), level=('main', 'main_party_level_unit_to_target'))
    addresses, hashes = {}, {}
    for key, (target, symbol) in symbols.items():
        text = (root/'sources/fft_decomp/target'/f'{target}.yaml').read_text()
        match = re.search(r'addr: (0x[0-9a-f]+), size: (\d+), name: '+symbol+r', hash: ([0-9a-f]+)', text)
        assert match, symbol
        addr, size = int(match[1], 16), int(match[2]); blob = world if target == 'world' else exe
        offset = addr-(breeding_rules.WORLD_LOAD if target == 'world' else battle.EXE)
        sha = hashlib.sha256(blob[offset:offset+size]).hexdigest()
        assert sha.startswith(match[3]), symbol
        addresses[key] = addr; hashes[symbol] = sha
    base, created, levelled, rolls = 0x801DD000, [], [], []
    count, month, day = 0, 1, 2
    def external(machine, address, size, data):
        nonlocal count
        key = next(k for k, value in addresses.items() if value == address)
        a = machine.reg_read(reg.UC_MIPS_REG_A0); result = 0
        if key == 'get': result = base+256*a
        elif key == 'rng': result = rolls[count]; count += 1
        elif key == 'skillset': result = exe[battle.JOBS-battle.EXE+48*a]
        elif key == 'variable':
            assert a in (0x2e, 0x2f), hex(a)
            result = month if a == 0x2e else day
        elif key == 'create':
            created.append((a, machine.reg_read(reg.UC_MIPS_REG_A1), machine.reg_read(reg.UC_MIPS_REG_A2)))
            result = next((i for i in range(16) if machine.mem_read((base&0x1fffff)+256*i+1, 1)[0] == 255), 0xffffffff)
        elif key == 'level':
            levelled.append((a-base, machine.reg_read(reg.UC_MIPS_REG_A1)))
        machine.reg_write(reg.UC_MIPS_REG_V0, result)
        machine.reg_write(reg.UC_MIPS_REG_PC, machine.reg_read(reg.UC_MIPS_REG_RA))
    for key, addr in addresses.items():
        if key not in ('daily', 'zodiac'):
            uc.hook_add(unicorn.UC_HOOK_CODE, external, begin=addr, end=addr)
    def call():
        uc.reg_write(reg.UC_MIPS_REG_A0, 1); uc.reg_write(reg.UC_MIPS_REG_RA, 0x801FF000)
        uc.reg_write(reg.UC_MIPS_REG_SP, 0x801FE000)
        uc.emu_start(addresses['daily'], 0x801FF000, count=100000)
        assert uc.reg_read(reg.UC_MIPS_REG_PC) == 0x801FF000
    def roster(n):
        rows = bytearray(20*256)
        for i in range(20):
            rows[256*i+1] = i if i < n else 255
            rows[256*i+2] = 74; rows[256*i+0x16] = 7
        return rows
    for case in cases:
        parent, roll, timer, expected_job, expected_days, expected_level = case
        rows = roster(2); rows[256+2] = parent; rows[256+4] = 32; rows[256+0x16] = 42
        uc.mem_write(base&0x1fffff, bytes(rows)); created.clear(); levelled.clear()
        rolls[:] = [roll, 13, timer, 3, 1]; count = 0; call()
        assert created == [(expected_job, expected_days*17, 3)], (case, created)
        assert levelled == [(512, expected_level-1)], (case, levelled)
        assert count == 5
    # Full roster: original calls create but returns before donor selection.
    rows = roster(16); rows[258] = 95; rows[260] = 32
    uc.mem_write(base&0x1fffff, bytes(rows)); created.clear(); levelled.clear()
    rolls[:] = [0, 0, 0, 0]; count = 0; call()
    assert count == 4 and len(created) == 1 and levelled == []
    # Last egg hatches, with the original real zodiac routine, then cannot be
    # selected as a parent/donor until the following daily scan.
    rows = roster(1); rows[2] = 94; rows[4] = 36; rows[5] = 0x21
    uc.mem_write(base&0x1fffff, bytes(rows)); created.clear(); levelled.clear()
    rolls[:] = []; count = 0; month, day = 3, 21; call()
    hatched = bytes(uc.mem_read(base&0x1fffff, 256))
    assert hatched[4:7] == bytes((32, 80, 0)), hatched[4:7]
    assert not count and not created and not levelled
    return dict(cases=len(cases)+2, unicorn=unicorn.__version__, function_sha256=hashes,
                scope='original daily policy and hatch zodiac; external creation/leveling/RNG/roster/script fixtures')
