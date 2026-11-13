#!/usr/bin/env python3
"""Bounded original HP-script/importance outcome study, no PS1 boot or TI run."""
import argparse
import json
from pathlib import Path
import struct

import surface_data as s
from status_state_oracle import bind_function, mask


def main():
    import unicorn
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=s.HERE/'campaign/x/objectives/oracle')
    out = ap.parse_args().out
    out.mkdir(parents=True, exist_ok=True)
    disc = s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names = {'outcome': ('battle_action_check_battle_outcome', 'battle'),
             'condition': ('battle_script_check_scenario_condition', 'battle'),
             'lookup': ('battle_unit_get_by_unit_id', 'battle')}
    hashes = {k: bind_function(k, name, part, disc) for k, (name, part) in names.items()}
    o = s.Original(disc)
    o.uc.mem_write(0x10000, disc.exe[0x800:])
    o.instruction_limit = 100000
    for key, name in [('init', 'main_status_init_check_data'), ('status', 'main_unit_has_status_in_set')]:
        hashes[key] = bind_function(key, name, 'main', disc)
    o.call('init')
    unit_base, query_unit = 0x801908cc, 0x801d7000
    found = True

    def lookup(uc, address, size, data):
        # Isolate identity/misc binding, not the HP comparator or status checks.
        uc.reg_write(o.reg.UC_MIPS_REG_V0, query_unit if found else 0xffffffff)
        uc.reg_write(o.reg.UC_MIPS_REG_PC, uc.reg_read(o.reg.UC_MIPS_REG_RA))

    at = s.FUNCTIONS['lookup'][0]
    o.uc.hook_add(unicorn.UC_HOOK_CODE, lookup, begin=at, end=at)
    hp_rows = []
    for found in (False, True):
        for hp in (0, 1, 100):
            o.uc.mem_write(query_unit & 0x1fffff, bytes(448))
            o.uc.mem_write((query_unit & 0x1fffff)+0x28, struct.pack('<HH', hp, 100))
            for condition, value in ((6, 0), (5, 1)):
                result = o.call('condition', condition, 7, value, 0)
                expected = int(found and (hp == 0 if condition == 6 else hp >= 1))
                assert result == expected, (found, hp, condition, result)
                hp_rows.append(dict(present=found, hp=hp, condition=condition, value=value, result=result))

    def outcome(blue=None, red=None, blue_disabled=False, red_disabled=False,
                ordinary_blue_dead=False, ordinary_red_dead=False):
        units = bytearray(21*448)
        for i in range(21):
            units[448*i+1] = units[448*i+0x183] = 255
        for i, team, status, disabled in ((0, 0, 2 if ordinary_blue_dead else None, False),
                                          (1, 16, 2 if ordinary_red_dead else None, False),
                                          (2, 3, blue, blue_disabled), (3, 19, red, red_disabled)):
            if i >= 2 and status is None and not disabled:
                continue
            off = 448*i
            units[off+1] = 255 if disabled else i
            units[off+0x183] = 128 if disabled else 1
            units[off+0x1ba] = team
            if status is not None:
                units[off+0x58:off+0x5d] = mask(status)
        o.uc.mem_write(unit_base & 0x1fffff, bytes(units))
        result = o.call('outcome')
        return result if result < 0x80000000 else result-0x100000000

    cases = [('ordinary-live', {}, 1), ('ordinary-enemy-KO', dict(ordinary_red_dead=True), 0),
             ('ordinary-blue-KO', dict(ordinary_blue_dead=True), -1),
             ('important-target-KO-is-not-scripted-victory', dict(red=2), 1),
             ('important-ally-KO-is-not-scripted-defeat', dict(blue=2), 1),
             ('important-target-removed', dict(red_disabled=True), 0),
             ('important-ally-removed', dict(blue_disabled=True), -1),
             ('important-target-crystal', dict(red=1), 0),
             ('important-ally-crystal', dict(blue=1), -1),
             ('simultaneous-important-losses', dict(blue=1, red=1), -1),
             ('important-target-Invite', dict(red=9), 0),
             ('important-ally-Invite', dict(blue=9), -1),
             ('important-target-Petrify-with-other-enemy', dict(red=8), 1)]
    outcomes = []
    for name, inputs, expected in cases:
        result = outcome(**inputs)
        assert result == expected, (name, result, expected)
        outcomes.append(dict(name=name, inputs=inputs, result=result))
    report = dict(hp_conditions=hp_rows, generic_outcomes=outcomes,
                  functions_sha256=hashes, exe_sha256=s.core_audit.sha(disc.exe),
                  battle_sha256=s.core_audit.sha(disc.bat),
                  scope='Original HP predicates and generic importance outcome. Unit lookup isolated for HP cases. '
                        'No authored event chain, action/Reraise ordering, native metadata binding or TI certification.')
    (out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'{len(hp_rows)} original HP predicates and {len(outcomes)} importance outcomes pass')


if __name__ == '__main__':
    main()
