#!/usr/bin/env python3
"""Read squad counts/records from the original deployment overlay and disc.

No original input, native map unlock or new placement zone is synthesized.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import extract,oracle

HERE=Path(__file__).resolve().parents[1]


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('disc');ap.add_argument('state',type=Path)
    ap.add_argument('--out',type=Path,default=HERE/'campaign/x/qa/flow/review-2/source-zones.json')
    args=ap.parse_args();p=oracle.PSX(args.disc);p.load(str(args.state))
    assert oracle.var(p,oracle.EVENT)==9,'Requires the live original deployment overlay'
    pointer=p.read(0x801cd270,4);assert 0x80000000<=pointer<0x80200000
    disk,files=extract.disc_files(args.disc);attack=disk.read(*files['EVENT/ATTACK.OUT']);multiple=[]
    for at in range(0x801cf938-0x801bf000,len(attack)-23,24):
        raw=attack[at:at+24];event=int.from_bytes(raw[:2],'little')
        if not event:break
        squads=struct.unpack_from('<HH',raw,9)
        if not all(squads):continue
        records=[]
        for squad in squads:
            addr=pointer+12*squad;assert addr+12<=0x80200000
            r=bytes(p.read(addr+i) for i in range(12))
            records.append(dict(id=squad,map=r[9],limit=r[8],center=list(r[4:6]),upper_level=r[6],
                                orientation_flags=r[7],bitmap=int.from_bytes(r[:4],'little'),record=r.hex()))
        multiple.append(dict(event=event,map=raw[2],squads=records))
    count=oracle.var(p,0x34);ids=[oracle.var(p,i) for i in range(0x35,0x39)]
    assert count==1 and ids==[256,0,0,0]
    assert multiple[0]['event']==39 and [r['limit'] for r in multiple[0]['squads']]==[2,2]
    report=dict(state_sha256=hashlib.sha256(args.state.read_bytes()).hexdigest(),
                gariland=dict(count=count,ids=ids),multiple_scenarios=multiple,
                controls=dict(roster='L1/R1; native F1/F5',next_squad='START',previous_squad='CROSS',
                              markers='The deployment cursor and orientation arrow are also drawn with one squad.'),
                native_limit='Current active deployment is Gariland only; future squads need per-zone capacity, '
                             'upper-level/world-coordinate records and unique roster assignments, never a duplicated Gariland grid.')
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(f'Original squads: Gariland has one; {len(multiple)} scenario records have two; first example MAP034 has2+2slots')


if __name__=='__main__':main()
