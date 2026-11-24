#!/usr/bin/env python3
"""Measure lossless human pose parts and costume increments from the local disc.

This is an archive/pose-preparation study, not a shipping art generator. Every
part recipe reconstructs the input's complete four planes byte for byte. Fixed
row bands are NOT an anatomical head-swap rig. No approved bank is rewritten.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct

import numpy as np

import core_audit
import extract
import management_research
import opening_assets
import units

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
NATIVE = list(dict.fromkeys(units.FRAMES + [86, 87, 89, 90, 135, 136, 138, 139]))
NORMAL_CHOICES = {'Ramza': 1, 'Delita': 4, 'Zalbag': 8, 'Gafgarion': 23,
                  'Malak': 26, 'Alma': 48, 'Mustadio': 34, 'Rafa': 25,
                  'Agrias': 52, 'Wiegraf': 32, 'Meliadoul': 47,
                  'Celia': 45, 'Lede': 46}
STORY_COSTUMES = (2, 3, 5, 6, 17, 40)
RECRUITS = {'Ramza', 'Agrias', 'Mustadio', 'Rafa', 'Malak', 'Orlandu',
            'Meliadoul', 'Reis', 'Beowulf', 'Cloud'}
IMPORTANT = RECRUITS | {'Delita', 'Alma', 'Ovelia', 'Gafgarion', 'Wiegraf',
                        'Dycedarg', 'Zalbag', 'Elmdor'}
SCENE_ONLY = {'Ovelia', 'Larg', 'Goltana', 'Funeral', 'Simon', 'Teta',
              'Barinten', 'Balmafula', 'Rudvich', 'Draclau', 'Ajora'}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bands(raw, cuts):
    p = np.frombuffer(raw, np.uint8).reshape(4, 26, 2)
    start = 0
    out = []
    for end in cuts:
        out.append(p[:, start:end].tobytes())
        start = end
    return out


def join(parts, cuts):
    out = np.empty((4, 26, 2), np.uint8)
    start = 0
    for raw, end in zip(parts, cuts):
        out[:, start:end] = np.frombuffer(raw, np.uint8).reshape(4, end-start, 2)
        start = end
    return out.tobytes()


class Packer:
    def __init__(self, out):
        self.folder = out / 'streams'
        self.folder.mkdir(exist_ok=True)
        self.checked = set()
        self.calls = 0

    def __call__(self, raw):
        key = sha(raw)
        path = self.folder / (key + '.zx0')
        if not path.exists():
            # Production optimal ZX0, with the library's independent dzx0 check.
            path.write_bytes(extract.zx0pack.pack(raw))
            self.checked.add(key)
            self.calls += 1
        elif key not in self.checked:
            import subprocess
            import tempfile
            with tempfile.TemporaryDirectory() as folder:
                dest = Path(folder) / 'raw'
                subprocess.run([ROOT/'tools/bin/dzx0', '-f', path, dest],
                               check=True, capture_output=True)
                assert dest.read_bytes() == raw
            self.checked.add(key)
        return path.read_bytes()


def shards(streams, record_header=0):
    """Current u16 offset table,65000-byte bound; DAT trailer+size word included."""
    sizes = [2]
    for packed in streams:
        size = 2 + record_header + len(packed)
        if sizes[-1] + size > 65000:
            sizes.append(2)
        sizes[-1] += size
    return dict(banks=len(sizes), bank_bytes=[n+8 for n in sizes],
                stored_bytes=sum(n+8 for n in sizes))


def table_bytes(length):
    # Hypothetical recipe/mapping banks, with a16-byte cold-format header and
    # the same DAT+size word; no Flash-sector/VAT fragmentation assumption.
    count = max(1, (length+65495)//65496)
    return length + 24*count


def measure(rows, cuts, pack):
    frames = {r for _, _, r in rows}
    mapping = table_bytes(4*len(rows))  # source pose plus its horizontal mirror
    physical = {piece for raw in frames for piece in bands(raw, cuts)}
    # Per-part header: rows,flags; each recipe stores2/3 u16 physical IDs.
    streams = shards([pack(r) for r in sorted(physical)],
                     record_header=0 if len(cuts) == 1 else 2)
    recipes = 0 if len(cuts) == 1 else table_bytes(2*len(cuts)*len(frames))
    checks = 0
    for raw in frames:
        assert join(bands(raw, cuts), cuts) == raw
        checks += 1
    return dict(logical_source_poses=len(rows), complete_frames=len(frames),
                physical_records=len(physical), stream_banks=streams,
                mapping_bytes=mapping, recipe_bytes=recipes,
                stored_bytes=streams['stored_bytes']+mapping+recipes,
                exact_reassemblies=checks, cuts=cuts)


def main():
    ap = argparse.ArgumentParser(__doc__)
    ap.add_argument('--disc', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/actor-storage')
    args = ap.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    (out/'frames').mkdir(exist_ok=True)
    disc, files = extract.disc_files(str(args.disc))
    bat = disc.read(*files['BATTLE.BIN'])
    small = disc.read(*files['EVENT/SMALL.OUT'])
    at = 0xec + struct.unpack_from('<I', small, 0x70)[0]
    names = ['\n'.join(management_research.decode(x)[0]) for x in
             small[at:].replace(b'\xff', b'\xfe').split(b'\xfe')[:256]]
    source_hashes = {'BATTLE.BIN': sha(bat), 'EVENT/SMALL.OUT': sha(small)}
    shapes, shape_bytes, boundaries = {}, {}, {}
    for typ in ('TYPE1', 'TYPE2'):
        data = disc.read(*files[f'BATTLE/{typ}.SHP'])
        shape_bytes[typ] = data
        shapes[typ] = units.shp_frames(data)
        boundaries[typ] = struct.unpack_from('<HH', data, 4)
        source_hashes[f'BATTLE/{typ}.SHP'] = sha(data)
    code_hash = sha((Path(units.__file__).read_bytes() +
                     Path(opening_assets.__file__).read_bytes() +
                     Path(__file__).read_bytes()))
    model, frames = {}, {}
    for index, identity in enumerate(list(range(1, 53)) + list(range(96, 134))):
        shp = bat[0x80094748-0x80067000+4*identity]
        if shp not in (0, 1):
            raise ValueError(f'body{identity}: unsupported shape{shp}')
        typ = ('TYPE1', 'TYPE2')[shp]
        lba, size = struct.unpack_from('<II', bat, 0x80094cd4-0x80067000+8*identity)
        path = next(k for k, v in files.items() if v[0] == lba)
        raw = disc.read(*files[path])
        assert (len(raw)+2047)//2048*2048 == size
        source_hashes[path] = sha(raw)
        # Named extended sample covers every regular-body shape, plus the
        # existing native proxy. Attack/SP2 pages are not a full-animation rip.
        wanted = sorted(set(NATIVE) | (set(range(boundaries[typ][0])) if identity < 96 else set()))
        key = sha(raw + shape_bytes[typ] + code_hash.encode() + bytes(wanted))
        cache = out/'frames'/f'{identity:03}-{key}.npz'
        meta_path = cache.with_suffix('.json')
        if cache.exists() and meta_path.exists():
            meta = json.loads(meta_path.read_text())
            with np.load(cache, allow_pickle=False) as data:
                records = {int(k): bytes(v) for k, v in zip(data['ids'], data['poses'])}
        else:
            records, failures, empty = {}, [], []
            for frame in wanted:
                try:
                    sprite = units.sprite(raw, shapes[typ][frame], units.LIFT.get(frame, 0))
                    if not np.any(sprite >= 0):
                        empty.append(frame)
                        continue
                    pose = opening_assets.packed_frame(sprite)
                    assert len(pose) == 208
                    records[frame] = min(pose, opening_assets.mirror(pose))
                except (ValueError, IndexError, AssertionError) as e:
                    failures.append([frame, str(e)])
            meta = dict(identity=identity, name=names[identity] if identity < 96 else path,
                        path=path, shape=typ, source_sha256=sha(raw), wanted=wanted,
                        normal_body_end=boundaries[typ][0], failures=failures, empty=empty,
                        converted=len(records), cache_key=key)
            np.savez_compressed(cache, ids=np.array(sorted(records), np.uint16),
                                poses=np.array([list(records[k]) for k in sorted(records)], np.uint8))
            meta_path.write_text(json.dumps(meta, indent=2)+'\n')
        model[identity] = meta
        frames[identity] = records
        if index % 8 == 0:
            print(f'converted {index+1}/90 models; current{identity} {path}', flush=True)
    canonical = {}
    for identity in range(1, 53):
        canonical.setdefault(names[identity], identity)
    canonical.update(NORMAL_CHOICES)
    assert len(canonical) == 38
    canon_ids = set(canonical.values())
    jobs = set(range(96, 134))
    case_ids = {
        'canonical_named': canon_ids,
        'story_costumes_named': canon_ids | set(STORY_COSTUMES),
        'all_human_identity_variants': set(range(1, 53)),
        'canonical_plus_jobs': canon_ids | jobs,
        'story_costumes_plus_jobs': canon_ids | set(STORY_COSTUMES) | jobs,
        'all_variants_plus_jobs': set(range(1, 53)) | jobs,
    }
    profiles = ('native30', 'extended_regular')
    pack = Packer(out)
    report = dict(schema='FFT_ACTOR_STORAGE_STUDY_1', date='2026-11-24',
                  source_hashes=source_hashes, code_hash=code_hash,
                  canonical_ids=canonical, important_unique=sorted(IMPORTANT),
                  scene_only_candidates=sorted(SCENE_ONLY), models=list(model.values()),
                  cases={}, threshold_bytes=50000, threshold_scope='whole-campaign incremental stored bytes')
    all_case_rows = {}
    for profile in profiles:
        report['cases'][profile] = {}
        for label, ids in case_ids.items():
            rows = [(i, f, raw) for i in sorted(ids) for f, raw in frames[i].items()
                    if profile != 'native30' or f in NATIVE]
            # Generic jobs use the native30 control in the extended named study.
            all_case_rows[(profile, label)] = rows
            schemes = {}
            for scheme, cuts in [('whole', [26]), ('two', [8, 26]), ('three', [8, 18, 26])]:
                schemes[scheme] = measure(rows, cuts, pack)
                print(profile, label, scheme, schemes[scheme]['stored_bytes'], flush=True)
            report['cases'][profile][label] = schemes
        (out/'progress.json').write_text(json.dumps(report, indent=2)+'\n')

    # Same input/codec/metadata in every subtraction. The extra source IDs are
    # distinct from purely palette-only swaps, which remain excluded.
    report['increments'] = {}
    for profile in profiles:
        c = report['cases'][profile]
        report['increments'][profile] = {
            scheme: dict(
                six_story_outfits=c['story_costumes_plus_jobs'][scheme]['stored_bytes']-c['canonical_plus_jobs'][scheme]['stored_bytes'],
                all_fourteen_same_name_variants=c['all_variants_plus_jobs'][scheme]['stored_bytes']-c['canonical_plus_jobs'][scheme]['stored_bytes'],
                story_outfits_under_50k=(c['story_costumes_plus_jobs'][scheme]['stored_bytes']-c['canonical_plus_jobs'][scheme]['stored_bytes']) < 50000,
                all_variants_under_50k=(c['all_variants_plus_jobs'][scheme]['stored_bytes']-c['canonical_plus_jobs'][scheme]['stored_bytes']) < 50000)
            for scheme in ('whole', 'two', 'three')}

    # Scene-only pose scopes are separate probes, not a claim that10/16 poses
    # suffice for every future scene. Compare exact same actors' full proxy.
    scene_ids = {canonical[n] for n in SCENE_ONLY}
    report['scene_pose_scopes'] = {}
    for label, wanted in [('standing_walking10', set(range(9, 19))),
                          ('with_hurt_kneel_ko16', set(range(9, 25))),
                          ('native30', set(NATIVE)), ('extended_regular', None)]:
        rows = [(i, f, raw) for i in sorted(scene_ids) for f, raw in frames[i].items()
                if wanted is None or f in wanted]
        report['scene_pose_scopes'][label] = measure(rows, [26], pack)

    # Potential archive-only head substitution: important named actors stay whole;
    # secondary heads are added to an already required generic job body catalogue.
    # No altered character image is generated or benchmarked by this count.
    report['head_substitution_count_only'] = {}
    for profile in profiles:
        rows = all_case_rows[(profile, 'canonical_plus_jobs')]
        kept = [(i, f, raw) for i, f, raw in rows if i in jobs or names[i] in IMPORTANT]
        secondary = [(i, f, raw) for i, f, raw in rows if i not in jobs and names[i] not in IMPORTANT]
        head_raw = {bands(raw, [8, 26])[0] for _, _, raw in secondary}
        hbank = shards([pack(r) for r in sorted(head_raw)], record_header=2)
        kept_cost = measure(kept, [26], pack)['stored_bytes']
        # Every secondary pose receives an explicit head/body recipe and logical
        # mapping, even if some could also be pooled. No free lookup descriptors.
        recipe = table_bytes(4*len(secondary)) + table_bytes(4*len(secondary))
        total = kept_cost + hbank['stored_bytes'] + recipe
        whole = report['cases'][profile]['canonical_plus_jobs']['whole']['stored_bytes']
        report['head_substitution_count_only'][profile] = dict(
            kept_full_bytes=kept_cost, secondary_poses=len(secondary), unique_heads=len(head_raw),
            head_bank_bytes=hbank['stored_bytes'], recipe_mapping_bytes=recipe,
            hypothetical_total=total, saving_vs_full=whole-total,
            note='Count only; generic lower bodies already budgeted. Fixed top8 rows are not anatomical heads. '
                 'No compatible rig/anchors/outline seams, selected job bodies or approved new appearances. '
                 'Do not credit this as a shipping saving or infer render quality/CPU from it.')

    # Prepare a bounded pixel-identical native/PC reconstruction fixture set.
    fixtures = []
    for identity in (1, 4, 48, 52, 23, 13, 47, 12, 19, 10, 11, 96, 100, 101, 108):
        for frame in (9, 14, 23):
            if frame in frames[identity]:
                fixtures.append((identity, frame, frames[identity][frame]))
    blobs, arrays = {}, []
    def c_blob(raw):
        key = sha(raw)
        if key not in blobs:
            name = 'blob'+str(len(blobs))
            blobs[key] = name
            arrays.append(extract.zx0pack.c_array(name+'_raw', raw))
            arrays.append(extract.zx0pack.c_array(name+'_zx', pack(raw)))
        return blobs[key]
    records = []
    for identity, frame, raw in fixtures:
        names_ = [c_blob(raw)]
        for cuts in ([8, 26], [8, 18, 26]):
            names_.extend(c_blob(r) for r in bands(raw, cuts))
        values = ','.join('{'+n+'_zx,'+n+'_raw}' for n in names_)
        records.append('    {'+str(identity)+','+str(frame)+', {'+values+'}},')
    header = '// Private disc-derived sizing fixtures; never commit.\n'+ '\n'.join(arrays)
    header += '\nstatic const StudyFixture study_fixtures[] = {\n'+'\n'.join(records)+'\n};\n'
    header += '#define STUDY_FIXTURES '+str(len(fixtures))+'\n'
    (out/'fixtures.h').write_text(header)
    report['native_fixture_count'] = len(fixtures)
    report['compressor_roundtrips'] = len(pack.checked)
    report['caveats'] = [
        'Native30 reproduces the prior adapted pose scope. Extended named coverage includes regular-body SHP frames and that control; generic jobs remain native30.',
        'Frames beyond SHP normal_body_end require attack/SP2 source decoding for full fidelity; this study keeps existing converter output as a sizing proxy.',
        'Empty/unfitted shapes are explicit, not repaired or certified source animations. Final scene choreography has not been ripped.',
        'Two/three parts are fixed row bands with all four planes retained; reassembly is exact. They are not anatomical heads/feet for arbitrary poses.',
        'Whole-frame mirroring is pooled; independent part mirrors are not introduced, keeping native reconstruction simple.',
        'Counts include logical mirror maps, compressed streams, u16 offsets, part headers, recipe tables, bounded bank headers and DAT/size words. Flash fragmentation is excluded.',
        'Six story outfit candidates are Ramza2/3,Delita2/3,executioner Gafgarion and later Wiegraf. All fourteen same-name variants also include wounded/guest/undead variants.',
        'No selected body/palette changes, campaign ABI changes, save writes, installed group rebuild or live emulator operations.',
    ]
    (out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    print('INCREMENTS', json.dumps(report['increments']), flush=True)
    print('SCENE SCOPES', json.dumps({k:v['stored_bytes'] for k,v in report['scene_pose_scopes'].items()}), flush=True)
    print('HEAD COUNT ONLY', json.dumps(report['head_substitution_count_only']), flush=True)
    print('DONE', len(pack.checked), 'stream roundtrips;', len(fixtures), 'native fixtures', flush=True)


if __name__ == '__main__':
    main()
