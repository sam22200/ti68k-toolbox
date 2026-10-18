#!/usr/bin/env python3
"""Convex planar proxies for transient low-detail camera motion; no tactical data changes."""
import collections
import json
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parents[1]


def prepare(destination, min_area2=3136):
    raw = (HERE/'fftmesh.bin').read_bytes()
    nv,nf,_,np = struct.unpack_from('>4H',raw)
    points = [struct.unpack_from('>3h',raw,2120+i*6) for i in range(nv)]
    faces = [struct.unpack_from('>3H2B',raw,2120+nv*6+i*8) for i in range(nf)]
    planes = [struct.unpack_from('>4h',raw,2120+nv*6+nf*8+i*8) for i in range(np)]
    extra = 2120+nv*6+nf*8+np*8; ne = struct.unpack_from('>H',raw,extra)[0]
    materials = raw[extra+2+ne*4:extra+2+ne*4+nf]
    edges = collections.defaultdict(list); parent = list(range(nf))
    for i,f in enumerate(faces):
        for a,b in zip(f[:3],(f[1],f[2],f[0])): edges[tuple(sorted((a,b)))].append(i)

    def root(i):
        while parent[i] != i: parent[i] = parent[parent[i]]; i = parent[i]
        return i

    for ids in edges.values():
        for a in ids:
            for b in ids:
                if faces[a][4] == faces[b][4] and materials[a] == materials[b]: parent[root(a)] = root(b)
    groups = collections.defaultdict(list)
    for i in range(nf): groups[root(i)].append(i)

    def cross(a,b,c): return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    vertices = []; lookup = {}; records = []; indices = []; discarded = 0
    for ids in groups.values():
        plane = faces[ids[0]][4]; normal = planes[plane][:3]
        omit = max(range(3),key=lambda k:abs(normal[k])); axes = [k for k in range(3) if k != omit]
        projected = {tuple(points[v][k] for k in axes):points[v] for i in ids for v in faces[i][:3]}
        ordered = sorted(projected); hull = []
        for sequence in [ordered,ordered[::-1]]:
            chain = []
            for p in sequence:
                while len(chain)>1 and cross(chain[-2],chain[-1],p)<=0: chain.pop()
                chain.append(p)
            hull.extend(chain[:-1])
        if len(hull)<3: continue
        area2 = abs(sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(hull,hull[1:]+hull[:1])))
        # Remove facade/trim fragments; the proxy intentionally loses small creases.
        if area2<min_area2: discarded += 1; continue
        world = [projected[p] for p in hull]
        a,b,c = world[:3]; u = [b[k]-a[k] for k in range(3)]; v = [c[k]-a[k] for k in range(3)]
        n = [u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
        if sum(n[k]*normal[k] for k in range(3))<0: world.reverse()
        assert len(world)<=24
        start = len(indices)
        for p in world:
            if p not in lookup: lookup[p]=len(vertices); vertices.append(p)
            indices.append(lookup[p])
        colour = [0,1,2,2,1,1,2,2][materials[ids[0]] & 7]
        records.append((start,len(world),colour,plane,round(256/len(world))))
    assert len(vertices)<=500 and len(records)<=128
    data = bytearray(struct.pack('>4H',len(vertices),len(records),len(indices),1))
    for p in vertices: data.extend(struct.pack('>3h',*p))
    for record in records: data.extend(struct.pack('>H4B',*record))
    data.extend(struct.pack('>'+str(len(indices))+'H',*indices))
    destination.write_bytes(data)
    report = dict(source_vertices=nv,source_triangles=nf,proxy_vertices=len(vertices),
                  proxy_convex_faces=len(records),discarded_small_patches=discarded,min_projected_area2=min_area2,bytes=len(data),
                  limitations='Convex hulls can close recesses/holes. Small trim is removed; painter ordering is approximate. Transition-only art, never collision/target geometry.')
    destination.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__ == '__main__':
    path = HERE/'x/ultra-transition'; path.mkdir(parents=True,exist_ok=True)
    print(json.dumps(prepare(path/'fftlod.bin'),indent=2))
