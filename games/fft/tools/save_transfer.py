#!/usr/bin/env python3
"""Exchange FFT campaign saves between PC files and TI-89 variables, losslessly.

This wraps existing FSV bytes; it does not serialize raw runtime .state dumps.
No calculator is contacted. Existing destinations require explicit --replace.
"""
import argparse
import hashlib
import os
from pathlib import Path
import tempfile

import chapter_packages as packages

NAMES = ('ffsave', 'ffsave1', 'ffsave2', 'ffsave3')
LENGTHS = dict(zip((b'FSV' + bytes([v]) for v in range(ord('2'), ord('9') + 1)),
                   (3110, 3138, 3202, 3204, 3918, 3992, 4024, 4684)))
TRAILER = b'\0sav\0\xf8'


def validate(raw):
    """Check format/length and decoder bounds; gameplay validation stays in save.h."""
    if raw[:4] not in LENGTHS:
        raise ValueError('Expected an FSV2..FSV9 campaign save; raw .state dumps are platform-specific')
    if len(raw) != LENGTHS[raw[:4]]:
        raise ValueError(f'{raw[:4].decode()}: expected {LENGTHS[raw[:4]]} bytes, got {len(raw)}')
    if raw[36] > 16:
        raise ValueError('Invalid party count (maximum 16)')
    if raw[:4] != b'FSV2' and raw[3115] > 5:
        raise ValueError('Invalid deployment count (maximum 5)')
    if raw[:4] == b'FSV9':
        if raw[4024] > 8 or any(raw[4030 + 9*i] > 3 for i in range(raw[4024])):
            raise ValueError('Invalid proposition team count')
    return raw


def read_save(path, name=None):
    """Read a raw save or select a save from a checksum-checked TI file/group."""
    path = Path(path)
    raw = path.read_bytes()
    if raw[:8] != b'**TI89**':
        if name is not None:
            raise ValueError('--name selects a variable in a TI file only')
        return path.stem if path.stem in NAMES else None, validate(raw)
    variables = packages.read_group(path)
    if name is None:
        candidates = [v for v in variables if v in NAMES]
        if len(candidates) != 1:
            raise ValueError('Select one save with --name; available: ' + ', '.join(candidates or ['none']))
        name = candidates[0]
    if name not in NAMES or name not in variables:
        raise ValueError('Selected campaign save is absent from the TI file')
    var = variables[name]
    if var.kind != 0x1c or not var.payload.endswith(TRAILER):
        raise ValueError(f'{name}: expected an OTH variable with the sav trailer')
    return name, validate(var.payload[:-len(TRAILER)])


def destination_name(slot, inferred):
    if slot is not None:
        return 'ffsave' if slot == 'session' else 'ffsave' + slot
    if inferred in NAMES:
        return inferred
    raise ValueError('Use --slot 1, 2, 3 or session when the source has no standard save name')


def write_file(path, data, replace=False):
    """Publish a complete file; no accidental clobber or partial destination."""
    path = Path(path)
    # The directory is chosen/created by the caller, never a live install path.
    with tempfile.NamedTemporaryFile(prefix='.fft-save-', dir=path.parent, delete=False) as f:
        temporary = Path(f.name)
        try:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        except BaseException:
            temporary.unlink()
            raise
    try:
        if replace:
            os.replace(temporary, path)
        else:
            os.link(temporary, path)  # Exclusive publication, even against concurrent writers.
    finally:
        temporary.unlink(missing_ok=True)


def to_ti(source, output, slot=None, replace=False):
    source, output = Path(source), Path(output)
    if source.read_bytes()[:8] == b'**TI89**':
        raise ValueError('to-ti expects a raw PC campaign .sav file')
    inferred, raw = read_save(source)
    name = destination_name(slot, inferred)
    if output.suffix.lower() != '.89y':
        raise ValueError('TI output must have the .89y extension')
    if source.resolve() == output.resolve():
        raise ValueError('Source and destination must differ')
    var = packages.Variable(name, 0x1c, 3, raw + TRAILER)
    write_file(output, packages.group_bytes([var], single=True), replace)
    return name, raw, output


def to_pc(source, directory, name=None, slot=None, replace=False):
    source, directory = Path(source), Path(directory)
    if source.read_bytes()[:8] != b'**TI89**':
        raise ValueError('to-pc expects a TI .89y variable or .89g group')
    inferred, raw = read_save(source, name)
    target = destination_name(slot, inferred)
    output = directory / (target + '.sav')
    if source.resolve() == output.resolve():
        raise ValueError('Source and destination must differ')
    directory.mkdir(parents=True, exist_ok=True)
    write_file(output, raw, replace)
    return target, raw, output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest='command', required=True)
    export = subs.add_parser('to-ti', help='Wrap a PC campaign save as a TI variable in main')
    export.add_argument('source', type=Path)
    export.add_argument('output', type=Path)
    receive = subs.add_parser('to-pc', help='Extract one TI campaign save into the PC working directory')
    receive.add_argument('source', type=Path)
    receive.add_argument('directory', type=Path)
    receive.add_argument('--name', choices=NAMES, help='Source variable, required for groups with multiple saves')
    for sub in (export, receive):
        sub.add_argument('--slot', choices=('1', '2', '3', 'session'), help='Destination slot (default: preserve source name)')
        sub.add_argument('--replace', action='store_true', help='Explicitly replace an existing destination file')
    args = parser.parse_args()
    try:
        if args.command == 'to-ti':
            name, raw, output = to_ti(args.source, args.output, args.slot, args.replace)
        else:
            name, raw, output = to_pc(args.source, args.directory, args.name, args.slot, args.replace)
    except (OSError, ValueError) as exc:
        parser.exit(1, f'Error: {exc}\n')
    print(f'{name}: {raw[:4].decode()}, {len(raw)} bytes, SHA256 {hashlib.sha256(raw).hexdigest()} -> {output}')


if __name__ == '__main__':
    main()
