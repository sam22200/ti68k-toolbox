#!/usr/bin/env python3
"""Lossless projected-block storage control, preserving all reviewed source pixels.

This is a storage experiment, not a claim that arbitrary pixel blocks form a good
semantic material bank. Coverage streams are retained exactly and charged in full.
"""
import hashlib
import json
import struct
import numpy as np
from build import OUT,extract

def packed(raw):
    cache=OUT/'compression';cache.mkdir(exist_ok=True)
    p=cache/hashlib.sha256(raw).hexdigest()
    if not p.exists():p.write_bytes(extract.zx0pack.pack(raw))
    return p.read_bytes()

def main():
    views=[];cover_bytes=0
    for name in ('gariland','orbonne'):
        for rot in range(4):
            raw=(OUT/(name+str(rot)+'-original.raw')).read_bytes();n=len(raw)//6
            cover=packed(raw[2*n:]);cover_bytes+=len(cover)
            views.append((name,rot,np.frombuffer(raw[:2*n],np.uint8).reshape(2,n//40,40)))
    results=[]
    for height in (1,2,4,8):
        sprites=[];lookup={};uses=[set(),set()];command_bytes=0;commands=[]
        for name,rot,planes in views:
            planes=np.pad(planes,((0,0),(0,(-planes.shape[1])%height),(0,0)))
            ids=[]
            for y in range(0,planes.shape[1],height):
                for x in range(40):
                    tile=planes[:,y:y+height,x].tobytes()
                    if tile not in lookup:lookup[tile]=len(sprites);sprites.append(tile)
                    id=lookup[tile];ids.append(id);uses[name=='orbonne'].add(id)
            raw=struct.pack('>'+str(len(ids))+'H',*ids);stream=packed(raw)
            commands.append(stream);command_bytes+=len(stream)
            # Exact reconstruction checks padding and both bitplanes.
            reconstructed=np.zeros_like(planes)
            for i,id in enumerate(ids):
                y=(i//40)*height;x=i%40
                reconstructed[:,y:y+height,x]=np.frombuffer(sprites[id],np.uint8).reshape(2,height)
            assert np.array_equal(planes,reconstructed)
        bank=b''.join(sprites);raw_bank=len(bank)+8
        # Four descriptor offsets/map, two stream offsets/view, and bank header.
        total=raw_bank+32+32+command_bytes+cover_bytes
        record=dict(block_size=[8,height],unique_sprites=len(sprites),
                    exact_shared_sprites_between_maps=len(uses[0]&uses[1]),
                    common_bank_read_in_place_bytes=raw_bank,
                    compressed_command_bytes=command_bytes,compressed_coverage_bytes=cover_bytes,
                    total_bytes=total,all_eight_views_reconstructed_exactly=True)
        results.append(record);print(json.dumps(record),flush=True)
    (OUT/'blocks.json').write_text(json.dumps(results,indent=2)+'\n')
    snapshot=OUT.parent/'RESULTS.json'
    if snapshot.exists():
        data=json.loads(snapshot.read_text());data['lossless_blocks']=results
        snapshot.write_text(json.dumps(data,indent=2)+'\n')

if __name__=='__main__':main()
