#!/usr/bin/env python3
"""All native damage state fields, PAL launch fields, LCDs and TI frame costs."""
import json
import re
from concurrent.futures import ThreadPoolExecutor
from reference import OUT
from check_terrain import GAME, command, run, values

WORDS=36


def check(keys,frames,scenario):
    ti=values(run('yjdamageh.89z',keys,frames,scenario))
    pc=values(command([str(GAME/'yjprobe_test'),'--damagetrace',str(keys),str(frames),str(scenario)]))
    assert len(ti)==WORDS*frames+1 and ti[-1]==frames
    assert ti==pc, f'complete damage states differ: scenario{scenario}'
    return [ti[f*WORDS:(f+1)*WORDS] for f in range(frames)]


def screen(keys,frames,scenario):
    ti=int(re.search(r'checksum ([0-9A-Fa-f]+)',run('yjprobec.89z',keys,frames,scenario)).group(1),16)
    pc=command([str(GAME/'yjprobe_pc'),'--headless','--scenario',str(scenario),
                '--keys',str(keys),'--frames',str(frames)])
    assert ti==int(re.search(r'checksum ([0-9A-Fa-f]+)',pc).group(1),16),(scenario,frames,ti,pc)


def profile(keys,count,scenario,name):
    def totals(n):
        output=run('yjprobec.89z',keys,n,scenario)
        return [int(re.search(r'^\d+\s+'+z+r'\s+\d+\s+(\d+)',output,re.M).group(1)) for z in ('update','render')]
    previous=[0,0]; rows=[]
    # Independent headless prefixes can run concurrently; every frame is
    # still measured, including TileMap's cold builds and cache rebuilds.
    with ThreadPoolExecutor(max_workers=4) as pool:
        for n,current in enumerate(pool.map(totals,range(1,count+1)),1):
            delta=[a-b for a,b in zip(current,previous)]; previous=current
            rows.append({'frame':n-1,'update':delta[0],'render':delta[1],'total':sum(delta)})
            if n%200==0: print(name,'profiled',n,'frames',flush=True)
    peak=max(rows,key=lambda r:r['total'])
    report={'peak':peak,'average':sum(r['total'] for r in rows)//len(rows),'frames':rows,
            'scope':'complete native update/render, cold/rebuilt scene cache; hardware grayscale excluded'}
    (GAME/f'x/{name}_cycles.json').write_text(json.dumps(report,indent=2)+'\n')
    print(name,'mean',report['average'],'peak',peak,flush=True)
    assert peak['total']<210000,(name,'exceeds provisional 51.2Hz frame budget',peak)


def main():
    idle=GAME/'x/idle.txt'; idle.write_text('0\n')
    dense=GAME/'x/dense.txt'; dense.write_text('0 RIGHT\n')
    contact=check(idle,100,54)
    original=json.loads((OUT/'damage/reference.json').read_text())['cases'][0]['samples']
    for p,r in zip(contact[1:25],original[1:25]):
        got={'bx':p[30]>>16,'by':p[30]&65535,'bvx':((p[31]>>16)^32768)-32768,
             'bvy':((p[31]&65535)^32768)-32768,'xsub':p[34]>>24,
             'ysub':(p[34]>>16)&255,'phase':(p[33]>>16)&255}
        assert got=={k:r[k] for k in got},(r['frame'],got,r)
    failure=check(idle,540,55)
    assert failure[511][32]>>16==5 and failure[512][33]>>24==4
    assert all(p==failure[512] for p in failure[512:])
    rescue=check(idle,75,56)
    assert rescue[0][33]>>24==3 and rescue[32][33]>>24==0
    assert all(p[32]>>16==2560 for p in rescue)
    check(idle,3,57)
    stress=check(dense,80,58)
    assert {p[5]>>16&15 for p in stress}==set(range(16))
    route=check(GAME/'keys/actors.txt',1300,0)
    finish=next(f for f,p in enumerate(route) if p[6]&65536)
    assert route[finish][35]&65535==0x0202 and route[finish][9]&255==2
    assert all(p==route[finish] for p in route[finish:])
    print('24 PAL launch states PC=TI=ROM; 2098 complete native PC/TI states; ten real seconds, rescue/failure and two-hit/two-rescue finish at',finish,flush=True)
    for n in range(1,81): screen(dense,n,58)
    for scenario,counts in ((54,(1,2,25,33,65,100)),(55,(1,52,104,160,257,461,512,513,540)),
                             (56,(1,16,33,75)),(57,(1,2,3))):
        for n in counts: screen(idle,n,scenario)
    for n in (358,401,585,731,1193,1300): screen(GAME/'keys/actors.txt',n,0)
    print('108 damage/return/failure/dense/traversal LCD checksums PC=TI',flush=True)
    profile(dense,80,58,'damage_dense')
    profile(GAME/'keys/actors.txt',1300,0,'damage_route')
    profile(idle,540,55,'damage_expiry')


if __name__=='__main__': main()
