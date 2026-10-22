#!/usr/bin/env python3
"""Convert chapter view banks (ffov0..3, ffgv0..3) from one ZX0 stream of six planes to the
split format of extract.pack_view (colours, then depths/masks), keeping the contour tail of
view 0 (tools/contour_assets.py re-attaches it). Idempotent: split banks start with b'V2'.
usage: view_split.py opening|campaign ...
"""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import extract  # noqa: E402

PREFIX = {'opening': 'ffov', 'campaign': 'ffgv'}


def unzx0(stream):
    with tempfile.TemporaryDirectory() as d:
        src = Path(d) / 'i'; dst = Path(d) / 'o'; src.write_bytes(stream)
        subprocess.run([HERE.parents[1] / 'tools/bin/dzx0', '-f', src, dst], check=True, capture_output=True)
        return dst.read_bytes()


for chapter in sys.argv[1:]:
    for rot in range(4):
        path = HERE / chapter / 'data' / ('%s%d.bin' % (PREFIX[chapter], rot))
        data = path.read_bytes()
        if data[:2] == b'V2':
            print(path.name, 'already split'); continue
        tail = b''
        if len(data) >= 10 and data[-10:-6] in (b'FC01', b'FC02'):
            original = struct.unpack('>H', data[-6:-4])[0]
            data = data[:original]; tail = 'contour'
        raw = extract.unrowxor_masks(unzx0(data))      # pack_view XORs the mask rows again
        out = extract.pack_view(raw)
        path.write_bytes(out)
        print(path.name, len(data), '->', len(out), 'bytes', '(contour tail to re-attach)' if tail else '')
    first = HERE / chapter / 'data' / ('%s0.bin' % PREFIX[chapter])
    subprocess.run([sys.executable, HERE / 'tools/contour_assets.py', '--attach', first], check=True)
