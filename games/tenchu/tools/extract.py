#!/usr/bin/env python3
"""Local Tenchu USA training geometry, from the user's disc, never distributed.
AFS metadata is big endian; MAD and embedded unlit TMDs are little endian.
MAD centres are in ten world units; the native world scales PS1 units by 1/64.
Exact coplanar rectangle merges precede coordinate quantisation. Textures omitted.
"""
import argparse,collections,hashlib,json,math,struct,subprocess,sys,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.claude/skills/ti-port-ps1/scripts'))
from psxiso import Disc,walk

def archive(disc):
    d=Disc(str(disc));root=d.sector(16)[156:190];files=[]
    walk(d,struct.unpack_from('<I',root,2)[0],struct.unpack_from('<I',root,10)[0],'',files)
    entries={p:(l,s) for p,l,s in files}
    b=d.read(*entries['TENCHU/DATA.VOL'])
    assert b[:12]==b'AFS_VOL_200\0'
    count,table=struct.unpack_from('>II',b,12)
    assert count==545 and table+count*36==len(b)
    assert zlib.crc32(b)==0x1f83a85a,'Unsupported disc revision'
    paths={0:Path('K:')};out={};manifest=[]
    for i in range(count):
        m,kind,pos,size,dup=struct.unpack_from('>HHIII',b,table+i*36)
        assert m==0x4958 and kind in (1,2)
        name=b[table+i*36+16:table+i*36+36].split(b'\0')[0].decode('ascii')
        if i==0:continue
        parent,stem=name[1:].split('_',1);parent=int(parent)
        assert parent<i and parent in paths
        path=paths[parent]/stem
        if kind==2:paths[i]=path
        else:
            assert size==dup and 40<=pos and pos+size<=table
            payload=b[pos:pos+size]
            manifest.append(dict(path=str(path),offset=pos,size=size,sha256=hashlib.sha256(payload).hexdigest()))
            if path.parent.name=='TANREN':out[stem]=payload
    return out,manifest,hashlib.sha256(b).hexdigest()

def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def normal(poly):return cross(tuple(b-a for a,b in zip(poly[0],poly[1])),tuple(b-a for a,b in zip(poly[0],poly[2])))
def mad(b,ram=None):
    magic,count=struct.unpack_from('<II',b)
    assert magic==0x96102900 and count==72
    polys=[];modes=collections.Counter();verified=0
    base_ram=ram.find(b[:128]) if ram else -1
    if ram is not None:assert base_ram>=0,'MAP.MAD header absent from training RAM'
    for i in range(count):
        rec=struct.unpack_from('<6hI',b,8+i*16);t=8+rec[-1]
        assert struct.unpack_from('<III',b,t)==(65,0,1)
        base=t+12;vo,nv,no,nn,po,np,scale=struct.unpack_from('<7I',b,base)
        assert scale==0 and nn==0
        raw=b[base+vo:base+vo+nv*8]
        if ram is not None:
            assert ram[base_ram+base+vo:base_ram+base+vo+nv*8]==raw
            verified+=nv
        vertices=[tuple(v+10*rec[k+2] for k,v in enumerate(struct.unpack_from('<4h',raw,j*8)[:3])) for j in range(nv)]
        p=base+po
        for j in range(np):
            olen,ilen,flag,mode=struct.unpack_from('<4B',b,p)
            assert flag&1 and mode in (0x3d,0x35,0x39,0x2d)
            n=4 if mode&8 else 3
            ids=struct.unpack_from('<4H',b,p+4+ilen*4-8)[:n]
            assert all(k<nv for k in ids)
            # TMD quads are triangle strips, our convex faces are boundary ordered.
            ids=(ids[2],ids[3],ids[1],ids[0]) if n==4 else tuple(reversed(ids))
            # X right, Y forward, Z up. Normal direction now matches native winding.
            poly=[(vertices[k][0],vertices[k][2],-vertices[k][1]) for k in ids]
            assert any(normal(poly))
            polys.append(poly);modes[mode]+=1;p+=4+ilen*4
    return polys,dict(cells=count,vertices_verified_in_ram=verified,ram_base=hex(base_ram) if base_ram>=0 else None,primitive_modes=dict(modes))

def merge_rectangles(polys):
    groups=collections.defaultdict(list);keep=[]
    original_area=merged_area=0
    for p in polys:
        n=normal(p);nz=[i for i,x in enumerate(n) if x]
        if len(p)!=4 or len(nz)!=1:keep.append(p);continue
        axis=nz[0];a,b=[i for i in range(3) if i!=axis]
        aa=sorted(set(v[a] for v in p));bb=sorted(set(v[b] for v in p))
        if len(aa)!=2 or len(bb)!=2 or len(set((v[a],v[b]) for v in p))!=4:keep.append(p);continue
        original_area+=(aa[1]-aa[0])*(bb[1]-bb[0])
        groups[(axis,p[0][axis],1 if n[axis]>0 else -1)].append((aa[0],aa[1],bb[0],bb[1]))
    for (axis,plane,sign),rects in groups.items():
        changed=True
        while changed:
            changed=False
            for i,r in enumerate(rects):
                for j in range(i+1,len(rects)):
                    s=rects[j];merged=None
                    if r[:2]==s[:2] and (r[3]==s[2] or s[3]==r[2]):merged=(r[0],r[1],min(r[2],s[2]),max(r[3],s[3]))
                    elif r[2:]==s[2:] and (r[1]==s[0] or s[1]==r[0]):merged=(min(r[0],s[0]),max(r[1],s[1]),r[2],r[3])
                    if merged:
                        rects[i]=merged;rects.pop(j);changed=True;break
                if changed:break
        a,b=[i for i in range(3) if i!=axis]
        for lo,hi,bot,top in rects:
            p=[]
            for x,y in ((lo,bot),(hi,bot),(hi,top),(lo,top)):
                v=[0,0,0];v[axis]=plane;v[a]=x;v[b]=y;p.append(tuple(v))
            if (normal(p)[axis]>0)!=(sign>0):p.reverse()
            keep.append(p);merged_area+=(hi-lo)*(top-bot)
    assert original_area==merged_area
    return keep,original_area

