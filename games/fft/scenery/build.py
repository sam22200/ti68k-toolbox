#!/usr/bin/env python3
"""Build a genuinely shared terrain-piece bank, two metadata maps and exact controls.

Retail source pixels remain under ignored x/. Does not alter production banks.
"""
from pathlib import Path
import argparse
import collections
import json
import struct
import subprocess
import sys
import tempfile
import numpy as np
from PIL import Image, ImageDraw

HERE=Path(__file__).resolve().parent
GAME=HERE.parent
ROOT=GAME.parents[1]
sys.path.insert(0,str(GAME/'tools'))
import extract

OUT=HERE/'x'
MATERIAL_NAMES={3:'grass',6:'earth',14:'water',17:'deep water',20:'wood',21:'paving',
                22:'roof',30:'surface30',31:'surface31',33:'surface33',35:'surface35'}


def view_xy(rot,x,z,nx,nz):
    return ((x,z),(nz-1-z,x),(nx-1-x,nz-1-z),(z,nx-1-x))[rot]


def heights(t,rot):
    return tuple(t[(8-c-rot)&3] for c in range(4))


def shape_key(h):
    lo=min(h)
    assert max(h)-lo<=3
    return sum((v-lo)<<(2*i) for i,v in enumerate(h))


def raster(vertices,uv,texture,side=False):
    """Small convex/split-quad piece, nearest sampling and an interior black edge."""
    vertices=np.asarray(vertices,float)
    ox=int(vertices[:,0].min());oy=int(vertices[:,1].min())
    width=int(vertices[:,0].max())-ox;rows=int(vertices[:,1].max())-oy
    assert width<=24 and rows<128 and -128<=oy<128
    canvas=np.full((rows,24),-1,np.int8)
    Y,X=np.mgrid[oy:oy+rows,ox:ox+24];X=X+.5;Y=Y+.5
    for tri in ((0,1,2),(0,2,3)):
        v=vertices[list(tri)];ax,bx,cx=v[:,0];ay,by,cy=v[:,1]
        den=(by-cy)*(ax-cx)+(cx-bx)*(ay-cy)
        if abs(den)<1e-8:continue
        a=((by-cy)*(X-cx)+(cx-bx)*(Y-cy))/den
        b=((cy-ay)*(X-cx)+(ax-cx)*(Y-cy))/den;c=1-a-b
        mask=(a>=0)&(b>=0)&(c>=0)
        coords=np.asarray(uv)[list(tri)]
        u=np.floor(a*coords[0,0]+b*coords[1,0]+c*coords[2,0]).astype(int)&7
        w=np.floor(a*coords[0,1]+b*coords[1,1]+c*coords[2,1]).astype(int)&7
        color=texture[w,u].copy()
        if side:color=np.clip(color+int(side==2),1,3)
        canvas[mask]=color[mask]
    # A narrow top/wall seam supplies monochrome relief. Deliberately a material
    # approximation: it is compared against the exact source below, never hidden.
    if side:
        for x in range(24):
            y=np.flatnonzero(canvas[:,x]>=0)
            if len(y):canvas[y[0],x]=3
    data=bytearray()
    for row in canvas:
        planes=[]
        for mask in ((row>=0)&((row&1)!=0),(row>=0)&((row&2)!=0),row>=0):
            value=sum(int(v)<<(31-x) for x,v in enumerate(mask));planes.append(value)
        data+=struct.pack('>3I',*planes)
    return (32,rows,ox,oy,bytes(data)),canvas


