#!/usr/bin/env python3
"""Split a built FFT group into one shared core and replaceable chapter groups.

Only the selected output directory is written. No live calculator, save, historical
group or input asset is changed. Chapter specs contain data, never executable code.
"""
import argparse
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re
import struct
import chapter_resources as resources

HERE = Path(__file__).resolve().parents[1]
NAME = re.compile(r'[a-z][a-z0-9]{0,7}\Z')
RESERVED = {'ffcore', 'ffpack', 'ffslot', 'ffcfg', 'ffsave', 'ffsave1', 'ffsave2', 'ffsave3'}
PROGRAMS = {'scene': 1, 'battle': 2, 'deploy': 3}
HEADER, CORE_HEADER = 48, 24


def digest(data):
    return hashlib.sha256(data).hexdigest()


def check(data):
    a = b = 0
    for value in data:
        a = (a + value) % 255
        b = (b + a) % 255
    return b << 8 | a


@dataclass(frozen=True)
class Variable:
    name: str
    kind: int
    attr: int
    payload: bytes
    folder: bytes = b'main\0\0\0\0'

    @property
    def raw(self):
        if self.kind != 0x1c or not self.payload.endswith(b'\0dat\0\xf8'):
            raise ValueError(f'{self.name}: expected a dat bank')
        return self.payload[:-6]

    @property
    def block(self):
        data = struct.pack('>H', len(self.payload)) + self.payload
        return bytes(4) + data + struct.pack('<H', sum(data) & 65535)


def read_group(path):
    data = Path(path).read_bytes()
    if len(data) < 82 or data[:8] != b'**TI89**':
        raise ValueError('Invalid TI-89 group')
    count = struct.unpack_from('<H', data, 58)[0]
    end = 60 + count * 16 + 6
    if end > len(data) or struct.unpack_from('<I', data, end-6)[0] != len(data) or data[end-2:end] != b'\xa5\x5a':
        raise ValueError('Invalid group table/size')
    result = {}
    folder = data[10:18]
    for i in range(count):
        offset, name, kind, attr, _ = struct.unpack_from('<I8sBBH', data, 60+16*i)
        if kind == 0x1f:
            if name != data[10:18] or offset != end or attr:
                raise ValueError('Invalid package folder entry')
            folder = name
            continue
        name = name.rstrip(b'\0').decode('ascii')
        if not NAME.fullmatch(name) or name in result or offset != end or end+8 > len(data):
            raise ValueError('Invalid/duplicate variable or offset')
        size = struct.unpack_from('>H', data, end+4)[0]
        stop = end+8+size
        if stop > len(data) or struct.unpack_from('<H', data, stop-2)[0] != sum(data[end+4:stop-2]) & 65535:
            raise ValueError(f'{name}: invalid length/checksum')
        result[name] = Variable(name, kind, attr, data[end+6:stop-2], folder)
        end = stop
    if end != len(data) or not result:
        raise ValueError('Empty group or trailing data')
    return result


def group_bytes(variables, single=False):
    variables = list(variables)
    if not variables or len({v.name for v in variables}) != len(variables):
        raise ValueError('Empty group or duplicate variable')
    folder = variables[0].folder
    if any(v.folder != folder for v in variables):
        raise ValueError('All packages must use one folder')
    count = len(variables) + (not single)
    offset = 60 + count*16 + 6
    header = bytearray(b'**TI89**\x01\x00' + folder + b'FFT chapter packages'.ljust(40, b'\0'))
    header += struct.pack('<H', count)
    if not single:
        header += struct.pack('<I8sBBH', offset, folder, 0x1f, 0, len(variables))
    blocks = bytearray()
    for v in variables:
        if len(v.payload) > 65520:
            raise ValueError(f'{v.name}: exceeds the AMS variable envelope')
        header += struct.pack('<I8sBBH', offset+len(blocks), v.name.encode(), v.kind, v.attr, 0)
        blocks += v.block
    return bytes(header + struct.pack('<I', offset+len(blocks)) + b'\xa5\x5a' + blocks)


