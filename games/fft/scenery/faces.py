#!/usr/bin/env python3
"""Per-face pattern prototype: every top and wall face gets its own pattern index.

Patterns are world-space 8x8 four-grey textures sampled back from the reviewed
production views (all four orientations vote), then reduced to a shared set whose
members come from Gariland only; Orbonne reuses them (optionally plus a few extras).
Tops and walls only: the off-grid decorative geometry is still absent.
Outputs stay under ignored x/faces/.
"""
from pathlib import Path
import argparse
import collections
import json
import numpy as np
from PIL import Image, ImageDraw

import build

OUT=build.OUT/'faces'
T=8          # texels per face side
CORNER_UV=[(0,0),(8,0),(8,8),(0,8)]
HIDDEN=np.zeros((T,T),np.int8)  # faces covered in every orientation


def raster_geo(vertices,uv):
    """build.raster's geometry, returning texel coordinates instead of colors."""
    vertices=np.asarray(vertices,float)
    ox=int(vertices[:,0].min());oy=int(vertices[:,1].min())
    rows=int(vertices[:,1].max())-oy
    mask=np.zeros((rows,24),bool);tu=np.zeros((rows,24),np.int8);tw=np.zeros((rows,24),np.int8)
    Y,X=np.mgrid[oy:oy+rows,ox:ox+24];X=X+.5;Y=Y+.5
    for tri in ((0,1,2),(0,2,3)):
        v=vertices[list(tri)];ax,bx,cx=v[:,0];ay,by,cy=v[:,1]
        den=(by-cy)*(ax-cx)+(cx-bx)*(ay-cy)
        if abs(den)<1e-8:continue
        a=((by-cy)*(X-cx)+(cx-bx)*(Y-cy))/den
        b=((cy-ay)*(X-cx)+(ax-cx)*(Y-cy))/den;c=1-a-b
        inside=(a>=0)&(b>=0)&(c>=0)
        coords=np.asarray(uv)[list(tri)]
        u=np.floor(a*coords[0,0]+b*coords[1,0]+c*coords[2,0]).astype(int)&7
        w=np.floor(a*coords[0,1]+b*coords[1,1]+c*coords[2,1]).astype(int)&7
        mask[inside]=True;tu[inside]=u[inside];tw[inside]=w[inside]
    return ox,oy,mask,tu,tw


def geometry(m,rot):
    """Front-to-back pieces: (face key, piece key, cx, cy, diag).

    face key: ('top',tile) or ('wall',tile,w0,level) with w0 the world corner
    where texture u=0 (identical for both orientations showing that face) and
    level 0 = highest unit + slope cap, 1 = lower units.
    piece key: ('top',shape,rot) / ('unit',side) / ('cap',side,delta).
    """
    tiles=m['tiles'];nx,nz=m['nx'],m['nz'];vw=nz if rot&1 else nx;vh=nx if rot&1 else nz
    def index(u,v):
        x,z=((u,v),(v,nz-1-u),(nx-1-u,nz-1-v),(nx-1-v,u))[rot]
        return z*nx+x if 0<=x<nx and 0<=z<nz else None
    out=[]
    for diag in range(vw+vh-2,-1,-1):
        for u in range(max(0,diag-vh+1),min(vw-1,diag)+1):
            v=diag-u;i=index(u,v);h=build.heights(tiles[i],rot);low=min(h)
            cx=320-(u-v+vh)*12;cy=64+diag*6
            out.append((('top',i),('top',build.shape_key(h),rot),cx,cy-low*6,diag))
            for side,(a,b) in enumerate(((2,3),(1,2))):
                k=min(h[a],h[b]);n=index(u+int(side==0),v+int(side==1));bottom=0
                if n is not None:
                    nh=build.heights(tiles[n],rot);bottom=min(nh[0],nh[1 if side==0 else 3])
                w0=(-2-rot)&3 if side==0 else (-1-rot)&3
                delta=h[a]-h[b]
                if delta:out.append((('wall',i,w0,0),('cap',side,delta),cx,cy-k*6,diag))
                for hh in range(k,bottom,-1):
                    out.append((('wall',i,w0,int(hh!=k)),('unit',side),cx,cy-(hh-1)*6,diag))
    return out


class Shapes:
    def __init__(self):self.cache={}
    def __call__(self,piece):
        if piece not in self.cache:
            if piece[0]=='top':
                key,rot=piece[1],piece[2];h=[(key>>(2*c))&3 for c in range(4)]
                uv=[CORNER_UV[(8-c-rot)&3] for c in range(4)]
                vert=[(0,-6*h[0]),(12,6-6*h[1]),(0,12-6*h[2]),(-12,6-6*h[3])]
            elif piece[0]=='unit':
                side=piece[1];uv=CORNER_UV
                vert=[(0,6),(-12,0),(-12,6),(0,12)] if side==0 else [(12,0),(0,6),(0,12),(12,6)]
            else:
                side,delta=piece[1],piece[2];a=max(delta,0);b=max(-delta,0);uv=CORNER_UV
                vert=([(0,12-6*a),(-12,6-6*b),(-12,6),(0,12)] if side==0 else
                      [(12,6-6*a),(0,12-6*b),(0,12),(12,6)])
            self.cache[piece]=raster_geo(vert,uv)
        return self.cache[piece]


