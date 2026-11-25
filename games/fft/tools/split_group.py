#!/usr/bin/env python3
"""Split a TI-89 group into several smaller ones, in order.

An installation group that cannot be sent in one piece is sent in parts: a link
that drops, a calculator without room for the whole group at once, or a transfer
somebody wants to make in stages.

It is NOT needed for TiEmu, despite how this tool came about. The 684 KB
`fft-core.89g` transfers there in 57.5 s; the apparent failure was
`ti-mem wait NAME` reporting archived variables as absent
(ti68k-c-patterns.md).

The parts are consecutive, so the original order -- which is the order the
packager chose -- is preserved, and each part is a valid group AMS installs on
its own. A variable larger than the limit gets a part to itself; nothing is ever
cut in two, since AMS has no way to rejoin it.

The split is purely a transport concern. The variables, their contents, their
archive attributes and their folder are untouched: concatenating the parts'
variable lists gives back exactly the source group's, which --check verifies.

usage: split_group.py GROUP.89g [--max-bytes N] [--out DIR] [--check]
"""
import argparse
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import chapter_packages as cp   # noqa: E402

# One variable may reach the AMS envelope (65,520), and FFT's largest banks are
# just under it (ffca 64,899), so a part holds either one big bank or several
# small ones. 96 KB keeps a part well inside what TiEmu transfers reliably.
DEFAULT_MAX = 96 * 1024


def split(variables, limit):
    """Consecutive runs whose group stays within limit, never cutting a variable."""
    parts, run = [], []
    for v in variables:
        if run and len(cp.group_bytes(run + [v])) > limit:
            parts.append(run)
            run = []
        run.append(v)
        if len(cp.group_bytes(run)) > limit:     # this one alone is over: its own part
            parts.append(run)
            run = []
    if run:
        parts.append(run)
    return parts


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('group', type=Path)
    ap.add_argument('--max-bytes', type=int, default=DEFAULT_MAX)
    ap.add_argument('--out', type=Path, help='default: beside the source group')
    ap.add_argument('--check', action='store_true',
                    help='read the parts back and prove they rebuild the source')
    args = ap.parse_args()

    source = cp.read_group(args.group)
    parts = split(list(source.values()), args.max_bytes)
    out = args.out or args.group.parent
    out.mkdir(parents=True, exist_ok=True)
    stem = args.group.stem
    written = []
    for i, part in enumerate(parts, 1):
        data = cp.group_bytes(part)
        path = out / ('%s-%02d.89g' % (stem, i))
        path.write_bytes(data)
        written.append(path)
        print('%-28s %7d bytes  %2d vars  %s'
              % (path.name, len(data), len(part), ' '.join(v.name for v in part)))
    print('\n%d parts, %d bytes in all (source %d)'
          % (len(written), sum(p.stat().st_size for p in written), args.group.stat().st_size))

    if args.check:
        back = {}
        for path in written:
            for name, v in cp.read_group(path).items():
                if name in back:
                    raise SystemExit('%s appears in two parts' % name)
                back[name] = v
        if list(back) != list(source):
            raise SystemExit('the parts do not rebuild the source group, in order')
        for name, v in source.items():
            b = back[name]
            if (b.payload, b.kind, b.attr, b.folder) != (v.payload, v.kind, v.attr, v.folder):
                raise SystemExit('%s: contents or attributes changed' % name)
        print('check: %d variables, same order, same bytes, same attributes' % len(back))


if __name__ == '__main__':
    main()
