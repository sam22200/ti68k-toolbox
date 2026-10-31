#!/usr/bin/env python3
"""Focused original horizontal intermediate-column query with actor records."""
import argparse,json,struct
from pathlib import Path
import surface_data as s
s.FUNCTIONS.update(middle=(0x8017622c,684,'551a8a31fcf7897c'))

def floor(h,blocked=False):return bytes((h,0,0,128 if blocked else 0))
def actor(standing,top,enemy=True):return bytes((64 if enemy else 0,top-standing,standing,top))

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/core-jump-actors/oracle')
    out=ap.parse_args().out;out.mkdir(parents=True,exist_ok=True)
    o=s.Original(s.battle.Disc(str(s.core_audit.DEFAULT_DISC)))
    o.write(0x8018f4f0,0x801d6000);o.write(0x8018f4e8,0x801d7000)
    base=floor(0)+floor(0,True);empty=bytes(8)
    specs=[('no_actor',base,empty,(0,128),6,6),
        ('enemy_overlaps',base,actor(6,12)+bytes(4),(4,128),6,6),
        ('ally_overlaps',base,actor(6,12,False)+bytes(4),(4,128),6,6),
        ('enemy_top_touches_feet',base,actor(0,6)+bytes(4),(4,128),6,6),
        ('enemy_low_touches_head',base,actor(12,18)+bytes(4),(4,128),6,6),
        ('small_body_waist_blocks',base,actor(4,6)+bytes(4),(4,128),6,6),
        ('waist_touches_feet',base,actor(3,6)+bytes(4),(4,128),6,6),
        ('mountable_enemy_record_skipped',base,actor(6,12)+bytes(4),(12,128),6,6),
        ('blocked_occupied_enemy',base,actor(0,6)+bytes(4),(132,128),6,6),
        ('blocked_occupied_ally',base,actor(0,6,False)+bytes(4),(132,128),6,6),
        ('blocked_mountable_occupied',base,actor(0,6)+bytes(4),(140,128),6,6),
        ('blocked_unoccupied_skipped',floor(9)+floor(0,True),empty,(128,128),6,6),
        ('upper_enemy_overlap',floor(0)+floor(0),bytes(4)+actor(6,12),(0,4),6,6),
        ('upper_ally_ignored',floor(0)+floor(0),bytes(4)+actor(6,12,False),(0,4),6,6),
        ('upper_blocked_occupied',base,bytes(4)+actor(0,6),(0,132),6,6),
        ('upper_mountable_skipped',floor(0)+floor(0),bytes(4)+actor(6,12),(0,12),6,6),
        ('two_enemies_clear',floor(0)+floor(0),actor(0,6)+actor(12,18),(4,4),6,6),
        ('two_enemies_upper_blocks',floor(0)+floor(0),actor(0,6)+actor(6,12),(4,4),6,6),
        ('body4_head_touch',base,actor(10,16)+bytes(4),(4,128),6,4),
        ('body10_same_actor_blocks',base,actor(10,16)+bytes(4),(4,128),6,10),
        # Isolates the shared lowest-span reduction, not a played placement.
        ('synthetic_low_actor_changes_lowest',floor(9)+floor(0,True),actor(0,6)+bytes(4),(4,128),6,6),
        ('same_terrain_without_actor',floor(9)+floor(0,True),empty,(0,128),6,6)]
    records=[];rows=[]
    for name,column,actors,frontier,height,body in specs:
        o.bind(column);o.uc.mem_write(0x1d001b,bytes((body,)))
        for layer in range(2):
            o.uc.mem_write(0x1d6000+256*layer,bytes((frontier[layer],)))
            o.uc.mem_write(0x192dd8+1280*layer+2,bytes((layer,)))
            o.uc.mem_write(0x1d7000+7*layer,bytes(3)+actors[4*layer:4*layer+4])
        for side in (0,2,6,4):
            clear=int(o.call('middle',0,side,side^2,height)==0)
            records.append(column+actors+bytes(frontier)+bytes((height,body,side,clear)))
            rows.append(dict(name=name,side=side,clear=clear))
    (out/'fja00.bin').write_bytes(b'FJA1'+struct.pack('>H',len(records))+b''.join(records))
    (out/'source.json').write_text(json.dumps(dict(cases=len(records),case_bytes=22,functions=o.function_hashes,unicorn=o.version,rows=rows,
        scope='Unchanged original intermediate-column function with explicit frontier/record fixture RAM, both layers; no movement-record producer or full played movement trace.'),indent=2)+'\n')
    print('Original actor-column queries:',len(records),flush=True)

if __name__=='__main__':main()
