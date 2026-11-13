#!/usr/bin/env python3
"""Project R4 Chapter I archive cost from dated estimates and real package bytes.

Host-only: no disc extraction, compilation, emulator or live save access.
This is planning evidence, never a full-content or Flash certification.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

import chapter_packages as packages

HERE = Path(__file__).resolve().parents[1]


def load(path):
    return json.loads(path.read_text())


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def project(plan, manifest, spec):
    if (plan['schema'] != 'FFT_CHAPTER1_ARCHIVE_PLAN_1' or
        manifest['schema'] != 'FFT_CHAPTER_PACKAGES_1' or
        plan['baseline_view_format'] != 'R2' or plan['target_view_format'] != 'R4' or
        spec.get('view_format') != 'R4' or plan['production_width_mask']):
        raise ValueError('Expected the dated R2 baseline and an R4 core without width masks')
    envelope = spec['envelope']
    if manifest['envelope'] != envelope:
        raise ValueError('Package and core envelopes differ')
    rows = plan['maps']
    if len({r['map'] for r in rows}) != len(rows) or not rows:
        raise ValueError('Expected distinct physical maps')
    depth_max = int(re.search(r'#define FFT_CORE_DEPTH_MAX (\d+)',
                             (HERE/'core_limits.h').read_text())[1])
    # Keep all old compressed bytes and add every raw colour byte. Deliberately
    # overcounts the replaced colour streams; do not present it as actual size.
    legacy = sum(r['legacy_four_views_bytes'] for r in rows)
    calibrated = (legacy * plan['scenery_calibration_percent'] + 99) // 100
    raw = len(rows) * 4 * envelope['scene_rows'] * 40 * 2
    other = sum(plan['other_upper_estimates'].values())
    projected = calibrated + raw + other
    allowance = manifest['planning_budget_bytes'] - manifest['reserved_bytes'] - manifest['core']['bytes']
    return dict(schema='FFT_CHAPTER1_ARCHIVE_PROJECTION_1', maps=len(rows),
                evidence_date=plan['evidence_date'], core_key=manifest['core_key'],
                core_bytes=manifest['core']['bytes'],
                pilot_chapter_bytes=[c['bytes'] for c in manifest['chapters']],
                chapter_allowance_bytes=allowance, legacy_view_estimate_bytes=legacy,
                calibrated_legacy_views_bytes=calibrated, raw_colour_upper_bytes=raw,
                other_upper_estimates=plan['other_upper_estimates'],
                projected_chapter_bytes=projected, planning_margin_bytes=allowance-projected,
                fits_planning_allowance=projected<=allowance,
                cropped_depth_envelope_ok=all(len(r['cropped_depth_bytes'])==4 and
                    all(0<n<=depth_max for n in r['cropped_depth_bytes']) for r in rows),
                width_crop_maps=[r['map'] for r in rows if r['width_px']>320],
                remaining=['Assemble and audit all actual chapter banks',
                           'Review per-map crops/placement; width flags are not artwork validation',
                           'Measure installed Flash/VAT overhead and fragmentation'],
                caveat='Dated estimates, not a full chapter build. Raw colours are added without '
                       'subtracting legacy colours. Other content retains dated upper estimates; '
                       'this is not an absolute bound on unfinished content. RAM is separate.')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--packages', type=Path, required=True, help='real packages.json and adjacent groups')
    ap.add_argument('--plan', type=Path, default=HERE/'campaign/packages/chapter1-plan.json')
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/chapter1-archive-plan/projection.json')
    args = ap.parse_args()
    manifest, spec = load(args.packages), load(HERE/'campaign/packages/core.json')
    # Reject stale/copied manifests, and bind the installed core to the current
    # R4 specification through the existing package validator.
    for group in [manifest['core'], *manifest['chapters']]:
        path = args.packages.parent/group['file']
        if path.stat().st_size != group['bytes'] or sha(path) != group['sha256']:
            raise ValueError(f'{path}: package bytes differ from their manifest')
    core_path = args.packages.parent/manifest['core']['file']
    content = packages.reuse_core({}, core_path, spec)
    _, key = packages.core_package(content, spec)
    if key.hex() != manifest['core_key']:
        raise ValueError('Package manifest core key differs')
    report = project(load(args.plan), manifest, spec)
    report['inputs'] = {str(p):sha(p) for p in (args.plan, args.packages, HERE/'campaign/packages/core.json')}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2)+'\n')
    print(f"Chapter I projection: {report['projected_chapter_bytes']} bytes; "
          f"allowance {report['chapter_allowance_bytes']}; margin {report['planning_margin_bytes']}")
    print('Planning only; unfinished content, Flash installation and RAM remain separate.')
    if not report['fits_planning_allowance'] or not report['cropped_depth_envelope_ok']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
