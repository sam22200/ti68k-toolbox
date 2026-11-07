#!/usr/bin/env python3
"""Actual 68000 Academy/Gariland pixels and portable state; no emulator timings."""
from pathlib import Path
import subprocess,re,json,sys,hashlib,tempfile
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OUT=HERE/'x/check'
sys.path.insert(0,str(HERE.parent/'tools'));import save_dump
from review_files import write_changed
def run(args,cwd=HERE/'data'):
    # Tests may reproduce identical pictures. Publish only changed bytes, leaving
    # their gallery dates intact; temporary outputs never appear in the gallery.
    with tempfile.TemporaryDirectory(prefix='fft-check-images-') as folder:
        argv=list(map(str,args));images=[]
        for i,arg in enumerate(argv):
            if arg.endswith('.png'):
                staged=Path(folder)/(str(i)+'.png');images.append((staged,Path(arg)));argv[i]=str(staged)
        p=subprocess.run(argv,cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        if p.returncode:print(p.stdout);p.check_returncode()
        for staged,dest in images:
            if staged.exists():write_changed(dest,staged.read_bytes())
        return p.stdout
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    run([ROOT/'tools/pyenv/bin/python',HERE/'../tools/contour_assets.py','--attach',HERE/'data/ffgv0.bin'])
    empty=OUT/'none.keys';empty.write_text('0\n')
    if '--carousel-only' in sys.argv:
        results=manage(OUT/'ffsave_won.sav',OUT/'ffsave_won.89y')
        print('Carousel: %d moving/settled PC/68000 replays passed'%len(results))
        return
    if '--map-only' in sys.argv:
        results=worldmap(OUT/'ffsave_won.sav',OUT/'ffsave_won.89y')
        print('World map: %d free-cursor/travel/state PC/68000 replays passed'%len(results))
        return
    if '--menus-only' in sys.argv:
        results=manage(OUT/'ffsave_won.sav',OUT/'ffsave_won.89y')
        results+=worldmap(OUT/'ffsave_won.sav',OUT/'ffsave_won.89y')
        (OUT/'menus.json').write_text(json.dumps(results,indent=2)+'\n')
        print('Campaign menus/map: %d actual-68000 pixels/state replays passed'%len(results))
        return
    advance=OUT/'advance.keys';advance.write_text(''.join(f'{f} ENTER\n{f+1}\n' for f in range(30,1001,15)))
    rotate=OUT/'rotate.keys';rotate.write_text('12 F5\n13\n50 F1\n51\n')
    fast=OUT/'fast.keys';fast.write_text('0 A\n')
    status=OUT/'status.keys';status.write_text('1 UP\n2\n3 UP\n4\n5 ENTER\n6\n30 B\n31\n')
    wide=OUT/'wide.keys';wide.write_text('20 DOWN\n21\n22 ENTER\n23\n24 DOWN\n25\n26 ENTER\n27\n28 ENTER\n29\n')
    reach=OUT/'reach.keys';reach.write_text('20 ENTER\n21\n45 RIGHT\n46\n60 LEFT\n61\n80 MODE\n81\n110 B\n111\n130 ENTER\n131\n155 F5\n156\n190 F1\n191\n220 B\n221\n')
    won=OUT/'won.keys';won.write_text('0 A\n'+''.join(f'{f} A ENTER\n{f+1} A\n' for f in range(9000,20001,40)))
    settings=OUT/'settings.keys';settings.write_text('20 ENTER\n21\n40 CATALOG\n41\n43 ENTER\n44\n46 ESC\n47\n80 F5\n81\n115 CATALOG\n116\n118 ENTER\n119\n122 CATALOG\n123\n145 APPS\n146\n155 APPS\n156\n180 B\n181\n')
    results=[]
    for name,flags,banks,cases in (
        ('scene',['-DCHECK_SCENE'],['ffs1','ffflow','ffcp','ffcu','ffca','ffcb'],[('narration',1,30,empty),('chapter',1,120,advance),('academy-complete',1,1000,advance)]),
        ('gariland',['-DCHECK_GAR'],['ffbm','ffbn','ffbo','ffgv0','ffgv1','ffgv2','ffgv3','ffga','ffgi','ffgd','ffcp','ffcu','ffcw','ffca','ffcb','ffcj'],
         [('manual',61,90,empty),('rotations',61,100,rotate),('ai',16,9500,fast),('status',61,90,status),('reach',61,260,reach),('reach-settings',61,200,settings),('reach-contour',61,70,settings),('reach-menu',61,44,settings),('won',16,20000,won),('party',70,90,empty),('wide',70,90,wide)])):
        if '--status-only' in sys.argv:
            cases=[c for c in cases if c[0]=='status']
            if not cases:continue
        if '--academy-only' in sys.argv and name!='scene':continue
        common=[HERE/'check_native.c',ROOT/'runtime/core/rt_core.c']
        run(['cc','-std=gnu99','-O1',*flags,'-o',OUT/name,*common,ROOT/'runtime/platform-sw/rt_sw.c'])
        run([ROOT/'tools/bin/ti-cc','-o',OUT/(name+'c'),'-DRT_CYCLES',*flags,*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        for case,scenario,frames,keys in cases:
            sav=HERE/'data/ffsave.sav';sav.unlink(missing_ok=True);(OUT/'ffsave.89y').unlink(missing_ok=True)
            if case in ('party','wide'):sav.write_bytes((OUT/'ffsave_won.sav').read_bytes())
            if case=='wide':
                # A legal Ramza Chemist with Phoenix Down at source position 13;
                # bit 15 also survives loading even though that action slot is empty.
                fixture=bytearray(sav.read_bytes());at=save_dump.HEAD
                fixture[at]=0x4B;fixture[at+25:at+30]=bytes([0x9D,0xBA,0xD0,1,0])
                fixture[at+116:at+176]=bytes(60);fixture[at+120]=5
                sav.write_bytes(fixture)
                door=OUT/'wide-save';door.mkdir(exist_ok=True);(door/'ffsave.sav').write_bytes(fixture)
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','ffsave.sav','ffsave'],cwd=door)
            pc=run([OUT/name,scenario,frames,keys,OUT/(name+'-'+case+'-pc.png')])
            command=[ROOT/'tools/bin/ti-cycles','--max',10000,'--arg',scenario,'--frames',frames,'--keys',keys,'--png',OUT/(name+'-'+case+'-ti.png')]
            for bank in banks:command+=['--file',HERE/'data'/(bank+'.89y')]
            if case=='won':command+=['--save-dir',OUT]
            if case=='party':command+=['--file',OUT/'ffsave_won.89y']
            if case=='wide':command+=['--file',OUT/'wide-save/ffsave.89y']
            ti=run([*command,OUT/(name+'c.89z')]);(OUT/(name+'-'+case+'.log')).write_text(ti)
            executed=int(re.search(r'executed (\d+)',pc)[1]);checksum=re.search(r'checksum (\w+)',pc)[1];state=int(re.search(r'state (\w+)',pc)[1],16)
            assert checksum.lower()==re.search(r'checksum\s+(\w+)',ti)[1].lower(),(name,case,'pixels')
            values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
            expected=[executed,0xacad0001,state]
            if name=='gariland':
                identity=int(re.search(r'identity (\w+)',pc)[1],16);expected.append(identity)
                if case=='wide':assert identity==0x017DA000,(case,pc)
            assert values[-len(expected):]==expected,(name,case,'state',values,pc)
            row=dict(chapter=name,case=case,executed_frames=executed,checksum=checksum,state_hash=state)
            if case=='won':                    # the campaign save (save.h): the same bytes, Gariland's flag, its party
                saved=save_dump.payload(sav);assert saved==save_dump.payload(OUT/'ffsave.89y'),(name,case,'save')
                party=save_dump.decode(saved);assert party['flags']==[1] and party['party'],party
                row['save']=hashlib.sha1(saved).hexdigest()[:12]
                sav.rename(OUT/'ffsave_won.sav');(OUT/'ffsave.89y').rename(OUT/'ffsave_won.89y')   # the menus' party
            if case in ('party','wide'):sav.unlink(missing_ok=True)
            results.append(row);print(json.dumps(row),flush=True)
    if '--status-only' not in sys.argv and '--academy-only' not in sys.argv:
        results+=manage(OUT/'ffsave_won.sav',OUT/'ffsave_won.89y')
        results+=worldmap(OUT/'ffsave_won.sav',OUT/'ffsave_won.89y')
    (OUT/('status.json' if '--status-only' in sys.argv else 'academy.json' if '--academy-only' in sys.argv else 'checked.json')).write_text(json.dumps(results,indent=2)+'\n')
    print('Campaign: %d actual-68000 pixels/state replays passed'%len(results))
def keys(path,events):
    path.write_text(''.join(f'{f} {k}\n{f+1}\n' for f,k in events));return path

def program(name,source,banks,magic,cases,pc_save,ti_save,check_save,workdir=None):
    """A campaign program (manage.c, worldmap.c) on the party Gariland's victory saved, per key
    script: PC and 68000 screens and states alike, and the save it writes (the last case)."""
    common=[HERE/source,ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-o',OUT/name,*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-o',OUT/(name+'c'),'-DRT_CYCLES',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    workdir=Path(workdir) if workdir else HERE/'data'
    sav=workdir/'ffsave.sav';out=[]
    for k,(case,door,frames,script) in enumerate(cases):
        sav.write_bytes(pc_save.read_bytes());(OUT/'ffsave.89y').unlink(missing_ok=True)
        pc=run([OUT/name,door,frames,script,OUT/(name+'-'+case+'-pc.png')],cwd=workdir)
        command=[ROOT/'tools/bin/ti-cycles','--max',2000,'--arg',door,'--frames',frames,'--keys',script,'--png',OUT/(name+'-'+case+'-ti.png')]
        for bank in banks:command+=['--file',HERE/'data'/(bank+'.89y')]
        ti=run([*command,'--file',ti_save,'--save-dir',OUT,OUT/(name+'c.89z')])
        write_changed(OUT/(name+'-'+case+'.log'),ti.encode())
        write_changed(OUT/(name+'-'+case+'-pc.log'),pc.encode())
        executed=int(re.search(r'executed (\d+)',pc)[1]);checksum=re.search(r'checksum (\w+)',pc)[1];state=int(re.search(r'state (\w+)',pc)[1],16)
        assert checksum.lower()==re.search(r'checksum\s+(\w+)',ti)[1].lower(),(name,case,'pixels')
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
        assert values[-2:]==[magic,state],(name,case,'state',values,pc)
        row=dict(chapter=name,case=case,executed_frames=executed,checksum=checksum,state_hash=state)
        if check_save and k==len(cases)-1:    # the save written: the same by both
            saved=save_dump.payload(sav);assert saved==save_dump.payload(OUT/'ffsave.89y'),(name,'save')
            check_save(save_dump.decode(saved));row['save']=hashlib.sha1(saved).hexdigest()[:12]
        sav.unlink(missing_ok=True);out.append(row);print(json.dumps(row),flush=True)
    return out

def manage(pc_save,ti_save):
    """The management menus: each screen, then a tour that makes Ramza a Knight and leaves."""
    empty=OUT/'none.keys'
    tour=keys(OUT/'tour.keys',[(2,'C'),(4,'DOWN'),(6,'DOWN'),(8,'DOWN'),(10,'ENTER'),
        (12,'DOWN'),(24,'DOWN'),(36,'ENTER'),(38,'ENTER'),(40,'B'),(42,'UP'),(44,'UP'),(46,'UP'),
        (48,'ENTER'),(50,'ENTER'),(52,'B'),(54,'B'),(56,'DOWN'),(58,'DOWN'),(60,'ENTER'),(62,'ENTER'),
        (64,'ENTER'),(66,'B'),(68,'B'),(70,'ESC')])
    def knight(s): assert s['party'][0]['job']==0x4C,'Ramza a Knight'
    if '--carousel-only' in sys.argv:
        spin=keys(OUT/'carousel.keys',[(2,'RIGHT')])
        return program('manage','check_manage.c',['ffmg','ffcp','ffcj'],0x4d4e4701,
                       [('spin-start',5,4,spin),('spin-middle',5,6,spin),('spin-settled',5,16,spin)],
                       pc_save,ti_save,None)
    slide=keys(OUT/'formation-slide.keys',[(2,'RIGHT')])
    confirmation=keys(OUT/'job-confirm.keys',[(2,'RIGHT'),(14,'RIGHT'),(26,'ENTER')])
    change=keys(OUT/'job-change.keys',[(2,'RIGHT'),(14,'RIGHT'),(26,'ENTER'),(28,'ENTER')])
    return program('manage','check_manage.c',['ffmg','ffcp','ffcj'],0x4d4e4701,
                   [('formation',0,4,empty),('status',1,4,empty),('equip',2,4,empty),('abilities',3,4,empty),
                    ('learn',4,4,empty),('jobs',5,4,empty),('menu',6,4,empty),
                    ('slide-start',0,3,slide),('slide-mid',0,5,slide),('slide-done',0,8,slide),
                    ('job-confirm',5,28,confirmation),('job-change-start',5,30,change),
                    ('job-change-mid',5,78,change),('job-change-done',5,130,change),
                    ('tour',0,80,tour)],pc_save,ti_save,knight)

def worldmap(pc_save,ti_save):
    """The world map: Gariland, Mandalia chosen, the walk, the arrival's notice, then ESC."""
    empty=OUT/'none.keys'
    # Move the free hotspot to the actual Mandalia source coordinates, then release
    # into its magnetic radius. ENTER, separately, commits the route.
    import struct
    bank=(HERE/'data/ffwm.bin').read_bytes();at=struct.unpack_from('>H',bank,8)[0]
    x,y=struct.unpack_from('>hh',bank,at+6*8);tx,ty=struct.unpack_from('>hh',bank,at+24*8)
    events=[];f=2
    for delta,negative,positive in ((tx-x,'LEFT','RIGHT'),(ty-y,'UP','DOWN')):
        if delta:
            events.extend([(f,negative if delta<0 else positive),(f+abs(delta)//2,'')]);f+=abs(delta)//2+2
    def script(name,tail):
        p=OUT/name;p.write_text(''.join(f'{n} {k}\n' for n,k in events+tail));return p
    choose=script('map-choose.keys',[])
    walk=script('map-walk.keys',[(f,'ENTER'),(f+1,'')])
    trip=script('map-trip.keys',[(f,'ENTER'),(f+1,''),(f+400,'ENTER'),(f+401,''),
                               (f+402,'B'),(f+403,''),(f+404,'ESC'),(f+405,'')])
    def mandalia(s): assert s['location']==24,'Ramza at Mandalia'
    menu=keys(OUT/'map-menu.keys',[(2,'C')]);last=keys(OUT/'map-menu-last.keys',[(2,'C'),(4,'UP')])
    blank=OUT/'map-blank.keys';blank.write_text('0 LEFT UP\n60\n64 ENTER\n65\n')
    return program('worldmap','check_worldmap.c',['ffwm','ffmg'],0x4d415001,
                   [('gariland',0,4,empty),('menu',0,4,menu),('menu-last',0,8,last),
                    ('blank-return',0,68,blank),('choose',0,f,choose),('walk',0,f+12,walk),('arrived',0,f+401,trip),('trip',0,f+410,trip)],
                   pc_save,ti_save,mandalia)

if __name__=='__main__':main()
