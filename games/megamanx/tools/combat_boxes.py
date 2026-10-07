#!/usr/bin/env python3
"""Small ROM hitboxes and constant-time age profiles, verified by source probes."""
import json
from reference import GAME,OUT

def signed(v):return v if v<128 else v-256
def main():
    data=json.loads((OUT/'collision/reference.json').read_text())
    boxes=[(0,0,0,0)]
    def index(raw):
        value=(signed(raw[0]),signed(raw[1]),raw[2],raw[3])
        if value not in boxes:boxes.append(value)
        return boxes.index(value)
    ids={k:index(v) for k,v in data['boxes'].items()};profiles=[]
    for p in data['profiles']:
        values=[index(r['box']) for r in p['rows']]
        repeat=next(( (start,period) for start in range(len(values)) for period in range(1,17)
                     if len(values)-start>=period*3 and all(values[i]==values[start+(i-start)%period] for i in range(start,len(values)))),None)
        assert repeat,(p['kind'],'flight period not established')
        start,period=repeat
        profiles.append((p['kind'],[values[i] if i<len(values) else values[start+(i-start)%period] for i in range(256)]))
        print('Hitbox profile',p['kind'],'prefix',start,'period',period)
    # Probe data confirms the descriptor interpretation and facing inversion.
    armor=data['boxes']['armor']
    for case in data['cases']:
        pointer={0:0xbe1d,1:0xbe31,3:0xbea4}[case['kind']]
        raw=next(r['box'] for p in data['profiles'] if p['kind']==case['kind'] for r in p['rows'] if r['pointer']==pointer)
        ox=signed(raw[0])*(1 if case['leftward'] else -1);rx=raw[2]+armor[2];ry=raw[3]+armor[3]
        assert case['actual_bounds']=={'x':[-rx-ox,rx-ox],'y':[-ry-signed(raw[1]),ry-signed(raw[1])]},case
    lines=['/* Generated local ROM hitboxes; ignored. */','typedef struct { s8 x,y; u8 rx,ry; } MxBox;']
    lines+= [f'#define MXBOX_{k.upper()} {v}' for k,v in ids.items()]
    lines+=['static const MxBox mx_boxes[]={']+['{'+','.join(map(str,b))+'},' for b in boxes]+['};']
    for kind,values in profiles:
        if kind==0:
            assert len(set(values))==1
            lines.append(f'#define MXBOX_PELLET {values[0]}');continue
        lines.append(f'static const u8 mx_shotbox_{kind}[256]={{')
        lines+= [','.join(map(str,values[i:i+32]))+',' for i in range(0,256,32)]
        lines.append('};')
    (GAME/'generated/combat_boxes.h').write_text('\n'.join(lines)+'\n')
    print(len(boxes),'four-byte descriptors; two 256-byte charged profiles')
if __name__=='__main__':main()
