#!/usr/bin/env python3
"""Local FFT mesh -> indexed, flat shaded perspective bank. No ROM data in git."""
from pathlib import Path
import re
import math
import struct
import subprocess
import sys
import numpy as np
import extract


def main():
    disc, files = extract.disc_files(sys.argv[1])
    records = extract.gns_records(disc, files)
    _, sector, size = next(r for r in records if r[0] == 0x2e01)
    mesh = disc.read(sector, size)
    terrain = struct.unpack_from('<I', mesh, 0x68)[0]
    nx, nz = mesh[terrain:terrain+2]
    _, sector, size = next(r for r in records if r[0] == 0x1701)
    raw = np.frombuffer(disc.read(sector, size), np.uint8)
    tex = np.empty((1024, 256), np.uint8)
    tex[:, ::2] = (raw & 15).reshape(1024, 128)
    tex[:, 1::2] = (raw >> 4).reshape(1024, 128)
    pal = np.array(struct.unpack_from('<256H', mesh, struct.unpack_from('<I', mesh, 0x44)[0])).reshape(16, 16)
    lum = ((pal & 31) * .299 + ((pal >> 5) & 31) * .587 + ((pal >> 10) & 31) * .114) / 31
    verts, lookup, faces, planes, plane_lookup = [], {}, [], [], {}
    materials, adjacency, edge_faces = [], {}, {}
    for points, uv, palette, page in extract.polygons(mesh):
        indices = []
        for x, y, z in points:
            key = (int(x), int(-y), int(z))
            if key not in lookup:
                lookup[key] = len(verts)
                verts.append(key)
            indices.append(lookup[key])
        # Average samples inside the original UV triangle, rather than one texel.
        samples = []
        if uv:
            for a in range(9):
                for b in range(9-a):
                    u, v = np.rint((np.array(uv[0])*a + np.array(uv[1])*b + np.array(uv[2])*(8-a-b))/8).astype(int)
                    samples.append(lum[palette, tex[min(1023, v+256*page), min(255, u)]])
        value = float(np.mean(samples)) if samples else .2
        shade = int(np.clip(round((1-value)*2.5), 0, 2))
        world = np.array([verts[i] for i in indices], dtype=np.int64)
        normal = np.cross(world[1]-world[0], world[2]-world[0])
        divisor = math.gcd(*map(int, normal))
        if not divisor:
            continue
        normal //= divisor
        plane = (*map(int, normal), int(normal @ world[0]))
        if plane not in plane_lookup:
            plane_lookup[plane] = len(planes)
            planes.append(plane)
        faces.append((*indices, shade, plane_lookup[plane]))
        # Quiet, directional materials for the native fixed view. Keep the previous
        # texture means in face byte 6 for the original three-camera experiment.
        if normal[1] > 0:
            material = 2 if (normal[0] or normal[2]) and world[:, 1].mean() >= 36 else 0
            tx, tz = int(world[:, 0].mean() // 28), int(world[:, 2].mean() // 28)
            if 0 <= tx < nx and 0 <= tz < nz:
                tile = mesh[terrain+2+8*(tz*nx+tx):terrain+10+8*(tz*nx+tx)]
                surface = 12*(tile[2]+(tile[3] >> 5))
                if tile[0] & 63 == 14 and not normal[0] and not normal[2] and np.all(world[:, 1] == surface):
                    material = 5          # visible water surface, above the submerged floor
        else:
            material = 1 if normal[0]+normal[2] >= 0 else 2
        materials.append(material)
        for a, b in zip(indices, indices[1:]+indices[:1]):
            adjacency.setdefault(tuple(sorted((a, b))), []).append(plane_lookup[plane])
            edge_faces.setdefault(tuple(sorted((a, b))), []).append(len(faces)-1)
    tool = Path(__file__).resolve().parents[3] / 'tools/bin/ti-table'
    def table(*args):
        source = subprocess.check_output([str(tool), *args], text=True)
        return list(map(int, re.findall(r'-?\d+', source.split('{', 1)[1].split('}', 1)[0])))
    sine = table('sin', '--n', '64', '--scale', '127')
    recip = table('recip', '--n', '1024', '--shift', '16')
    # recip[1] does not fit a word; z >= 16 is the near plane.
    recip[1] = 65535
    assert len(verts) <= 500 and len(planes) <= 128
    out = bytearray(struct.pack('>4H', len(verts), len(faces), 4, len(planes)))
    out += struct.pack('>64b', *sine)
    out += struct.pack('>1024H', *recip)
    for v in verts:
        out += struct.pack('>3h', *v)
    for f in faces:
        out += struct.pack('>3H2B', *f)
    for plane in planes:
        out += struct.pack('>4h', *plane)
    edges = []
    for (a, b), ids in adjacency.items():
        normals = [np.array(planes[i][:3]) for i in ids]
        crease = any(float(x@y) < .95*np.linalg.norm(x)*np.linalg.norm(y)
                     for x in normals for y in normals)
        va, vb = verts[a], verts[b]
        ledge = len(ids) == 1 and va[1] == vb[1] and va[1] >= 36
        if crease or ledge:
            edges.append((a, b))
    out += struct.pack('>H', len(edges))
    for a, b in edges:
        out += struct.pack('>2H', a, b)
    out += bytes(materials)
    # Connected architecture above the ordinary street: hide complete components, not
    # individual triangles (which would leave diagonal holes in walls). Shared ground
    # edges alone must not join neighboring buildings. Water/street faces stay group 0.
    architecture = [not materials[i] & 4 and max(verts[v][1] for v in f[:3]) > 36
                    for i, f in enumerate(faces)]
    parent = list(range(len(faces)))
    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for (a, b), ids in edge_faces.items():
        ids = [i for i in ids if architecture[i]]
        if max(verts[a][1], verts[b][1]) > 24:
            for i in ids[1:]:
                parent[root(i)] = root(ids[0])
    components = {}
    groups = []
    for i in range(len(faces)):
        if not architecture[i]:
            groups.append(0)
            continue
        key = root(i)
        if key not in components:
            components[key] = len(components)+1
        groups.append(components[key])
    assert len(components) < 128
    out += struct.pack('>H', len(components))
    for g in range(1, len(components)+1):
        pts = np.array([verts[v] for i, f in enumerate(faces) if groups[i] == g for v in f[:3]])
        box = tuple(int(v) for axis in (0, 2, 1) for v in (pts[:, axis].min(), pts[:, axis].max()))
        out += struct.pack('>6h', *box)
    out += bytes(groups)
    out += bytes(next((groups[i] for i in edge_faces[e] if groups[i]), 0) for e in edges)
    Path('fftmesh.bin').write_bytes(out)
    print(f'perspective: {len(verts)} vertices, {len(faces)} triangles, {len(edges)} creases, '
          f'{len(components)} cutaway components, {len(out)} archived bytes')


if __name__ == '__main__':
    main()