def maps_from_disc(disc,files):
    maps=[]
    for name,number,rows in (('gariland',22,218),('orbonne',56,192)):
        extract.MAP=number;records=extract.gns_records(disc,files)
        mesh=disc.read(*next(r[1:] for r in records if r[0]==0x2e01))
        at=struct.unpack_from('<I',mesh,0x68)[0];nx,nz=mesh[at:at+2]
        tiles=[]
        for i in range(nx*nz):
            b=mesh[at+2+8*i:at+10+8*i]
            corners=extract.corners(b[2],b[3]&31,b[4] if b[3]&31 else 0)
            tiles.append(tuple(corners)+(b[0]&63,))
        raw=np.frombuffer(disc.read(*next(r[1:] for r in records if r[0]==0x1701)),np.uint8).reshape(1024,128)
        texture=np.empty((1024,256),np.uint8);texture[:,::2]=raw&15;texture[:,1::2]=raw>>4
        palette=np.frombuffer(mesh,'<u2',256,struct.unpack_from('<I',mesh,0x44)[0]).reshape(16,16)
        maps.append(dict(name=name,number=number,rows=rows,nx=nx,nz=nz,tiles=tiles,mesh=mesh,texture=texture,palette=palette))
    return maps


def swatches(maps,materials):
    found={};sources={}
    for m in maps: # Gariland takes priority, even when the other map is prepared first.
        for vertex,uv,pal,page in extract.polygons(m['mesh']):
            x,z=np.floor(vertex[:,[0,2]].mean(0)/28).astype(int)
            if uv is None or np.ptp(vertex[:,1]) or not(0<=x<m['nx'] and 0<=z<m['nz']):continue
            material=m['tiles'][z*m['nx']+x][4]
            if material not in materials or material in found:continue
            xy=np.column_stack([vertex[:,0]/28-x,vertex[:,2]/28-z,np.ones(3)])
            if abs(np.linalg.det(xy))<1e-7:continue
            matrix=np.linalg.solve(xy,np.asarray(uv))
            yy,xx=np.mgrid[:8,:8];sample=np.stack([(xx+.5)/8,(yy+.5)/8,np.ones_like(xx)],-1)@matrix
            u=np.clip(np.floor(sample[:,:,0]).astype(int),0,255)
            v=np.clip(np.floor(sample[:,:,1]).astype(int),0,255)+page*256
            colors=m['palette'][pal][m['texture'][v,u]].astype(int)
            lum=(colors&31)*299+((colors>>5)&31)*587+((colors>>10)&31)*114
            # Same two midtones across maps, retaining the source material pattern.
            pattern=np.where(lum<np.median(lum),2,1).astype(np.int8)
            found[material]=pattern;sources[material]=dict(map=m['name'],tile=[int(x),int(z)],palette=int(pal),page=int(page))
    for material in materials:
        if material not in found:
            # Some terrain flags have no isolated horizontal textured face. Their
            # material is explicitly aliased, not silently assigned source fidelity.
            fallback=14 if material==17 else 21
            found[material]=found[fallback];sources[material]=dict(alias=fallback)
    return found,sources


