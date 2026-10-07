#!/usr/bin/env python3
"""Complete explicit PC/TI states, LCD checks and individual TI frame budgets."""
import json,re,subprocess
from reference import GAME,ROOT
from schema import WORDS

CASES=[(0,'traverse',340),(1,'traverse',340),(2,'wall',125),
       (3,'wall',125),(4,'idle',12),(5,'shoot',85),(5,'charge',100),
       (6,'idle',60),(7,'idle',60),(8,'shoot',65),(9,'idle',5),
       (101,'run-fire',80),(101,'run-charge',90),(101,'jump-fire',50),
       (101,'jump-charge',95),(102,'wall-shoot',50),
       (101,'left-fire',55),(101,'left-medium',55),(101,'left-charge',85),
       (110,'idle',6),(111,'idle',6),(112,'idle',6),
       (113,'idle',6),(114,'idle',6),(115,'idle',6),
       (101,'stop-fire',70),(101,'stop-medium',65),(101,'stop-charge',90),
       (16,'roller-double',95),(16,'roller-spaced',95),(16,'roller-behind',95),
       (17,'idle',70),(18,'idle',70),
       (7,'damage-charge',90),(16,'damage-charge',90),
       (16,'damage-release',60),(7,'damage-jump',55),
       (7,'damage-held-jump',55),
       (101,'capacity-rapid',45),(101,'capacity-medium',60),
       (101,'capacity-large',85),
       (102,'wall-kick-fire',60),(119,'wall-grip-charge',60),
       (120,'wall-slide-charge',60),(119,'wall-windup-charge',60),
       (120,'wall-launch-charge',60),(123,'wall-deferred',60),
       (124,'wall-deferred',60),(121,'normal-fraction',40),
       (122,'normal-fraction',40),(0,'traverse-wall',340),
       (125,'ledge-last-jump',40),(125,'ledge-late-jump',40),
       (125,'ledge-stop',40),(125,'ledge-stop-jump',40),
       (125,'ledge-turn',40),(125,'ledge-turn-jump',40),
       (101,'stop-jump',70),(101,'run-jump-fire',65),
       (101,'run-jump-medium',85),(101,'run-jump-large',110),
       (125,'ledge-fire',40),(125,'ledge-stop-fire',30),
       (143,'right',40),(144,'right',40)]
CASES += [(n,'right',6) for n in range(126,142)]
def execute(args):
    return subprocess.run([str(a) for a in args],cwd=GAME,text=True,capture_output=True,check=True).stdout
def ti(binary,n,keys,count):
    return execute([ROOT/'tools/bin/ti-cycles','--arg',n,'--frames',count,'--keys',keys,
                    '--file','mmxmap.89y','--file','mmxart.89y',binary])
def main():
    out=GAME/'x';out.mkdir(exist_ok=True);screens=states=0;profiles=[]
    for n,name,count in CASES:
        keys=f'keys/{name}.txt'
        pc=execute([GAME/'mmx_test','--trace',n,count,keys])
        rows=[line.split('|') for line in pc.splitlines()]
        expected=[int(v) for words,_ in rows for v in words.split()]
        assert len(expected)==count*WORDS
        result=ti('mmxtrace.89z',n,keys,count)
        actual=[int(v)&65535 for v in re.findall(r'^value: (-?\d+)',result,re.M)]
        assert actual==expected+[count],f'PC/TI state mismatch: {n}/{name}'
        states+=count
        # Prefix runs give each frame's LCD and datasheet cost without relying
        # on aggregate averages. The same uninstrumented binary is profiled.
        previous=[0,0]
        for f,(_,checksum) in enumerate(rows,1):
            text=ti('mmxc.89z',n,keys,f)
            actual=re.search(r'shot 0 checksum ([0-9A-F]+)',text)[1]
            assert actual==checksum.strip(),f'LCD mismatch: {n}/{name}/{f}'
            totals=[int(re.search(r'^\d+\s+'+zone+r'\s+\d+\s+(\d+)',text,re.M)[1]) for zone in ('update','render')]
            delta=[v-p for v,p in zip(totals,previous)];previous=totals
            profiles.append(dict(scenario=n,keys=name,frame=f,update=delta[0],render=delta[1],total=sum(delta)))
            screens+=1
        print(f'{n}/{name}: {count} complete states and LCD frames agree',flush=True)
    peak=max(profiles,key=lambda p:p['total'])
    assert peak['total']<360000,f'30Hz frame budget exceeded: {peak}'
    report=dict(states=states,state_words=WORDS,screens=screens,peak=peak,
                average=sum(p['total'] for p in profiles)//len(profiles),budget=360000,frames=profiles)
    (out/'checks.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'{states} PC/TI complete states; {screens} LCDs; peak {peak}, average {report["average"]}')
if __name__=='__main__':main()
