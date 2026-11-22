#!/usr/bin/env python3
"""Source-correct town speakers and the Bar room, for an isolated visual review.

No shipping bank is rewritten. WORLD's four WLDFACE uploads and its portrait
texture routine define the page/CLUT layout; the user's reference pictures
identify the four service faces. The existing shop extractor's linear atlas
and palette-zero interpretation must not be reused for these records.
"""
import struct

import numpy as np
from PIL import Image

import interface
import shop_scene

SPEAKERS = {'store': 114, 'office': 115, 'fur': 116, 'bar': 117}
PAGE_BYTES = 0x8000


def face_source(raw, index, palette=0):
    """The original 32x48 indices and BGR555 CLUT, before TI adaptation.

    OBSERVED: world_gfx_load_wldface_to_frame_buffer uploads four 256x256
    4bpp pages. world_gfx_set_portrait_poly_texture uses forty faces per page,
    with the CLUTs at y240; faces63..82 have a separate palette block at250.
    """
    if len(raw) != 0x20000 or not 0 <= index < 160 or not 0 <= palette < 4:
        raise ValueError('invalid WLDFACE page, face or palette')
    page, cell = divmod(index, 40)
    packed = np.frombuffer(raw, np.uint8, count=PAGE_BYTES,
                           offset=page * PAGE_BYTES).reshape(256, 128)
    pixels = np.empty((256, 256), np.uint8)
    pixels[:, ::2] = packed & 15
    pixels[:, 1::2] = packed >> 4
    row, col = divmod(cell, 8)
    face = pixels[row * 48:(row + 1) * 48, col * 32:(col + 1) * 32]
    if 63 <= index <= 82:
        pal_page, pal_row = divmod(index - 63, 6)
        pal_at = pal_page * PAGE_BYTES + (250 + pal_row) * 128 + palette * 32
    else:
        pal_row, pal_col = divmod(cell, 4)
        pal_at = page * PAGE_BYTES + (240 + pal_row) * 128 + pal_col * 32
    colors = np.frombuffer(raw, '<u2', count=16, offset=pal_at).astype(int)
    return face, colors


def portrait(raw, index):
    """Keep the shared 32x29 record, 17x27 face, outline and four-grey mapping."""
    face, colors = face_source(raw, index)
    small = np.asarray(Image.fromarray(face).resize((17, 27), Image.Resampling.NEAREST))
    grey = np.full((29, 32), -1, dtype=int)
    grey[1:28, 1:18] = shop_scene.face_greys(small, colors)
    interface.outline(grey)
    return bytes(interface.planes(grey, 32))


def corrected_rooms(current, faces):
    """Replace only three existing FSK4 portrait records; preserve every room byte."""
    if current[:6] != b'FSK4\x03\x00':
        raise ValueError('review expects the three-room FSK4 bank')
    out = bytearray(current)
    for slot, role in enumerate(('store', 'office', 'fur')):
        at = struct.unpack_from('>H', current, 6 + 2 * slot)[0]
        end = struct.unpack_from('>H', current, 8 + 2 * slot)[0]
        if at & 1 or end - at != 348:
            raise ValueError('invalid/alignment-unsafe portrait record')
        out[at:end] = portrait(faces, SPEAKERS[role])
    return bytes(out)


def bar_source(raw):
    """WLDBK set0: sixty CD sectors, one raw256x240 BGR555 background.

    OBSERVED: wldcore_bar_push_menu_level chooses set0 (or22/23 for the other
    town styles); wldcore_gfx_load_background_set uses its sector directory.
    It is a stored picture with its people, not a new room to synthesize.
    """
    if len(raw) < 60 * 2048:
        raise ValueError('truncated WLDBK Bar image')
    p = np.frombuffer(raw, '<u2', count=256 * 240).reshape(240, 256).astype(int)
    return np.stack([p >> shift & 31 for shift in (0, 5, 10)], axis=2)


def bar_greys(rgb):
    """Review proposal: unscaled160x100 window retaining the owner and Ramza.

    ADAPTED crop(50,65), percentile15/97 and gamma1.2; no actor relocation.
    This mapping/crop is pending the user's visual decision.
    """
    crop = rgb[65:165, 50:210]
    lum = crop[:, :, 0] * .299 + crop[:, :, 1] * .587 + crop[:, :, 2] * .114
    lo, hi = np.percentile(lum, (15, 97))
    norm = np.clip((lum - lo) / max(hi - lo, 1e-6), 0, 1) ** 1.2
    return 3 - np.floor(norm * 3 + .5).astype(int)