def manifest(name, data, folder):
    data = bytes(data) + struct.pack('>H', check(data))
    return Variable(name, 0x1c, 3, data + b'\0dat\0\xf8', folder)


def dependencies(variables):
    return b''.join(struct.pack('>8sHH', v.name.encode(), len(v.payload), check(v.payload)) for v in variables)


def number(value, low, high, name):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f'{name}: expected {low}..{high}')
    return value


def select(source, names):
    if not names or len(names) > 255 or len(set(names)) != len(names):
        raise ValueError('Variable names must be unique, count 1..255')
    result = []
    for name in names:
        if not NAME.fullmatch(name) or name in RESERVED:
            raise ValueError(f'{name}: reserved/invalid package member')
        if name not in source:
            raise ValueError(f'{name}: missing input variable')
        v = source[name]
        if v.folder != b'main\0\0\0\0':
            raise ValueError(f'{name}: install packages in main')
        if v.attr != (0 if name == 'fftstory' else 3):
            raise ValueError(f'{name}: expected archived member (supervisor stays in RAM)')
        result.append(v)
    return result


def core_package(source, spec):
    if spec['schema'] != 'FFT_CORE_SPEC_1':
        raise ValueError('Unsupported core specification')
    variables = select(source, spec['variables'])
    envelope = spec['envelope']
    expected = {}
    for key, name in [('units', 'FFT_CORE_UNIT_MAX'), ('tiles', 'FFT_CORE_TILE_MAX'),
                      ('axis', 'FFT_CORE_AXIS_MAX'), ('layers', 'FFT_CORE_LAYER_MAX')]:
        expected[key] = int(re.search(r'#define\s+'+name+r'\s+(\d+)', (HERE/'core_limits.h').read_text())[1])
    expected['scene_rows'] = int(re.search(r'#define SC_HMAX (\d+)', (HERE/'chapters.h').read_text())[1])
    if envelope != expected:
        raise ValueError('Core envelope differs from the compiled headers')
    key = hashlib.sha256(json.dumps(spec, sort_keys=True).encode() + b''.join(
        v.name.encode()+bytes((v.kind, v.attr))+v.payload for v in variables)).digest()[:16]
    end = CORE_HEADER+12*len(variables)+2
    meta = manifest('ffcore', b'FKR1'+struct.pack('>HH', end, len(variables))+key+dependencies(variables), variables[0].folder)
    return variables+[meta], key


