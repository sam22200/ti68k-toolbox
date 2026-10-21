#!/usr/bin/env python3
"""Common interface bank of the chapter packages: ffcu = the ffti sections every chapter
uses unchanged (character advances, font, slope markers, banners; 7,916 bytes), stored
once on the calculator instead of inside each chapter's ffoi/ffgi.

usage: common_ui.py opening|campaign ...
Writes <chapter>/data/ffcu.bin and ui_common.h (UIC_* offsets, read through fft.c's
cgfx), then strips those sections from the chapter bank and shifts every later offset:
its interface.h (campaign: a new campaign/data/interface.h from the root one) and the
igfx table/OPENING_* offsets of its battle.h. Idempotent: a stripped header carries a
marker and is only checked again. The shared bytes must equal ffti's, or it stops.
"""
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parents[1]
SHARED = ('ADV', 'FONT', 'SLOPES', 'BANNERS')
MARK = '// Common sections ADV FONT SLOPES BANNERS live in ffcu (tools/common_ui.py).'


def sections(header):
    """(name, start, end) in bank order; BANNERS ends after BN_STRIP_BYTES."""
    defs = sorted(((int(v), k) for k, v in re.findall(r'#define UI_(\w+) (\d+)UL', header)))
    banners = int(re.search(r'#define BN_STRIP_BYTES (\d+)', header)[1])
    out = []
    for i, (start, name) in enumerate(defs):
        end = defs[i + 1][0] if i + 1 < len(defs) else start + banners
        out.append((name, start, end))
    return out


def common():
    header = (HERE / 'interface.h').read_text(); blob = (HERE / 'ffti.bin').read_bytes()
    parts = {n: blob[a:b] for n, a, b in sections(header) if n in SHARED}
    data = b''; lines = ['// Generated locally by tools/common_ui.py; never committed.']
    for n in SHARED:
        lines.append('#define UIC_%s %dUL' % (n, len(data))); data += parts[n]
    lines += re.findall(r'#define BN_\w+ [^\n]+', header)       # the banners' layout
    (HERE / 'ui_common.h').write_text('\n'.join(lines) + '\n')
    return data, parts


def strip(chapter, data, parts):
    folder = HERE / chapter / 'data'
    bank_path = folder / ('ffoi.bin' if chapter == 'opening' else 'ffgi.bin')
    out_header = folder / 'interface.h'
    source = out_header if chapter == 'opening' else HERE / 'interface.h'
    (folder / 'ffcu.bin').write_bytes(data)
    header = source.read_text(); bank = bytearray(bank_path.read_bytes())
    # A regenerated bank still holds the sections at the source header's offsets
    # (campaign_assets rewrites ffgi/battle.h but not campaign/data/interface.h).
    font = next((a, b) for n, a, b in sections(header) if n == 'FONT')
    if MARK in header or bytes(bank[font[0]:font[1]]) != parts['FONT']:
        assert MARK in out_header.read_text(), (chapter, 'neither stripped nor original')
        print(chapter, 'already uses ffcu'); return
    cuts = []
    for name, a, b in sections(header):
        if name in SHARED:
            assert bytes(bank[a:b]) == parts[name], (chapter, name, 'differs from ffti')
            cuts.append((a, b))
    def moved(offset):
        return offset - sum(b - a for a, b in cuts if b <= offset)
    for a, b in reversed(cuts):
        del bank[a:b]
    header = re.sub(r'(#define UI_\w+ )(\d+)UL', lambda m: m[1] + '%dUL' % moved(int(m[2])), header)
    out_header.write_text(header.rstrip('\n') + '\n' + MARK + '\n')
    tables = folder / 'battle.h'; text = tables.read_text()
    names = re.search(r'#define OPENING_NAMES (\d+)UL', text)
    if names:  # its directory holds absolute bank offsets of the names
        at = moved(int(names[1])); count = (int.from_bytes(bank[at:at + 2], 'big') - int(names[1])) // 2
        for i in range(count):
            v = int.from_bytes(bank[at + 2 * i:at + 2 * i + 2], 'big')
            bank[at + 2 * i:at + 2 * i + 2] = moved(v).to_bytes(2, 'big')
    text = re.sub(r'(igfx \+ )(\d+)UL', lambda m: m[1] + '%dUL' % moved(int(m[2])), text)
    text = re.sub(r'(#define OPENING_\w+ )(\d+)UL', lambda m: m[1] + '%dUL' % moved(int(m[2])), text)
    tables.write_text(text); bank_path.write_bytes(bank)
    print(chapter, bank_path.name, 'shrinks by', sum(b - a for a, b in cuts), 'bytes')


if __name__ == '__main__':
    data, parts = common()
    for chapter in sys.argv[1:]:
        strip(chapter, data, parts)
