#!/usr/bin/env python3
"""Complete Sweegy combat/save replays on PC by default; commercial fixtures stay local.

The real MAP074 collision columns and ENTD180 monster/guest definitions are used.
Levels, generation rolls, party deployment and learned human actions are frozen
test inputs, not a claim to reproduce retail encounter-generation RNG. No artwork
is generated or reviewed here; these runs exercise game_update without rendering.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import re

from check_flow import HERE, ROOT, data_links, fixture
sys.path.insert(0, str(HERE.parent / 'tools'))
import battle
import core_audit
import extract
import management_data
import surface_data


def sha(data):
    return hashlib.sha256(data).hexdigest()


def build_fixture(out, party_level, enemy_level):
    """Private FCD5/FGS2 carrier; never replace the installed pilot banks."""
    disc = battle.Disc(str(core_audit.DEFAULT_DISC))
    disk, files = extract.disc_files(str(core_audit.DEFAULT_DISC))
    terrain, meta, provenance = surface_data.terrain(disk, files, 74)
    width, height = meta['width'], meta['height']
    desc, _ = surface_data.detach_descriptor((HERE / 'data/ffgd.bin').read_bytes())
    desc = bytearray(desc)
    interface = bytearray((HERE / 'data/ffgi.bin').read_bytes())
    # Use the checked-in descriptor field definitions, including appended fields.
    fields = {k: int(v) for k, v in re.findall(r'^#define CD_(\w+) (\d+)$',
              (HERE.parent / 'chapter.h').read_text(), re.M)}

    def get(name):
        return struct.unpack_from('>H', desc, 2 * fields[name])[0]

    def put(name, value):
        struct.pack_into('>H', desc, 2 * fields[name], value)

    def append(target, data):
        if len(target) & 1:
            target.append(0)
        at = len(target)
        target.extend(data)
        return at

    # Keep coordinates in the source convention: x right, source y = world z.
    tiles = bytearray()
    for i in range(width * height):
        b = terrain[8*i:8*(i+1)]
        h, depth, slope, flags = b[2], b[3] & 31, b[4], b[6]
        corners = extract.corners(h, depth, slope if depth else 0)
        walk = 0 if flags & 1 else 2 if flags & 2 else 1
        tiles.extend(bytes((*corners, 0, 0, 0, 0, 0, walk,
                            2*h + (depth if slope else 0), int((b[0] & 63) == 14))))
    put('TILES', append(interface, tiles))
    put('MAP_W', width)
    put('MAP_H', height)
    put('SOURCE_MAP',74)
    put('SC_TOP', 0)
    put('START', 0)
    put('MAP_SCRIPT', 0)
    put('WATER', 0)
    put('RAMZA', 0)
    put('PARTY', 3)                 # persistent party, read from the private save
    put('WON', 8)                   # private progress flag, never Gariland's flag
    world = disk.read(*files['WORLD/WORLD.BIN'])
    names = management_data.names(world, 0x8e1a0, 32)
    location = next(i for i, name in enumerate(names) if 'Sweegy' in name)
    put('LOCATION', location)

    initial = bytearray(b'FSV4' + fixture()[4:] + bytes(64))
    initial[3114] = 4
    initial[3115] = 0             # no retained Gariland deployment
    initial[3116:3118] = bytes(2)
    for i in range(initial[36]):
        initial[294 + 176*i + 4] = party_level
    # A bounded legal deployment on the west side; original guests retain ENTD positions.
    placements = [(2, 3), (2, 4), (2, 5), (2, 6), (3, 4)]
    original_defs = get('UNIT_DEFS')
    definitions = bytearray()
    raw_stats = bytearray()
    zodiacs = bytearray()
    rewards = bytearray()
    for i, source_index in enumerate((6, 7, 8, 9, 10)):
        row = bytearray(desc[original_defs + 28*source_index:original_defs + 28*(source_index+1)])
        row[8:12] = bytes((*placements[i], 0, 0))
        definitions.extend(row)
        raw_stats.extend(bytes(20))  # overwritten by production party_load
        zodiacs.append(0)
        rewards.extend(bytes(2))
    source_rows = []
    for slot in range(8):
        original = disc.entd(0x180, slot)
        assert original[0]
        job = original[10]
        monster = job >= 94
        generation_type = 3 if monster else 0
        level = enemy_level if monster else party_level
        eq = []
        for k, cat in enumerate((0x20, 0x10, 0x08, 0x80, 0x40)):
            item = original[18+k]
            if monster:
                item = 255
            elif item == 254:
                candidates = disc.candidates(job, level, cat)
                item = candidates[0] if candidates else 255
            eq.extend((item, 255))
        job_set = disc.job(job)['set']
        learned = 7 if monster else sum(1 << k for k, ab in enumerate(disc.skillset(job_set)) if ab)
        row = bytes((255, job, generation_type, level, 60, 60)) + struct.pack('>H', 1)
        row += bytes((original[25], original[26], battle.FACE[original[27] & 3],
                      1 if monster else 2, *eq, job_set, 0)) + struct.pack('>HH', learned, 0)
        assert len(row) == 28
        definitions.extend(row)
        base, variation, _ = disc.gen(generation_type)
        raw = [(a << 14) + (v << 13) for a, v in zip(base, variation)]
        raw_stats.extend(struct.pack('>5I', *raw))
        zodiacs.append(12 if monster else 0)
        rewards.extend(original[30:32])
        source_rows.append(dict(slot=slot, runtime_slot=5+slot, job=job, set=job_set,
                                x=original[25], z=original[26], level=level,
                                source_hex=original.hex()))
    assert len(definitions) == 13*28
    for x, z in placements:
        assert tiles[12*(z*width+x)+9] == 1
    put('UNIT_COUNT', 13)
    put('UNIT_DEFS', append(desc, definitions))
    put('UNIT_ZODIAC', append(desc, zodiacs))
    put('UNIT_REWARDS', append(desc, rewards))
    put('UNIT_RAW', append(interface, raw_stats))
    put('UNIT_NAMES', 0)
    geometry = surface_data.encode(terrain, width, height, with_terrain=True)
    descriptor = surface_data.attach_descriptor(desc, geometry)
    out.mkdir(parents=True, exist_ok=True)
    for name, payload in (('ffgd.bin', descriptor), ('ffgi.bin', interface), ('initial.sav', initial)):
        (out / name).write_bytes(payload)
    return dict(**provenance, encounter=0x180, location=location, width=width, height=height,
                party_level=party_level, enemy_level=enemy_level, units=source_rows,
                descriptor_sha256=sha(descriptor), interface_sha256=sha(interface),
                initial_save_sha256=sha(initial), catalogue_sha256=sha((HERE/'data/ffcj.bin').read_bytes()),
                rendering=False, generation='fixed levels and midpoint raw rolls; human actions enabled')


def run(args, cwd, log_path=None):
    if log_path:
        with log_path.open('w') as stream:
            p = subprocess.run(list(map(str, args)), cwd=cwd, text=True, stdout=stream,
                               stderr=subprocess.STDOUT, timeout=180)
        output = log_path.read_text()
    else:
        p = subprocess.run(list(map(str, args)), cwd=cwd, text=True, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=180)
        output = p.stdout
    if p.returncode:
        print('\n'.join(output.splitlines()[-25:]))
        p.check_returncode()
    return output


def native_boot(out):
    """One native run of the changed PARTY/SAVE boundaries, not a battle sweep."""
    import save_dump
    data = out/'native-data'
    data.mkdir(exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python', HERE.parent/'tools/module.py', data/'ffbm.bin',
         HERE/'battle.c', 28, '--second', data/'ffbn.bin', '--third', data/'ffbo.bin',
         '--companion', 27], HERE, out/'native-modules.log')
    assert (data/'modules.h').read_bytes() == (HERE/'data/modules.h').read_bytes(), 'Native header changed; rebind the checker before running'
    source = [HERE/'check_monster_boot.c', ROOT/'runtime/core/rt_core.c']
    run(['cc', '-std=gnu99', '-O1', '-w', '-o', out/'boot', *source,
         ROOT/'runtime/platform-sw/rt_sw.c'], HERE)
    with tempfile.TemporaryDirectory(prefix='fft-monster-boot-') as temp:
        folder = Path(temp)
        data_links(folder)
        for name in ('ffgd', 'ffgi'):
            (folder/(name+'.bin')).unlink()
            (folder/(name+'.bin')).symlink_to(out/'ordinary'/(name+'.bin'))
        (folder/'ffsave.sav').write_bytes((out/'ordinary/initial.sav').read_bytes())
        pc = run([out/'boot'], folder, out/'boot-pc.log')
        saved = (folder/'ffsave.sav').read_bytes()
    match = re.search(r'monster_boot (\w+) failures 0 checks (\d+)', pc)
    assert match, pc
    run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'bootc', *source,
         ROOT/'lib/unpack68k.s', ROOT/'tools/extgraph/lib/tilemap.a'], HERE,
        out/'native-build.log')
    files = {name: HERE/'data'/(name+'.89y') for name in
             'ffgv0 ffgv1 ffgv2 ffgv3 ffga ffcp ffcu ffcw ffca ffcb ffcj ffflow ffmg'.split()}
    for name in ('ffgd', 'ffgi'):
        (data/(name+'.bin')).write_bytes((out/'ordinary'/(name+'.bin')).read_bytes())
    for name in ('ffbm', 'ffbn', 'ffbo', 'ffgd', 'ffgi'):
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat', name+'.bin', name], data)
        files[name] = data/(name+'.89y')
    (data/'ffsave.sav').write_bytes((out/'ordinary/initial.sav').read_bytes())
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'sav', 'ffsave.sav', 'ffsave'], data)
    files['ffsave'] = data/'ffsave.89y'
    destination = out/'native-save'
    destination.mkdir(exist_ok=True)
    command = [ROOT/'tools/bin/ti-cycles', '--frames', 0, '--arg', 216, '--max', 2000,
               '--save-dir', destination]
    for path in files.values():
        command += ['--file', path]
    ti = run([*command, out/'bootc.89z'], data, out/'boot-native.log')
    values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', ti)]
    assert values[-3:] == [int(match[1], 16), 0, int(match[2])] and '_main returned' in ti, ti[-3000:]
    assert save_dump.payload(destination/'ffsave.89y') == saved, 'Native saved bytes differ'
    result = dict(checks=int(match[2]), hash=match[1], native_equal=True,
                  save_equal=True, native_runs=1, full_native_battle=False,
                  scope='fixed-unit innate Float and non-Gariland reward/save overlay boundaries')
    (out/'native.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result), flush=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'x/chapter1-monster-battle')
    ap.add_argument('--quick', action='store_true', help='development run without sanitizer compilation')
    ap.add_argument('--case', choices=('ordinary', 'ordinary-second-seed', 'outmatched'))
    ap.add_argument('--native', action='store_true', help='also run one selected native initialization/save check')
    args = ap.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sources = [HERE/'check_monster_battle.c', ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c']
    common = ['cc', '-std=gnu99', '-O1', '-g', '-w']
    run([*common, '-o', out/'check', *sources], HERE)
    binaries = ['check']
    if not args.quick:
        run([*common, '-fsanitize=address,undefined', '-fno-sanitize=shift', '-o', out/'check-san', *sources], HERE)
        binaries.append('check-san')
    results = []
    for name, party, enemy, seed in (('ordinary', 3, 3, 0x9e3779b1),
                                     ('ordinary-second-seed', 3, 3, 0x3c6ef363),
                                     ('outmatched', 1, 12, 0xdaa66d15)):
        if args.case and args.case != name:
            continue
        folder = out/name
        provenance = build_fixture(folder, party, enemy)
        (folder/'fixture.json').write_text(json.dumps(provenance, indent=2)+'\n')
        logs = []
        for binary in binaries:
            with tempfile.TemporaryDirectory(prefix='fft-monster-battle-') as temp:
                data = Path(temp)
                data_links(data)
                for bank in ('ffgd', 'ffgi'):
                    (data/(bank+'.bin')).unlink()
                    (data/(bank+'.bin')).symlink_to(folder/(bank+'.bin'))
                (data/'ffsave.sav').write_bytes((folder/'initial.sav').read_bytes())
                log = run(['env', 'ASAN_OPTIONS=detect_leaks=0', out/binary, seed], data,
                          folder/(binary+'.log'))
                saved = (data/'ffsave.sav').read_bytes()
                (folder/(binary+'.sav')).write_bytes(saved)
                logs.append((log, saved))
        if len(logs) == 2:
            assert logs[0] == logs[1], (name, 'sanitizer replay/save mismatch')
        result = json.loads(logs[0][0].splitlines()[-1])
        assert result['failures'] == 0
        assert result['outcome'] == (2 if name == 'outmatched' else 1), (name, result)
        events = [json.loads(line) for line in logs[0][0].splitlines()]
        counts = {}
        for i, event in enumerate(events):
            if event.get('event') != 'action' or event['job'] < 94:
                continue
            ability = event['ability']
            counts[ability] = counts.get(ability, 0) + 1
            if ability == 0x115:
                following = events[i+1:]
                end = next((k for k, row in enumerate(following) if row.get('event') == 'action'), len(following))
                assert any(row.get('event') == 'hp' and row['unit'] == event['actor'] and row['to'] == 0
                           for row in following[:end]), 'Self Destruct did not kill its caster'
        if name == 'outmatched':
            assert all(counts.get(ability, 0) for ability in (0x110, 0x111, 0x115)), counts
        result['monster_abilities'] = {f'{key:03X}': value for key, value in sorted(counts.items())}
        result.update(case=name, sanitizers_equal=len(logs)==2, trace_sha256=sha(logs[0][0].encode()),
                      save_sha256=sha(logs[0][1]), source=provenance)
        results.append(result)
        print(json.dumps({k: v for k, v in result.items() if k != 'source'}), flush=True)
    (out/'results.json').write_text(json.dumps(results, indent=2)+'\n')
    if args.native:
        assert not args.case or args.case == 'ordinary', '--native needs the ordinary fixture'
        native_boot(out)


if __name__ == '__main__':
    main()
