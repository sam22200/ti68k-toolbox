#!/usr/bin/env python3
"""Two real battle children, Save/Continue and guest roles through fp_story.

Private authored QA chain on reused Gariland assets, not retail story content.
PC/sanitizers by default; only the changed progress reader has opt-in native.
"""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from check_flow import HERE, ROOT, fixture
import chapter_packages as packages
import objective_bank
import guest_bank


def run(command, log, cwd=ROOT, allowed=(0,)):
    p = subprocess.run(list(map(str, command)), cwd=cwd, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(p.stdout)
    if p.returncode not in allowed:
        print(p.stdout); p.check_returncode()
        raise AssertionError(('unexpected child result', p.returncode))
    return p.stdout


def chain_fixture(source, guests=False):
    core = json.loads((HERE/'packages/core.json').read_text())
    spec = deepcopy(json.loads((HERE/'packages/chapter-01.json').read_text()))
    spec.pop('persistent_guests', None)
    spec.update(title='Private QA: two borrowed arenas, forced guest and checkpoint',
                world_map=False, requires_flag=0, completion_flag=9, next=65535,
                routes=[dict(band=2, prefix='g'), dict(band=3, prefix='h')],
                steps=[dict(program='battle', argument=216, skip_flag=8, save_prompt=True),
                       dict(program='battle', argument=316, skip_flag=9)])
    spec['variables'] = ['ffca', 'ffcb', 'ffflow', 'ffgo'] + [f'ff{p}{s}' for p in ('g', 'h') for s in ('v0', 'v1', 'v2', 'v3', 'a', 'i', 'd')]
    source = dict(source)
    for prefix, won in [('g', 8), ('h', 9)]:
        for suffix in ('v0', 'v1', 'v2', 'v3', 'a', 'i', 'd'):
            old = source['ffg'+suffix]
            source['ff'+prefix+suffix] = packages.Variable('ff'+prefix+suffix, old.kind, old.attr, old.payload)
        # Only encounter metadata changes: no re-extraction, placement or art.
        old = source['ff'+prefix+'d']; desc = bytearray(old.raw)
        for word, value in [(37, 0), (42, 0), (43, 3), (44, won), (45, 0)]:
            struct.pack_into('>H', desc, 2*word, value)
        unit_defs = struct.unpack_from('>H', desc, 34)[0]
        desc[unit_defs+3] = 15  # fixed guest test level; party level comes from the save
        for slot in range(1, 6): desc[unit_defs+28*slot+3] = 1
        source[old.name] = packages.Variable(old.name, old.kind, old.attr, bytes(desc)+packages.resources.TRAILER)
    goals = objective_bank.build([dict(route=p, protect=dict(slot=0, name='Delita')) for p in ('g', 'h')])
    source['ffgo'] = packages.Variable('ffgo', 0x1c, 3, goals+packages.resources.TRAILER)
    if guests:
        spec['variables'].append('ffgu')
        source['ffgu'] = packages.Variable('ffgu', 0x1c, 3, guest_bank.build([dict(route=p, slot=0, id=4) for p in ('g', 'h')])+packages.resources.TRAILER)
    source = {name: source[name] for name in core['variables']+spec['variables']}
    initial = bytearray(b'FSV6'+fixture()[4:]+bytes(3918-len(fixture())))
    initial[4] = 3; initial[36] = 16; initial[3114] = 4; initial[3115:3138] = bytes(23)
    for slot in range(7, 16):
        initial[294+176*slot:294+176*(slot+1)] = initial[470:646]
        initial[294+176*slot+3] = slot*11
    for slot in range(16): initial[294+176*slot+4:294+176*slot+6] = bytes((15, 99))
    return source, core, spec, bytes(initial)


def run_cases(out, variables, initial, binaries):
    results = []
    for variant, label in enumerate(('save-between-wins', 'decline-between-wins', 'save-defeat-continue')):
        logs, snapshots = [], []
        for mode in ('pc', 'san'):
            with tempfile.TemporaryDirectory(prefix='fft-chain-') as folder:
                data = Path(folder)
                for v in variables:
                    (data/(v.name+'.payload')).write_bytes(v.payload)
                    if v.kind == 0x1c and v.payload.endswith(packages.resources.TRAILER): (data/(v.name+'.bin')).write_bytes(v.raw)
                (data/'ffsave.sav').write_bytes(initial); (data/'ffsave1.sav').write_bytes(initial)
                (data/'ffslot.sav').write_bytes(b'FSL1\x01\0')
                log = run(['env', 'ASAN_OPTIONS=detect_leaks=0', 'UBSAN_OPTIONS=halt_on_error=1',
                           binaries['supervisor', mode], binaries['battle', mode], binaries['title', mode], variant],
                          out/f'{label}-{mode}.log', data)
                logs.append(log)
                snapshots.append({p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in data.glob('*.sav')})
                if mode == 'pc':
                    (out/f'{label}-session.sav').write_bytes((data/'ffsave.sav').read_bytes())
                    (out/f'{label}-slot.sav').write_bytes((data/'ffsave1.sav').read_bytes())
        assert logs[0] == logs[1], (label, 'sanitizer replay differs')
        assert snapshots[0] == snapshots[1], (label, 'PC/sanitizer save bytes differ')
        match = re.search(r'chain-supervisor variant\d+ hash(\w+) checks(\d+) failures0', logs[0]); assert match
        results.append(dict(case=label, hash=match[1], supervisor_checks=int(match[2]), sanitizer_equal=True,
                            save_sha256=snapshots[0]))
        print(label+': '+match[0], flush=True)
    return results


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input', type=Path, required=True, help='current core/chapter source.89g built once beforehand')
    ap.add_argument('--out', type=Path, default=HERE/'x/chains')
    ap.add_argument('--native-progress', action='store_true')
    ap.add_argument('--guests', action='store_true', help='retain Delita by stable identity across both fights')
    args = ap.parse_args(); out = args.out.resolve(); out.mkdir(parents=True, exist_ok=True)
    source, core, spec, initial = chain_fixture(packages.read_group(args.input.resolve()), args.guests)
    report, shared, content = packages.build(source, core, [spec], out/'fixture-package')
    variables = shared+content[0][1]
    # Authoring failures must be rejected before distributing an unusable checkpoint.
    for change in [dict(skip_flag=0), dict(skip_flag=7), dict(save_prompt='yes')]:
        bad = deepcopy(spec); bad['steps'][0].update(change)
        try: packages.chapter_package(source, bad, core, bytes.fromhex(report['core_key']))
        except ValueError: pass
        else: raise AssertionError(('bad checkpoint accepted', change))
    if args.guests:
        for raw in (b'FGP1', guest_bank.build([dict(route='g', slot=1, id=4)])):
            bad_source = dict(source)
            bad_source['ffgu'] = packages.Variable('ffgu', 0x1c, 3, raw+packages.resources.TRAILER)
            try: packages.chapter_package(bad_source, spec, core, bytes.fromhex(report['core_key']))
            except ValueError: pass
            else: raise AssertionError('invalid guest binding accepted')
    binaries = {}; common = [ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c']
    for mode, flags in [('pc', []), ('san', ['-fsanitize=address,undefined', '-fno-sanitize=shift', '-fno-pie', '-no-pie'])]:
        for name, filename, runtime in [('battle', 'check_chain_battle.c', True), ('title', 'check_chain_title.c', True),
                                        ('supervisor', 'check_chain_supervisor.c', False), ('progress', 'test_save_progress.c', False)]:
            binary = out/f'{name}-{mode}'; binaries[name, mode] = binary
            run(['cc', '-std=gnu99', '-O1', '-g', '-w', *flags, '-o', binary, HERE/filename, *(common if runtime else [])], out/f'{name}-{mode}-build.log')
        run([binaries['progress', mode]], out/f'progress-{mode}.log')
    results = run_cases(out, variables, initial, binaries)
    if args.native_progress:
        run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'svprog', HERE/'test_save_progress.c'], out/'progress-native-build.log')
        log = run([ROOT/'tools/bin/ti-cycles', '--frames', 0, '--max', 100, out/'svprog.89z'], out/'progress-native.log')
        expected = re.search(r'save-progress (\w+) failures 0 checks (\d+)', (out/'progress-pc.log').read_text())
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
        assert values[-3:] == [int(expected[1], 16), 0, int(expected[2])], values
    summary = dict(cases=results, native_progress=args.native_progress, private_chain_not_retail_content=True,
                   core_key=report['core_key'], abi=91, save_bytes=3918, persistent_guests=args.guests,
                   initial_sha256=hashlib.sha256(initial).hexdigest(),
                   preserved=json.loads((HERE/'x/objectives/verification.json').read_text())['protected'])
    (out/'verification.json').write_text(json.dumps(summary, indent=2)+'\n')


if __name__ == '__main__': main()
