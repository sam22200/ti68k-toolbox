#!/usr/bin/env python3
"""Academy bridge + mirrored Gariland banks, reusing the reviewed local art."""
import argparse,json,struct,subprocess,sys,re
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import numpy as np
from PIL import Image
import extract,battle,units,interface,story_text

HERE=Path(__file__).resolve().parents[1];OUT=HERE/'campaign/data'

def main():
    ap=argparse.ArgumentParser(__doc__);ap.add_argument('disc');ap.add_argument('--reuse-views',action='store_true');args=ap.parse_args()
    OUT.mkdir(parents=True,exist_ok=True);disc,files=extract.disc_files(args.disc)
    rows=json.loads((HERE/'x/opening/choreography/scene-06.json').read_text())
    messages=[];seen=set();raw=disc.read(*files['EVENT/TEST.EVT'])
    for row in rows:
        if row['event']!=8:continue
        for m in row['messages']:
            mid=m['message']
            if mid>30 or mid in seen:continue
            seen.add(mid);name,body=story_text.message(raw,8,mid)
            messages.append((name,body))
    assert len(messages)>=15
    # Academy (MAP024), one fixed source-facing view, never four room images.
    extract.MAP=24;extract.SOURCE_CAMERA=True;records=extract.gns_records(disc,files)
    mesh=disc.read(*next(r[1:] for r in records if r[0]==0x2e01))
    at=struct.unpack_from('<I',mesh,0x68)[0];nx,nz=mesh[at:at+2]
    texture=disc.read(*next(r[1:] for r in records if r[0]==0x1701))
    b=np.frombuffer(texture,np.uint8).reshape(1024,128);tex=np.empty((1024,256),np.uint8);tex[:,::2]=b&15;tex[:,1::2]=b>>4
    pal=np.frombuffer(mesh,'<u2',256,struct.unpack_from('<I',mesh,0x44)[0]).reshape(16,16)
    lum,diag,face,dep=extract.render(extract.polygons(mesh),tex,pal,2,nx,nz,212,return_depth=True)
    im=lum.reshape(212,4,320,4).mean((1,3));bg=(lum==extract.BG).reshape(212,4,320,4).mean((1,3))>.5
    q=np.percentile(im[~bg],[20,55,90]);grey=np.where(im>q[2],0,np.where(im>q[1],1,np.where(im>q[0],2,3)));grey[bg]=0
    # A source view centered on the academy group; the dialogue covers the bottom.
    first=next(r for r in rows if r['event']==8 and r['messages'])
    actors=[u for u in first['actors'] if u['id']<8 and u['sheet']<135]
    def project(u):
        x,y,z=u['xyz'];return round(320-(nx-(x-z)/28)*12),round(64+(nx+nz-(x+z)/28)*6+y/2)
    xy=[project(u) for u in actors];cx=max(0,min(160,round(np.mean([a[0] for a in xy]))-80));cx=cx//8*8
    cy=max(0,min(112,min(a[1] for a in xy)-28))
    canvas=grey[cy:cy+100,cx:cx+160].copy()
    actor_data=bytearray();bd=battle.Disc(args.disc)
    for u in sorted(actors,key=lambda u:-(u['xyz'][0]+u['xyz'][2])):
        lba,size=struct.unpack_from('<II',bd.bat,0x80094cd4-battle.BAT+8*u['sheet'])
        path=next(k for k,v in files.items() if v[0]==lba)
        shp_id=bd.bat[0x80094748-battle.BAT+4*u['sheet']];typ='TYPE1' if shp_id==0 else 'TYPE2'
        spr=disc.read(*files[path]);shp=units.shp_frames(disc.read(*files['BATTLE/'+typ+'.SHP']))
        # Static standing poses, reusing existing sheet converters and outlines.
        g=units.sprite(spr,shp[8 if u['facing']>=2048 else 2]);x,y=project(u);x-=cx;y-=cy
        actor_data+=struct.pack('>hh',x-8,y-units.FOOT)+interface.planes(g,16)
    font=(HERE/'ffti.bin').read_bytes()[:571]
    data=bytearray(font);data+=bytes(len(data)&1);bg_off=len(data)
    data+=b''.join(np.packbits(a,axis=1).tobytes() for a in ((canvas&1)!=0,(canvas&2)!=0))
    actor_off=len(data);data+=actor_data
    msg_off=len(data);data+=bytes(2*len(messages))
    for i,(name,body) in enumerate(messages):
        struct.pack_into('>H',data,msg_off+2*i,len(data));data+=name.encode('ascii')+b'\0'+body.encode('ascii')+b'\0'
    (OUT/'ffac.bin').write_bytes(data)
    (OUT/'academy.h').write_text('// Generated from the local original; never committed.\n#define AC_BG %d\n#define AC_ACTORS %d\n#define AC_NACTORS %d\n#define AC_MESSAGES %d\n#define AC_NMESSAGES %d\n'%(bg_off,actor_off,len(actors),msg_off,len(messages)))
    (OUT/'academy.json').write_text(json.dumps(dict(map=24,bytes=len(data),messages=messages,camera=[cx,cy],actors=len(actors)),indent=2)+'\n')
    Image.fromarray(np.uint8(255-canvas*85)).save(OUT/'academy.png')
    # Flip colours and masks bitwise; depth values are reversed in byte order only.
    # No retail texture is replaced or recomputed, preserving reviewed relief art.
    TOP=16                                     # fft.h SC_TOP: rows above every view's scenery
    rev=np.array([int(f'{i:08b}'[::-1],2) for i in range(256)],np.uint8)
    def mirror_view(rot):
        raw=OUT/('view%d.raw'%rot)
        subprocess.run([HERE/'../../tools/bin/dzx0','-f',HERE/f'fftv{rot}.bin',raw],check=True,capture_output=True)
        planes=np.frombuffer(raw.read_bytes(),np.uint8).reshape(6,-1,40)[:,:,::-1].copy()
        for i in (0,1,3,5):planes[i]=rev[planes[i]]
        assert not planes[:,:TOP].any(),('scenery above the crop',rot)
        planes=planes[:,TOP:].copy()
        (OUT/f'ffgv{rot}.bin').write_bytes(extract.pack_view(planes.tobytes()));raw.unlink()
    if not args.reuse_views:
        with ThreadPoolExecutor(4) as pool:list(pool.map(mirror_view,range(4)))
    # Move unchanged byte tables out of the 64K executable, as in Orbonne.
    work=HERE/'campaign/x';work.mkdir(exist_ok=True)
    exporter=work/'tables.c'
    exporter.write_text('''#include <stdio.h>
#include <stddef.h>
#include "../../fft.h"
_Static_assert(sizeof(Job)==14 && sizeof(Item)==26 && sizeof(UnitDef)==24,"disk layout");
_Static_assert(offsetof(UnitDef,day)==6,"birthday offset");
int main(void){
const void *p[5]={jobs,items,unit_defs,abilities,skill_sets};unsigned sizes[5]={sizeof jobs,sizeof items,sizeof unit_defs,sizeof abilities,sizeof skill_sets};
unsigned i;for(i=0;i<5;i++){putchar(sizes[i]>>8);putchar(sizes[i]&255);fwrite(p[i],1,sizes[i],stdout);}return 0;}
''')
    subprocess.run(['cc','-std=gnu11','-o',work/'tables',exporter],check=True)
    tables=subprocess.check_output([work/'tables']);offset=0
    data=bytearray((HERE/'ffti.bin').read_bytes());header=(HERE/'battle.h').read_text()
    # A unit's job and equipment are FFT's ids (the shared tables ffcj, tools/common_tables.py)
    job_ids=[int(v,16) for v in re.search(r'job_ids\[NJOB\][^=]*= \{([^}]*)\}',header)[1].split(',')]
    item_ids=[int(v,16) for v in re.findall(r'^    \{ (0x[0-9A-F]{2}), ',re.search(r'static const Item items\[NITEM\] = \{.*?\n\};',header,re.S)[0],re.M)]
    for name,typ in (('jobs','Job'),('items','Item'),('unit_defs','UnitDef'),('abilities','Ability'),('skill_sets','SkillSet')):
        count=struct.unpack_from('>H',tables,offset)[0];offset+=2;block=bytearray(tables[offset:offset+count]);offset+=count
        if name in ('jobs','items'):continue
        if name=='unit_defs':
            for at in range(0,count,24):
                block[at+6:at+8]=struct.pack('>H',struct.unpack_from('<H',block,at+6)[0])
                block[at+1]=job_ids[block[at+1]]
                for k in range(12,22):block[at+k]=255 if block[at+k]==255 else item_ids[block[at+k]]
        if name=='abilities':
            for at in range(0,count,24):block[at+20:at+22]=struct.pack('>H',struct.unpack_from('<H',block,at+20)[0])
        if len(data)&1:data.append(0)
        replacement='#define %s ((const %s *)(igfx + %dUL))'%(name,typ,len(data))
        header,n=re.subn(r'static const '+typ+r' '+name+r'\[[^]]+\] = \{.*?\n\};',replacement,header,flags=re.S);assert n==1,name
        data+=block
    (OUT/'battle.h').write_text(header);(OUT/'ffgi.bin').write_bytes(data)
    for name in ('fftu','fftw'):
        target=OUT/(name+'.bin');target.unlink(missing_ok=True);target.symlink_to(HERE/(name+'.bin'))
    print('Campaign: Academy',len(messages),'source messages,',len(data),'bytes; four mirrored Gariland views')

if __name__=='__main__':
    main()
    import subprocess
    subprocess.run([sys.executable,HERE/'tools/contour_assets.py','--attach',OUT/'ffgv0.bin'],check=True)
    subprocess.run([sys.executable,HERE/'tools/common_ui.py','campaign'],check=True)  # ffcu, shared with the other chapter
    subprocess.run([sys.executable,HERE/'tools/common_actors.py'],check=True)  # ffca/ffcb/ffcw, shared
    subprocess.run([sys.executable,HERE/'tools/chapter_desc.py','campaign'],check=True)  # its descriptor
