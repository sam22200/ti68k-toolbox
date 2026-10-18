#!/usr/bin/env python3
"""Check complete banners/loot effects against the 68000 binary, including frame budgets."""
import json
import hashlib
import re
import struct
import subprocess
from pathlib import Path
import check_blows as checks

OUT = Path('x/events')


def original_banner_pixels():
    # SHA256s of the former C vector renderer's three 32-row, three-plane buffers.
    # These independent pre-change fixtures catch missing final letters/font drift.
    golden = ['0ec5eeb3606fb46755100823386b290458e827426f9591ab4e474c27eba8e17e',
              '54ec2f427e1599bc2f21d89f5c77ac387c930965f7e0ce6539965e5f9632a9e5',
              '6317df19fff55af424e02836cae65d8ea969115a122791c20bce605237e3197a']
    off = int(re.search(r'UI_BANNERS (\d+)', Path('interface.h').read_text())[1])
    bank = Path('ffti.bin').read_bytes()[off:]
    offsets = struct.unpack_from('>4H', bank)
    for lines, expected in zip(((0, 1), (2,), (3,)), golden):
        output = bytearray(3 * 32 * 30)
        for line in lines:
            pos = offsets[line]; x, y, h, stride = bank[pos:pos+4]
            data = bank[pos+4:]; size = h * stride
            for plane in range(3):
                for row in range(h):
                    dst = plane * 32 * 30 + (y + row) * 30 + x // 8
                    src = plane * size + row * stride
                    for col in range(stride):
                        output[dst+col] |= data[src+col]
        assert hashlib.sha256(output).hexdigest() == expected
    print('Four text strips match the former C vector renderer exactly.')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    subprocess.run(['make', 'pc', 'cycles', 'CFLAGS_GAME=-DFFT_MEASURE'], check=True)
    original_banner_pixels()
    count, peaks, replay_peaks = 0, {}, {}
    for scenario in (0, 59, 60, 69, 70, 71, 72):
        for frame in (1, 4, 8, 12, 16, 24, 28, 32, 40, 48, 64, 80, 96, 150):
            result = checks.ti(scenario, frame)
            checks.equal(scenario, frame, result)
            count += 1
        checks.budget(scenario, 150, result)
        (OUT / ('cycles-%u.log' % scenario)).write_text(result)
        values = [int(v, 16) for v in re.findall(r'value: -?\d+ \(0x([0-9a-f]+)\)', result)]
        peaks[scenario] = max(c for m, c in zip(values[::2], values[1::2]) if not m >> 31)
    # Natural conversions, late-battle defeat, and held fast-forward must also stay in budget.
    for scenario, frames, keys in ((16, 16000, None), (16, 6000, 'fast'), (15, 4000, 'defeat')):
        result = checks.ti(scenario, frames, keys)
        checks.equal(scenario, frames, result, keys)
        checks.budget(scenario, frames, result, keys)
        (OUT / ('replay-%u-%s.log' % (scenario, keys or 'none'))).write_text(result)
        values = [int(v, 16) for v in re.findall(r'value: -?\d+ \(0x([0-9a-f]+)\)', result)]
        replay_peaks['%u-%s' % (scenario, keys or 'none')] = max(
            c for m, c in zip(values[::2], values[1::2]) if not m >> 31)
        count += 1
    (OUT / 'checked.json').write_text(json.dumps({
        'paired_screens': count, 'scene_peak_cycles': peaks, 'replay_peak_cycles': replay_peaks,
        'measurement': 'MC68000 datasheet CPU cycles; no emulator, grayscale or frame limiter',
        'executable_bytes': Path('fftc.89z').stat().st_size,
        'unit_bank_bytes': Path('fftu.bin').stat().st_size,
        'interface_bank_bytes': Path('ffti.bin').stat().st_size,
        'source_sha256': {f: hashlib.sha256(Path(f).read_bytes()).hexdigest() for f in
            ('fft.c', 'fft.h', 'tools/banner.py', 'tools/interface.py', 'tools/units.py',
             'test_events.c', 'tools/check_events.py')},
        'bank_sha256': {f: hashlib.sha256(Path(f).read_bytes()).hexdigest()
                        for f in ('fftu.bin', 'ffti.bin')},
        'test_binary_sha256': hashlib.sha256(Path('fftc.89z').read_bytes()).hexdigest(),
    }, indent=2) + '\n')
    print('%u event/replay PC/TI screens and ordinary frame budgets passed' % count)


if __name__ == '__main__':
    main()
