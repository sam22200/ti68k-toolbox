#!/usr/bin/env python3
"""Small offline ROM tables for lobs and the two measured specials."""
from pathlib import Path
import hashlib
import struct

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
def main():
    program = (ROOT / 'sources/windjammers_neogeo/program.be.bin').read_bytes()
    from measure_actions import PROGRAM_SHA
    assert hashlib.sha256(program).hexdigest() == PROGRAM_SHA
    def words(address, count, kind='H'):
        size = struct.calcsize(kind)
        return struct.unpack('>' + kind * count, program[address:address + size * count])
    lines = ['/* Local ROM data; generated offline. No runtime trig/sqrt/division. */']
    def table(kind, name, data):
        lines.append('static const ' + kind + ' ' + name + '[' + str(len(data)) + '] = {' +
                     ','.join(str(v) + ('L' if kind in ('s32', 'u32') else '') for v in data) + '};')
    table('s16', 'advanced_trig', words(0x2a7fe, 129, 'h'))
    table('u32', 'distance_limits', [(i * 4) ** 2 for i in range(97)])
    table('s32', 'lob_speed', [words(0x2a3e6 + i * 16, 1, 'i')[0] for i in range(49)])
    table('s32', 'lob_height', [words(0x2a3e6 + i * 16 + 4, 1, 'i')[0] for i in range(49)])
    table('s32', 'superlob_speed', words(0x2a68e, 97, 'i'))
    table('u8', 'lob_direction', program[0x1d76e:0x1d77e])
    table('u8', 'lob_rows', words(0x28176, 64))
    # The selected Beach is arena 1: offset 0x4e, 20-pixel target rows.
    table('u8', 'lob_targets', words(0x28244, 7))
    table('u8', 'superlob_dy', program[0x286a8:0x286d6])
    table('u16', 'small_reciprocal', [0] + [(65536 + n - 1) // n if n > 1 else 0 for n in range(1, 33)])
    table('u8', 'gesture_directions', program[0x1be94:0x1bea4])
    table('u8', 'curve_aims', program[0x1d444:0x1d464])
    table('u16', 'curve_turn', [words(0x27c2e + character, 1)[0] for character in (20, 12)])
    def pointer(address): return words(address, 1, 'I')[0]
    delays = []
    for subtype in (4, 8):
        action = pointer(pointer(pointer(0x20a3e + 20) + 20) + subtype)
        for facing in range(8):
            animation = pointer(action + facing * 4)
            event = pointer(action + 32 + facing * 4)
            i = 0
            while words(event + i * 2, 1)[0]: i += 1
            delays.append(i * program[animation + 1])
    table('u8', 'curve_delays', delays)
    (GAME / 'generated/advanced_tables.h').write_text('\n'.join(lines) + '\n')
    print('Advanced tables: original quantized trig, flight buckets, targets, reciprocal divisions')


if __name__ == '__main__':
    main()
