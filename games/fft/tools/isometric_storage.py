#!/usr/bin/env python3
"""Measure original/contrast/edge ZX0 banks without replacing the original assets."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import struct

from texture_review import HERE,extract

OUT = HERE/'x/isometric-storage'


def job(task):
    name,rot,raw = task
    folder = OUT/name; folder.mkdir(parents=True,exist_ok=True)
    packed = extract.zx0pack.pack(raw)  # verifies decoder round-trip before returning
    (folder/f'fftv{rot}.bin').write_bytes(packed)
    return name,rot,len(packed),hashlib.sha256(raw).hexdigest()


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    bank = (HERE/'fftiso.bin').read_bytes()
    _,version,rowbytes,height = struct.unpack_from('>4H',bank)
    assert version == 1; plane = rowbytes*height
    tasks = []; original = []; original_hashes = []
    for rot in range(4):
        raw = (HERE/f'x/isometric-style/original-{rot}.bin').read_bytes()
        assert len(raw) == plane*6
        original.append((HERE/f'fftv{rot}.bin').stat().st_size)
        original_hashes.append(hashlib.sha256(raw).hexdigest())
        for style,name in [(0,'contrast'),(1,'edges')]:
            start = 8+(style*8+rot*2)*plane
            candidate = bank[start:start+2*plane]+raw[2*plane:]
            assert len(candidate) == len(raw) and candidate[2*plane:] == raw[2*plane:]
            tasks.append((name,rot,candidate))
    sizes = dict(original=original,contrast=[0]*4,edges=[0]*4)
    hashes = dict(original=original_hashes,contrast=['']*4,edges=['']*4)
    with ThreadPoolExecutor(max_workers=4) as workers:
        for name,rot,size,sha in workers.map(job,tasks):
            sizes[name][rot] = size; hashes[name][rot] = sha
            print(name,rot,size,flush=True)
    total = {name:sum(values) for name,values in sizes.items()}
    result = dict(codec='Project ZX0 v2, verified round-trips, full six-plane views; bare stream bytes.',
                  bytes_per_view=sizes,total_bytes=total,
                  edges_vs_contrast=total['edges']-total['contrast'],edges_vs_original=total['edges']-total['original'],
                  raw_total_bytes=plane*6*4,raw_size_delta=0,occlusion_data_delta=0,
                  runtime_edge_draw_calls=0,production_ram_delta=0,
                  scope='Select and pack one style; PC three-style comparison bank is not production overhead.',
                  raw_sha256=hashes)
    (OUT/'storage.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
