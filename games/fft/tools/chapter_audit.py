#!/usr/bin/env python3
"""Chapter I coverage audit: every unit of the chapter's story battles (ATTACK rows up to Fort
Zeakden, ENTD 0x180..0x191) and of the first-tier random formations of its world-map routes,
read from the local disc, against what the installed core executes: an action ability is
executable when its record in ffcj (CJ_BATTLE, 12 bytes, kind first) is not 255.

Lists, per ability absent from the core: its users (encounter, job, identity). Primary and
secondary skill sets are counted whole (ENTD units draw their learned actions; the conservative
case is the whole set). Reaction/support/movement are listed with their ids only: the core's
78 R/S/M consumers are checked by the R/S/M suites, not by this table.

usage: chapter_audit.py [--out campaign/x/chapter1-audit.json]
"""
import argparse
import json
import struct
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import battle            # noqa: E402
import core_audit        # noqa: E402
import extract           # noqa: E402
import management_data   # noqa: E402

HUMAN, MONSTER = 0x80064a94, 0x80065bc4          # common_tables.py: 176 human, 48 monster sets
CJ_BATTLE, CJ_EXECUTABLES = 13, 640
LAST_STORY = 0x191                               # Fort Zeakden, the chapter's last battle
ROUTE_MAPS = (85, 72, 74, 77, 76)                # Mandalia, Fovoham, Sweegy, Lenalia, Zeklaus


def executable(cj):
    """Ability ids the core executes (ffcj CJ_BATTLE kind != 255)."""
    at = struct.unpack_from('>H', cj, 4 + 2 * CJ_BATTLE)[0]
    return {a for a in range(CJ_EXECUTABLES) if cj[at + 12 * a] != 255}


def set_actions(d, k):
    """The action ids of skill set k (human 16, monster 4)."""
    if k >= 0xB0:
        b = d.at(MONSTER + 5 * (k - 0xB0), 5)
        return [b[1 + i] | (0x100 if b[0] & 0x80 >> i else 0) for i in range(4) if b[1 + i]]
    return [a for a in d.skillset(k) if a]


def story_encounters(disk, files):
    rows = core_audit.scenarios(disk.read(*files['EVENT/ATTACK.OUT']))
    out = []
    for e in rows:
        if e['entd'] >= 0x180 and e['entd'] not in [x for x, _ in out]:
            out.append((e['entd'], 'story map %d' % e['map']))
        if e['entd'] == LAST_STORY:
            break
    return out


def random_encounters(disk, files):
    world = disk.read(*files['WORLD/WLDCORE.BIN']); base = 0x80067000
    start = struct.unpack_from('<I', world, 0x80096a40 - base)[0] - base
    out = []
    for i in range(57):
        row = world[start + 24 * i:start + 24 * (i + 1)]
        if row[22] in ROUTE_MAPS:
            for k in (14, 15):                     # first progress tier: two formations
                if (row[k], 'random map %d' % row[22]) not in out:
                    out.append((row[k], 'random map %d' % row[22]))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/chapter1-audit.json')
    args = ap.parse_args()
    d = battle.Disc(str(core_audit.DEFAULT_DISC)); disk, files = extract.disc_files(str(core_audit.DEFAULT_DISC))
    ok = executable((HERE / 'campaign/data/ffcj.bin').read_bytes())
    abil = management_data.names(d.bat, management_data.ABILITY_NAMES, 512)
    setn = management_data.names(d.bat, management_data.SET_NAMES, 64)
    jobn = management_data.names(d.bat, management_data.JOB_NAMES, 160)
    used = defaultdict(set); rsm = defaultdict(set); units = []
    for entd, source in story_encounters(disk, files) + random_encounters(disk, files):
        for slot in range(16):
            b = d.entd(entd, slot)
            if not b[0]:
                continue
            job = b[10]
            sets = [b[0x1d] if b[0x1d] not in (0, 0xFF, 0xFE) else d.job(job)['set']]
            if b[11] not in (0, 0xFF, 0xFE):
                sets.append(b[11])
            who = '%s ENTD %03X slot %d: %s (identity %02X)' % (source, entd, slot, jobn[job] if job < len(jobn) else '%02X' % job, b[0])
            units.append(dict(encounter=entd, source=source, slot=slot, identity=b[0], job=job,
                              job_name=jobn[job] if job < len(jobn) else '', sets=sets,
                              rsm=[struct.unpack_from('<H', b, 12 + 2 * i)[0] for i in range(3)]))
            for k in sets:
                for a in set_actions(d, k):
                    used[a].add(who)
            for r in units[-1]['rsm']:
                if r not in (0, 0x1FE, 0x1FF, 0xFFFF):
                    rsm[r].add(who)
    missing = sorted(a for a in used if a < 512 and a not in ok)
    report = dict(schema='FFT_CHAPTER_AUDIT_1', encounters=sorted({u['encounter'] for u in units}),
                  units=len(units), actions=len(used), executable=len([a for a in used if a in ok]),
                  missing=[dict(id=a, name=abil[a], users=sorted(used[a])) for a in missing],
                  rsm=[dict(id=r, name=abil[r] if r < 512 else '', users=len(rsm[r])) for r in sorted(rsm)],
                  sets=sorted({(k, setn[k] if k < len(setn) else 'monster %02X' % k) for u in units for k in u['sets']}),
                  unit_rows=units)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=1, default=list) + '\n')
    print('encounters %d, units %d, actions %d, executable %d, missing %d' % (
        len(report['encounters']), len(units), len(used), report['executable'], len(missing)))
    for m in report['missing']:
        print('  %03X %-16s %s' % (m['id'], m['name'], '; '.join(m['users'][:3]) + (' ...' if len(m['users']) > 3 else '')))
    print('sets:', ', '.join('%02X %s' % s for s in report['sets']))
    print('R/S/M:', ', '.join('%03X %s' % (r['id'], r['name']) for r in report['rsm']))


if __name__ == '__main__':
    main()