def chapter_package(source, spec, core_spec, key):
    if spec['schema'] != 'FFT_CHAPTER_SPEC_1':
        raise ValueError('Unsupported chapter specification')
    variables = select(source, spec['variables'])
    if set(spec['variables']) & set(core_spec['variables']):
        raise ValueError('Chapter duplicates shared core variables')
    if any(v.kind != 0x1c or v.name.endswith('core') for v in variables):
        raise ValueError('Chapters contain data only')
    if not {'ffca', 'ffcb'} <= set(spec['variables']):
        raise ValueError('Chapter needs both actor-frame banks')
    if spec.get('world_map') and 'ffwm' not in spec['variables']:
        raise ValueError('World-map continuation requires ffwm')
    pack_id = number(spec['id'], 0, 65534, 'id')
    routes = bytearray()
    bands, prefixes = set(), set()
    if not 1 <= len(spec['routes']) <= 254 or len(spec['steps']) > 255:
        raise ValueError('Invalid route/step count')
    for route in spec['routes']:
        band = number(route['band'], 1, 254, 'route band')
        prefix = route['prefix']
        if not re.fullmatch('[a-z0-9]', prefix) or prefix == 'c' or band in bands or prefix in prefixes:
            raise ValueError('Duplicate/invalid route')
        bands.add(band); prefixes.add(prefix)
        routes += bytes((band, ord(prefix), 0, 0))
        for suffix in ('v0', 'v1', 'v2', 'v3', 'a', 'i', 'd'):
            if 'ff'+prefix+suffix in core_spec['variables']:
                raise ValueError('Encounter names collide with shared core variables')
            if 'ff'+prefix+suffix not in spec['variables']:
                raise ValueError(f'Route {band}: missing ff{prefix}{suffix}')
        desc = resources.resolve(source,'ff'+prefix+'d',set(spec['variables'])|set(core_spec['variables']))
        if len(desc) < 104 or desc[:4] != b'FCD4':
            raise ValueError('Unsupported encounter descriptor')
        words = struct.unpack_from('>52H', desc)
        if words[42] and 'ff'+prefix+'st' not in spec['variables']:
            raise ValueError('Scripted encounter requires its story bank')
        e = core_spec['envelope']
        if not (1 <= words[50] <= e['units'] and 1 <= words[9] <= e['axis'] and
                1 <= words[10] <= e['axis'] and words[9]*words[10] <= e['tiles'] and
                1 <= words[13] <= e['scene_rows']):
            raise ValueError('Encounter exceeds the core envelope; regenerate/adapt content')
    steps = bytearray()
    for step in spec['steps']:
        op = PROGRAMS[step['program']]
        if op == 3 and 'ffflow' not in spec['variables']:
            raise ValueError('Deployment requires chapter flow data')
        arg = number(step['argument'], 0, 25499, 'step argument')
        if (op == 1 and (arg > 9 or 'ffs'+str(arg) not in spec['variables'])) or (op != 1 and arg//100 not in bands):
            raise ValueError('Step refers to an unavailable scene/encounter')
        steps += struct.pack('>BBHHBB', op, 0, arg,
                             number(step.get('skip_flag', 0), 0, 256, 'skip flag'),
                             number(step.get('skip_flow', 0), 0, 255, 'skip flow'), 0)
    d = dependencies(variables)
    allowed=set(spec['variables'])|set(core_spec['variables'])
    for v in variables:
        raw=resources.resolve(source,v.name,allowed)
        if raw[:4]==b'FSC2':
            pool=raw[12:20].rstrip(b'\0').decode('ascii')
            if pool not in allowed or resources.resolve(source,pool,allowed)[:4]!=b'FSP1':
                raise ValueError('Scene refers to an unavailable resource pool')
    step_at, dep_at = HEADER+len(routes), HEADER+len(routes)+len(steps)
    data = b'FKP1'+struct.pack('>HH', dep_at+len(d)+2, pack_id)+key
    data += struct.pack('>12H', number(spec.get('requires_flag', 0), 0, 256, 'prerequisite'),
                        number(spec.get('completion_flag', 0), 0, 256, 'completion'),
                        number(spec.get('next', 65535), 0, 65535, 'next'), len(bands), len(spec['steps']), len(variables),
                        HEADER, step_at, dep_at, int(spec.get('world_map', False)),
                        number(spec.get('previous', 65535), 0, 65535, 'previous'), 0)
    meta = manifest('ffpack', data+routes+steps+d, variables[0].folder)
    return variables+[meta]


def details(variables, filename):
    data = group_bytes(variables)
    return dict(file=filename, bytes=len(data), sha256=digest(data),
                stored_variable_bytes=sum(len(v.payload)+2 for v in variables),
                variables=[dict(name=v.name, type=v.kind, attr=v.attr, bytes=len(v.payload), sha256=digest(v.payload)) for v in variables])


def reuse_core(source, core_path, spec):
    installed = read_group(core_path)
    if set(installed) != set(spec['variables']) | {'ffcore'}:
        raise ValueError('Installed core membership differs from its specification')
    content = {n:v for n,v in installed.items() if n != 'ffcore'}
    rebuilt, _ = core_package(content, spec)
    if rebuilt[-1] != installed['ffcore']:
        raise ValueError('Installed core metadata/content mismatch')
    for name in set(source) & set(content):
        if source[name] != content[name]:
            raise ValueError(f'{name}: chapter input attempts to replace installed core')
    merged={**content, **source}
    normalized=resources.shared_contours(merged)
    if any(normalized[n]!=v for n,v in content.items()):
        raise ValueError('Installed core predates shared resources; rebuild the core once')
    return merged


def chapter_files(directories, specs):
    source = {}
    pending=sorted({n for s in specs for n in s['variables']})
    while pending:
        name=pending.pop(0)
        if name in source:continue
        if not NAME.fullmatch(name) or name in RESERVED:
            raise ValueError('Invalid chapter variable')
        for folder in directories:
            path = folder/(name+'.89y')
            if path.exists():
                entries = read_group(path)
                if set(entries) != {name}:
                    raise ValueError(f'{path}: file and internal variable name disagree')
                if name in source and source[name] != entries[name]:
                    raise ValueError(f'{name}: conflicting data directories')
                source[name] = entries[name]
        if name not in source:
            raise ValueError(f'{name}: missing built chapter data')
        raw=source[name].raw
        if raw[:4]==b'FRF1' and len(raw)==16:
            pending.append(raw[4:12].rstrip(b'\0').decode('ascii'))
        elif raw[:4]==b'FSC2':
            pending.append(raw[12:20].rstrip(b'\0').decode('ascii'))
    return source


def build(source, core_spec, chapters, out, budget=2700000, reserve=128*1024, allow_unused=False):
    original=source
    source=resources.shared_contours(source)
    core, key = core_package(source, core_spec)
    packs = []
    ids = set()
    used = set(core_spec['variables'])
    for spec in chapters:
        if spec['id'] in ids:
            raise ValueError('Duplicate chapter ID')
        ids.add(spec['id']); used.update(spec['variables'])
        prepared,names=resources.prepare_chapter(source,spec,core_spec['variables'])
        # Preserve authored ordering; generated scene pools belong to the pack.
        optimized={**spec,'variables':[n for n in spec['variables'] if n in names]+
                   sorted(names-set(spec['variables']))}
        used.update(names)
        packs.append((optimized, chapter_package(prepared, optimized, core_spec, key)))
    used.update(n for n in source if re.fullmatch('ffsp[0-9]?',n))
    if not packs:
        raise ValueError('At least one chapter specification is required')
    if not allow_unused and set(source)-used:
        raise ValueError('Unassigned input variables: '+', '.join(sorted(set(source)-used)))
    if set(source) & RESERVED:
        raise ValueError('Input group contains save/configuration or package metadata')
    report = dict(schema='FFT_CHAPTER_PACKAGES_1', core_key=key.hex(),
                  planning_budget_bytes=budget, reserved_bytes=reserve,
                  caveat='Serialized bytes plus size words; AMS VAT/archive headers, Flash fragmentation and other installed programs are not measured. RAM is a separate constraint.',
                  envelope=core_spec['envelope'], core=details(core, 'fft-core.89g'), chapters=[])
    for spec, variables in packs:
        row = details(variables, f"chapter-{spec['id']:02d}.89g")
        row.update(id=spec['id'], title=spec['title'], complete=spec.get('complete', False))
        row['installed_with_core_bytes'] = row['stored_variable_bytes']+report['core']['stored_variable_bytes']
        row['budget_headroom_after_reserve'] = budget-reserve-row['installed_with_core_bytes']
        if row['budget_headroom_after_reserve'] < 0:
            raise ValueError(f"Chapter {spec['id']} exceeds the selected installation budget")
        report['chapters'].append(row)
    resource_report=dict(core=resources.audit(core,original),chapters=[
        dict(id=spec['id'],**resources.audit(variables,original)) for spec,variables in packs])
    for _,variables in packs:resources.audit(core+variables,original)
    out = Path(out); out.mkdir(parents=True, exist_ok=True)
    (out/'fft-core.89g').write_bytes(group_bytes(core))
    (out/'core-variables.txt').write_text('\n'.join(v.name for v in core)+'\n')
    for (spec, variables), row in zip(packs, report['chapters']):
        (out/row['file']).write_bytes(group_bytes(variables))
        (out/f"chapter-{spec['id']:02d}-variables.txt").write_text('\n'.join(v.name for v in variables)+'\n')
    (out/'INSTALL.md').write_text(
        '# FFT core and chapter installation\n\n'
        'Install `fft-core.89g` once, then exactly one `chapter-NN.89g`, in `main`. '
        'Run `fftstory()`. Keep the archive attributes in the groups; only the small supervisor stays in RAM.\n\n'
        'To change chapters, exit to HOME. Keep the core and all saves. Remove only the '
        'variables in the previous `chapter-NN-variables.txt`, then send the next chapter group. '
        'This replaces `ffpack` and any reused content-bank names. Run `fftstory()` and Continue.\n\n'
        '**Always preserve** `ffsave`, `ffsave1`, `ffsave2`, `ffsave3`, `ffslot` and `ffcfg`. '
        'They belong to neither installation group. A missing or mismatched pack stops before gameplay. '
        'Install the requested pack; no save conversion is required.\n\n'
        'Install a core and chapters with the same core key from `packages.json`. '
        '`source.89g` is a build intermediate, not an installation group. '
        'The supplied Chapter I pack is the existing Orbonne/Academy/Gariland pilot, not a complete chapter.\n\n'
        'Storage figures are planning estimates with a reserve, not a measurement of free Flash or RAM.\n')
    (out/'packages.json').write_text(json.dumps(report, indent=2)+'\n')
    (out/'resources.json').write_text(json.dumps(resource_report,indent=2)+'\n')
    return report, core, packs


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input', type=Path, help='fresh source group')
    ap.add_argument('--reuse-core', type=Path, help='existing fft-core.89g; no compiler is invoked')
    ap.add_argument('--data-dir', type=Path, action='append', help='with --reuse-core, read only chapter members from built .89y files')
    ap.add_argument('--core', type=Path, default=HERE/'campaign/packages/core.json')
    ap.add_argument('--chapter', type=Path, action='append', help='repeat for each authored chapter')
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/packages')
    ap.add_argument('--combined', type=Path, help='optional legacy all-in-one output for the first chapter')
    ap.add_argument('--budget', type=int, default=2700000)
    ap.add_argument('--reserve', type=int, default=128*1024)
    args = ap.parse_args()
    if args.reserve < 0 or args.budget <= args.reserve:
        ap.error('Budget must exceed the nonnegative reserve')
    specs = [json.loads(p.read_text()) for p in (args.chapter or [HERE/'campaign/packages/chapter-01.json'])]
    try:
        if bool(args.input) == bool(args.data_dir) or (args.data_dir and not args.reuse_core):
            raise ValueError('Use --input, or --reuse-core with --data-dir')
        core_spec = json.loads(args.core.read_text())
        source = read_group(args.input) if args.input else chapter_files(args.data_dir, specs)
        if args.reuse_core:
            source = reuse_core(source, args.reuse_core, core_spec)
        report, core, packs = build(source, core_spec, specs,
                                    args.out, args.budget, args.reserve)
        if args.combined:
            args.combined.parent.mkdir(parents=True, exist_ok=True)
            args.combined.write_bytes(group_bytes(core+packs[0][1]))
    except (ValueError, KeyError) as error:
        ap.error(str(error))
    print(f"Core: {report['core']['bytes']} transport bytes")
    for pack in report['chapters']:
        print(f"Chapter {pack['id']}: {pack['bytes']} transport bytes; core+chapter {pack['installed_with_core_bytes']} stored bytes; {pack['budget_headroom_after_reserve']} planning bytes after reserve")
    print('Packages:', args.out)


if __name__ == '__main__':
    main()
