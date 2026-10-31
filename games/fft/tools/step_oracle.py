#!/usr/bin/env python3
"""Focused original forward ordinary-terrain endpoint checks, no actor-top paths."""
import argparse,json,struct
from pathlib import Path
import surface_data as s
s.FUNCTIONS.update(flags=(0x80174e84,1028,'ff6f1c4c293f3bbc'),
    transfer=(0x801755a8,64,'dbfe81c711a9e41a'),
    source=(0x8017567c,732,'30b7924ca24f1d23'),
    destination=(0x80175958,1352,'8934859d8d97de21'))


def floor(h,half=0,slope=0,depth=0,blocked=False):return bytes((h,half,slope,depth|(128 if blocked else 0)))
def col(lower,upper=None):return lower+(upper or floor(0,blocked=True))


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,default=s.HERE/'campaign/x/core-steps/oracle')
    out=ap.parse_args().out;out.mkdir(parents=True,exist_ok=True)
    o=s.Original(s.battle.Disc(str(s.core_audit.DEFAULT_DISC)));o.instruction_limit=100000
    o.write(0x8018f4f0,0x801d6000);o.uc.mem_write(0x1d5000,bytes([1]*64))
    specs=[('flat',col(floor(2)),col(floor(2)),0,0,2,0),
        ('rise_equal',col(floor(1)),col(floor(3)),0,0,2,0),
        ('rise_too_high',col(floor(1)),col(floor(4)),0,0,2,0),
        ('fall_equal',col(floor(3)),col(floor(1)),0,0,2,0),
        ('fall_too_far',col(floor(4)),col(floor(1)),0,0,2,0),
        ('source_ramp',col(floor(2,3,0xe4)),col(floor(2)),0,0,2,0),
        ('destination_ramp',col(floor(2)),col(floor(2,3,0x1b)),0,0,2,0),
        ('source_corner_limit',col(floor(2,3,0x01)),col(floor(2)),0,0,5,0),
        ('destination_corner_limit',col(floor(2)),col(floor(2,3,0x04)),0,0,5,0),
        ('source_nonsteep',col(floor(2,2,0x01)),col(floor(2)),0,0,5,0),
        ('jump_threshold_slope',col(floor(2,3,0x00)),col(floor(2)),0,0,2,0),
        ('jump_allows_slope',col(floor(2,3,0x00)),col(floor(2)),0,0,3,0),
        ('source_headroom_equal',col(floor(0),floor(4,depth=1)),col(floor(0)),0,0,2,0),
        ('source_headroom_short',col(floor(0),floor(3,depth=1)),col(floor(0)),0,0,2,0),
        ('dest_headroom_equal',col(floor(0)),col(floor(2),floor(6,depth=1)),0,0,3,0),
        ('dest_headroom_short',col(floor(0)),col(floor(2),floor(5,depth=1)),0,0,3,0),
        ('upper_to_lower',col(floor(0),floor(3)),col(floor(2)),1,0,2,0),
        ('lower_to_upper',col(floor(2)),col(floor(0),floor(3)),0,1,2,0),
        ('blocked_destination',col(floor(2)),col(floor(2,blocked=True)),0,0,2,0),
        ('water_exit_normal',col(floor(0,64)),col(floor(4)),0,0,3,0),
        ('water_exit_ability',col(floor(0,64)),col(floor(4)),0,0,3,1),
        ('water_entry',col(floor(0)),col(floor(0,64)),0,0,1,0),
        ('jump_level',col(floor(2)),col(floor(2)),0,0,2,0,1),
        ('jump_descends',col(floor(3)),col(floor(1)),0,0,2,0,1),
        ('jump_cannot_rise',col(floor(1)),col(floor(3)),0,0,2,0,1),
        ('jump_cap_blocks_eight',col(floor(0)),col(floor(8)),0,0,8,0),
        ('jump_cap_allows_seven',col(floor(0)),col(floor(7)),0,0,255,0),
        ('jump_cap_steep',col(floor(2,8,0)),col(floor(2)),0,0,8,0)]
    cases=[];rows=[]
    for spec in specs:
        name,src,dst,sl,dl,jump,water=spec[:7];extra=spec[7]if len(spec)>7 else 0
        for side in (0,2,6,4):
            o.uc.mem_write(0x1d0000,bytes(0x30));o.uc.mem_write(0x1d3000,bytes(0x80))
            o.uc.mem_write(0x1d0018,bytes((2,1,0,6,128 if water else 0)))
            capped=min(jump,7);o.uc.mem_write(0x1d0002,bytes((2*capped,0,capped,0,31)))
            for index,c in enumerate((src,dst)):
                for level in range(2):
                    h,dh,slope,flags=c[4*level:4*level+4]
                    o.uc.mem_write((s.TILES&0x1fffff)+8*(index+256*level),bytes((0,0,h,dh,slope,flags&31,flags>>7,0)))
            o.call('flags',3)
            si=sl*256;di=dl*256+1;panel=0x80192dd8+si*5
            o.write(0x801d3000,s.TILES+si*8);o.write(0x801d3008,panel)
            o.uc.mem_write(0x1d301a,struct.pack('<h',si));o.uc.mem_write(0x1d3030,struct.pack('<hh',1,0))
            o.uc.mem_write(0x1d3044,bytes((side,side^2)));o.uc.mem_write(panel&0x1fffff,bytes((32,0,0,0,0)))
            o.call('transfer');start=o.call('source',0);end=o.call('destination',dl,extra)if not start else 255
            expected=int(start==0 and end==0)
            steep_a=int(bool(o.uc.mem_read(0x1d6000+si,1)[0]&64));steep_b=int(bool(o.uc.mem_read(0x1d6000+di,1)[0]&64))
            cases.append(src+dst+bytes((sl,dl,side,jump,water,6,expected,steep_a,steep_b,extra)))
            rows.append(dict(name=name,side=side,accepted=expected,source_result=start,destination_result=end))
    (out/'fst00.bin').write_bytes(b'FST1'+struct.pack('>H',len(cases))+b''.join(cases))
    (out/'source.json').write_text(json.dumps(dict(cases=len(cases),case_bytes=26,functions=o.function_hashes,unicorn=o.version,rows=rows,
        scope='Original terrain flags, source panel and forward destination propagation with ample budget; no units, special movement or full spread/timing.'),indent=2)+'\n')
    print('Focused original step cases:',len(cases),flush=True)


if __name__=='__main__':main()
