#!/usr/bin/env python3
"""Focused full original horizontal-spread fixtures; geometry only, eastward lane."""
import argparse,json,struct
from pathlib import Path
import step_oracle
s=step_oracle.s
s.FUNCTIONS.update(long_spread=(0x801764d8,476,'87e689eaa38fd655'),
    middle=(0x8017622c,684,'551a8a31fcf7897c'),
    east=(0x801754d0,36,'21380e6d5d58f243'),
    coordinates=(0x801755e8,48,'3685b0b0b943fe6a'),
    bounds=(0x80175618,100,'817c8e48177c2808'),
    movement_config=(0x80174430,1884,'24f5bfdb9efb73c1'))
floor,col=step_oracle.floor,step_oracle.col


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/core-long-jump/oracle')
    out=ap.parse_args().out;out.mkdir(parents=True,exist_ok=True)
    o=s.Original(s.battle.Disc(str(s.core_audit.DEFAULT_DISC)));o.instruction_limit=100000
    o.write(0x8018f4f0,0x801d6000);o.uc.mem_write(0x1d5000,bytes([1]*64))
    o.write(0x800e4e9c,5);o.write(0x800e4ea0,1)
    # Verify the original callback table, never inject executable instructions.
    assert struct.unpack('<I',o.uc.mem_read(0x18f500,4))[0]==s.FUNCTIONS['east'][0]
    specs=[]
    for jump in (0,1,2,3,4,5,6,7,8,255):specs.append((f'flat_jump_{jump}',jump,[col(floor(3))]*5,0,5))
    for name,index,value in [('blocked_gap',1,col(floor(0,blocked=True))),
                             ('raised_first_span',1,col(floor(4))),
                             ('raised_second_span',2,col(floor(4))),
                             ('raised_third_span',3,col(floor(4))),
                             ('high_bridge',1,col(floor(0),floor(9,depth=1))),
                             ('low_bridge',1,col(floor(0),floor(5,depth=1))),
                             ('upper_landing',3,col(floor(0),floor(3))),
                             ('descending_landings',2,col(floor(1))),
                             ('ascending_landing',4,col(floor(4)))]:
        columns=[col(floor(3))]*5;columns[index]=value;specs.append((name,6,columns,0,5))
    columns=[col(floor(3))]*5;columns[0]=col(floor(0),floor(3))
    specs.append(('upper_source',6,columns,1,5))
    specs.append(('map_edge',6,[col(floor(3))]*5,0,4))
    specs.append(('no_landing_in_bounds',6,[col(floor(3))]*5,0,2))
    records=[];rows=[]
    for name,jump,columns,source_layer,width in specs:
        capped=min(jump,7)
        o.uc.mem_write(0x1d0000,bytes(0x30));o.uc.mem_write(0x1d3000,bytes(0x80))
        o.uc.mem_write(0x1d0018,bytes((width,1,0,6,0)));o.uc.mem_write(0x1d0009,bytes((source_layer,)))
        o.uc.mem_write(0x1d0002,bytes((2*capped,0,capped,capped>>1,15)))
        for index,column in enumerate(columns):
            for level in range(2):
                h,dh,slope,flags=column[4*level:4*level+4]
                o.uc.mem_write((s.TILES&0x1fffff)+8*(index+256*level),bytes((0,0,h,dh,slope,flags&31,flags>>7,0)))
        index=source_layer*256;panel=0x80192dd8+index*5
        o.call('flags',3);o.write(0x801d3000,s.TILES+index*8);o.write(0x801d3008,panel)
        o.uc.mem_write(0x1d301a,struct.pack('<h',index));o.uc.mem_write(panel&0x1fffff,bytes((16,0,0,0,0)))
        o.uc.mem_write(0x1d3054,b'\x01\x02')
        o.call('long_spread')
        masks=[];costs=[]
        for gap in (1,2,3):
            mask=0;cost=[]
            for level in range(2):
                remaining=o.uc.mem_read(0x192dd8+5*(level*256+gap+1),1)[0]
                if remaining:mask|=1<<level
                cost.append(16-remaining if remaining else None)
            masks.append(mask);costs.append(cost)
        records.append(bytes((jump,source_layer))+b''.join(columns)+bytes(masks)+bytes((width,1,0)))
        rows.append(dict(name=name,raw_jump=jump,configured_jump=capped,source_layer=source_layer,width=width,masks=masks,costs=costs))
    (out/'flj00.bin').write_bytes(b'FLJ1'+struct.pack('>H',len(records))+b''.join(records))
    (out/'source.json').write_text(json.dumps(dict(cases=len(records),case_bytes=48,functions=o.function_hashes,
        unicorn=o.version,rows=rows,scope='Full original horizontal spread, no actors, east lane, terrain cost1; unit Jump clamp is static code evidence used to configure the fixture, not an executed unit-config trial.'),indent=2)+'\n')
    print('Original horizontal-spread fixtures:',len(records),flush=True)


if __name__=='__main__':main()
