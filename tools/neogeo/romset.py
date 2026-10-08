#!/usr/bin/env python3
"""Audit a local Windjammers chip set and prepare reference inputs, without BIOS."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ROM = ROOT / 'roms/neogeo/wjammers'
DRIVER_REV = '0282ae7e0dc647316043f2f6b2cd312d3164218a'
# FBNeo d_neogeo.cpp, wjammersRomDesc, at DRIVER_REV. No ROM data here.
CHIPS = (
    ('065-p1', 'p1', 0x100000, 0x6692c140, '68000 program'),
    ('065-s1', 's1', 0x020000, 0x074b5723, 'FIX graphics'),
    ('065-c1', 'c1', 0x100000, 0xc7650204, 'sprite graphics'),
    ('065-c2', 'c2', 0x100000, 0xd9f3e71d, 'sprite graphics'),
    ('065-c3', 'c3', 0x100000, 0x40986386, 'sprite graphics'),
    ('065-c4', 'c4', 0x100000, 0x715e15ff, 'sprite graphics'),
    ('065-m1', 'm1', 0x020000, 0x52c23cfc, 'Z80 program'),
    ('065-v1', 'v1', 0x100000, 0xce8b3698, 'sample data'),
    ('065-v2', 'v2', 0x100000, 0x659f9b96, 'sample data'),
    ('065-v3', 'v3', 0x100000, 0x39f73061, 'sample data'),
    ('065-v4', 'v4', 0x100000, 0x5dee7963, 'sample data'),
)


def read_chips(source):
    """Accept a chip directory or ZIP with local .bin or driver chip names."""
    source = Path(source)
    if source.is_dir():
        names = [p.name for p in source.iterdir() if p.is_file()]
        read = lambda name: (source / name).read_bytes()
        archive = None
    else:
        archive = zipfile.ZipFile(source)
        names = archive.namelist()
        read = archive.read
    try:
        chips = {}
        for stem, extension, size, crc, role in CHIPS:
            matches = [name for name in names
                       if Path(name).name in (stem + '.bin', stem + '.' + extension)]
            if len(matches) != 1:
                raise ValueError(f'{stem}: expected exactly one chip, found {len(matches)}')
            data = read(matches[0])
            actual_crc = zlib.crc32(data)
            if len(data) != size or actual_crc != crc:
                raise ValueError(f'{stem}: wrong chip (size {len(data)}, CRC32 {actual_crc:08x}); '
                                 f'expected {size}, {crc:08x}')
            chips[stem + '.' + extension] = data
        return chips
    finally:
        if archive is not None:
            archive.close()


def cpu_program(raw):
    """Normalize this verified P1 chip to 68000 big-endian byte order."""
    if len(raw) != 0x100000 or zlib.crc32(raw) != 0x6692c140:
        raise ValueError('program conversion requires the verified Windjammers P1')
    output = bytearray(len(raw))
    output[0::2], output[1::2] = raw[1::2], raw[0::2]
    if output[0x100:0x108] != b'NEO-GEO\0':
        raise ValueError('normalized program has no NEO-GEO header')
    if int.from_bytes(output[0x108:0x10a], 'big') != 0x0065:
        raise ValueError('normalized program has the wrong cartridge ID')
    return bytes(output)


def report(chips):
    program = cpu_program(chips['065-p1.p1'])
    return {
        'set': 'wjammers',
        'driver_revision': DRIVER_REV,
        'chips': [dict(name=stem + '.' + ext, role=role, bytes=size,
                       crc32=f'{crc:08x}',
                       sha256=hashlib.sha256(chips[stem + '.' + ext]).hexdigest())
                  for stem, ext, size, crc, role in CHIPS],
        'program': {
            'byte_order': '68000 big-endian; adjacent bytes swapped from chip dump',
            'sha256': hashlib.sha256(program).hexdigest(),
            'cartridge_id_hex': f'{int.from_bytes(program[0x108:0x10a], "big"):04x}',
            'initial_sp': f'{int.from_bytes(program[:4], "big"):08x}',
            'reset_vector': f'{int.from_bytes(program[4:8], "big"):08x}',
        },
        'gameplay_verified': False,
        'bios': 'separate local dependency; not contained in this cartridge set',
    }


def pack(chips, target):
    """Preserve chip bytes; only normalize names and deterministic ZIP metadata."""
    target = Path(target)
    if target.name != 'wjammers.zip':
        raise ValueError('FBNeo identifies the driver by archive name; use wjammers.zip')
    with zipfile.ZipFile(target, 'w', compression=zipfile.ZIP_STORED) as archive:
        for name, data in chips.items():
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path, nargs='?', default=DEFAULT_ROM)
    parser.add_argument('--manifest', type=Path, help='write chip identities as JSON')
    parser.add_argument('--program', type=Path, help='write a big-endian CPU image for Ghidra')
    parser.add_argument('--pack', type=Path, help='write an unchanged-chip wjammers.zip for FBNeo')
    args = parser.parse_args()
    try:
        chips = read_chips(args.rom)
        inventory = report(chips)
        # Avoid destroying the input when the source itself is a ZIP or chip file.
        outputs = [p.resolve() for p in (args.manifest, args.program, args.pack) if p]
        inputs = ({p.resolve() for p in args.rom.iterdir() if p.is_file()}
                  if args.rom.is_dir() else {args.rom.resolve()})
        if len(outputs) != len(set(outputs)) or inputs.intersection(outputs):
            raise ValueError('output paths must be distinct and must not overwrite input chips')
        for path in (args.manifest, args.program, args.pack):
            if path:
                path.parent.mkdir(parents=True, exist_ok=True)
        if args.manifest:
            args.manifest.write_text(json.dumps(inventory, indent=2) + '\n')
        if args.program:
            args.program.write_bytes(cpu_program(chips['065-p1.p1']))
        if args.pack:
            pack(chips, args.pack)
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        parser.exit(1, f'ROM preparation failed: {error}\n')
    print(f'Windjammers: {len(chips)} chip sizes/CRCs checked; cartridge ID '
          f'{inventory["program"]["cartridge_id_hex"]}; CPU byte order verified')
    print('ROM audit only; original gameplay checks require a separate local Neo Geo BIOS.')


if __name__ == '__main__':
    main()
