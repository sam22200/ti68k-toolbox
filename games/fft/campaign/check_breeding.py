#!/usr/bin/env python3
"""PC-first breeding backend/source-model checks; --native opts into one carrier.

This is not a retail execution oracle or a worldmap/save integration certificate.
Only x/breeding is written. Existing installed banks and historical saves stay put.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

from check_flow import HERE, ROOT, run
import breeding_rules
import breeding_oracle


def child_model(bank, parent, roll, draws, occupied=()):
    """Independent source-read model of monster generation into SaveUnit bytes.

    Source: world_unit_update_monster_breeding, main_party_create_monster_egg,
    main_unit_generate_out_of_battle, main_unit_generate_base_raw_stats. This
    uses preselected15-bit draws, not the PS1's wall-clock srand/PRNG state.
    """
    stream = iter(draws)
    rarity = 3 if roll < 6 else 2 if roll < 16 else 1
    next(stream)  # parent among one
    job = bank[59+4*(parent-94)+rarity]
    days = max(2, next(stream) % bank[59+4*(job-94)] + 1)
    color = next(stream) % 4
    next(stream)  # temporary birthday
    brave, faith = [next(stream)*31//32768+40 for _ in range(2)]
    raw = [base*16384+next(stream)*variance//2 for base, variance in zip(bank[251:256], bank[256:261])]
    name = next(stream)*255//32768
    while name in occupied:
        name = next(stream)*255//32768
    level = (7, 42)[next(stream) % 2]  # ordinary human then parent, original roster order
    limits = struct.unpack_from('>12H', bank, 261)
    zodiac = (sum(days*17 >= limit for limit in limits)+9) % 12
    unit = bytes((job, 255, 3, name, level, 0, brave, faith, zodiac, 0))
    unit += b''.join(value.to_bytes(3, 'big') for value in raw)
    unit += bytes(176-len(unit))
    return unit, days, color, len(draws)-len(list(stream))


def fixtures(bank):
    # Rarity edges, the base Chocobo's unusual rare -> same-base row, advanced
    # offspring ranges and the two-day clamp. Whole child records are compared.
    lines = ['// Private local-disc rules and independent source-model records.']
    emit = lambda name, raw: 'static const u8 '+name+'[]={'+','.join(map(str, raw))+'};'
    lines.append(emit('rules', bank))
    cases, original_cases = [], []
    for parent, roll, timer in [(94, 0, 0), (94, 5, 2), (95, 6, 3), (96, 15, 4),
                                (112, 16, 0), (124, 31, 8), (139, 0, 8), (141, 6, 0)]:
        draws = [13, timer, 3, 32767, 0, 32767, 32767, 1, 0, 8191, 16384, 12345, 1]
        unit, days, color, count = child_model(bank, parent, roll, draws)
        original_cases.append((parent, roll, timer, unit[0], days, unit[4]))
        cases.append((parent, roll, days, color, count))
        lines.append(emit(f'expected_{len(cases)-1}', unit))
        lines.append('static const u16 draws_'+str(len(cases)-1)+'[]={'+','.join(map(str, draws))+'};')
    lines.append('static const struct {u8 parent,roll,days,color,count;const u16 *draws;const u8 *unit;} cases[]={')
    for i, case in enumerate(cases):
        lines.append('{'+','.join(map(str, case))+f',draws_{i},expected_{i}'+'},')
    lines.append('};')
    return '\n'.join(lines)+'\n', original_cases


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--native', action='store_true')
    ap.add_argument('--disc', type=Path, default=ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue')
    args = ap.parse_args(); out = HERE/'x/breeding'; out.mkdir(exist_ok=True)
    bank, proof = breeding_rules.build(args.disc)
    fixture, original_cases = fixtures(bank)
    (out/'ffwb.bin').write_bytes(bank); (out/'fixture.h').write_text(fixture)
    proof['original_mips'] = breeding_oracle.check(args.disc, original_cases)
    (out/'oracle.json').write_text(json.dumps(proof['original_mips'], indent=2)+'\n')
    logs = []
    for mode, flags in [('pc', []), ('san', ['-fsanitize=address,undefined', '-fno-pie', '-no-pie'])]:
        (out/f'{mode}-build.log').write_text(run(['cc', '-std=gnu99', '-O1', '-g', '-Wall', '-Wextra',
            '-Wno-unused-function', *flags, '-o', out/mode, HERE/'test_breeding.c']))
        log = run(['env', 'ASAN_OPTIONS=detect_leaks=1', 'UBSAN_OPTIONS=halt_on_error=1', out/mode])
        (out/f'{mode}.log').write_text(log); logs.append(log)
    assert logs[0] == logs[1]
    proof.update(result=logs[0].strip(), native=args.native, integrated=False,
                 sources={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in [HERE/'breeding.h', HERE/'test_breeding.c', Path(__file__),
                                    HERE.parent/'tools/breeding_rules.py', HERE.parent/'tools/breeding_oracle.py']})
    if args.native:
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'breedck', HERE/'test_breeding.c']))
        native = run([ROOT/'tools/bin/ti-cycles', '--frames', 0, '--max', 2000, out/'breedck.89z'])
        (out/'native.log').write_text(native)
        match = re.search(r'breeding (\w+) failures 0 checks (\d+)', logs[0]); assert match
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', native)]
        assert values[-3:] == [int(match[1], 16), 0, int(match[2])], values
    (out/'results.json').write_text(json.dumps(proof, indent=2)+'\n'); print(json.dumps(proof, indent=2))


if __name__ == '__main__': main()
