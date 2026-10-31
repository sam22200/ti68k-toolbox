#!/usr/bin/env python3
"""Focused original-instruction recoil cases, not an exhaustive map sweep."""
import argparse,json,struct
from pathlib import Path
import surface_data as surface

surface.FUNCTIONS.update(knockback=(0x80184610,712,'fb9351b2bfdb7550'),
    location=(0x801810a0,52,'3200325a78320b2b'),
    targetable=(0x8017dc88,32,'42a1cc17fbffa4a8'),
    find=(0x8017dca8,552,'60e500d728c8441b'))


def layer(height,half=0,slope=0,depth=0,blocked=False):
    return bytes((height,half,slope,depth|(128 if blocked else 0)))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=surface.HERE/'campaign/x/core-knockback/oracle')
    out=ap.parse_args().out;out.mkdir(parents=True,exist_ok=True)
    original=surface.Original(surface.battle.Disc(str(surface.core_audit.DEFAULT_DISC)))
    original.uc.mem_write(0x1908cc,bytes(21*0x1c0))
    original.write(0x800e4e9c,2);original.write(0x800e4ea0,1);original.write(0x80192d98,0x801d7000)
    original.uc.mem_write(0x1d7047,b'\x00');original.uc.mem_write(0x1d7048,b'\x00\x00')
    # Cover distinct choices and exact headroom boundaries, not a Cartesian sweep.
    fixtures=[('lower_drop',layer(4),layer(2)+layer(0,blocked=True),3,0),
        ('upper_wins',layer(4),layer(1)+layer(3),3,0),
        ('equal_prefers_lower',layer(4),layer(3)+layer(3),3,0),
        ('both_above',layer(2),layer(4)+layer(5),3,0),
        ('upper_clearance_equal',layer(3),layer(0)+layer(6,depth=2),3,0),
        ('upper_clearance_short',layer(3),layer(0)+layer(6,depth=3),3,0),
        ('lower_ceiling_equal',layer(3),layer(6,depth=2)+layer(0),3,0),
        ('lower_ceiling_short',layer(3),layer(6,depth=3)+layer(0),3,0),
        ('lower_blocked',layer(5),layer(0,blocked=True)+layer(2),3,0),
        ('both_blocked',layer(5),layer(1,blocked=True)+layer(2,blocked=True),3,0),
        ('source_slope',layer(2,half=3,slope=0xe4),layer(2)+layer(0,blocked=True),3,0),
        ('destination_slope',layer(3),layer(2,half=3,slope=0x1b)+layer(0,blocked=True),3,0),
        ('water_geometry',layer(4,half=64),layer(2,half=32)+layer(0,blocked=True),3,0),
        ('occupied_upper_no_fallback',layer(4),layer(1)+layer(3),3,2),
        ('occupied_lower_only',layer(4),layer(1)+layer(3),3,1),
        ('untargetable_upper_no_fallback',layer(4),layer(1)+layer(3),1,0),
        ('negative_underside',layer(0),layer(0)+layer(1,depth=5),3,0)]
    records=[];rows=[]
    for name,src,dst,legal,occupied in fixtures:
        for side in (0,2,6,4):
            for index,col in ((0,src+layer(0,blocked=True)),(1,dst)):
                for level in range(2):
                    h,dh,slope,flags=col[4*level:4*level+4]
                    raw=bytes((0,0,h,dh,slope,flags&31,(flags>>7)|(2 if index==1 and not legal&(1<<level)else 0),0))
                    original.uc.mem_write((surface.TILES&0x1fffff)+8*(index+256*level),raw)
            for i in range(21):original.uc.mem_write(0x1908cc+i*0x1c0+1,b'\xff')
            for level in range(2):
                if occupied&(1<<level):
                    at=0x1908cc+level*0x1c0;original.uc.mem_write(at+1,b'\x00')
                    original.uc.mem_write(at+0x47,b'\x01');original.uc.mem_write(at+0x48,struct.pack('<H',level<<15))
            original.uc.mem_write(0x1938ee,b'\xee\xdd')
            direction={0:3,2:1,6:2,4:0}[side]
            value=original.call('knockback',direction,1,0);selected=0 if value==0xffffffff else value+1
            assert selected in (0,1,2)
            fall=original.uc.mem_read(0x1938ee,1)[0]if selected else 0
            records.append(src+dst+bytes((side,legal,occupied,selected,fall,0)))
            rows.append(dict(name=name,side=side,selected=selected,fall=fall))
    (out/'fkb00.bin').write_bytes(b'FKB1'+struct.pack('>H',len(records))+b''.join(records))
    report=dict(cases=len(records),case_bytes=18,functions=original.function_hashes,
                unicorn=original.version,rows=rows,scope='Original recoil routine plus original occupancy lookup, explicit fixture RAM; no full battle/render observation.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Focused original recoil cases:',len(records),flush=True)


if __name__=='__main__':main()