def paint(m,pieces,shapes,colour_of):
    """Back-to-front painter; returns color image and per-pixel (face, u, w)."""
    rows=m['rows'];color=np.zeros((rows,320),np.int8);owner=np.full((rows,320),-1,np.int32)
    tu=np.zeros((rows,320),np.int8);tw=np.zeros((rows,320),np.int8)
    for n,(face,piece,cx,cy,d) in reversed(list(enumerate(pieces))):
        ox,oy,mask,pu,pw=shapes(piece);x=cx+ox;y=cy+oy;hh=mask.shape[0]
        x0=max(0,x);x1=min(320,x+24);y0=max(0,y);y1=min(rows,y+hh)
        if x0>=x1 or y0>=y1:continue
        sl=(slice(y0-y,y1-y),slice(x0-x,x1-x));dst=(slice(y0,y1),slice(x0,x1))
        mk=mask[sl]
        owner[dst][mk]=n;tu[dst][mk]=pu[sl][mk];tw[dst][mk]=pw[sl][mk]
        if colour_of is not None:
            pat=colour_of(face);color[dst][mk]=pat[pw[sl][mk],pu[sl][mk]]
    return color,owner,tu,tw


def original_planes(m,rot):
    raw=build.original(m,rot);n=m['rows']*40
    bits=np.unpackbits(np.frombuffer(raw[:6*n],np.uint8).reshape(6,m['rows'],40),axis=2)
    return (bits[0]+2*bits[1]).astype(np.int8),(bits[3]|bits[5]).astype(bool)


def sample_patterns(m,shapes):
    votes=collections.defaultdict(lambda:np.zeros((T,T,4),np.int32))
    views=[]
    for rot in range(4):
        pieces=geometry(m,rot);_,owner,tu,tw=paint(m,pieces,shapes,None)
        orig,scenery=original_planes(m,rot);views.append((pieces,orig,scenery))
        ys,xs=np.nonzero(owner>=0)
        for y,x in zip(ys,xs):
            votes[pieces[owner[y,x]][0]][tw[y,x],tu[y,x],orig[y,x]]+=1
    patterns={}
    for face,v in votes.items():
        seen=v.sum(2)>0;mode=int(np.argmax(v.sum((0,1))))
        p=np.argmax(v,2).astype(np.int8);p[~seen]=mode;patterns[face]=p
    # Faces never visible in any orientation (fully hidden) need no pattern.
    return patterns,views


def distance(a,b):
    return np.abs(a[:,None,:].astype(np.int16)-b[None,:,:]).sum(2)


def kmedoids(vectors,weights,k,seed=0,iters=12):
    """Weighted k-medoids (L1 over grey levels), k-means++ seeded."""
    n=len(vectors)
    if k>=n:return np.arange(n)
    rng=np.random.default_rng(seed);d=distance(vectors,vectors)
    med=[int(np.argmax(weights))]
    for _ in range(k-1):
        near=d[:,med].min(1).astype(float)*weights
        med.append(int(rng.choice(n,p=near/near.sum())) if near.sum() else int(np.argmax(weights)))
    med=np.array(med)
    for _ in range(iters):
        assign=np.argmin(d[:,med],1);new=med.copy()
        for c in range(k):
            idx=np.flatnonzero(assign==c)
            if len(idx):new[c]=idx[np.argmin((d[np.ix_(idx,idx)]*weights[idx][:,None]).sum(0))]
        if (new==med).all():break
        med=new
    return med


def choose_bank(pats,k_top,k_wall,extras):
    """Return {face key: shared pattern} per map. Gariland supplies the medoids."""
    chosen={}
    for kind,k in (('top',k_top),('wall',k_wall)):
        garf=[f for f in pats['gariland'] if f[0]==kind]
        uniq={};w=collections.Counter()
        for f in garf:
            key=pats['gariland'][f].tobytes();uniq.setdefault(key,pats['gariland'][f]);w[key]+=1
        keys=list(uniq);vec=np.array([uniq[q].ravel() for q in keys],np.int8)
        weights=np.array([w[q] for q in keys],float)
        bank=vec[kmedoids(vec,weights,k)] if k else vec
        orbf=[f for f in pats['orbonne'] if f[0]==kind]
        ovec=np.array([pats['orbonne'][f].ravel() for f in orbf],np.int8)
        for _ in range(extras):  # greedy: the Orbonne pattern that most reduces error
            err=distance(ovec,bank).min(1)
            gain=np.maximum(err[:,None]-distance(ovec,ovec),0).sum(0)
            bank=np.vstack([bank,ovec[int(np.argmax(gain))]])
        chosen[kind]=bank
    out={}
    for name in ('gariland','orbonne'):
        out[name]={}
        for kind in ('top','wall'):
            faces=[f for f in pats[name] if f[0]==kind]
            if not faces:continue
            vec=np.array([pats[name][f].ravel() for f in faces],np.int8)
            pick=np.argmin(distance(vec,chosen[kind]),1)
            for f,p in zip(faces,pick):out[name][f]=(int(p),chosen[kind][p].reshape(T,T))
    return chosen,out


