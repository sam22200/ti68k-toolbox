#!/usr/bin/env python3
"""Which upcoming maps can share one 8x8 projected-block bank (survey.py renders)?

1. Cross-map affinity: map B's non-blank blocks matched against ALL distinct blocks of
   map A; mean grey error per pixel (0 = A contains every block of B).
2. Average-linkage groups on the symmetric affinity, Gariland's group first.
3. Each group gets one K=1024 bank (weighted k-medoids on a frequency-first sample of
   the group's blocks); each member's wrong pixels and colour storage are measured
   against its own K=1024 bank and its current ZX0 colour planes.
Coverage planes are not compared here (unchanged by any colour bank).
"""
import json
import numpy as np
from PIL import Image, ImageDraw

import survey
import vq
from blocks import packed

OUT=survey.OUT
K=1024


def load():
    maps=[]
    for number,name in survey.MAPS:
        z=np.load(OUT/('map%03d.npz'%number));views=[z['arr_%d'%i] for i in range(4)]
        b=np.vstack([vq.blocks(v) for v in views])
        u,c=np.unique(b,axis=0,return_counts=True)
        keep=u.any(1)                       # all-white background blocks cost nothing anywhere
        maps.append(dict(number=number,name=name,views=views,uniq=u[keep],count=c[keep]))
    return maps


def mean_error(blocks,weights,bank):
    err=np.empty(len(blocks),np.float32)
    for i in range(0,len(blocks),4096):
        _,e=vq.nearest(blocks[i:i+4096],bank);err[i:i+4096]=e
    return float((err*weights).sum()/weights.sum()/64)


def group_bank(members,k=K,sample=6000,seed=0):
    u=np.vstack([m['uniq'] for m in members]);w=np.concatenate([m['count'] for m in members]).astype(np.float32)
    u,inv=np.unique(u,axis=0,return_inverse=True);w=np.bincount(inv.ravel(),w).astype(np.float32)
    if len(u)>sample:  # frequent blocks first, then a random remainder
        order=np.argsort(-w);top=order[:sample//2]
        rest=np.random.default_rng(seed).choice(order[sample//2:],sample-len(top),replace=False)
        pick=np.concatenate([top,rest]);u,w=u[pick],w[pick]
    return vq.kmedoids(u,w,k)


def encode(m,bank):
    wrong=0;scenery=0;index=0;current=0;images=[]
    for v in m['views']:
        b=vq.blocks(v);idx,_=vq.nearest(b,bank)
        blank=~b.any(1);idx=np.where(blank,0xFFFF,idx)
        img=vq.unblock(np.where(blank[:,None],0,bank[np.minimum(idx,len(bank)-1)]),*v.shape)
        images.append(img);s=v!=0
        wrong+=int((img!=v).sum());scenery+=int(s.sum())
        index+=len(packed(idx.astype('>u2').tobytes()))
        bits=lambda a:np.packbits(a,axis=1).tobytes()
        current+=len(packed(bits(v&1)+bits(v>>1)))
    return dict(wrong_scenery_pixels=wrong/scenery,index_bytes=index,current_colour_bytes=current),images


def main():
    maps=load();n=len(maps)
    E=np.zeros((n,n))
    for a in range(n):
        for b in range(n):
            E[a,b]=0 if a==b else mean_error(maps[b]['uniq'],maps[b]['count'].astype(np.float32),maps[a]['uniq'])
    S=(E+E.T)/2
    # Average-linkage agglomeration with a cut at the median off-diagonal affinity.
    cut=float(np.median(S[~np.eye(n,dtype=bool)]))
    groups=[[i] for i in range(n)]
    while True:
        best=None
        for i in range(len(groups)):
            for j in range(i+1,len(groups)):
                d=np.mean([S[a,b] for a in groups[i] for b in groups[j]])
                if best is None or d<best[0]:best=(d,i,j)
        if best is None or best[0]>cut:break
        _,i,j=best;groups[i]+=groups.pop(j)
    groups.sort(key=lambda g:(1 not in g,-len(g)))
    report=dict(maps=[(m['number'],m['name']) for m in maps],affinity=E.round(4).tolist(),cut=cut,groups=[])
    for gi,g in enumerate(groups):
        members=[maps[i] for i in g]
        shared=group_bank(members) if len(g)>1 else None
        entry=dict(maps=[(m['number'],m['name']) for m in members],members=[])
        sheet_rows=[]
        for m in members:
            own=group_bank([m]);r_own,_=encode(m,own)
            r,imgs=encode(m,shared) if shared is not None else (r_own,None)
            entry['members'].append(dict(number=m['number'],name=m['name'],
                own_bank_wrong=r_own['wrong_scenery_pixels'],group_bank_wrong=r['wrong_scenery_pixels'],
                current_colour_bytes=r['current_colour_bytes'],own_bank_colour_bytes=16*K+r_own['index_bytes'],
                group_index_bytes=r['index_bytes']))
            if imgs is not None:sheet_rows.append((m,imgs[0]))
        if shared is not None:
            cur=sum(x['current_colour_bytes'] for x in entry['members'])
            new=16*len(shared)+sum(x['group_index_bytes'] for x in entry['members'])
            entry.update(current_colour_bytes=cur,group_colour_bytes=new)
            cols=2;w=330
            sheet=Image.new('L',(cols*2*w,len(sheet_rows)*240),255);d=ImageDraw.Draw(sheet)
            for r,(m,img) in enumerate(sheet_rows):
                for c,(im,label) in enumerate(((m['views'][0],'%03d %s (style uniforme)'%(m['number'],m['name'])),(img,'banque commune du groupe'))):
                    pic=Image.fromarray((255-im.astype(int)*85).astype(np.uint8));pic.thumbnail((2*w-10,220))
                    sheet.paste(pic,(c*2*w,r*240+16));d.text((c*2*w+4,r*240+2),label,fill=0)
            sheet.save(OUT/('group%d.png'%gi))
        report['groups'].append(entry)
        print('group',gi,[m['name'] for m in members],flush=True)
        for x in entry['members']:
            print('   %03d %-26s own %.1f%%  group %.1f%%'%(x['number'],x['name'],100*x['own_bank_wrong'],100*x['group_bank_wrong']))
        if shared is not None:print('   colour: current %d B -> group bank %d B'%(entry['current_colour_bytes'],entry['group_colour_bytes']))
    (OUT/'groups.json').write_text(json.dumps(report,indent=2)+'\n')
    # Affinity heat map with names.
    cell=18;pad=190;img=Image.new('L',(pad+n*cell,pad+n*cell),255);d=ImageDraw.Draw(img)
    for i,m in enumerate(maps):
        d.text((2,pad+i*cell+3),'%03d %s'%(m['number'],m['name'][:24]),fill=0)
        d.text((pad+i*cell+3,pad-14),'%d'%i,fill=0)
        for j in range(n):
            v=int(255*min(1,S[i,j]/(2*cut)))
            d.rectangle([pad+j*cell,pad+i*cell,pad+j*cell+cell-2,pad+i*cell+cell-2],fill=v)
    img.save(OUT/'affinity.png')

if __name__=='__main__':main()
