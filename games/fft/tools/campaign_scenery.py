#!/usr/bin/env python3
"""Restore the selected Gariland ISO style in source-handed chapter banks.

Only colours change; source geometry, depth and cover bytes are asserted equal.
The historical original views remain extraction controls, never active defaults.
"""
from pathlib import Path
import subprocess
import tempfile

import numpy as np
from PIL import Image
import extract
import isometric_style as style

HERE = Path(__file__).resolve().parents[1]


def view(rot):
    selected = HERE / f'x/isometric-style/variant-{rot}.raw'
    if not selected.exists():
        import isometric_style
        isometric_style.prepare_iso()
    with tempfile.TemporaryDirectory() as tmp:
        raw = Path(tmp) / 'original.raw'
        subprocess.run([HERE.parents[1]/'tools/bin/dzx0','-f',HERE/f'fftv{rot}.bin',raw],
                       check=True,capture_output=True)
        original = raw.read_bytes()
    styled = selected.read_bytes()
    assert len(styled) == len(original) and len(styled) % 240 == 0
    plane = len(styled)//6
    assert styled[2*plane:] == original[2*plane:], ('depth/mask changed',rot)
    cache=selected.parent
    grey=((255-np.array(Image.open(cache/f'contrast-{rot}.png').convert('L')).astype(np.uint16))//85).astype(np.uint8)
    mask=np.array(Image.open(cache/f'mask-{rot}.png').convert('L'))!=0
    assert style.pack(style.edge_colours(grey,mask))==styled[:2*plane_size(styled)]
    return chapter_view(styled,rot)


def plane_size(raw):
    assert len(raw)%6==0
    return len(raw)//6


def chapter_view(styled,rot):
    """Shared source-handed reflection/crop, also used by isolated colour reviews."""
    return extract.pack_view(chapter_planes(styled,rot))


def chapter_planes(styled,rot):
    planes = np.frombuffer(styled,np.uint8).reshape(6,-1,40)[:,:,::-1].copy()
    rev = np.array([int(f'{i:08b}'[::-1],2) for i in range(256)],np.uint8)
    for k in (0,1,3,5):
        planes[k] = rev[planes[k]]
    assert not planes[:,:16].any(),('scenery above crop',rot)
    return planes[:,16:].tobytes()


def dual_view(one,two,rot):
    """R3: 1px colours, 2px width mask, shared ragged depth (extract.view_bank)."""
    one=chapter_planes(one,rot);two=chapter_planes(two,rot);size=plane_size(one)
    assert one[2*size:]==two[2*size:]
    mask=bytes((a^b)|(c^d) for a,b,c,d in zip(one[:size],two[:size],one[size:2*size],two[size:2*size]))
    # At every changed pixel the adaptive edge is the opposite of its source dark bit.
    expected=bytes((a&~m)|(~b&m) for a,b,m in zip(one[:size],one[size:2*size],mask))
    expected+=bytes(a^m for a,m in zip(one[size:2*size],mask))
    assert expected==two[:2*size]
    colours=extract.zx0pack.pack(one[:2*size]);stroke=extract.zx0pack.pack(mask)
    return extract.view_bank(b'R3',colours,stroke,extract.ragged_depth(one))


def main():
    changed=[]
    for rot in range(4):
        p=HERE/f'campaign/data/ffgv{rot}.bin'
        fresh=view(rot)
        previous=p.read_bytes() if p.exists() else b''
        # ffv0 may carry archived UI tables; contour_assets reattaches them below.
        if previous[:len(fresh)] != fresh:
            p.write_bytes(fresh);changed.append(rot)
    print('Gariland selected contrast/edges: restored views',changed,'; original depth/masks preserved')


if __name__=='__main__':
    main()
