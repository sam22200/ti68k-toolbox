#!/usr/bin/env python3
"""Focused original full spread with explicit costs, budgets and water geometry."""
import argparse,json,struct
from pathlib import Path
import long_jump_oracle as long
s=long.s
floor,col=long.floor,long.col

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/core-jump-cost/oracle')
    out=ap.parse_args().out;out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC));o=s.Original(disc);o.instruction_limit=100000
    weather=disc.bat[0x80174370-s.battle.BAT:0x80174370-s.battle.BAT+88]
    weather_sha=s.core_audit.sha(weather);assert weather_sha.startswith('93bdb40ee3440793')
    table=disc.at(0x8005ea50,384);assert len(table)==384
    assert set(table)<=set((0,1,2,255))
    o.write(0x8018f4f0,0x801d6000);o.write(0x800e4e9c,5);o.write(0x800e4ea0,1)
    assert struct.unpack('<I',o.uc.mem_read(0x18f500,4))[0]==s.FUNCTIONS['east'][0]
    specs=[]
    for cost in (1,2,3,255):
        for gap in (1,2,3):
            points=sorted(set((1,16,min(255,cost+gap),min(255,cost+gap+1))))
            for budget in points:specs.append((cost,gap,budget,0,0))
    for source_water,dest_water in ((0,0),(0,1),(1,0),(1,1)):
        for gap in (1,2,3):specs.append((2,gap,16,source_water,dest_water))
    records=[];rows=[]
    for cost,gap,available,source_water,dest_water in specs:
        o.uc.mem_write(0x1d0000,bytes(0x30));o.uc.mem_write(0x1d3000,bytes(0x80))
        o.uc.mem_write(0x1d0018,bytes((5,1,0,6,0)));o.uc.mem_write(0x1d0002,bytes((12,0,6,3,15)))
        o.uc.mem_write(0x1d5000,bytes((1,cost))+bytes([1]*62))
        columns=[col(floor(0,blocked=True))]*5
        columns[0]=col(floor(3,half=source_water<<5))
        columns[gap+1]=col(floor(3-dest_water,half=dest_water<<5))
        for index,column in enumerate(columns):
            for layer in range(2):
                h,half,slope,flags=column[4*layer:4*layer+4]
                surface=1 if index==gap+1 else 0
                o.uc.mem_write((s.TILES&0x1fffff)+8*(index+256*layer),bytes((surface,0,h,half,slope,flags&31,flags>>7,0)))
        o.call('flags',3);o.write(0x801d3000,s.TILES);o.write(0x801d3008,0x80192dd8)
        o.uc.mem_write(0x1d301a,b'\0\0');o.uc.mem_write(0x192dd8,bytes((available,0,0,0,0)))
        o.uc.mem_write(0x1d3054,b'\x01\x02');o.call('long_spread')
        remaining=o.uc.mem_read(0x192dd8+5*(gap+1),1)[0]
        records.append(bytes((cost,gap,available,source_water,dest_water,remaining)))
        rows.append(dict(terrain_cost=cost,gap=gap,available=available,source_water=source_water,destination_water=dest_water,remaining=remaining))
    (out/'fjc00.bin').write_bytes(b'FJC1'+struct.pack('>H',len(records))+b''.join(records))
    (out/'source.json').write_text(json.dumps(dict(cases=len(records),case_bytes=6,functions=o.function_hashes,unicorn=o.version,
        static_weather_function=dict(address='0x80174370',bytes=88,sha256=weather_sha,executed=False),
        terrain_tables=dict(address='0x8005ea50',bytes=384,sha256=s.core_audit.sha(table),row_values=[sorted(set(table[i*64:i*64+64]))for i in range(6)]),rows=rows,
        scope='Unchanged original full eastward spread, no actors, supplied terrain costs/budgets and normal water-exit mode. Terrain table/weather producer not executed; its transformation is static source evidence. Unknown special movement costs are not inferred.'),indent=2)+'\n')
    print('Original cost/budget spread queries:',len(records),flush=True)

if __name__=='__main__':main()
