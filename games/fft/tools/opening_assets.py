#!/usr/bin/env python3
"""Build the local Orbonne opening banks. No retail pixels/text are checked in.

Requires opening_oracle.py --story once. Assets use the existing terrain, sprite,
UI and ZX0 converters; only a single chapel viewpoint is retained. Actor records
are deduplicated *before* compression; mirrored poses share the same record.
"""
from pathlib import Path
import argparse, contextlib, json, os, struct, sys
import numpy as np
from scipy import ndimage
from PIL import Image
import extract, units, weapons, interface, battle, management_research as text

HERE=Path(__file__).resolve().parents[1]; OUT=HERE/'opening/data'; REF=HERE/'x/opening/reference'
# Canonical indices preserve the renderer's two fixed-hero identities.
CHARS=[('RAMZA','RAMUZA2','TYPE1'),('DELITA','DILY2','TYPE1'),
       ('AGRIAS','AGURI','TYPE1'),('GAFGARION','H61','TYPE1'),
       ('KNIGHT_F','KNIGHT_W','TYPE1'),('KNIGHT_M','KNIGHT_M','TYPE1'),
       ('ARCHER_M','YUMI_M','TYPE1'),('CHEMIST_M','ITEM_M','TYPE2'),
       ('SQUIRE_M','MINA_M','TYPE1'),('OVELIA','HIME','TYPE2'),('SIMON','SIMON','TYPE2')]
# Existing class comparisons only need the two weapon shape families.
GFX=[0,2,3,4,4,5,6,6,6,7,8]

def packed_frame(s):
    s=s.copy();opaque=s>=0;inside=ndimage.binary_erosion(opaque,iterations=2)
    from art import SHADOW
    for r,row in enumerate(SHADOW.strip('\n').split('\n')):
        for x,c in enumerate(row):
            y=units.FOOT-1+r
            if c!='.' and s[y,x]<0:s[y,x]=2;inside[y,x]=True
    return b''.join(np.packbits(p,axis=1).tobytes() for p in ((s>=0)&((s&1)!=0),(s>=0)&((s&2)!=0),s<0,inside))

def mirror(raw):
    return b''.join(int('{:016b}'.format(v)[::-1],2).to_bytes(2,'big') for v in struct.unpack('>104H',raw))

def battle_greys(small,bg):
    """Keep Orbonne's night-lit scenery in the two middle greys.

    Its first camera mostly sees shaded walls, so reusing that camera's upper
    threshold bleaches the opposite courtyard/facade. Calibrate each view after
    mild texture filtering, excluding the sky and black solid cutaway sides.
    Only the bottom 1% and top 4% of lit scenery receive extreme tones. Actors,
    silhouettes, geometry and depth masks are untouched; all work is offline.
    """
    visible=~bg
    weight=ndimage.gaussian_filter(visible.astype(float),.55)
    filtered=ndimage.gaussian_filter(np.where(visible,small,0),.55)/np.maximum(weight,1e-9)
    lum=(small+filtered)*.5
    q=np.percentile(lum[visible & (lum>1)],[1,50,96])
    return np.where(lum>q[2],0,np.where(lum>q[1],1,np.where(lum>q[0],2,3))).astype(np.uint8)

def shared_floor_materials(disc,files,target):
    """Reuse Gariland's wood/stone UV artwork on the same source terrain types.

    Offline only: no duplicate tile atlas or additional runtime asset is stored.
    Heights, faces, visibility and coverage are the target map's original ones.
    """
    saved=extract.MAP;extract.MAP=22;records=extract.gns_records(disc,files);extract.MAP=saved
    mesh=disc.read(*next(r[1:] for r in records if r[0]==0x2e01))
    raw=np.frombuffer(disc.read(*next(r[1:] for r in records if r[0]==0x1701)),np.uint8).reshape(1024,128)
    tex=np.empty((1024,256),np.uint8);tex[:,::2]=raw&15;tex[:,1::2]=raw>>4
    pal=np.frombuffer(mesh,'<u2',256,struct.unpack_from('<I',mesh,0x44)[0]).reshape(16,16)
    def terrain(m):
        at=struct.unpack_from('<I',m,0x68)[0];nx,nz=m[at:at+2]
        return nx,nz,[m[at+2+8*i]&63 for i in range(nx*nz)]
    nx,nz,surfaces=terrain(mesh);swatches={}
    for v,uv,pl,pg in extract.polygons(mesh):
        x,z=np.floor(v[:,[0,2]].mean(0)/28).astype(int)
        if uv is None or not(0<=x<nx and 0<=z<nz) or np.ptp(v[:,1])>0:continue
        material=surfaces[z*nx+x]
        if material not in (20,21) or material in swatches:continue
        matrix=np.linalg.solve(np.column_stack([v[:,0]/28-x,v[:,2]/28-z,np.ones(3)]),np.array(uv))
        swatches[material]=(matrix,pl,pg)
    assert len(swatches)==2
    nx,nz,surfaces=terrain(target);groups={20:[],21:[]}
    for v,uv,pl,pg in extract.polygons(target):
        x,z=np.floor(v[:,[0,2]].mean(0)/28).astype(int)
        if uv is None or not(0<=x<nx and 0<=z<nz) or np.ptp(v[:,1])>0:continue
        material=surfaces[z*nx+x]
        if material not in swatches:continue
        matrix,pl,pg=swatches[material]
        uv=np.column_stack([v[:,0]/28-x,v[:,2]/28-z,np.ones(3)])@matrix
        groups[material].append((v,uv,pl,pg))
    return groups,tex,pal

