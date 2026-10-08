#!/usr/bin/env python3
"""Local ROM integration: chip identity, CPU mapping, lossless staging, bad inputs."""
import hashlib
from pathlib import Path
import tempfile
import zipfile

from romset import DEFAULT_ROM, cpu_program, pack, read_chips, report


def rejected(action):
    try:
        action()
    except ValueError:
        return
    raise AssertionError('invalid input accepted')


def main():
    chips = read_chips(DEFAULT_ROM)
    assert len(chips) == 11 and sum(map(len, chips.values())) == 9699328
    assert hashlib.sha256(chips['065-p1.p1']).hexdigest() == (
        'cd2684eaa7fe3f457573c026099b70e9b30fcda6c2bf2e27683151b91b674767')
    program = cpu_program(chips['065-p1.p1'])
    assert program[0x100:0x108] == b'NEO-GEO\0'
    assert program[:8] == bytes.fromhex('0010f30000c00402')
    assert int.from_bytes(program[0x108:0x10a], 'big') == 0x0065
    restored = bytearray(len(program))
    restored[0::2], restored[1::2] = program[1::2], program[0::2]
    assert restored == chips['065-p1.p1']
    rejected(lambda: cpu_program(program))
    rejected(lambda: cpu_program(chips['065-s1.s1']))
    assert report(chips)['gameplay_verified'] is False
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        packed = root / 'wjammers.zip'
        pack(chips, packed)
        assert read_chips(packed) == chips, 'staging must preserve every chip byte'
        original_hash = hashlib.sha256(packed.read_bytes()).digest()
        pack(chips, packed)
        assert hashlib.sha256(packed.read_bytes()).digest() == original_hash
        rejected(lambda: pack(chips, root / 'renamed.zip'))
        with zipfile.ZipFile(root / 'broken.zip', 'w') as archive:
            for name, data in chips.items():
                if name == '065-p1.p1':
                    data = bytes([data[0] ^ 1]) + data[1:]
                archive.writestr(name, data)
        rejected(lambda: read_chips(root / 'broken.zip'))
        with zipfile.ZipFile(root / 'missing.zip', 'w') as archive:
            archive.writestr('065-p1.p1', chips['065-p1.p1'])
        rejected(lambda: read_chips(root / 'missing.zip'))
        with zipfile.ZipFile(root / 'ambiguous.zip', 'w') as archive:
            archive.writestr('065-p1.p1', chips['065-p1.p1'])
            archive.writestr('065-p1.bin', chips['065-p1.p1'])
        rejected(lambda: read_chips(root / 'ambiguous.zip'))
    print('Windjammers: 11 chip identities, CPU header/vectors, reversible byte order, '
          'lossless deterministic ZIP, corrupt/missing/ambiguous chip rejection passed')


if __name__ == '__main__':
    main()
