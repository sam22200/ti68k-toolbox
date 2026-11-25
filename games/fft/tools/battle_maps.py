#!/usr/bin/env python3
"""Which maps the player actually fights on, and which are only scenes.

The depth budget's arbitration differs completely between the two. On a battle map
the camera is tactical and every unit has to stay visible, so a wall that hides the
field is a candidate for removal. On a scene map nothing is played: the framing is
the author's, and geometry that costs nothing on the dearest view can stay.

The join is exact, not by name. The GameFAQs battle compendium prints a "Debug
Number" for every battle, and that number is the **ENTD** id, not the event id --
checked against this project's own extraction, where Orbonne Monastery is ENTD 387
and Gariland is 0x184 = 388. EVENT/ATTACK.OUT's scenario table gives every event
its map and its ENTD, so a map hosts a battle iff one of the compendium's ENTDs
plays there. Two maps share the name "St. Murond Temple", which is why matching
names would not do.

**The positive list is sound; the negative one is not.** A map named here really
does host that battle, because the join is on an id both sides agree about. A map
absent from it only means no compendium entry landed there, and this compendium is
incomplete: it prints no Debug Number at all for Sweegy Woods (ENTD 384), and the
entry it labels "Mandalia Plains / 388" is really Gariland, as ATTACK.OUT shows
388 playing on map 22. So read the second list as "no battle confirmed", never as
"this is a scene". For a particular map, look at the ENTDs its scenarios use: map
51 runs ENTDs 326 and 327, which no battle in the compendium claims, while map 50
runs 460, Murond Holy Place.

usage: battle_maps.py [--guide fft_guides/battle_list.txt] [--json OUT]
"""
import argparse
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import core_audit   # noqa: E402
import extract      # noqa: E402

GUIDE = HERE.parents[1] / 'fft_guides/battle_list.txt'
NAMES = HERE / 'campaign/x/core-audit/map-viewer-names.json'
ENTRY = re.compile(r'^\s*\\(\S.*?)\s*$')
DEBUG = re.compile(r'^\s*Debug Number:\s*(\d+)\s*$')


def guide_battles(path):
    """[(name, debug number)] for every battle the compendium details."""
    out, pending = [], None
    for line in Path(path).read_text(errors='replace').splitlines():
        m = ENTRY.match(line)
        if m and not DEBUG.match(line):
            pending = m.group(1)
            continue
        m = DEBUG.match(line)
        if m and pending:
            out.append((pending, int(m.group(1))))
            pending = None
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('disc', nargs='?', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--guide', type=Path, default=GUIDE)
    ap.add_argument('--json', type=Path)
    args = ap.parse_args()

    disc, files = extract.disc_files(str(args.disc))
    rows = core_audit.scenarios(disc.read(*files['EVENT/ATTACK.OUT']))
    by_entd = {}
    for r in rows:
        by_entd.setdefault(r['entd'], set()).add(r['map'])
    names = json.loads(NAMES.read_text())

    battles, unmatched = {}, []
    for name, debug in guide_battles(args.guide):
        maps = by_entd.get(debug)
        if not maps:
            unmatched.append((name, debug))
            continue
        for mp in maps:
            battles.setdefault(mp, []).append(dict(name=name, entd=debug))

    fought = sorted(battles)
    scenes = sorted(int(k) for k in names if int(k) not in battles)
    print('%d maps host a battle the compendium details, %d have none confirmed\n'
          % (len(fought), len(scenes)))
    for m in fought:
        print('map %3d %-30s %s' % (m, names.get(str(m), '?')[:30],
                                    ', '.join('%s (%d)' % (b['name'][:34], b['entd'])
                                              for b in battles[m])))
    print('\nno battle confirmed (NOT the same as a scene, see the docstring): %s'
          % ' '.join(str(m) for m in scenes))
    if unmatched:
        print('\nno scenario for: %s' % ', '.join('%s (%d)' % u for u in unmatched))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(dict(
            schema='FFT_BATTLE_MAPS_1',
            battles={str(k): v for k, v in battles.items()},
            no_battle_confirmed=scenes), indent=2) + '\n')


if __name__ == '__main__':
    main()
