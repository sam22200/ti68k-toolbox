#!/usr/bin/env python3
"""Review-only source egg art. No installed bank or renderer is changed.

WORLD's g_world_formation_shadow_egg_sprites supplies the body/crack UVs.
world_menu_load_common_graphics loads EVENT/UNIT.BIN's second32768-byte page
at VRAM(320,0),64x256 words. world_formation_draw_egg_sprite selects its four
CLUTs at(320+16*color,255), and overlays the crack when countdown<2.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image
import extract
import interface


def build(cue):
    disc, files = extract.disc_files(str(cue))
    with disc.f:
        source = disc.read(*files['EVENT/UNIT.BIN'])
        world = disc.read(*files['WORLD/WORLD.BIN'])
    assert len(source) == 65536
    assert struct.unpack_from('<8H', world, 0x8ED94) == (256, 0, 64, 256, 320, 0, 64, 256)
    page = np.frombuffer(source[32768:], np.uint8).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8)
    sheet[:, ::2] = page & 15; sheet[:, 1::2] = page >> 4
    palette = np.frombuffer(source[-128:], '<u2').reshape(4, 16).astype(int)
    layers = []
    for i in (1, 2):
        _, _, w, h, u, v, uw, vh, _, tpage = struct.unpack_from('<10H', world, 0xAC704+20*i)
        assert w == uw and h == vh and tpage == 0x65
        layers.append(sheet[v:v+h, u:u+w].copy())
    assert layers[0].shape == (24, 18) and layers[1].shape == (16, 16)
    frames = []
    for color in range(4):
        # Four original hues collapse unevenly on the LCD: the green egg can
        # otherwise become a solid dark blob. Fit each body's existing shades
        # to the same four-grey contrast range, keeping the source pixels/mask.
        pal=palette[color]
        lum=(pal&31)*.299+(pal>>5&31)*.587+(pal>>10&31)*.114
        used=np.unique(layers[0]);used=used[used!=0]
        lo,hi=lum[used].min(),lum[used].max()
        fitted=(lum-lo)*31/(hi-lo)
        shades=np.where(fitted<=7,3,np.where(fitted<=16,2,np.where(fitted<=22,1,0)))
        for cracked in (False, True):
            pixels = layers[0].copy()
            if cracked:
                patch = pixels[:16, :16]; crack = layers[1]
                patch[crack != 0] = crack[crack != 0]
            scaled = np.array(Image.fromarray(pixels).resize((12, 16), Image.Resampling.NEAREST))
            im = np.full((26, 16), -1, dtype=int)
            im[8:24, 2:14] = np.where(scaled==0,-1,shades[scaled])
            frames.append(interface.outline(im))
    body = b'FEG1'+bytes((8, 16, 26, 0))+b''.join(interface.planes(im, 16) for im in frames)
    proof = dict(review_only=True, bytes=len(body), source_sha256=hashlib.sha256(source).hexdigest(),
                 descriptors_sha256=hashlib.sha256(world[0xAC704:0xAC704+60]).hexdigest(),
                 scale='18x24 source body ->12x16, outlined inside16x26 formation cell',
                 contrast='per-color source-body luminance range fitted to four greys',
                 layout='header8; four colors x intact/cracked; each16x26 three BE planes156bytes')
    return body, proof


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('cue'); ap.add_argument('out', type=Path)
    args = ap.parse_args(); bank, proof = build(args.cue)
    args.out.parent.mkdir(parents=True, exist_ok=True); args.out.write_bytes(bank)
    args.out.with_suffix('.json').write_text(json.dumps(proof, indent=2)+'\n')
    print(json.dumps(proof, indent=2))


if __name__ == '__main__': main()