def serialize_bank(maps):
    materials=sorted({t[4] for m in maps for t in m['tiles']})
    styles,source=swatches(maps,materials)
    shapes=sorted({shape_key(heights(t,rot)) for m in maps for t in m['tiles'] for rot in range(4)})
    shape_index={s:i for i,s in enumerate(shapes)}
    pieces=[];pictures=[];dedup={};uses=collections.defaultdict(set)
    def add(piece,picture,owner):
        if piece not in dedup:dedup[piece]=len(pieces);pieces.append(piece);pictures.append(picture)
        index=dedup[piece];uses[index].add(owner);return index
    top={}
    for m in maps:
        for t in m['tiles']:
            mat=t[4]
            for rot in range(4):
                h=heights(t,rot);low=min(h);h=tuple(v-low for v in h);key=(mat,shape_key(h),rot)
                # Rotate the source material with its world-space cell.
                uv=[[(0,0),(8,0),(8,8),(0,8)][(8-c-rot)&3] for c in range(4)]
                vertices=[(0,-6*h[0]),(12,6-6*h[1]),(0,12-6*h[2]),(-12,6-6*h[3])]
                piece,picture=raster(vertices,uv,styles[mat])
                top[key]=add(piece,picture,m['name'])
    units={};caps={}
    for wall,material in enumerate((21,20)):
        texture=styles[material]
        for side in range(2):
            if side==0:v=[(0,6),(-12,0),(-12,6),(0,12)]
            else:v=[(12,0),(0,6),(0,12),(12,6)]
            uv=[(0,0),(8,0),(8,8),(0,8)]
            units[wall,side]=add(*raster(v,uv,texture,side=side+1),'shared walls')
            for delta in range(-3,4):
                if not delta:caps[wall,side,delta]=65535;continue
                a=max(delta,0);b=max(-delta,0)
                v=([(0,12-6*a),(-12,6-6*b),(-12,6),(0,12)] if side==0 else
                   [(12,6-6*a),(0,12-6*b),(0,12),(12,6)])
                caps[wall,side,delta]=add(*raster(v,uv,texture,side=side+1),'shared walls')
    bank=bytearray(20);shapes_off=len(bank)
    bank+=bytes(shape_index.get(i,255) for i in range(256));tops_off=len(bank)
    for mat in materials:
        for key in shapes:
            for rot in range(4):bank+=struct.pack('>H',top.get((mat,key,rot),65535))
    units_off=len(bank)
    for wall in range(2):
        for side in range(2):bank+=struct.pack('>H',units[wall,side])
    caps_off=len(bank)
    for wall in range(2):
        for side in range(2):
            for delta in range(-3,4):bank+=struct.pack('>H',caps[wall,side,delta])
    directory=len(bank);bank+=bytes(8*len(pieces))
    for i,(width,rows,ox,oy,data) in enumerate(pieces):
        assert not(len(bank)&1)
        struct.pack_into('>BBbbHH',bank,directory+i*8,width,rows,ox,oy,len(bank),len(data));bank+=data
    assert len(bank)<=65512,('split the shared bank instead of truncating',len(bank))
    struct.pack_into('>4sHBB6H',bank,0,b'SBN1',len(pieces),len(materials),len(shapes),shapes_off,tops_off,units_off,caps_off,directory,len(bank))
    (OUT/'sbcommon.bin').write_bytes(bank)
    for m in maps:
        data=bytearray(struct.pack('>4sBB5H',b'SCM1',m['nx'],m['nz'],m['rows'],16,0,len(m['tiles']),0))
        for t in m['tiles']:data+=bytes((*t[:4],materials.index(t[4]),int(t[4]==20),int(t[4] in (14,17)),1))
        (OUT/('sb'+m['name'][:3]+'.bin')).write_bytes(data)
        m['metadata_bytes']=len(data)
    return bytes(bank),materials,pieces,pictures,top,units,caps,source,uses


def draw_commands(m,rot,top,units,caps):
    tiles=m['tiles'];nx,nz=m['nx'],m['nz'];vw=nz if rot&1 else nx;vh=nx if rot&1 else nz
    def world(u,v):
        x,z=((u,v),(v,nz-1-u),(nx-1-u,nz-1-v),(nx-1-v,u))[rot]
        return tiles[z*nx+x] if 0<=x<nx and 0<=z<nz else None
    commands=[]
    for diag in range(vw+vh-2,-1,-1):
        for u in range(max(0,diag-vh+1),min(vw-1,diag)+1):
            v=diag-u;t=world(u,v);h=heights(t,rot);low=min(h);mat=t[4];wall=int(mat==20)
            cx=320-(u-v+vh)*12;cy=64+diag*6
            commands.append((top[mat,shape_key(h),rot],cx,cy-low*6,diag))
            for side,(a,b) in enumerate(((2,3),(1,2))):
                k=min(h[a],h[b]);neighbor=world(u+int(side==0),v+int(side==1))
                bottom=0
                if neighbor:
                    nh=heights(neighbor,rot);bottom=min(nh[0],nh[1 if side==0 else 3])
                delta=h[a]-h[b]
                if delta:commands.append((caps[wall,side,delta],cx,cy-k*6,diag))
                for hh in range(k,bottom,-1):commands.append((units[wall,side],cx,cy-(hh-1)*6,diag))
    return commands


