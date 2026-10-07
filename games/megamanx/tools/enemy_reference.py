#!/usr/bin/env python3
"""Natural roller reactions: single/repeated shots, braking and charge."""
from reference import *
from measure import row
from combat_reference import actors

def sample(s):
    result=row(s)
    result['enemies']=actors(s,0xe68,0x1228)
    result['shots']=actors(s,0x1228,0x1428)
    result['roller_raw']=bytes(s.ram[0xe68:0xea8]).hex()
    return result

def main():
    directory=OUT/'enemy';directory.mkdir(exist_ok=True)
    s=SNES(ROM);cases=[]
    definitions=[('no_fire',{0:[]},180),('single_tap',{0:['Y'],1:[]},180),
        ('double_tap',{0:['Y'],1:[],12:['Y'],13:[]},180),
        ('spaced_tap',{0:['Y'],1:[],32:['Y'],33:[]},130),
        ('behind_double',{0:['RIGHT'],12:['RIGHT','B'],47:['RIGHT'],55:['LEFT','Y'],56:[],75:['Y'],76:[]},180)]
    try:
        for name,keys,count in definitions:
            s.load(OUT/'combat/natural-260.state')
            initial=sample(s);samples=[];held=[]
            for frame in range(count):
                if frame in keys:held=keys[frame]
                run(s,held);samples.append(sample(s))
                if name=='single_tap' and frame==0:s.save(directory/'collider-pellet.state')
                if name=='double_tap' and frame==20:s.save(directory/'brake.state')
            cases.append(dict(name=name,initial=initial,keys=keys,samples=samples))
            changed=[];previous=None
            for frame,r in enumerate(samples):
                e=next((e for e in r['enemies'] if e['kind']==21),None)
                value=(r['hp'],e and (e['hp']&127),e and e['vx'])
                if value!=previous:changed.append((frame,value,e and e['x']))
                previous=value
            print(name,'transitions',changed[:18],'; final',changed[-3:])
        s.load(OUT/'start.state')
        for frame in range(260):run(s,['RIGHT','Y'])
        s.save(directory/'charged-260.state');initial=sample(s);samples=[]
        for frame in range(90):
            run(s,[]);samples.append(sample(s))
            if frame==7:s.save(directory/'collider-large.state')
        cases.append(dict(name='charged_release',initial=initial,keys={0:[]},samples=samples))
        # Charge below the armor threshold before triggering the roller spawn;
        # the first pellet has left the screen before entering its area.
        s.load(OUT/'start.state')
        for frame in range(215):run(s,['RIGHT'])
        for frame in range(31):run(s,['Y'])
        for frame in range(45):run(s,['RIGHT','Y'])
        s.save(directory/'medium-ready.state');initial=sample(s);samples=[]
        roller=next(e for e in initial['enemies'] if e['kind']==21)
        assert roller['hp']==2 and not initial['shots']
        for frame in range(100):
            run(s,[]);samples.append(sample(s))
            if frame==11:s.save(directory/'collider-medium.state')
        cases.append(dict(name='medium_release',initial=initial,keys={0:[]},samples=samples))
        (directory/'reference.json').write_text(json.dumps(dict(isolation='Natural existing roller state or cold-boot traversal with held charge; no RAM injections.',cases=cases),indent=2)+'\n')
    finally:s.close()
if __name__=='__main__':main()
