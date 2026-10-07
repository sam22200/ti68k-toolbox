#!/usr/bin/env python3
"""Contact immunity probe: move the enemy to X, leave player/timing untouched."""
from reference import *
from measure import row
def main():
    s=SNES(ROM);samples=[];hits=[]
    try:
        s.load(OUT/'hit.state');previous=s.read(0x7e0bcf,1)
        initial=bytes(s.ram[:0x2000]);history=[]
        for f in range(210):
            # Deliberate enemy-only anchoring, disclosed in the report. Preserve
            # the initialized roller; its normal collision routine still runs.
            a=0xe68;s.ram[a]=1;s.ram[a+1]=2;s.ram[a+10]=21;s.ram[a+0x27]=2
            x=s.read(X,2)+12;y=s.read(Y,2)-3
            s.write(0x7e0000+a+5,x,2);s.write(0x7e0000+a+8,y,2)
            run(s,[]);p=row(s);samples.append(p);history.append(bytes(s.ram[:0x2000]))
            if p['hp']!=previous:hits.append(f);print('new hit relative to first',f+1,previous,'->',p['hp']);previous=p['hp']
        candidates=[]
        for a in range(0x2000):
            if history[1][a]>=32 and all(history[f][a]==history[1][a]-f+1 for f in range(1,14)):
                candidates.append(dict(address=hex(0x7e0000+a),start=history[1][a]))
        (OUT/'combat/hurt.json').write_text(json.dumps(dict(isolation='Enemy-only anchoring to player; no writes to player, timers, camera or collision map.',hits=hits,candidates=candidates,samples=samples),indent=2)+'\n')
        print('countdown candidates',candidates)
    finally:s.close()
if __name__=='__main__':main()
