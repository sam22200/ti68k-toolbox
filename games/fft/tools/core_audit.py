#!/usr/bin/env python3
"""Read-only retail-US census and deterministic production-risk report.

No map rendering, ROM modification, live save writes or TiEmu input. Resource
counts are not encounter counts; populated ENTD records are not active actors.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

import battle
import extract

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
DEFAULT_DISC = ROOT / 'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
SCENARIO_START = 0x801CF938 - 0x801BF000
MESH_TYPES = (0x2E01, 0x2F01, 0x3001)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def terrain(mesh, context):
    if len(mesh) < 0x6C:
        raise ValueError(f'{context}: mesh header truncated')
    offset = struct.unpack_from('<I', mesh, 0x68)[0]
    if offset == 0:
        return None
    # Retail copies 4096 bytes after dimensions, with a FIXED 256-tile layer
    # stride, not width*height. Ignore padded cells when finding real extrema.
    if offset < 0x6C or offset + 4098 > len(mesh):
        raise ValueError(f'{context}: invalid terrain span {offset}+4098/{len(mesh)}')
    width, height = mesh[offset:offset + 2]
    count = width * height
    if not width or not height or count > 256:
        raise ValueError(f'{context}: invalid dimensions {width}x{height}')
    layers = []
    for level in range(2):
        cells = [mesh[offset + 2 + 8 * (level * 256 + i):
                      offset + 10 + 8 * (level * 256 + i)] for i in range(count)]
        # These are raw flag classifications, not a movement oracle. Fly,
        # Float, ceilings and special surfaces still affect reachability.
        traversable = [c for c in cells if not c[6] & 1]
        stoppable = [c for c in cells if not c[6] & 3]
        corners = [v for c in traversable for v in
                   extract.corners(c[2], c[3] & 31, c[4] if c[3] & 31 else 0)]
        layers.append(dict(level=level, unblocked=len(traversable),
                           not_blocked_or_untargetable=len(stoppable),
                           max_base_height=max((c[2] for c in traversable), default=0),
                           max_corner_height=max(corners, default=0),
                           max_depth=max((c[3] >> 5 for c in traversable), default=0),
                           surfaces=sorted(set(c[0] & 63 for c in traversable))))
    return dict(width=width, height=height, cells_per_layer=count,
                terrain_offset=offset, terrain_sha256=sha(mesh[offset:offset + 4098]),
                layers=layers)


def maps(disc, files):
    rows, gns_hashes = [], {}
    for name in sorted(files):
        if not re.fullmatch(r'MAP/MAP\d{3}\.GNS', name):
            continue
        raw = disc.read(*files[name]); gns_hashes[name] = sha(raw)
        ended = False
        for offset in range(0, len(raw) - 19, 20):
            kind = struct.unpack_from('<H', raw, offset + 4)[0]
            if kind == 0x3101:
                ended = True
                break
            if kind not in MESH_TYPES:
                continue
            lba, size = struct.unpack_from('<II', raw, offset + 8)
            mesh = disc.read(lba, size)
            data = terrain(mesh, f'{name} record{offset // 20}')
            if data is None:
                continue
            rows.append(dict(map=int(name[7:10]), gns=name, record=offset // 20,
                             type=f'{kind:04X}', lba=lba, resource_bytes=size,
                             resource_sha256=sha(mesh), **data))
        if not ended:
            raise ValueError(f'{name}: no GNS terminator')
    return rows, gns_hashes


def scenarios(raw):
    rows = []
    for offset in range(SCENARIO_START, len(raw) - 23, 24):
        b = raw[offset:offset + 24]
        event = int.from_bytes(b[:2], 'little')
        if not event:
            return rows
        squads = struct.unpack_from('<HH', b, 9)
        rows.append(dict(event=event, map=b[2], entd=int.from_bytes(b[7:9], 'little'),
                         squads=list(squads), offset=offset, raw=b.hex()))
    raise ValueError('ATTACK: no scenario-table terminator')


def encounters(source):
    rows = []
    for encounter in range(512):
        units = []
        for slot in range(16):
            b = source.entd(encounter, slot)
            if len(b) != 40:
                raise ValueError(f'ENTD {encounter}/{slot}: short record')
            # Retail initializer tests sprite_set, not level/job/nonzero record.
            if b[0] == 0:
                continue
            units.append(dict(slot=slot, identity=b[0], job=b[10], level=b[3],
                              presence=b[24] & 0xC0, team_flags=b[24],
                              x=int.from_bytes(b[25:26], 'little', signed=True),
                              y=b[26], upper=b[27] >> 7, source_unit_id=b[32]))
        rows.append(dict(entd=encounter, populated=len(units), units=units))
    return rows


def catalogue(source):
    default = source.at(battle.ABILITY_RANGE, 368 * 14)
    statuses = source.at(0x80065DE4, 40 * 16)
    if len(default) != 368 * 14 or len(statuses) != 640:
        raise ValueError('SCUS: truncated action/status table')
    forms = Counter(default[14 * a + 8] for a in range(368))
    return dict(primary_ability_ids=512, default_action_records=368,
                generic_jobs=20, internal_job_records=160, item_ids=256,
                human_skillsets=176, human_entries=22, action_positions=16,
                rsm_positions=6, monster_skillsets=48, monster_entries=4,
                status_ids=40, status_bytes=5, timed_status_slots=16,
                formula_histogram={str(k): v for k, v in sorted(forms.items())},
                nonzero_ct_actions=[a for a in range(368) if default[14 * a + 12]],
                area_actions=[a for a in range(368) if default[14 * a + 1]],
                mp_actions=[a for a in range(368) if default[14 * a + 13]],
                default_table_sha256=sha(default), status_policy_sha256=sha(statuses),
                statuses=[dict(id=i, ct=statuses[16*i+3], flags=list(statuses[16*i+4:16*i+6]),
                               cancels=statuses[16*i+6:16*i+11].hex(),
                               cannot_stack=statuses[16*i+11:16*i+16].hex()) for i in range(40)])


def package(path):
    """Measure the group actually supplied, not possibly stale loose files."""
    raw = path.read_bytes()
    if raw[:8] != b'**TI89**' or len(raw) < 82:
        raise ValueError(f'{path}: invalid TI group')
    count = struct.unpack_from('<H', raw, 0x3A)[0]
    table_end = 0x3C + count * 16 + 6
    if table_end > len(raw) or struct.unpack_from('<I', raw, table_end-6)[0] != len(raw):
        raise ValueError(f'{path}: corrupt table/length')
    entries = []
    previous = table_end
    for i in range(count):
        offset, name, kind, attr, unused = struct.unpack_from('<I8sBBH', raw, 0x3C + 16*i)
        if kind == 0x1F:
            continue
        if offset != previous or offset + 8 > len(raw):
            raise ValueError(f'{path}: invalid variable offset')
        n = struct.unpack_from('>H', raw, offset + 4)[0]
        end = offset + n + 8
        if end > len(raw):
            raise ValueError(f'{path}: truncated variable')
        payload = raw[offset+6:offset+6+n]
        check = struct.unpack_from('<H', raw, end-2)[0]
        if sum(raw[offset+4:end-2]) & 65535 != check:
            raise ValueError(f'{path}: variable checksum mismatch')
        label = name.rstrip(b'\0').decode('ascii')
        if label.endswith('core') or kind == 0x21 or label in ('ffbm','ffbn'):
            category = 'code_and_embedded_constants'
        elif re.fullmatch(r'ff[go]v[0-3]', label):
            category = 'battle_views_and_occlusion'
        elif label in ('ffca', 'ffcb', 'ffga', 'ffoa'):
            category = 'actors_and_pose_indices'
        elif label in ('ffflow', 'ffs0', 'ffs1', 'ffs2'):
            category = 'scenes_flow_and_shared_font'
        elif label == 'ffwm':
            category = 'world_map'
        elif label in ('ffmg', 'ffcp', 'ffcu', 'ffcw', 'ffgi', 'ffoi', 'fftitle'):
            category = 'ui_portraits_weapons_title'
        else:
            category = 'catalogue_encounters_and_descriptors'
        entries.append(dict(name=label, type=kind, attr=attr, payload_bytes=n,
                            block_bytes=n+8, category=category, sha256=sha(payload)))
        # TTPack's little-endian input length is distinct from compressed
        # archive bytes. Checked with a known100-byte host packer fixture.
        if kind==0x1C and label.endswith('core') and len(payload)>=8 and payload[2:4]==b'TP':
            unpacked=struct.unpack_from('<H',payload)[0]
            entries[-1].update(ttpack_input_bytes=unpacked,ttpack_limit_bytes=64936,
                               ttpack_headroom_bytes=64936-unpacked)
        previous = end
    if previous != len(raw):
        raise ValueError(f'{path}: trailing data')
    totals = Counter()
    for e in entries:
        totals[e['category']] += e['payload_bytes']
    return dict(bytes=len(raw), sha256=sha(raw), variables=len(entries),
                payload_bytes=sum(e['payload_bytes'] for e in entries),
                transport_overhead=len(raw)-sum(e['payload_bytes'] for e in entries),
                categories=dict(sorted(totals.items())), entries=entries,
                caveat='Packed code includes constants. Shared font is inside flow; portraits/UI '
                       'and scene imagery need section instrumentation for a finer split. '
                       'Archive bytes are not peak RAM; no audio bank is currently packaged.')


def source_evidence():
    local = ROOT / 'sources/fft_decomp'
    paths = ['include/fft/unit.h', 'include/fft/data.h', 'include/fft/map.h',
             'include/fft/main.h', 'include/fft/thread.h',
             'src/battle/battle_unit_init_deployed_units_data.c',
             'src/battle/battle_unit_load_entd_units.c',
             'src/battle/battle_map_copy_xy_coords_and_tile_data.c',
             'src/battle/battle_map_get_tile_data_pointer.c',
             'src/battle/battle_effect_test_position_for_obstacle.c',
             'src/battle/battle_move_calculate_tile_ceiling.c',
             'src/battle/battle_move_update_candidate.c',
             'src/battle/battle_move_check_knockback_destination.c',
             'src/main/main_party_find_free_slot.c',
             'src/battle/battle_state_announce_next_charged_action.c',
             'src/battle/battle_reaction_prepare_next.c',
             'src/battle/battle_action_init_current_ability_strike_data.c',
             'src/battle/battle_action_finalize_attack_and_flag_reactions.c',
             'src/battle/battle_unit_add_unique_entd_spritesheet.c',
             'src/main/main_unit_calculate_actual_stats.c',
             'src/main/main_unit_check_level_up.c',
             'src/main/main_job_calculate_level.c',
             'src/main/main_job_add_proposition_jp.c',
             'src/battle/battle_unit_store_jp_and_calculate_unlocked_jobs.c',
             'src/battle/battle_action_increment_item_quantity_for_steal_break.c',
             'src/battle/battle_action_add_war_funds.c',
             'src/world/world_item_cash_out_excess_inventory.c']
    commit = subprocess.check_output(['git', '-C', local, 'rev-parse', 'HEAD'], text=True).strip()
    return dict(commit=commit, files={p: sha((local/p).read_bytes()) for p in paths},
                upstream='https://github.com/adamrt/fft_decomp/tree/'+commit,
                confidence='Static reading of local decompilation; this audit does not rebuild '
                           'or independently byte-match that decompilation.')


def original_slots(disc, state):
    import oracle
    p = oracle.PSX(str(disc)); p.load(str(state))
    rows = []
    for i in range(21):
        a = oracle.UNITS + i*oracle.UNIT_SIZE
        rows.append(dict(slot=i, entd_slot=p.read(a+1), team=p.read(a+5),
                         hp=p.read(a+0x28,2), max_hp=p.read(a+0x2A,2),
                         x=p.read(a+0x47), y=p.read(a+0x48)))
    present = [r['slot'] for r in rows if r['entd_slot'] != 255 and r['max_hp']]
    # Existing Gariland fixture demonstrates deployed slots16..20. It is NOT
    # evidence that every one of the21 simulation slots can fight concurrently.
    if present != [0,1,2,3,4,5,16,17,18,19,20]:
        raise ValueError(f'Expected known Gariland first-turn fixture; got {present}')
    return dict(state_sha256=sha(state.read_bytes()), present=present, slots=rows,
                provenance='oracle.py boot/deploy/first_turn; controlled fixture includes '
                           'documented scenario/deployment pokes, not an untouched playthrough')


def deployment_limits(disc, state, events):
    import oracle
    p=oracle.PSX(str(disc));p.load(str(state))
    if oracle.var(p,oracle.EVENT)!=9:
        raise ValueError('Deployment census needs original Gariland deployment overlay')
    pointer=p.read(0x801CD270,4)
    ids=sorted({s for e in events for s in e['squads'] if s})
    if pointer<0x80000000 or pointer+(max(ids)+1)*12>0x80200000:
        raise ValueError('Deployment overlay pointer invalid')
    squads={}
    for squad in ids:
        raw=bytes(p.read(pointer+squad*12+i) for i in range(12))
        squads[squad]=dict(id=squad,bitmap=int.from_bytes(raw[:4],'little'),
                           center=list(raw[4:6]),upper=raw[6],flags=raw[7],
                           capacity=raw[8],map=raw[9],raw=raw.hex())
    sums=[dict(event=e['event'],map=e['map'],squads=e['squads'],
               declared_sum=sum(squads[s]['capacity'] for s in e['squads']if s)) for e in events]
    return dict(state_sha256=sha(state.read_bytes()),pointer=f'{pointer:08X}',
                squads=list(squads.values()),scenario_capacities=sums,
                max_declared_sum=max(r['declared_sum'] for r in sums),
                over_five=[r for r in sums if r['declared_sum']>5],
                caveat='Raw table capacity sums, not verified simultaneous deployment. '
                       'Outlier event445 declares5+2 and needs script/runtime research; '
                       'normal deployed-unit initializer reads only five records.')


def build(disc_path, group, state=None, deployment_state=None):
    source = battle.Disc(str(disc_path)); disk, files = extract.disc_files(str(disc_path))
    geometry, gns_hashes = maps(disk, files)
    encounter_rows = encounters(source)
    attack = disk.read(*files['EVENT/ATTACK.OUT']); events = scenarios(attack)
    summary = dict(gns_files=len(gns_hashes), terrain_resources=len(geometry),
                   maps_with_terrain=len(set(r['map'] for r in geometry)),
                   max_axis=max(max(r['width'],r['height']) for r in geometry),
                   max_cells_per_layer=max(r['cells_per_layer'] for r in geometry),
                   resources_with_upper_unblocked=sum(bool(r['layers'][1]['unblocked']) for r in geometry),
                   max_unblocked_corner_height=max(l['max_corner_height'] for r in geometry for l in r['layers']),
                   encounter_records=len(encounter_rows),
                   max_populated_entd=max(r['populated'] for r in encounter_rows),
                   scenario_records=len(events),
                   two_squad_records=sum(all(r['squads']) for r in events))
    return dict(schema='FFT_CORE_AUDIT_1', scope='Retail US static census, optional existing '
                'controlled Gariland runtime snapshot; not full behavioral certification.',
                source=dict(exe_sha256=sha(source.exe), battle_sha256=sha(source.bat),
                            entd_sha256={str(k):sha(v) for k,v in source.ent.items()},
                            attack_sha256=sha(attack), gns_sha256=gns_hashes),
                decompilation=source_evidence(), summary=summary, maps=geometry,
                encounters=encounter_rows, scenarios=events, catalogue=catalogue(source),
                package=package(group),
                original_gariland=original_slots(disc_path,state) if state else None,
                deployment=deployment_limits(disc_path,deployment_state,events) if deployment_state else None,
                production_ready=False,
                port_envelope=dict(units=13,compact_tile_ids=256,tile_index_bits=8,
                                   count_bits=16,reserved_invalid_tile=None,
                                   policy='User-selected scope; see PORT_LIMITS.md and port_limits.py'),
                blockers=['conditional encounter activation/lifecycle not fully traced; descriptor-driven13-unit storage is implemented (CORE_CAPACITY.md)',
                          '13-actor/256-node tactical adapters and original-MIPS solid/ceiling/headroom queries pass; full source upper-floor paths, ballistic/player targeting and raster cover/extrema remain pending (CORE_SURFACES.md)',
                          'four-status approximation; absent cast/Jump/performance/reaction mechanisms',
                          'persistent guests/dual weapons/monsters/conditional campaign events incomplete',
                          'peak heap/fragmentation/stack and expanded-map timing not yet measured',
                          'validated versioned map/scene v1 and editors not yet implemented'])


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('disc', nargs='?', type=Path, default=DEFAULT_DISC)
    ap.add_argument('--group', type=Path, default=HERE/'campaign/x/fftcampaign.89g')
    ap.add_argument('--state', type=Path)
    ap.add_argument('--deployment-state', type=Path)
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/core-audit/source.json')
    ap.add_argument('--compare', type=Path, help='previous audit JSON; report package byte delta')
    args = ap.parse_args(); report = build(args.disc,args.group,args.state,args.deployment_state)
    if args.compare:
        old=json.loads(args.compare.read_text())['package']
        report['comparison']=dict(previous_sha256=old['sha256'],
                                  byte_delta=report['package']['bytes']-old['bytes'])
    args.out.parent.mkdir(parents=True,exist_ok=True)
    result=json.dumps(report,indent=2,sort_keys=True)+'\n'
    if not args.out.exists() or args.out.read_text()!=result:
        args.out.write_text(result)
    print(json.dumps(report['summary'],sort_keys=True))
    print('Group:', report['package']['bytes'],'bytes;',report['package']['variables'],'variables')
    print('Production gate: CLOSED (see blockers); report:',args.out)


if __name__ == '__main__':
    main()
