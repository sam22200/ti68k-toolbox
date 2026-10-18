#!/usr/bin/env python3
"""Lossless per-frame compression candidates, not a new runtime sprite cache."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import sys
import subprocess
import tempfile

import storage_audit as audit
sys.path.insert(0,str(audit.ROOT/'lib'))
import zx0pack


def main():
    root=audit.HERE; out=audit.OUT/'sprite-blocks'; out.mkdir(parents=True,exist_ok=True)
    # Preserve the uncompressed dictionary as the control after the cache is integrated.
    weapons=(root/'x/storage/before-blade-cache/fftw.bin').read_bytes()
    groups={'ordinary_and_loot':((root/'fftu.bin').read_bytes(),208),
            'armed_bodies':(weapons[:19968],208), 'unique_blades':(weapons[20016:],432)}
    result={}; samples={}
    for group,(raw,stride) in groups.items():
        frames=audit.chunks(raw,stride)
        def pack(item):
            i,frame=item; path=out/(group+'-%03u.zx0'%i)
            if not path.exists(): path.write_bytes(zx0pack.pack(frame))
            else:
                with tempfile.TemporaryDirectory() as tmp:
                    back=Path(tmp)/'decoded'
                    subprocess.run([zx0pack.DZX0,'-f',str(path),str(back)],check=True,
                                   stdout=subprocess.PIPE)
                    if back.read_bytes()!=frame: path.write_bytes(zx0pack.pack(frame))
            return path.read_bytes()
        with ThreadPoolExecutor(max_workers=3) as workers:
            encoded=list(workers.map(pack,enumerate(frames)))
        sizes=list(map(len,encoded)); directory=2*(len(frames)+1)
        result[group]=dict(raw_bytes=len(raw),frames=len(frames),frame_bytes=stride,
                           compressed_frame_bytes=sum(sizes),directory_bytes=directory,
                           candidate_bytes=sum(sizes)+directory,
                           saved_bytes=len(raw)-sum(sizes)-directory,
                           smallest_frame=min(sizes),largest_frame=max(sizes))
        for label,i in [('first',0),('largest',sizes.index(max(sizes)))]:
            name=group+'_'+label
            (audit.OUT/(name+'.zx0')).write_bytes(encoded[i]); samples[name]=frames[i]
    decoded=audit.decode_bench(samples)
    report=dict(groups=result,native_sample_decode=decoded,
                limits='Per-frame streams are candidates only. Largest means encoded size, not proven worst decode cycles. No runtime cache, eviction or all-actors miss benchmark is implemented.',
                memory_examples=dict(one_frame_per_eleven_units=11*208,one_blade_cache=432),
                mirror_ram_bytes=37440)
    (audit.OUT/'sprite-exploration.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__': main()