def cost(maps,assigned,views):
    """Pre-projected bank pieces actually referenced, plus per-tile metadata."""
    used=set()
    for m in maps:
        for pieces,_,_ in views[m['name']]:
            for face,piece,*_ in pieces:
                if face in assigned[m['name']]:used.add((face[0],assigned[m['name']][face][0],piece))
    shapes=Shapes();sprite=sum(8+12*shapes(p).__getitem__(2).shape[0] for _,_,p in used)
    shared=0
    for kind in ('top','wall'):
        g={v[0] for f,v in assigned['gariland'].items() if f[0]==kind}
        o={v[0] for f,v in assigned['orbonne'].items() if f[0]==kind}
        shared+=len(g&o)
    # tile: 4 heights + top pattern + 4 world edges x (upper, lower) wall patterns
    meta={m['name']:16+13*len(m['tiles']) for m in maps}
    return dict(bank_pieces=len(used),bank_bytes=sprite,metadata_bytes=meta,
                patterns_used_by_both_maps=shared)


def fidelity(m,views,colour_of,shapes):
    scores=[];images=[]
    for rot,(pieces,orig,scenery) in enumerate(views):
        color,owner,_,_=paint(m,pieces,shapes,colour_of)
        diff=orig!=color
        scores.append(dict(all=float(diff.mean()),scenery=float(diff[scenery].mean())))
        images.append(color)
    return scores,images


def main():
    parser=argparse.ArgumentParser(__doc__)
    parser.add_argument('--disc',default=str(build.ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'))
    parser.add_argument('--sweep',default='0:0:0,128:256:0,64:128:0,32:64:0,16:32:0,32:64:16')
    args=parser.parse_args();OUT.mkdir(parents=True,exist_ok=True)
    disc,files=build.extract.disc_files(args.disc);maps=build.maps_from_disc(disc,files)
    shapes=Shapes();pats={};views={}
    for m in maps:pats[m['name']],views[m['name']]=sample_patterns(m,shapes)
    report=dict(texels=T,maps={m['name']:dict(tiles=len(m['tiles']),
        top_faces=sum(f[0]=='top' for f in pats[m['name']]),
        wall_faces=sum(f[0]=='wall' for f in pats[m['name']])) for m in maps},runs=[])
    # Reference: Codex's per-material bank, same metric.
    base={}
    for m in maps:
        base[m['name']]=[]
        for rot in range(4):
            col=np.asarray(Image.open(build.OUT/(m['name']+str(rot)+'.png')).convert('L'))
            col=((255-col.astype(int))//85).astype(np.int8)
            _,orig,scenery=views[m['name']][rot];diff=orig!=col
            base[m['name']].append(dict(all=float(diff.mean()),scenery=float(diff[scenery].mean())))
    report['material_bank_baseline']=base
    for spec in args.sweep.split(','):
        kt,kw,ex=map(int,spec.split(':'))
        if kt==0:  # unshared: every face keeps its own sampled pattern
            assigned={n:{f:(hash(p.tobytes()),p) for f,p in pats[n].items()} for n in pats}
            label='own pattern per face'
        else:
            _,assigned=choose_bank(pats,kt,kw,ex);label='%d top + %d wall from Gariland, %d Orbonne extras'%(kt,kw,ex)
        run=dict(label=label,k_top=kt,k_wall=kw,orbonne_extras=ex,maps={})
        if kt:run.update(cost(maps,assigned,views))
        for m in maps:
            scores,images=fidelity(m,views[m['name']],lambda f,a=assigned[m['name']]:a[f][1] if f in a else HIDDEN,shapes)
            run['maps'][m['name']]=scores
            for rot,img in enumerate(images):
                Image.fromarray((255-img.astype(int)*85).astype(np.uint8)).save(OUT/('%s%d-%s.png'%(m['name'],rot,spec.replace(':','_'))))
        report['runs'].append(run)
        print(label,{n:'%.1f%%'%(100*np.mean([s['scenery'] for s in v])) for n,v in run['maps'].items()},
              run.get('bank_bytes'),run.get('bank_pieces'),flush=True)
    (OUT/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    specs=[s.replace(':','_') for s in args.sweep.split(',')]
    cols=['-original','']+['-'+s for s in specs[:3]]
    for name,rot in (('gariland',0),('orbonne',0)):
        sheet=Image.new('RGB',(320*len(cols),240),'#eeeeee');draw=ImageDraw.Draw(sheet)
        for c,suffix in enumerate(cols):
            src=build.OUT/(name+str(rot)+suffix+'.png') if c<2 else OUT/(name+str(rot)+suffix+'.png')
            im=Image.open(src).convert('RGB');im.thumbnail((320,218));sheet.paste(im,(c*320,20))
            draw.text((c*320+5,4),['source','material bank (Codex)']+['faces '+s for s in specs[:3]] and
                      (['source','material bank (Codex)']+['faces '+s for s in specs[:3]])[c],fill='black')
        sheet.save(OUT/('compare-%s.png'%name))


if __name__=='__main__':main()
