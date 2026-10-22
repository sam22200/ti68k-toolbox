#!/usr/bin/env python3
"""Actual 68000 Academy/Gariland pixels and portable state; no emulator timings."""
from pathlib import Path
import subprocess,re,json,sys,hashlib
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OUT=HERE/'x/check'
sys.path.insert(0,str(HERE.parent/'tools'));import save_dump
def run(args,cwd=HERE/'data'):
    p=subprocess.run(list(map(str,args)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.returncode:print(p.stdout);p.check_returncode()
    return p.stdout
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python',HERE/'../tools/contour_assets.py','--attach',HERE/'data/ffgv0.bin'])
    empty=OUT/'none.keys';empty.write_text('0\n')
    advance=OUT/'advance.keys';advance.write_text(''.join(f'{f} ENTER\n{f+1}\n' for f in range(30,1001,15)))
    rotate=OUT/'rotate.keys';rotate.write_text('12 F5\n13\n50 F1\n51\n')
    fast=OUT/'fast.keys';fast.write_text('0 A\n')
    status=OUT/'status.keys';status.write_text('1 UP\n2\n3 UP\n4\n5 ENTER\n6\n30 B\n31\n')
    reach=OUT/'reach.keys';reach.write_text('20 ENTER\n21\n45 RIGHT\n46\n60 LEFT\n61\n80 MODE\n81\n110 B\n111\n130 ENTER\n131\n155 F5\n156\n190 F1\n191\n220 B\n221\n')
    won=OUT/'won.keys';won.write_text('0 A\n'+''.join(f'{f} A ENTER\n{f+1} A\n' for f in range(9000,20001,40)))
    settings=OUT/'settings.keys';settings.write_text('20 ENTER\n21\n40 CATALOG\n41\n43 ENTER\n44\n46 ESC\n47\n80 F5\n81\n115 CATALOG\n116\n118 ENTER\n119\n122 CATALOG\n123\n145 APPS\n146\n155 APPS\n156\n180 B\n181\n')
    results=[]
    for name,flags,banks,cases in (
        ('scene',['-DCHECK_SCENE'],['ffs1','ffcp','ffcu','ffca','ffcb'],[('card',1,30,empty),('academy',1,120,advance),('academy-complete',1,1000,advance)]),
        ('gariland',['-DCHECK_GAR'],['ffbm','ffgv0','ffgv1','ffgv2','ffgv3','ffga','ffgi','ffgd','ffcp','ffcu','ffcw','ffca','ffcb'],
         [('manual',61,90,empty),('rotations',61,100,rotate),('ai',16,9500,fast),('status',61,90,status),('reach',61,260,reach),('reach-settings',61,200,settings),('reach-contour',61,70,settings),('reach-menu',61,44,settings),('won',16,20000,won)])):
        if '--status-only' in sys.argv:
            cases=[c for c in cases if c[0]=='status']
            if not cases:continue
        if '--academy-only' in sys.argv and name!='scene':continue
        common=[HERE/'check_native.c',ROOT/'runtime/core/rt_core.c']
        run(['cc','-std=gnu99','-O1',*flags,'-o',OUT/name,*common,ROOT/'runtime/platform-sw/rt_sw.c'])
        run([ROOT/'tools/bin/ti-cc','-o',OUT/(name+'c'),'-DRT_CYCLES',*flags,*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        for case,scenario,frames,keys in cases:
            sav=HERE/'data/ffsave.sav';sav.unlink(missing_ok=True);(OUT/'ffsave.89y').unlink(missing_ok=True)
            pc=run([OUT/name,scenario,frames,keys,OUT/(name+'-'+case+'-pc.png')])
            command=[ROOT/'tools/bin/ti-cycles','--max',10000,'--arg',scenario,'--frames',frames,'--keys',keys,'--png',OUT/(name+'-'+case+'-ti.png')]
            for bank in banks:command+=['--file',HERE/'data'/(bank+'.89y')]
            if case=='won':command+=['--save-dir',OUT]
            ti=run([*command,OUT/(name+'c.89z')]);(OUT/(name+'-'+case+'.log')).write_text(ti)
            executed=int(re.search(r'executed (\d+)',pc)[1]);checksum=re.search(r'checksum (\w+)',pc)[1];state=int(re.search(r'state (\w+)',pc)[1],16)
            assert checksum.lower()==re.search(r'checksum\s+(\w+)',ti)[1].lower(),(name,case,'pixels')
            values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
            assert values[-3:]==[executed,0xacad0001,state],(name,case,'state',values,pc)
            row=dict(chapter=name,case=case,executed_frames=executed,checksum=checksum,state_hash=state)
            if case=='won':                    # the campaign save (save.h): the same bytes, Gariland's flag, its party
                saved=save_dump.payload(sav);assert saved==save_dump.payload(OUT/'ffsave.89y'),(name,case,'save')
                party=save_dump.decode(saved);assert party['flags']==[1] and party['party'],party
                row['save']=hashlib.sha1(saved).hexdigest()[:12]
                sav.rename(OUT/'ffsave_won.sav');(OUT/'ffsave.89y').rename(OUT/'ffsave_won.89y')   # the menus' party
            results.append(row);print(json.dumps(row),flush=True)
    if '--status-only' not in sys.argv and '--academy-only' not in sys.argv:
        results+=manage(OUT/'ffsave_won.sav',OUT/'ffsave_won.89y')
        results+=worldmap(OUT/'ffsave_won.sav',OUT/'ffsave_won.89y')
    (OUT/('status.json' if '--status-only' in sys.argv else 'academy.json' if '--academy-only' in sys.argv else 'checked.json')).write_text(json.dumps(results,indent=2)+'\n')
    print('Campaign: %d actual-68000 pixels/state replays passed'%len(results))
def keys(path,events):
    path.write_text(''.join(f'{f} {k}\n{f+1}\n' for f,k in events));return path

def program(name,source,banks,magic,cases,pc_save,ti_save,check_save):
    """A campaign program (manage.c, worldmap.c) on the party Gariland's victory saved, per key
    script: PC and 68000 screens and states alike, and the save it writes (the last case)."""
    common=[HERE/source,ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-o',OUT/name,*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-o',OUT/(name+'c'),'-DRT_CYCLES',*common,ROOT/'tools/extgraph/lib/tilemap.a'])
    sav=HERE/'data/ffsave.sav';out=[]
    for k,(case,door,frames,script) in enumerate(cases):
        sav.write_bytes(pc_save.read_bytes());(OUT/'ffsave.89y').unlink(missing_ok=True)
        pc=run([OUT/name,door,frames,script,OUT/(name+'-'+case+'-pc.png')])
        command=[ROOT/'tools/bin/ti-cycles','--max',2000,'--arg',door,'--frames',frames,'--keys',script,'--png',OUT/(name+'-'+case+'-ti.png')]
        for bank in banks:command+=['--file',HERE/'data'/(bank+'.89y')]
        ti=run([*command,'--file',ti_save,'--save-dir',OUT,OUT/(name+'c.89z')])
        executed=int(re.search(r'executed (\d+)',pc)[1]);checksum=re.search(r'checksum (\w+)',pc)[1];state=int(re.search(r'state (\w+)',pc)[1],16)
        assert checksum.lower()==re.search(r'checksum\s+(\w+)',ti)[1].lower(),(name,case,'pixels')
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        assert values[-2:]==[magic,state],(name,case,'state',values,pc)
        row=dict(chapter=name,case=case,executed_frames=executed,checksum=checksum,state_hash=state)
        if k==len(cases)-1:                   # the save written: the same by both
            saved=save_dump.payload(sav);assert saved==save_dump.payload(OUT/'ffsave.89y'),(name,'save')
            check_save(save_dump.decode(saved));row['save']=hashlib.sha1(saved).hexdigest()[:12]
        sav.unlink(missing_ok=True);out.append(row);print(json.dumps(row),flush=True)
    return out

def manage(pc_save,ti_save):
    """The management menus: each screen, then a tour that makes Ramza a Knight and leaves."""
    empty=OUT/'none.keys'
    tour=keys(OUT/'tour.keys',[(2,'C'),(4,'DOWN'),(6,'DOWN'),(8,'DOWN'),(10,'ENTER'),(12,'DOWN'),(14,'DOWN'),(16,'ENTER'),
        (18,'ENTER'),(20,'B'),(22,'UP'),(24,'UP'),(26,'UP'),(28,'ENTER'),(30,'ENTER'),(32,'B'),(34,'B'),(36,'DOWN'),(38,'DOWN'),
        (40,'ENTER'),(42,'ENTER'),(44,'ENTER'),(46,'B'),(48,'B'),(50,'ESC')])
    def knight(s): assert s['party'][0]['job']==0x4C,'Ramza a Knight'
    return program('manage','check_manage.c',['ffmg'],0x4d4e4701,
                   [('formation',0,4,empty),('status',1,4,empty),('equip',2,4,empty),('abilities',3,4,empty),
                    ('learn',4,4,empty),('jobs',5,4,empty),('menu',6,4,empty),('tour',0,80,tour)],pc_save,ti_save,knight)

def worldmap(pc_save,ti_save):
    """The world map: Gariland, Mandalia chosen, the walk, the arrival's notice, then ESC."""
    empty=OUT/'none.keys'
    choose=keys(OUT/'map-choose.keys',[(2,'LEFT')])
    walk=keys(OUT/'map-walk.keys',[(2,'LEFT'),(4,'ENTER')])
    trip=keys(OUT/'map-trip.keys',[(2,'LEFT'),(4,'ENTER'),(60,'ENTER'),(62,'B'),(64,'ESC')])
    def mandalia(s): assert s['location']==24,'Ramza at Mandalia'
    return program('worldmap','check_worldmap.c',['ffwm','ffmg'],0x4d415001,
                   [('gariland',0,4,empty),('choose',0,6,choose),('walk',0,16,walk),('arrived',0,61,trip),('trip',0,80,trip)],
                   pc_save,ti_save,mandalia)

if __name__=='__main__':main()