def oracle(m,commands,pieces,pictures):
    """Independent full pixel-depth painter, then final two-layer byte packing."""
    rows=m['rows'];color=np.zeros((rows,320),np.uint8);depth=np.full((rows,320),-1,np.int16)
    for id,cx,cy,d in reversed(commands):
        width,hh,ox,oy,_=pieces[id];src=pictures[id];x=cx+ox;y=cy+oy
        x0=max(0,x);x1=min(320,x+24);y0=max(0,y);y1=min(rows,y+hh)
        if x0>=x1 or y0>=y1:continue
        crop=src[y0-y:y1-y,x0-x:x1-x];opaque=crop>=0
        color[y0:y1,x0:x1][opaque]=crop[opaque];depth[y0:y1,x0:x1][opaque]=d
    packed=[np.packbits(color&1,axis=1).tobytes(),np.packbits(color>>1,axis=1).tobytes()]
    for _ in range(2):
        threshold=depth.reshape(rows,40,8).max(2)
        front=(depth==np.repeat(threshold,8,1))&(depth>=0)
        packed.extend((np.maximum(threshold,0).astype(np.uint8).tobytes(),np.packbits(front,axis=1).tobytes()))
        depth[front]=-1
    return b''.join(packed),color


def original(m,rot):
    folder=GAME/('opening/data' if m['name']=='orbonne' else 'campaign/data')
    name=('ffov' if m['name']=='orbonne' else 'ffgv')+str(rot)
    with tempfile.TemporaryDirectory() as d:
        out=Path(d)/'raw'
        blob=(folder/(name+'.bin')).read_bytes()
        # Production v0 also contains the independently addressed contour tail.
        # The runtime decoder stops at ZX0 EOF; the host tool rejects extra input.
        if blob[-10:-6] in (b'FC01',b'FC02'):
            blob=blob[:struct.unpack_from('>H',blob,len(blob)-6)[0]]
        packed=Path(d)/'packed';packed.write_bytes(blob)
        subprocess.run([ROOT/'tools/bin/dzx0','-f',packed,out],check=True,capture_output=True)
        data=extract.unrowxor_masks(out.read_bytes())  # chapter views: row-XOR masks
    assert len(data)==6*m['rows']*40
    return data


def original_stream_size(m,rot):
    folder=GAME/('opening/data' if m['name']=='orbonne' else 'campaign/data')
    name=('ffov' if m['name']=='orbonne' else 'ffgv')+str(rot)
    blob=(folder/(name+'.bin')).read_bytes()
    return struct.unpack_from('>H',blob,len(blob)-6)[0] if blob[-10:-6] in (b'FC01',b'FC02') else len(blob)


def exact_patches(m,bases,originals):
    # Independent short streams avoid a second complete-view decompression buffer.
    bank=bytearray(12);tables=[];streams=[];dedup={}
    for base,target in zip(bases,originals):
        table=[]
        delta=bytes(a^b for a,b in zip(base,target))
        for at in range(0,len(delta),256):
            part=delta[at:at+256]
            if not any(part):continue
            if part not in dedup:dedup[part]=len(streams);streams.append(extract.zx0pack.pack(part))
            table.append((at,dedup[part]))
        tables.append(table)
    positions=[]
    for r,table in enumerate(tables):
        struct.pack_into('>H',bank,4+2*r,len(bank));positions.append(len(bank)+2)
        bank+=struct.pack('>H',len(table))+bytes(4*len(table))
    offsets=[]
    for data in streams:offsets.append(len(bank));bank+=data
    assert len(bank)<=65512,('exact residual needs bank splitting',m['name'],len(bank))
    for table,pos in zip(tables,positions):
        for k,(at,id) in enumerate(table):struct.pack_into('>HH',bank,pos+4*k,at,offsets[id])
    bank[:4]=b'SBP1';(OUT/('sp'+m['name'][:3]+'.bin')).write_bytes(bank)
    return dict(bytes=len(bank),patch_counts=[len(t) for t in tables],unique_patch_streams=len(streams))


