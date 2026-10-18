#!/usr/bin/env python3
"""Offline compatible-quad merge and conservative native turn face visibility."""
import hashlib
import json
from pathlib import Path
import re
import struct

import numpy as np

from turn_texture_assets import prepare as prepare_textures

HERE = Path(__file__).resolve().parents[1]


def project(vertices, sine, yaw, cx, cz, px=0, py=0, yshift=1, xshift=1):
    """Match the native integer projection, including both stages of rounding."""
    dx, height, dz = vertices.T.copy()
    dx -= cx * 28 + 14
    dz -= cz * 28 + 14
    sn, cs = int(sine[yaw]), int(sine[(yaw + 16) & 63])
    across = (dx * cs + dz * sn) >> 7
    forward = (dx * sn - dz * cs) >> 7
    return np.stack(((px + ((across * 155) >> 8)) >> xshift,
                     (py - (height >> 1) - ((forward * 77) >> 8)) >> yshift,
                     np.clip(128 - (forward >> 2) + (height >> 2), 1, 254)), axis=1)


def determinant(points):
    a, b = points[..., 1, :2] - points[..., 0, :2], points[..., 2, :2] - points[..., 0, :2]
    return a[..., 0] * b[..., 1] - a[..., 1] * b[..., 0]


def compatible(a, b, indices, vertices, uv, shades):
    """Merge consecutive opposite shared edges only; retain the first triangle basis."""
    if uv[a, 6] != uv[b, 6] or shades[a] != shades[b]:
        return None
    ia, ib = list(indices[a]), list(indices[b])
    if len(set(ia) | set(ib)) != 4:
        return None
    # First triangle [a,b,c], second [b,d,c]; rotate each triangle cyclically.
    for shift in range(3):
        ids = ia[shift:] + ia[:shift]
        first, left, right = ids
        for other in range(3):
            second = ib[other:] + ib[:other]
            if second[0] != left or second[2] != right:
                continue
            fourth = second[1]
            p = vertices[ids + [fourth]]
            n = np.cross(p[1] - p[0], p[2] - p[0])
            if not np.any(n) or np.dot(n, p[3] - p[0]) != 0:
                continue
            # A parallelogram keeps geometry and affine UVs exactly compatible offline.
            if not np.array_equal(p[3], p[1] + p[2] - p[0]):
                continue
            if uv[a, 6] != 255:
                ta, tb = uv[a, :6].reshape(3, 2).astype(int), uv[b, :6].reshape(3, 2).astype(int)
                ta = np.roll(ta, -shift, axis=0)
                tb = np.roll(tb, -other, axis=0)
                if not (np.array_equal(ta[1], tb[0]) and np.array_equal(ta[2], tb[2])
                        and np.array_equal(tb[1], ta[1] + ta[2] - ta[0])):
                    continue
            return ids + [fourth], shift
    return None


def prepare(destination, horizontal=False):
    destination.mkdir(parents=True, exist_ok=True)
    prepare_textures(destination)
    mesh = (HERE / 'fftmesh.bin').read_bytes()
    nv, nf = struct.unpack_from('>2H', mesh)
    vertices = np.frombuffer(mesh, '>i2', nv * 3, 2120).astype(np.int64).reshape(nv, 3)
    raw_faces = np.frombuffer(mesh, np.uint8, nf * 8, 2120 + nv * 6).reshape(nf, 8)
    indices = np.frombuffer(raw_faces[:, :6].tobytes(), '>u2').astype(int).reshape(nf, 3)
    uv = np.frombuffer((destination / 'fftuv.bin').read_bytes(), np.uint8, nf * 8, 8).reshape(nf, 8)
    sine = np.frombuffer(mesh, np.int8, 64, 8)
    shape = (HERE / 'map.h').read_text()
    width, height = [int(re.search(r'#define MAP_' + axis + r' (\d+)', shape)[1]) for axis in ['W', 'H']]
    records, sources, merged = [], [], 0
    i = 0
    while i < nf:
        pair = compatible(i, i + 1, indices, vertices, uv, raw_faces[:, 6]) if i + 1 < nf else None
        if pair is None:
            ids, shift, count = list(indices[i]) + [0], 0, 3
        else:
            ids, shift = pair
            count = 4
            merged += 1
        coords = np.roll(uv[i, :6].reshape(3, 2), -shift, axis=0).ravel().tobytes()
        # 22 bytes: four indices, first basis UVs/page/pad, shade/count, original pair IDs.
        records.append(struct.pack('>4H', *ids) + coords + bytes([int(uv[i, 6]), 0, int(raw_faces[i, 6]), count])
                       + struct.pack('>2H', i, i + 1 if count == 4 else i))
        sources.append([i, i + 1] if count == 4 else [i])
        i += 2 if count == 4 else 1
    # Enumerate every legal tile pivot and half-resolution anchor rounding phase.
    # Keep original primitive order: equal-depth and alpha ownership remain stable.
    shapes = [(2, 0)] if horizontal else [(1, 1), (1, 2)]
    masks = np.zeros((len(shapes), 16, nf), bool)
    for mode, (xshift, yshift) in enumerate(shapes):
        for angle in range(16):
            for cz in range(height):
                for cx in range(width):
                    for py in range(1 << yshift):
                        for px in range(1 << xshift):
                            points = project(vertices, sine, angle * 4, cx, cz, px, py, yshift, xshift)
                            masks[mode, angle] |= determinant(points[indices]) < 0
    record_masks = np.array([[np.any(mask[part]) for part in sources] for mask in masks.reshape(-1, nf)])
    # Header: magic/version/vertex count/triangle count/surface count/W/H/record bytes.
    # 32 directory entries (offset,count), records then ordered visibility lists.
    start = 16 + 32 * 4
    data = bytearray(struct.pack('>8H', 0x5453, 1, nv, nf, len(records), width, height, 22) + bytes(32 * 4))
    data.extend(b''.join(records))
    for slot, mask in enumerate(record_masks):
        selected = np.flatnonzero(mask).tolist()
        struct.pack_into('>2H', data, 16 + slot * 4, len(data), len(selected))
        data.extend(struct.pack('>' + 'H' * len(selected), *selected))
    if horizontal:
        # Keep the existing 32-entry directory layout; aliases need no extra list data.
        data[80:144] = data[16:80]
    assert len(data) < 65514
    bank = 'fftsurfx' if horizontal else 'fftsurf'
    (destination / (bank + '.bin')).write_bytes(data)
    report = dict(
        original_triangles=nf, merged_pairs=merged, surfaces=len(records), bytes=len(data),
        sha256=hashlib.sha256(data).hexdigest(), mesh_sha256=hashlib.sha256(mesh).hexdigest(),
        visibility='Conservative union of exact integer facing over every legal tile pivot and all anchor rounding phases; no occlusion pruning.',
        pivots=width * height, angles=16, resolutions=['40x100'] if horizontal else ['80x50', '80x25'],
        visible_surfaces_per_angle=record_masks.sum(1).reshape(len(shapes), 16).tolist(),
        bank=bank,
        limits='Merged affine surfaces can differ at rounded screen edges, UV and quantized-depth seams; original pair fallback remains available.',
    )
    (destination / 'surface-assets.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return report


if __name__ == '__main__':
    prepare(HERE / 'x/turn-surface/assets')
