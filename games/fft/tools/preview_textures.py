#!/usr/bin/env python3
"""Local PC atlas/UV bank for arbitrary textured camera transitions (not TI assets)."""
import hashlib
import json
import struct

import numpy as np
from attack_texture import load_scene
from texture_review import HERE


def prepare_preview():
    faces, tex, palettes, thresholds, strong, water = load_scene()
    q = [round(float(v)*256) for v in [*thresholds,*strong]]
    data = bytearray(struct.pack('>10H',0x5458,1,len(faces),0,*q))
    for i,(_,uv,palette,page) in enumerate(faces):
        data += bytes([*np.asarray(uv,dtype=np.uint8).ravel(),palette,page|(128 if water[i] else 0)]) if uv is not None else bytes([0]*6+[255,page])
    lum = ((palettes & 31)*.299+((palettes >> 5) & 31)*.587+((palettes >> 10) & 31)*.114)*255/31
    words = np.rint(lum*256).astype('>u2'); words[palettes == 0] = 65535
    data += words.tobytes()
    data += (tex[:,::2] | (tex[:,1::2] << 4)).tobytes()
    assert len(data) == 20+len(faces)*8+512+131072
    (HERE/'ffttex.bin').write_bytes(data)
    info = dict(scope='Local PC reference sampler, original atlas + UVs; no stored camera images.',
                faces=len(faces),bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
    out = HERE/'x/textured-transitions'; out.mkdir(parents=True,exist_ok=True)
    (out/'atlas.json').write_text(json.dumps(info,indent=2)+'\n')
    return info


if __name__ == '__main__':
    print(json.dumps(prepare_preview(),indent=2))
