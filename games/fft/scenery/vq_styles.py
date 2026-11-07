#!/usr/bin/env python3
"""Option 1: one K-block bank per style. Gariland uses its selected contrast + height
edges views (x/isometric-style/variant-*.raw mirrored to the campaign handedness),
Orbonne its own production views. Storage per map and side-by-side images in x/vq/.
"""
import json
import numpy as np
from PIL import Image, ImageDraw

import vq
from build import OUT
from blocks import packed

K=1024


def load(name,suffix):
    out=[]
    for rot in range(4):
        raw=(OUT/(name+str(rot)+suffix+'.raw')).read_bytes();n=len(raw)//6;rows=n//40
        bits=np.unpackbits(np.frombuffer(raw[:2*n],np.uint8).reshape(2,rows,40),axis=2)
        color=(bits[0]+2*bits[1]).astype(np.int8)
        out.append((np.pad(color,((0,(-rows)%8),(0,0))),rows,raw[:2*n],raw[2*n:]))
    return out


def main():
    report={}
    for name,suffix in (('gariland','-edged'),('orbonne','-original')):
        views=load(name,suffix)
        allb=np.vstack([vq.blocks(c) for c,*_ in views])
        uniq,count=np.unique(allb,axis=0,return_counts=True)
        bank=vq.kmedoids(uniq,count.astype(np.float32),K)
        index=cover=current=0;wrong=[];images=[]
        for color,rows,colour_raw,cov in views:
            idx,_=vq.nearest(vq.blocks(color),bank)
            img=vq.unblock(bank[idx],*color.shape)[:rows];images.append((color[:rows],img))
            index+=len(packed(idx.astype('>u2').tobytes()));cover+=len(packed(cov))
            current+=len(packed(colour_raw+cov));wrong.append(float((img!=color[:rows]).mean()))
        report[name]=dict(unique_blocks=len(uniq),bank_bytes=16*len(bank),index_bytes=index,
                          coverage_bytes=cover,total_bytes=16*len(bank)+index+cover,
                          current_style_zx0_bytes=current,wrong_pixels=float(np.mean(wrong)))
        print(name,report[name],flush=True)
        for rot in (0,2):
            src,img=images[rot];w=2*320;h=2*src.shape[0]
            sheet=Image.new('L',(2*w+10,h+18),255);d=ImageDraw.Draw(sheet)
            for c,(im,label) in enumerate(((src,'production'+(' (contraste + aretes)' if suffix=='-edged' else '')),
                                           (img,'banque K=%d propre au style'%K))):
                d.text((c*(w+10)+4,2),label,fill=0)
                sheet.paste(Image.fromarray((255-im.astype(int)*85).astype(np.uint8)).resize((w,h),Image.NEAREST),(c*(w+10),18))
            sheet.save(vq.VQ/('style-%s%d.png'%(name,rot)))
    (vq.VQ/'styles.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
