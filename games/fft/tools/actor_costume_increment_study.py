#!/usr/bin/env python3
"""Incremental wardrobe choices, reusing actor_storage_study's exact pose corpus."""
import argparse
import json
from pathlib import Path
import numpy as np

from actor_storage_study import HERE, NATIVE, IMPORTANT, SCENE_ONLY, Packer, measure


def main():
    ap = argparse.ArgumentParser(__doc__)
    ap.add_argument('--study', type=Path, default=HERE/'campaign/x/actor-storage')
    args = ap.parse_args()
    folder = args.study.resolve()
    study = json.loads((folder/'results.json').read_text())
    frames = {}
    for meta in study['models']:
        cache = folder/'frames'/f'{meta["identity"]:03}-{meta["cache_key"]}.npz'
        with np.load(cache, allow_pickle=False) as data:
            frames[meta['identity']] = {int(k): bytes(raw) for k, raw in zip(data['ids'], data['poses'])}
    canonical = set(study['canonical_ids'].values())
    jobs = set(range(96, 134))
    selections = {
        'ramza_only': {2, 3}, 'delita_only': {5, 6},
        'ramza_delita': {2, 3, 5, 6}, 'gafgarion_only': {17},
        'wiegraf_only': {40},
    }
    pack = Packer(folder)
    report = dict(schema='FFT_COSTUME_INCREMENT_CHOICES_1', retained=False,
                  source_code_hash=study['code_hash'], profiles={})
    for profile in ('native30', 'extended_regular'):
        report['profiles'][profile] = {}
        for name, extra in selections.items():
            rows = [(i, f, raw) for i in sorted(canonical | jobs | extra)
                    for f, raw in frames[i].items() if profile != 'native30' or f in NATIVE]
            choices = {}
            for scheme, cuts in [('whole', [26]), ('two', [8, 26]), ('three', [8, 18, 26])]:
                result = measure(rows, cuts, pack)
                delta = result['stored_bytes']-study['cases'][profile]['canonical_plus_jobs'][scheme]['stored_bytes']
                choices[scheme] = dict(increment_bytes=delta)
            report['profiles'][profile][name] = choices
            print(profile, name, json.dumps(choices), flush=True)
    report['hybrid_parts'] = {}
    for profile in ('native30', 'extended_regular'):
        rows = [(i, f, raw) for i in sorted(canonical | jobs)
                for f, raw in frames[i].items() if profile != 'native30' or f in NATIVE]
        names = {m['identity']: m['name'] for m in study['models']}
        whole = study['cases'][profile]['canonical_plus_jobs']['whole']['stored_bytes']
        report['hybrid_parts'][profile] = {}
        for label, chosen in [
            ('named_only', canonical),
            ('scene_only', {i for i in canonical if names[i] in SCENE_ONLY}),
            ('secondary_only', {i for i in canonical if names[i] not in IMPORTANT}),
        ]:
            kept = [(i, f, raw) for i, f, raw in rows if i not in chosen]
            split = [(i, f, raw) for i, f, raw in rows if i in chosen]
            kept_cost = measure(kept, [26], pack)['stored_bytes']
            choices = {}
            for scheme, cuts in [('two', [8, 26]), ('three', [8, 18, 26])]:
                total = kept_cost + measure(split, cuts, pack)['stored_bytes']
                choices[scheme] = dict(stored_bytes=total, saving_vs_all_whole=whole-total)
            report['hybrid_parts'][profile][label] = choices
            print('hybrid', profile, label, json.dumps(choices), flush=True)
    # Per-pack data costs are strict controls, not final authored pose inventories.
    cast_path = HERE/'campaign/x/pack-refinement/text-cast.json'
    if cast_path.exists():
        cast = json.loads(cast_path.read_text())
        stable = {'Ramza', 'Delita', 'Agrias', 'Mustadio', 'Rafa', 'Malak', 'Orlandu',
                  'Meliadoul', 'Reis', 'Beowulf', 'Cloud', 'Alma', 'Ovelia'}
        report['pack_whole_body_controls'] = {}
        for key in ('I', 'II-a', 'II-b', 'III-a', 'III-b', 'IV-1', 'IV-2', 'IV-3'):
            people = stable | set(cast['packs'][key]['cast'])
            ids = {study['canonical_ids'][n] for n in people} | jobs
            values = {}
            for profile in ('native30', 'extended_regular'):
                rows = [(i, f, raw) for i in sorted(ids) for f, raw in frames[i].items()
                        if profile != 'native30' or f in NATIVE]
                values[profile] = measure(rows, [26], pack)['stored_bytes']
            report['pack_whole_body_controls'][key] = values
            print('pack human control', key, json.dumps(values), flush=True)
    report['decision'] = (
        'No costume variant is retained: one canonical body per person, Ramza and Delita included. '
        'These increments price the dropped sets; the row-band columns show a second outfit shares no band.')
    (folder/'costume-choices.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