def native(p):return ((p[0]+8192)/64,(p[1]+8192)/64,p[2]/64)
def height_at(poly,x,y):
    n=normal(poly)
    if n[2]<=0:return None
    for i,a in enumerate(poly):
        b=poly[(i+1)%len(poly)]
        if (b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]) < -0.01:return None
    return poly[0][2]-(n[0]*(x-poly[0][0])+n[1]*(y-poly[0][1]))/n[2]

def bank(polys):
    pp=[[native(v) for v in p] for p in polys]
    floors=[set() for _ in range(4096)]
    walls=[[[255,0] for _ in range(4096)] for _ in range(2)]
    anchors=set()
    for p in pp:
        n=normal(p)
        if n[2]>0:
            xmin=max(0,int(min(v[0] for v in p))//16);xmax=min(63,int(max(v[0] for v in p))//16)
            ymin=max(0,int(min(v[1] for v in p))//16);ymax=min(63,int(max(v[1] for v in p))//16)
            for gy in range(ymin,ymax+1):
                for gx in range(xmin,xmax+1):
                    h=height_at(p,8+gx*16,8+gy*16)
                    if h is not None:floors[gy*64+gx].add(max(0,round(h/2)))
            if min(v[2] for v in p)>=12:
                cx=sum(v[0] for v in p)/len(p);cy=sum(v[1] for v in p)/len(p)
                for v in p:
                    x=v[0]+(4 if v[0]<cx else -4);y=v[1]+(4 if v[1]<cy else -4)
                    h=height_at(p,x,y)
                    if h is not None:anchors.add((round(x),round(y),round(h/2)*2))
        if n[2]==0 and (n[0]==0 or n[1]==0):
            axis=0 if n[0] else 1;other=1-axis
            q=round(p[0][axis]/16)
            if not 0<=q<64:continue
            lo=max(0,math.floor(min(v[other] for v in p)/16));hi=min(64,math.ceil(max(v[other] for v in p)/16))
            low=max(0,math.floor(min(v[2] for v in p)/2));high=math.ceil(max(v[2] for v in p)/2)
            for k in range(lo,hi):
                index=k*64+q if axis==0 else q*64+k
                w=walls[axis][index];w[0]=min(w[0],low);w[1]=max(w[1],high)
    assert max(map(len,floors))<=2,'More than two floor layers: expand format explicitly'
    ff=bytes(v for cell in floors for v in (sorted(cell)+[255,255])[:2])
    wallbytes=bytes(v for axis in walls for w in axis for v in w)
    vertices=[];lookup={};faces=[]
    for p in pp:
        poly=[tuple(round(x) for x in v) for v in p]
        n=normal(poly);indices=[]
        if not any(n):continue
        nonzero=[i for i,v in enumerate(n) if v]
        axis=1+nonzero[0]*2+(n[nonzero[0]]<0) if len(nonzero)==1 else 0
        for v in poly:
            if v not in lookup:lookup[v]=len(vertices);vertices.append(v)
            indices.append(lookup[v])
        if len(indices)==3:indices.append(indices[-1])
        faces.append((indices,len(poly),axis,2 if n[2]>0 else 1 if n[0] else 3))
    anchors=sorted(anchors,key=lambda a:(a[1],a[0],a[2]))
    assert len(anchors)<256
    # Header + 2 floor layers + X/Y wall intervals + anchors + vertices + faces.
    result=b'TE02'+struct.pack('>6H',len(vertices),len(faces),144,120,0,len(anchors))+ff+wallbytes
    result+=b''.join(struct.pack('>3h',*v) for v in anchors+vertices)
    result+=b''.join(struct.pack('>4H4B4h',*ids,n,axis,shade,0, min(vertices[i][0] for i in ids), min(vertices[i][1] for i in ids), max(vertices[i][0] for i in ids), max(vertices[i][1] for i in ids)) for ids,n,axis,shade in faces)
    assert len(result)<65000
    return result,dict(native_vertices=len(vertices),native_faces=len(faces),anchors=len(anchors),max_floor_layers=max(map(len,floors)),bytes=len(result))

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--disc',type=Path,default=ROOT/'roms/ps1/Tenchu - Stealth Assassins (USA)/Tenchu - Stealth Assassins (USA).cue')
    ap.add_argument('--ram',type=Path);a=ap.parse_args()
    data,manifest,sha=archive(a.disc)
    out=HERE/'local';out.mkdir(exist_ok=True)
    for name,b in data.items():(out/name).write_bytes(b)
    polys,stats=mad(data['MAP.MAD'],a.ram.read_bytes() if a.ram else None)
    merged,area=merge_rectangles(polys);blob,bs=bank(merged)
    (HERE/'tnmap.bin').write_bytes(blob)
    subprocess.run([str(ROOT/'tools/gcc4ti-bin/bin/ttbin2oth'),'-quiet','-89','dat','tnmap.bin','tnmap'],cwd=HERE,check=True)
    stats.update(bs,source_polygons=len(polys),merged_polygons=len(merged),rectangular_area_preserved=area,archive_sha256=sha,bank_sha256=hashlib.sha256(blob).hexdigest())
    (out/'manifest.json').write_text(json.dumps(dict(stats=stats,archive=manifest),indent=2)+'\n')
    print(json.dumps(stats,indent=2))
if __name__=='__main__':main()