def convert_variables():
    for path in OUT.glob('s*.bin'):
        subprocess.run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',path.name,path.stem],cwd=OUT,check=True,capture_output=True)
        variable=path.with_suffix('.89y');raw=bytearray(variable.read_bytes());raw[0x49]=3;variable.write_bytes(raw)


def main():
    parser=argparse.ArgumentParser(__doc__)
    parser.add_argument('--disc',default=str(ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'))
    parser.add_argument('--no-exact',action='store_true')
    args=parser.parse_args();OUT.mkdir(exist_ok=True)
    disc,files=extract.disc_files(args.disc);maps=maps_from_disc(disc,files)
    bank,materials,pieces,pictures,top,units,caps,sources,uses=serialize_bank(maps)
    report=dict(scope='Shared terrain metadata/material sprites; original geometry heights, simplified architectural appearance. Exact residual control restores original six-plane views.',
                common_bank_bytes=len(bank),sprite_count=len(pieces),material_sources=sources,maps=[])
    for m in maps:
        bases=[];originals=[];ratios=[]
        for rot in range(4):
            raw,color=oracle(m,draw_commands(m,rot,top,units,caps),pieces,pictures)
            target=original(m,rot);bases.append(raw);originals.append(target)
            (OUT/(m['name']+str(rot)+'.raw')).write_bytes(raw)
            (OUT/(m['name']+str(rot)+'-original.raw')).write_bytes(target)
            Image.fromarray(255-color*85).save(OUT/(m['name']+str(rot)+'.png'))
            n=m['rows']*40;old=np.unpackbits(np.frombuffer(target[:2*n],np.uint8).reshape(2,m['rows'],40),axis=2)
            orig=old[0]+2*old[1];ratios.append(float(np.mean(orig!=color)))
            Image.fromarray(255-orig*85).save(OUT/(m['name']+str(rot)+'-original.png'))
        entry=dict(name=m['name'],metadata_bytes=m['metadata_bytes'],terrain_tiles=len(m['tiles']),changed_color_fraction=ratios,
                   original_stream_bytes=sum(original_stream_size(m,r) for r in range(4)),view_bytes=len(bases[0]),color_bytes=len(bases[0])//3)
        if not args.no_exact:entry['exact_residual']=exact_patches(m,bases,originals)
        report['maps'].append(entry)
    report['pieces_shared_between_maps']=sum({'gariland','orbonne'}<=v for v in uses.values())
    report['common_wall_pieces']=sum('shared walls' in v for v in uses.values())
    (OUT/'storage.json').write_text(json.dumps(report,indent=2)+'\n')
    # Contact sheet keeps source and new bank differences visible at the same angle.
    sheet=Image.new('RGB',(640,940),'#eeeeee');draw=ImageDraw.Draw(sheet)
    for row,(name,rot) in enumerate((('gariland',0),('gariland',2),('orbonne',2),('orbonne',0))):
        for col,suffix in enumerate(('-original','')):
            im=Image.open(OUT/(name+str(rot)+suffix+'.png')).convert('RGB')
            # Same full-map image scale, source geometry not camera-framed differently.
            im.thumbnail((320,212));sheet.paste(im,(col*320,row*235+23))
            draw.text((col*320+5,row*235+5),name+' '+str(rot)+(' source' if col==0 else ' shared pieces'),fill='black')
    sheet.save(OUT/'comparison.png');convert_variables();print(json.dumps(report,indent=2),flush=True)


if __name__=='__main__':main()