def map_assets(disc,files,number,prefix,chapel=False):
    extract.MAP=number;recs=extract.gns_records(disc,files)
    mesh=disc.read(*next((r[1:] for r in recs if r[0]==0x2e01)))
    at=struct.unpack_from('<I',mesh,0x68)[0];nx,nz=mesh[at:at+2]
    if not chapel:
        for v,*_ in extract.polygons(mesh):
            x,z,h=v[:,0]/28,v[:,2]/28,-v[:,1]/12
            for u,w in ((x,z),(nz-z,x),(nx-x,nz-z),(z,nx-x)):
                assert (64+(u+w)*6-h*6).max()<=191,'Native scene would crop the mesh bottom'
    if chapel:
        # Retail event visibility removes the camera-side chapel walls. The
        # native still keeps source floor/altar/rear walls, no invented scenery.
        texrec=next(r for r in recs if r[0]==0x1701);t=np.frombuffer(disc.read(*texrec[1:]),np.uint8)
        tex=np.empty((1024,256),np.uint8);tex[:,::2]=(t&15).reshape(1024,128);tex[:,1::2]=(t>>4).reshape(1024,128)
        pal=np.frombuffer(mesh,dtype='<u2',count=256,offset=struct.unpack_from('<I',mesh,0x44)[0]).reshape(16,16)
        polys=[]
        for poly in extract.polygons(mesh):
            v=poly[0];x,z,h=v[:,0]/28,v[:,2]/28,-v[:,1]/12
            if poly[1] is not None and h.max()<=9 and not (h.max()-h.min()>1 and (x.mean()<2 or z.mean()<2)):
                polys.append(poly)
        lum,diag,face=extract.render(polys,tex,pal,2,nx,nz,212)
        small=lum.reshape(212,4,320,4).mean((1,3));bg=(lum==extract.BG).reshape(212,4,320,4).mean((1,3))>.5
        q=np.percentile(small[~bg],[25,55,82]);g=np.where(small>q[2],0,np.where(small>q[1],1,np.where(small>q[0],2,3)));g[bg]=0
        pack=lambda a:np.packbits(a,axis=1).astype(np.uint8)
        dep=diag[2::4,2::4];layers=[]
        for _ in range(2):
            thr=dep.reshape(212,40,8).max(2);front=(dep==np.repeat(thr,8,1))&(dep>=0)
            layers.extend([np.maximum(thr,0).astype(np.uint8),pack(front)]);dep=np.where(front,-1,dep)
        raw=[b''.join(a.tobytes() for a in [pack(g&1),pack(g>>1)]+layers)];shade=np.zeros((nx*nz,5),int)
        Image.fromarray(np.array([255,170,85,0],np.uint8)[g]).save(OUT/'chapel-cutaway.png')
    else:
        from isometric_style import ledges,outline
        edges=ledges(extract.polygons(mesh));glints=[];styles=[]
        shared,shared_tex,shared_pal=shared_floor_materials(disc,files,mesh)
        # Palette animations, not terrain immersion flags, identify Orbonne's
        # water artwork. The original ping-pongs palette row 5 every 15 ticks.
        anim=struct.unpack_from('<I',mesh,0x6c)[0];water_palettes=set()
        for i in range(32):
            b=mesh[anim+20*i:anim+20*(i+1)]
            if b[14] in (3,4):water_palettes.add(struct.unpack_from('<h',b)[0]//16)
        assert water_palettes=={5},water_palettes
        def detail(g,bg,depth,rot,polys,tex,pal):
            for material,material_polys in shared.items():
                lum,_,_,md=extract.render(material_polys,shared_tex,shared_pal,rot,nx,nz,212,return_depth=True)
                visible=(md[2::4,2::4]>-1e8)&(md[2::4,2::4]>=depth[2::4,2::4]-1e-5)&~bg
                sample=lum[2::4,2::4]
                if visible.any():g[visible]=1+(sample[visible]<np.median(sample[visible])).astype(np.uint8)
            bordered,edge_mask=outline(g,depth,edges,rot,nx,nz)
            edge_mask &= ~bg;g=np.where(edge_mask,bordered,g).astype(np.uint8)
            water_polys=[p for p in polys if p[1] is not None and p[2] in water_palettes]
            water_depth=extract.render(water_polys,tex,pal,rot,nx,nz,212,return_depth=True)[3]
            wd=water_depth[2::4,2::4];scene_depth=depth[2::4,2::4]
            wet=(wd>-1e8)&(wd>=scene_depth-1e-5)&~bg&~edge_mask
            # Retain dark/light texture structure, keep white for the moving glints.
            g[wet]=np.clip(g[wet],1,2)
            groups=[]
            for phase in range(4):
                points=[]
                for y in range(0,192,2):
                    for x in range(0,318,2):
                        if ((x//2+3*(y//2))&3)==phase and wet[y,x] and wet[y,x+1]:points.append((x,y))
                if len(points)>64:points=[points[i] for i in np.linspace(0,len(points)-1,64).astype(int)]
                groups.append(points)
            glints.append(groups);styles.append(dict(rotation=rot,edge_pixels=int(edge_mask[:192].sum()),water_pixels=int(wet[:192].sum()),glints=[len(p) for p in groups]))
            return g
        raw,shade=extract.views(disc,files,nx,nz,10,str(OUT/(prefix+'-preview.png')),quantize=battle_greys,decorate=detail)
        raw=[np.frombuffer(r,np.uint8).reshape(6,212,40)[:,:192].tobytes() for r in raw]
        bank=bytearray(8)
        for rot,groups in enumerate(glints):
            struct.pack_into('>H',bank,rot*2,len(bank))
            for points in groups:
                bank+=struct.pack('>H',len(points))
                for x,y in points:bank+=bytes((x>>1,y)) # Even x fits in one byte at half scale.
        (OUT/'water-fx.bin').write_bytes(bank)
        (OUT/'scene-style.json').write_text(json.dumps(dict(views=styles,water_palettes=sorted(water_palettes),water_fx_bytes=len(bank)),indent=2)+'\n')
    tiles=[]
    for i in range(nx*nz):
        b=mesh[at+2+8*i:at+10+8*i];h,st,sl,flags=b[2],b[3]&31,b[4],b[6]
        c=extract.corners(h,st,sl if st else 0)
        walk=0 if flags&1 else 2 if flags&(2|128) else 1
        # Surface 0x11 is also water. Its depth-four border cells cannot be
        # entered by this human party; the depth-one canal shader is unrelated.
        if 14<=(b[0]&63)<=17 and b[3]>>5>=4:walk=0
        tiles.append((*c,*shade[i],walk,2*h+(st if sl else 0),int((b[0]&63)==14)))
    if chapel:
        # Single camera. Retain the same depth layers used by the tactical renderer.
        w,h,x,y=224,100,-24,48
        # One fixed source-facing camera, with 64 pixels of horizontal pan.
        planes=np.frombuffer(raw[0],np.uint8).reshape(6,-1,40)[:,y:y+h,:25].copy()
        planes=np.pad(planes,((0,0),(0,0),(3,0)))
        (OUT/'ffoch.bin').write_bytes(extract.zx0pack.pack(planes.tobytes()))
        return tiles,(x,y,nx,nz)
    assert not any(t[11] for t in tiles),'Orbonne omits the depth-one immersion shader'
    (OUT/'tiles.bin').write_bytes(bytes(v for t in tiles for v in t))
    (OUT/'map.h').write_text('// Local disc-generated; never committed.\n#define MAP_W %d\n#define MAP_H %d\n'%(nx,nz))
    from concurrent.futures import ThreadPoolExecutor
    with ThreadPoolExecutor(4) as ex:
        for rot,data in enumerate(ex.map(extract.pack_view,raw)):(OUT/('ffov%d.bin'%rot)).write_bytes(data)
    return tiles,(nx,nz)

def source_title(disc,files):
    from management_assets import title_strip
    return title_strip(disc,files,55)[0]

def dialogue(disc,files,chapel_tiles,cam,scene_pose):
    raw=disc.read(*files['EVENT/TEST.EVT']);records=json.loads((REF/'story.json').read_text())
    # The original uses a normal and an internal companion window for each message.
    beats=[];seen=set();text.PUNCT.update({2064:'"',2073:'?',64:"'",234:'!'} )
    # 0xEA is an exclamation glyph; unlike the management audit it is not a substitution.
    def decode(tokens):
        toks=[]
        for c in tokens:toks.extend([62] if c==234 else [62,63] if c==64 else [217,193] if c==147 else [c])
        lines,unknown,dynamic=text.decode(toks)
        lines=[line.replace('What s',"What's").replace('He s',"He's").replace('There s',"There's").replace('there s',"there's").replace('We can t',"We can't").replace('Don t',"Don't").replace('You re',"You're").replace('Goltana s',"Goltana's") for line in lines]
        if unknown:raise ValueError(('unmapped dialogue glyph',unknown))
        return lines
    for q in records:
        event,mid=q['event'],q['message'];key=(event,mid)
        if key in seen:continue
        seen.add(key)
        b=raw[event*8192:(event+1)*8192];at=struct.unpack_from('<I',b)[0]
        starts=[at]+[i+1 for i in range(at,8192) if b[i]>=254]
        lines=decode(text.expanded(b,starts[mid]));name=lines.pop(0) if mid or event!=2 else 'Ovelia'
        if name=='Ramza':pass
        if not lines:continue
        body=' '.join(s.strip() for s in lines if s!='|').strip().replace('Rad, Ramza','Rad, Ramza')
        beats.append(dict(event=event,message=mid,speaker=name,text=body,actors=[],duration=0,pan=0))
    scene_rows=json.loads((HERE/'x/opening/choreography/scene-02.json').read_text())
    ending_rows=json.loads((HERE/'x/opening/choreography/scene-06.json').read_text())
    direction=((2,1,0,3),(0,3,1,2),(1,2,3,0),(3,0,2,1))
    sheet_gfx={2:0,5:1,52:2,23:3,101:4,100:5,102:6,98:7,96:8,12:9,19:10}
    def stage(row,chapel=False):
        result=[];rot=2 if chapel else 0
        for u in row['actors']:
            sheet=u['sheet'];x,y,z=u['xyz']
            if u['id']>=16 or sheet not in sheet_gfx or not (0<=x<=420 and 0<=z<=420) or u['frame']>=512:continue
            gfx=sheet_gfx[sheet]
            if chapel:
                # Source screen coordinates include sub-tile event movement and heights.
                sx=round(32+(x-z)*12/28)-cam[0]
                sy=round(64+(24-(x+z)/28)*6+y/2)-cam[1]
                dep=round(24-(x+z)/28)
            else:
                sx=round(320-(14+(x-z)/28)*12)
                sy=round(64+(x+z)/28*6+y/2)
                dep=round((x+z)/28)
            # Source frame already selects front/back; facing determines mirroring.
            face=direction[rot][battle.FACE[(u['facing']//1024)&3]]^2
            logical=scene_pose(gfx,u['frame'],bool(face&2),u)
            slot={12:6}.get(u['id'],u['id'])
            if slot>=11:continue
            result.append([(slot<<4)|gfx,sx,sy,(max(0,min(63,dep))<<2)|(face&3),logical])
        return sorted(result,key=lambda a:a[3]>>2)
    for beat in beats:
        event,mid=beat['event'],beat['message']
        rows=scene_rows if event==2 else ending_rows if event==6 else []
        row=next((r for r in rows if r['event']==event and any(m['message']==mid for m in r['messages'])),None)
        if event==2:
            assert row is not None,(event,mid)
            beat['actors']=stage(row,True);beat['pan']=48 if mid<=4 or mid==17 else 0
        elif event==6:
            if row:beat['actors']=stage(row)
    # The first source event-6 snapshot begins just after Ovelia's first cry.
    after=next(b for b in beats if b['event']==6)
    if not after['actors']:after['actors']=stage(ending_rows[0])
    # Use a single rear-of-monastery orientation throughout the conclusion.
    # Silent frames preserve punch/collapse and a departure, independent of text speed.
    for mid,at in ((12,1200),(14,1680)):
        index=next(i for i,b in enumerate(beats) if b['event']==6 and b['message']==mid)+1
        row=min((r for r in ending_rows if r['event']==6),key=lambda r:abs(r['frame']-at))
        fallen=stage(row)
        if mid==12:
            standing=next(b['actors'] for b in beats if b['event']==6 and b['message']==11)
            wind=[a.copy() for a in standing];blow=[a.copy() for a in standing]
            for group,frame in ((wind,27),(blow,31)):
                for a in group:
                    if (a[0]&15)==1:a[4]=scene_pose(1,frame,bool(a[3]&2),{})
                    if (a[0]&15)==9 and frame==31:a[4]=scene_pose(9,19,bool(a[3]&2),{})
            beats[index:index]=[dict(event=6,message=-1,speaker='',text='',actors=wind,duration=8,pan=0),
                                dict(event=6,message=-1,speaker='',text='',actors=blow,duration=8,pan=0),
                                dict(event=6,message=-1,speaker='',text='',actors=fallen,duration=36,pan=0)]
        else:beats.insert(index,dict(event=6,message=-1,speaker='',text='',actors=fallen,duration=36,pan=0))
    # The retail escape switches to movie choreography. Its native substitute
    # reuses the retail Chocobo sheet and the already extracted rider poses.
    departure=next(i for i,b in enumerate(beats) if b['event']==6 and b['message']==15)
    row=min((r for r in ending_rows if r['event']==6),key=lambda r:abs(r['frame']-1392))
    riders=stage(row);keepers=[a for a in riders if a[0]>>4 not in (5,6,7)]
    bird=scene_pose(11,2,False,{})
    pose_delita=next(a[4] for a in riders if (a[0]&15)==1)
    pose_ovelia=next(a[4] for a in riders if (a[0]&15)==9)
    keepers += [[(7<<4)|11,300,118,80,bird],[(5<<4)|1,300,110,84,pose_delita],[(6<<4)|9,306,107,85,pose_ovelia]]
    beats.insert(departure,dict(event=6,message=-2,speaker='',text='',actors=keepers,duration=100,pan=1))
    # Preserve the same source art/title strip that the management prototype uses.
    title=source_title(disc,files)
    # Bank: beat count, directory; each beat has event, actor count, actors, two NUL strings.
    out=bytearray(2+2*len(beats));struct.pack_into('>H',out,0,len(beats))
    for i,b in enumerate(beats):
        if len(out)&1:out.append(0)
        struct.pack_into('>H',out,2+2*i,len(out));out+=bytes((b['event'],len(b['actors']),b['duration'],b['pan']))
        for gfx,x,y,face,pose in b['actors']:out+=struct.pack('>BBhhH',gfx,face,x,y,pose)
        out+=b['speaker'].encode('ascii')+b'\0'+b['text'].encode('ascii')+b'\0'
    chapel=(OUT/'ffoch.bin').read_bytes()
    (OUT/'ffost.bin').write_bytes(struct.pack('>H',len(out)+2)+out+chapel+title);(OUT/'story.json').write_text(json.dumps(beats,indent=2)+'\n')
    return beats

def main():
    ap=argparse.ArgumentParser(__doc__);ap.add_argument('disc');ap.add_argument('--reuse-scenes',action='store_true');ap.add_argument('--facings-only',action='store_true');args=ap.parse_args();OUT.mkdir(parents=True,exist_ok=True)
    disc,files=extract.disc_files(args.disc);read=lambda s:disc.read(*files[s]);extract.SOURCE_CAMERA=True
    import oracle
    p=oracle.PSX(args.disc);p.load(str(REF/'event-05.state'))
    if args.facings_only:
        import re
        header=(OUT/'battle.h').read_text()
        offset=int(re.search(r'#define unit_defs .*?igfx \+ (\d+)UL',header)[1])
        path=OUT/'ffoi.bin';bank=bytearray(path.read_bytes())
        for i in range(11):
            a=oracle.UNITS+i*448;at=offset+24*i
            assert bank[at+8:at+10]==bytes((p.read(a+0x47),p.read(a+0x48)))
            bank[at+10]=battle.FACE[p.read(a+0x49)&3]
        path.write_bytes(bank)
        metadata=OUT/'units.json'
        if metadata.exists():
            records=json.loads(metadata.read_text())
            for i,record in enumerate(records):record['face']=bank[offset+24*i+10]
            metadata.write_text(json.dumps(records,indent=2)+'\n')
        print('Refreshed all opening tactical facings from unit records, not transient actor angles')
        return
    # Confirm actor files against the original's resolved sheet table and packed body.
    bat=read('BATTLE.BIN');source=[];bd=battle.Disc(args.disc)
    for i in range(11):
        a=oracle.UNITS+i*448;r=lambda o,k=1:p.read(a+o,k)
        sheet=r(0x15f);lba,size=struct.unpack_from('<II',bat,0x80094cd4-battle.BAT+8*sheet)
        path=next(k for k,v in files.items() if v[0]==lba);assert path=='BATTLE/'+CHARS[GFX[i]][1]+'.SPR',(i,path)
        raw=[r(0x72+3*k,3) for k in range(5)];eq=[r(0x1a+k) for k in (0,1,2,3,6)]
        got=[r(0x2a,2),r(0x2e,2),r(0x38),r(0x36),r(0x37),r(0x3a),r(0x3b)]
        want=battle.stats(bd,r(3),1,raw,[r(0x1a+k) for k in range(7)])
        assert got==[want[k] for k in ('hp','mp','sp','pa','ma','move','jump')],(i,got,want)
        # Stored PS1 raw stats have already grown to this level. Invert each
        # integer step exactly so the common native level-up model starts at level 1.
        for k in range(5):
            for lv in range(r(0x22),1,-1):
                den=bd.job(r(3))['growth'][k]+lv-1;v=raw[k];prev=v*den//(den+1)
                while prev+prev//den<v:prev+=1
                assert prev+prev//den==v
                raw[k]=prev
        name=interface.decode(bytes(p.ram[p.addr(a+0x12c):p.addr(a+0x13c)]).split(b'\xfe')[0]);name='Ramza' if i==0 else name
        # Tactical facing is authoritative; the scene actor angle may still be turning.
        source.append(dict(gfx=GFX[i],job=r(3),type=1 if r(6)&64 else 0,level=r(0x22),brave=r(0x24),faith=r(0x26),day=r(8,2)&511,
                           x=r(0x47),z=r(0x48),face=battle.FACE[r(0x49)&3],team=0 if i==0 else 1 if 5<=i<=9 else 2,
                           eq=eq,raw=raw,name=name,ct=0,exp=r(0x21),zodiac=r(9)>>4,stats=got))
    # Data tables for the common combat model. No fixture positions leak into code.
    jobs=sorted({u['job'] for u in source});items=sorted({v for u in source for v in u['eq'] if v!=255})
    sets=[('Guts',[0,1,2]),('Holy Sword',[3]),('Dark Sword',[4]),('Battle Skill',[5,6]),('Charge',[7,8]),('Item',[9,10]),('Basic Skill',[1,11])]
    abil=[('Accumulate','AB_RAISE',1,0,0,'AF_SELF',255),('Dash','AB_DAMAGE',4,1,2,'0',255),('Wish','AB_WISH',0,1,6,'0',255),
          ('Stasis Sword','AB_DAMAGE',2,2,255,'AF_WEAPON | AF_AREA',255),('Night Sword','AB_DAMAGE',0,3,255,'AF_WEAPON | AF_DRAIN',255),
          ('Speed Break','AB_BREAK',2,1,6,'AF_EVADE',255),('Magic Break','AB_BREAK',3,1,6,'AF_EVADE',255),
          ('Charge +1','AB_DAMAGE',1,5,255,'AF_WEAPON | AF_EVADE | AF_CHARGE',255),('Charge +2','AB_DAMAGE',2,5,255,'AF_WEAPON | AF_EVADE | AF_CHARGE',255),
          ('Potion','AB_HEAL',bd.ability(0x170)['y'],4,255,'AF_SELF | AF_ITEM',0),('Eye Drop','AB_CURE','ST_DARK',4,255,'AF_SELF | AF_ITEM',1),('Throw Stone','AB_DAMAGE',2,4,255,'AF_EVADE',255)]
    unitsets=[0,1,2,3,3,3,4,4,4,5,6];learned=[7,1,1,2,0,3,1,1,3,3,2]
    f=['// Generated from local original data; never committed.','#define NJOB %d\n#define NITEM %d\n#define NUNIT 11'%(len(jobs),len(items)),'static const Job jobs[NJOB] = {']
    for j in jobs:
        q=bd.job(j);f.append('{ {%s}, {%s}, %d,%d,%d,%d },'%(','.join(map(str,q['growth'])),','.join(map(str,q['mult'])),q['move'],q['jump'],q['cev'],j-0x4a if j>=0x4a else 0))
    f+=['};','static const Item items[NITEM] = {']
    # Item names are original labels, IDs/values are read from the disc.
    names={2:'Mythril Knife',20:'Long Sword',21:'Iron Sword',22:'Mythril Sword',83:'Long Bow',128:'Escutcheon',129:'Buckler',132:'Bronze Shield',133:'Round Shield',145:'Leather Helmet',146:'Bronze Helmet',148:'Iron Helmet',149:'Barbut',150:'Mythril Helm',157:'Leather Hat',158:'Feather Hat',159:'Red Hood',173:'Leather Armor',174:'Linen Cuirass',175:'Bronze Armor',177:'Chain Mail',178:'Mythril Armor',186:'Clothes',187:'Leather Outfit',189:'Leather Vest',208:'Battle Boots',215:'Power Wrist'}
    for item in items:
        q=bd.item(item);wp=read('SCUS_942.21')[battle.WEAPONS-battle.EXE+8*item+4] if q['flags']&128 else 0
        f.append('{%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"%s"},'%(item,q['hp'],q['mp'],q['pa'],q['ma'],q['sp'],q['move'],q['jump'],wp,2 if q['type']==12 else int(q['type']==1),names.get(item,'Item %02X'%item)[:12]))
    f+=['};','#define NABIL %d\nstatic const Ability abilities[NABIL] = {'%len(abil)]
    for name,kind,y,r,v,fl,bag in abil:f.append('{"%s",%s,%s,%d,%d,%s,%d,100,0,1},'%(name,kind,y,r,v,fl,bag))
    f+=['};','#define NSET %d\nstatic const SkillSet skill_sets[NSET] = {'%len(sets)]
    for name,ids in sets:f.append('{"%s",{%s}},'%(name,','.join(map(str,ids+[255]*(6-len(ids))))))
    f+=['};','#define SET_BASIC 6\n#define NRSM_PRE 1\n#define DEFEND_JP 50\n#define DEFEND_RATE 0\n#define NBAG 2','static const u8 rsm_pre[1][2]={{0,0}};','static const u8 bag_init[2]={5,1};','static const u8 gen_base[3][5]={{0}}, gen_var[3][5]={{0}};','static const u16 zodiac_days[12]={%s};'%','.join(map(str,struct.unpack('<12H',bd.at(battle.ZODIAC,24)))),'static const u8 zodiac_compat[12]={%s};'%','.join(map(str,bd.bat[battle.ZCOMPAT-battle.BAT:battle.ZCOMPAT-battle.BAT+12])),'static const UnitDef unit_defs[NUNIT] = {']
    for i,u in enumerate(source):
        f.append('{%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,{%s},%d,%d},'%(u['gfx'],jobs.index(u['job']),u['type'],u['level'],u['brave'],u['faith'],u['day'],u['x'],u['z'],u['face'],u['team'],','.join('{%d,255}'%(items.index(v) if v in items else 255) for v in u['eq']),unitsets[i],learned[i]))
    f+=['};','static const u32 opening_raw[NUNIT][5]={'+','.join('{'+','.join(str(v)+'UL' for v in u['raw'])+'}' for u in source)+'};','static const char *const opening_names[NUNIT]={'+','.join(json.dumps(u['name']) for u in source)+'};','static const u8 opening_zodiac[NUNIT]={'+','.join(str(u['zodiac']) for u in source)+'};']
    charge=bd.at(0x80061044,16)[::2]
    f+=['static const u8 opening_charge_ticks[8]={'+','.join(map(str,charge))+'};']
    (OUT/'battle.h').write_text('\n'.join(f)+'\n');(OUT/'units.json').write_text(json.dumps(source,indent=2)+'\n')
    if not args.reuse_scenes:map_assets(disc,files,56,'battle')
    chapel,cam=map_assets(disc,files,62,'chapel',True)
    # Existing converters, isolated working directory and patched source sheet list.
    units.CHARS=CHARS;interface.CHARS=CHARS
    old=os.getcwd();os.chdir(OUT)
    try:
        interface.main_args=None;argv=sys.argv;sys.argv=['interface.py',args.disc]
        # Absolute disc name is needed after changing directory.
        sys.argv[1]=str(Path(old,args.disc).resolve());interface.main();sys.argv=argv
    finally:os.chdir(old)
    # Generate logical base and armed frames; only 2 idle poses per view for one-off actors.
    shapes={t:units.shp_frames(read('BATTLE/%s.SHP'%t)) for t in ('TYPE1','TYPE2')}
    records=[];canonical={};mapping=[];full=[];spritepreview=[]
    def store(raw):
        flipped=mirror(raw)
        if raw in canonical:return canonical[raw]
        if flipped in canonical:return canonical[flipped]|0x8000
        key=len(records);canonical[raw]=key;records.append(raw);return key
    for gfx,(_,file,typ) in enumerate(CHARS):
        spr=read('BATTLE/%s.SPR'%file)
        if gfx in (5,6,7):spr=spr[64:96]+spr[32:] # Original enemy palette 2, replace palette zero.
        frames=[]
        for k in units.FRAMES:
            frame=(k if k<19 else 9) if gfx==1 else k if gfx<9 else (9 if k%2 else 10)+(5 if k>=14 else 0)
            s=units.sprite(spr,shapes[typ][frame],units.LIFT.get(frame,0));frames.append(packed_frame(s));spritepreview.append(s)
        frames+=list(map(mirror,frames));full+=frames;mapping.extend(map(store,frames))
    base=len(mapping)
    for gfx,(_,file,typ) in enumerate(CHARS):
        spr=read('BATTLE/%s.SPR'%file)
        if gfx in (5,6,7):spr=spr[64:96]+spr[32:]
        poses=[packed_frame(units.sprite(spr,shapes[typ][k])) for k in (86,87,89,90,135,136,138,139)] if gfx<9 and gfx!=1 else [full[gfx*44]]*8
        poses+=list(map(mirror,poses));full+=poses;mapping.extend(map(store,poses))
    # Reuse the exact Gariland crystal/chest frames, not an entire duplicate actor bank.
    oldunits=(HERE/'fftu.bin').read_bytes();objects=oldunits[6*44*208:];objbase=len(mapping)
    for at in range(0,len(objects),208):mapping.append(store(objects[at:at+208]))
    scene_keys={}
    def scene_pose(gfx,frame,flipped,u):
        key=(gfx,frame,flipped,u.get('sprite'),tuple(tuple(p) for p in u.get('parts',[])))
        if key not in scene_keys:
            _,file,typ=CHARS[gfx] if gfx<11 else ('CHOCOBO','CYOKO','CYOKO')
            if typ not in shapes:shapes[typ]=units.shp_frames(read('BATTLE/%s.SHP'%typ))
            spr=read('BATTLE/%s.SPR'%file)
            if u.get('sprite'):
                source=(HERE/'x/opening/choreography'/u['sprite']).read_bytes()
                spr=spr[:512]+source[512:]
                parts=[(x,y,tx,ty,w,h,bool(flags&2),bool(flags&4)) for x,y,w,h,tx,ty,flags in u['parts']]
            else:parts=shapes[typ][frame]
            scale=units.SCALE
            if gfx==11:units.SCALE=.4
            try:raw=packed_frame(units.sprite(spr,parts,units.LIFT.get(frame,0)))
            finally:units.SCALE=scale
            scene_keys[key]=len(mapping);mapping.append(store(mirror(raw) if flipped else raw))
        return scene_keys[key]
    beats=dialogue(disc,files,chapel,cam,scene_pose)
    packed=[extract.zx0pack.pack(r) for r in records];directory=bytearray(2*len(records));data=bytearray()
    for i,r in enumerate(packed):struct.pack_into('>H',directory,2*i,len(data));data+=r
    blob=struct.pack('>%dH'%len(mapping),*mapping)+directory+data
    (OUT/'ffoa.bin').write_bytes(blob)
    (OUT/'units.h').write_text('// Generated locally; never committed.\nenum {'+','.join('UG_'+n for n,_,_ in CHARS)+',UNIT_GFX_N};\n#define UNIT_FRAMES 22\n#define UNIT_SH 26\n#define UNIT_FOOT 21\n#define UNIT_FB 208\nenum {P_HIT=10,P_WIND=12,P_BLOW=14,P_KNEEL=16,P_KO=18,P_THROW=20};\nenum {OBJ_CRYSTAL,OBJ_CHEST,OBJ_CHEST_UP1=9,OBJ_CHEST_UP2,OBJ_N};\n#define CRYSTAL_FRAMES 8\n#define UG_CHEMIST_F UG_CHEMIST_M\n#define UG_SQUIRE_F UG_KNIGHT_F\n#define ACTOR_ARMED %d\n#define ACTOR_OBJECTS %d\n#define ACTOR_DIR %d\n#define ACTOR_DATA %d\n'%(base,objbase,2*len(mapping),2*(len(mapping)+len(records))))
    # Blades: tools/common_actors.py writes the shared ffcw and its weapons.h.
    # Only eight battle portraits and no unused 510-name dictionary. Scripted
    # characters use the fitted name badge, never their own portrait image.
    import re
    hdr=(OUT/'interface.h').read_text();blob=(OUT/'ffti.bin').read_bytes()
    matches=list(re.finditer(r'#define UI_(\w+) (\d+)UL',hdr));rebuilt=bytearray()
    for k,m in enumerate(matches):
        a=int(m[2]);b=int(matches[k+1][2]) if k+1<len(matches) else len(blob);part=blob[a:b]
        if m[1]=='PORTRAITS':part=b''.join(part[i*348:(i+1)*348] for i in (0,2,3,4,5,6,7,8))
        if m[1] in ('MALE','FEMALE'):part=b''
        hdr=hdr.replace(m[0],'#define UI_%s %dUL'%(m[1],len(rebuilt)));rebuilt+=part
    (OUT/'interface.h').write_text(hdr);(OUT/'ffoi.bin').write_bytes(rebuilt);(OUT/'ffti.bin').unlink()
    # Read immutable byte-only combat tables directly from archive. UnitDef's
    # sole 16-bit field (birthday) is unused here and serialized as zero.
    tables={}
    ud=bytearray()
    for i,u in enumerate(source):
        ud+=bytes((u['gfx'],u['job'],u['type'],u['level'],u['brave'],u['faith'],0,0,u['x'],u['z'],u['face'],u['team']))
        ud+=bytes(v for eq in u['eq'] for v in (eq,255))   # FFT's ids (ffcj: tools/common_tables.py)
        ud+=bytes((unitsets[i],learned[i]))
    assert len(ud)==11*24
    tables['unit_defs']=bytes(ud)
    symbols=dict(AB_DAMAGE=0,AB_CURE=1,AB_WISH=2,AB_HEAL=3,AB_REVIVE=4,AB_DEFEND=5,AB_RAISE=6,AB_BREAK=7,
                 AF_SELF=1,AF_EVADE=2,AF_ITEM=4,AF_WEAPON=8,AF_DRAIN=16,AF_AREA=32,AF_CHARGE=64,ST_DARK=4)
    def value(v):return v if isinstance(v,int) else sum(symbols[t.strip()] if t.strip() in symbols else int(t.strip()) for t in v.split('|'))
    tables['abilities']=b''.join(struct.pack('>13s7BHBx',name.encode('ascii'),value(kind),value(y),r,v,value(flags),bag,100,0,1) for name,kind,y,r,v,flags,bag in abil)
    tables['skill_sets']=b''.join(name.encode('ascii').ljust(12,b'\0')+bytes(ids+[255]*(6-len(ids))) for name,ids in sets)
    bank=bytearray((OUT/'ffoi.bin').read_bytes());hdr=(OUT/'battle.h').read_text()
    for name,typ in (('unit_defs','UnitDef'),('abilities','Ability'),('skill_sets','SkillSet')):
        if len(bank)&1:bank.append(0)
        replacement='#define %s ((const %s *)(igfx + %dUL))'%(name,typ,len(bank))
        hdr,n=re.subn(r'static const '+typ+r' '+name+r'\[[^]]+\] = \{.*?\n\};',replacement,hdr,flags=re.S);assert n==1,name
        bank+=tables[name]
    if len(bank)&1:bank.append(0)
    hdr=re.sub(r'static const u32 opening_raw[^;]+;','#define OPENING_RAW %dUL'%len(bank),hdr)
    bank+=struct.pack('>55I',*(v for u in source for v in u['raw']))
    hdr=re.sub(r'static const char \*const opening_names[^;]+;','#define OPENING_NAMES %dUL'%len(bank),hdr)
    directory=len(bank);bank+=bytes(2*len(source))
    for i,u in enumerate(source):
        struct.pack_into('>H',bank,directory+2*i,len(bank));bank+=u['name'].encode('ascii')+b'\0'
    if len(bank)&1:bank.append(0)
    hdr+='\n#define OPENING_WATER %dUL\n'%len(bank)
    bank+=(OUT/'water-fx.bin').read_bytes()
    (OUT/'battle.h').write_text(hdr);(OUT/'ffoi.bin').write_bytes(bank)
    (OUT/'story.h').write_text('#define OPENING_BEATS %d\n#define CHAPEL_X %d\n#define CHAPEL_Y %d\n#define CHAPEL_STRIDE 28\n#define CHAPEL_PLANE 2800\n#define OPENING_TITLE_FROM_END 756\n#define OPENING_CHAPEL_BYTES %d\n'%(len(beats),cam[0],cam[1],(OUT/'ffoch.bin').stat().st_size))
    report=dict(actor_logical_frames=len(mapping),actor_unique_frames=len(records),actor_raw_with_mirrors=len(mapping)*208,actor_bank_bytes=(OUT/'ffoa.bin').stat().st_size,chapel_only_raw_frames=2*44*208,chapel_only_unique_bytes=sum(len(packed[k]) for k in set(v&32767 for v in mapping[9*44:11*44])),archive={p.name:p.stat().st_size for p in OUT.glob('*.bin')},original_units_checked=11,source_disc_sha256=__import__('hashlib').sha256(read('SCUS_942.21')).hexdigest(),dialogue_beats=len(beats),scene_pose_count=len(scene_keys),charge_ticks=list(charge),potion_hp=bd.ability(0x170)['y']*10,view_crop='Orbonne decorative towers above native scene; walkable terrain unmodified')
    (OUT/'storage.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':
    main()
    import subprocess
    subprocess.run([sys.executable,HERE/'tools/contour_assets.py','--attach',OUT/'ffov0.bin'],check=True)
    subprocess.run([sys.executable,HERE/'tools/common_ui.py','opening'],check=True)  # ffcu, shared with the other chapter
    subprocess.run([sys.executable,HERE/'tools/common_actors.py'],check=True)  # ffca/ffcb/ffcw, shared
    subprocess.run([sys.executable,HERE/'tools/chapter_desc.py','opening'],check=True)  # its descriptor
    subprocess.run([sys.executable,HERE/'tools/scenes.py'],check=True)  # the scene player's sets
