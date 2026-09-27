import sys,os
sys.path.insert(0,os.path.dirname(__file__))
from murparse import M, room
from PIL import Image, ImageDraw, ImageFont
rooms=[int(x) for x in sys.argv[2:]]
Z=4
tiles=[]
for u in rooms:
    p='pictures/dec%d.png'%u
    im=Image.open(p).convert('RGB') if os.path.exists(p) else Image.new('RGB',(153,71),(90,0,0))
    im=im.resize((im.width*Z,im.height*Z),Image.NEAREST)
    c=Image.new('RGB',(160*Z,90*Z),(60,60,90)); c.paste(im,(0,0))
    d=ImageDraw.Draw(c)
    m=room(u)
    if m:
        dr=int(m[0][1])
        for ry in range(1,dr-1):
            for cx in range(1,len(m[ry])):
                v=m[ry][cx]; x=(cx-1)*9*Z; y=(ry-1)*9*Z
                if y>=80*Z: continue
                if v==1: continue
                s='%g'%float(v) if v!=int(v) else str(int(v))
                col=(255,0,0) if v==0 else (0,160,0) if 2<v<500 else (0,0,255) if v>=500 else (200,0,200)
                if v==0: d.rectangle([x+14,y+14,x+20,y+20],outline=col)
                else: d.text((x+2,y+10),s,fill=col)
    d.text((4,80*Z+8),'room %d'%u,fill=(255,255,0))
    tiles.append(c)
n=len(tiles); cols=3
sh=Image.new('RGB',(cols*160*Z,((n+cols-1)//cols)*90*Z))
for k,t in enumerate(tiles): sh.paste(t,((k%cols)*160*Z,(k//cols)*90*Z))
sh.save(sys.argv[1])
