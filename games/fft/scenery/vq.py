#!/usr/bin/env python3
"""Lossy projected-block bank: 8x8 colour blocks of the production views replaced by
the nearest of K shared blocks (k-medoids over Gariland's blocks; Orbonne reuses
them, optionally plus extras of its own). Coverage planes stay exact (ZX0, charged
in full). Storage and side-by-side images under x/vq/ for visual validation.
"""
import json
import numpy as np
from PIL import Image, ImageDraw

from build import OUT
from blocks import packed

VQ=OUT/'vq'
MAPS=('gariland','orbonne')


def load():
    views={}
    for name in MAPS:
        for rot in range(4):
            raw=(OUT/(name+str(rot)+'-original.raw')).read_bytes();n=len(raw)//6;rows=n//40
            bits=np.unpackbits(np.frombuffer(raw[:2*n],np.uint8).reshape(2,rows,40),axis=2)
            color=(bits[0]+2*bits[1]).astype(np.int8)
            pad=(-rows)%8
            views[name,rot]=(np.pad(color,((0,pad),(0,0))),rows,raw[2*n:])
    return views


def blocks(color):
    h,w=color.shape
    return color.reshape(h//8,8,w//8,8).transpose(0,2,1,3).reshape(-1,64)


def unblock(b,h,w):
    return b.reshape(h//8,w//8,8,8).transpose(0,2,1,3).reshape(h,w)


def thermo(v):
    # L1 between grey levels = Hamming between thermometer codes: matrix products.
    return np.concatenate([(v>=k) for k in (1,2,3)],1).astype(np.float32)


def nearest(x,bank):
    tx,tb=thermo(x),thermo(bank)
    d=tx.sum(1)[:,None]+tb.sum(1)[None,:]-2*tx@tb.T
    return np.argmin(d,1),d.min(1)


def kmedoids(vec,weight,k,iters=10,seed=0):
    if k>=len(vec):return vec
    rng=np.random.default_rng(seed);t=thermo(vec)
    d=t.sum(1)[:,None]+t.sum(1)[None,:]-2*t@t.T
    med=[int(np.argmax(weight))]
    for _ in range(k-1):
        near=d[:,med].min(1)*weight
        med.append(int(rng.choice(len(vec),p=near/near.sum())) if near.sum() else int(np.argmax(weight)))
    med=np.array(med)
    for _ in range(iters):
        assign=np.argmin(d[:,med],1);new=med.copy()
        for c in range(k):
            idx=np.flatnonzero(assign==c)
            if len(idx):new[c]=idx[np.argmin((d[np.ix_(idx,idx)]*weight[idx][:,None]).sum(0))]
        if (new==med).all():break
        med=new
    return vec[med]


def run(views,k,extras):
    gar=np.vstack([blocks(views['gariland',r][0]) for r in range(4)])
    uniq,count=np.unique(gar,axis=0,return_counts=True)
    bank=kmedoids(uniq,count.astype(np.float32),k)
    if extras:
        orb=np.vstack([blocks(views['orbonne',r][0]) for r in range(4)])
        ou,oc=np.unique(orb,axis=0,return_counts=True)
        for _ in range(extras):  # greedy: the Orbonne block removing the most error
            _,err=nearest(ou,bank)
            pick=int(np.argmax(err*oc));bank=np.vstack([bank,ou[pick]])
    out={};index_bytes=0;wrong=[]
    for (name,rot),(color,rows,cover) in views.items():
        b=blocks(color);idx,_=nearest(b,bank)
        img=unblock(bank[idx],*color.shape)[:rows]
        out[name,rot]=img
        width=1 if len(bank)<=256 else 2
        index_bytes+=len(packed(idx.astype('>u2' if width==2 else np.uint8).tobytes()))
        wrong.append((name,float((img!=color[:rows]).mean())))
    shared=None
    return out,dict(k=k,orbonne_extras=extras,bank_blocks=len(bank),bank_bytes=16*len(bank),
                    compressed_index_bytes=index_bytes,
                    wrong_pixels={n:float(np.mean([w for m,w in wrong if m==n])) for n in MAPS})


def main():
    VQ.mkdir(parents=True,exist_ok=True);views=load()
    cover=sum(len(packed(c)) for _,_,c in views.values())
    report=dict(compressed_coverage_bytes=cover,current_views_bytes=77529,runs=[])
    images={}
    for k,ex in ((1024,0),(512,0),(256,0),(512,128)):
        out,r=run(views,k,ex);r['total_bytes']=r['bank_bytes']+r['compressed_index_bytes']+cover
        report['runs'].append(r);images[k,ex]=out
        print(r,flush=True)
    (VQ/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    labels=[('production',None),('K=1024',(1024,0)),('K=512',(512,0)),('K=256',(256,0)),('K=512 + 128 Orbonne',(512,128))]
    for name in MAPS:
        for rot in (0,2):
            color,rows,_=views[name,rot];ims=[color[:rows]]+[images[key][name,rot] for _,key in labels[1:]]
            cols=3;w=640;h=2*rows+18
            sheet=Image.new('L',(cols*(w+10),2*h),255);d=ImageDraw.Draw(sheet)
            for i,(im,(label,key)) in enumerate(zip(ims,labels)):
                x=(i%cols)*(w+10);y=(i//cols)*h
                if key:
                    r=next(r for r in report['runs'] if (r['k'],r['orbonne_extras'])==key)
                    label+='  total %.1f Ko'%(r['total_bytes']/1000)
                d.text((x+4,y+2),label,fill=0)
                pic=Image.fromarray((255-im.astype(int)*85).astype(np.uint8)).resize((w,2*rows),Image.NEAREST)
                sheet.paste(pic,(x,y+16))
            sheet.save(VQ/('visuel-%s%d.png'%(name,rot)))


if __name__=='__main__':main()
