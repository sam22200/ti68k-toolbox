#!/usr/bin/env python3
"""Original radial footprints, layer choice and source-slot target lists; no battle presentation."""
import copy,json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask

def base(kind=0):
    w=h=5;terrain=[]
    for layer in range(2):
        for i in range(25):
            height=2+4*layer;dh=0;flags=1 if layer and kind==0 else 0
            if kind==2:height+=i%3;dh=(i%2)|((i%3)<<5)
            if kind==3:
                if not layer and i in (8,13,18):flags=1
                if layer and i in (11,17):flags=2
            terrain.append([height,dh,flags])
    actors=[[0,0,0,0,255,0],[2,2,0,1,255,0],[3,2,0,0,255,0],
        [2,3,0,2,2,0],[1,2,0,1,8,0],[2,1,0,0,31,0],
        [3,3,0,1,1,0],[1,1,0,0,5,0],[4,4,0,2,15,0],
        [0,2,0,1,255,0],[4,2,0,0,255,0],[2,4,0,1,255,1],[2,2,1,0,255,0]]
    return dict(w=w,h=h,actor=0,cx=2,cz=2,cl=0,follow=0,ref=12,aoe=1,vert=3,f1=0,f2=0,f4=0,terrain=terrain,actors=actors)

def main():
    out=s.HERE/'campaign/x/core-targets/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(green='battle_target_set_green_panels_for_action',vertical='battle_target_apply_vertical_tolerance',
        listing='battle_target_list_units_on_panels')
    for k,n in names.items():bind_function(k,n,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=2000000
    profiles=[]
    for kind in (0,1):
        for aoe in (0,1,2,255):
            for tol in (0,1,3):
                p=base(kind);p.update(aoe=aoe,vert=tol);profiles.append(p)
    for flags in (1,32,64,128,192,129):
        for radius in (1,255):p=base(1);p.update(f2=flags,aoe=radius);profiles.append(p)
    for sid in (255,1,5,2):
        p=base(1);p.update(follow=1);p['actors'][12][4]=sid;p['actors'][2][2]=1;profiles.append(p)
    for radius in (1,255):
        for flags in (1,64,128):
            for order in (0,1,2):
                p=base(1);p.update(aoe=radius,f2=flags);p['actors'][0][:3]=[2,2,0]
                p['actors'][2][:3]=[2,2,0]
                if order==1:p['actors'][1][3],p['actors'][2][3]=0,1
                if order==2:p['actor']=1
                profiles.append(p)
    for kind in (2,3):
        for radius in (1,2):
            for tol in (0,2,255):p=base(kind);p.update(aoe=radius,vert=tol,f2=32 if tol==2 else 0);profiles.append(p)
    # A non-playable upper surface still wins vertical selection over a lower actor.
    p=base(0);p['terrain'][12][0]=6;p['terrain'][25+11]=[6,0,2];p.update(aoe=2,vert=255);profiles.append(p)
    # Equal layer heights: even a follow-upper centre selects the lower panel.
    p=base(1)
    for row in p['terrain']:row[0]=2
    p.update(follow=1,aoe=0,vert=0);profiles.append(p)
    p=base(1);p.update(follow=1);p['actors'][12][:3]=[3,2,1];p['actors'][2][2]=1;profiles.append(p)
    p=base(0);p['terrain'][12]=[255,255,0];p.update(aoe=1,vert=255);profiles.append(p)
    p=base(0);p['actors'][0][:3]=[2,2,0];p.update(f1=1);profiles.append(p)
    # Two existing pilot maps only, not a repeated disc/map census.
    disc,files=s.extract.disc_files(str(s.core_audit.DEFAULT_DISC))
    for number in (22,56):
        raw,meta,provenance=s.terrain(disc,files,number);w,h=meta['width'],meta['height'];area=w*h
        terrain=[[raw[8*(i+256*l)+2],raw[8*(i+256*l)+3],raw[8*(i+256*l)+6]&3]for l in range(2)for i in range(area)]
        cells=[(i%w,i//w,l)for l in range(2)for i in range(area)if not terrain[l*area+i][2]]
        centre=cells[len(cells)//2];ordered=sorted(cells,key=lambda q:(abs(q[0]-centre[0])+abs(q[1]-centre[1]),q))
        p=base();p.update(w=w,h=h,cx=centre[0],cz=centre[1],cl=centre[2],terrain=terrain)
        for i in range(13):p['actors'][i]=[*ordered[i],i%3,255,0]
        p['source_map']=provenance;profiles.append(p)
    for radius in (0,255):
        p=base(1);p.update(w=8,h=16,terrain=[[2+4*l,0,0]for l in range(2)for i in range(128)],follow=1,aoe=radius)
        for i in range(13):p['actors'][i]=[i%8,i//8,0,i%3,255,0]
        p['actors'][12][:3]=[7,15,1];profiles.append(p)
    records=[];rows=[]
    for index,p in enumerate(profiles):
        w,h=p['w'],p['h'];area=w*h
        o.write(0x800e4e9c,w);o.write(0x800e4ea0,h)
        tiles=bytearray(4096)
        for i in range(512):tiles[8*i+6]=1
        for l in range(2):
            for i in range(area):
                height,dh,flags=p['terrain'][l*area+i];at=8*(l*256+i)
                tiles[at+2]=height;tiles[at+3]=dh;tiles[at+6]=flags
        o.uc.mem_write(s.TILES&0x1fffff,bytes(tiles))
        for i in range(21):
            at=(0x801908cc+448*i)&0x1fffff;o.uc.mem_write(at,bytes(448));o.uc.mem_write(at+1,b'\xff')
        for i,row in enumerate(p['actors']):
            x,z,l,team,sid,absent=row;at=(0x801908cc+448*i)&0x1fffff
            o.uc.mem_write(at+1,bytes((255 if absent else i,)))
            o.uc.mem_write(at+5,bytes((16 if team==1 else 0,)));o.uc.mem_write(at+0x1ba,bytes((16 if team==1 else 0,)))
            o.uc.mem_write(at+0x47,bytes((x,z,128 if l else 0)))
            if sid!=255:o.uc.mem_write(at+0x58,mask(sid))
        ability=bytearray(d.at(s.battle.ABILITY_RANGE+14*0x10,14))
        ability[1:3]=bytes((p['aoe'],p['vert']));ability[3]=p['f1'];ability[4]=p['f2'];ability[6]=p['f4']
        o.uc.mem_write((s.battle.ABILITY_RANGE+14*0x10)&0x1fffff,bytes(ability))
        action=struct.pack('<BBHHHBBBBHHHBB',p['actor'],11,0x10,0,0,0,0,6 if p['follow']else 5,p['ref'],p['cx'],p['cl'],p['cz'],0,0)
        assert len(action)==20;o.uc.mem_write(0x1d6000,action)
        o.write(0x8018f4f8,0);count=o.call('green',0x801d6000)
        if count&0x80000000:expected=-1;listed=[]
        else:
            n=o.call('listing',0x801d6100,0x801908cc+448*p['actor'])
            listed=list(o.uc.mem_read(0x1d6100,n));assert listed==sorted(listed)and all(i<13 for i in listed)
            expected=sum(1<<i for i in listed)
        header=bytes(p[k]for k in ('w','h','actor','cx','cz','cl','follow','ref','aoe','vert','f1','f2','f4'))
        record=header+b'\0'+b''.join(bytes(r)for r in p['terrain'])+b''.join(bytes(r)for r in p['actors'])+struct.pack('>h',expected)
        records.append(struct.pack('>H',len(record))+record)
        rows.append(dict(**p,expected_mask=expected,source_order=listed,marked_source_tiles=count))
    (out/'fat00.bin').write_bytes(b'FAT1'+struct.pack('>H',len(records))+b''.join(records))
    # Tiny independent column fixtures cover ties, water/half-height, blocked sentinels and u8 reference semantics.
    primitive=[]
    for lo,ld,lf,hi,hd,hf in ((2,0,0,6,0,0),(2,0,0,2,0,0),(2,65,0,8,1,0),
        (2,0,0,6,0,1),(2,0,1,6,0,0),(2,0,1,6,0,1),(255,255,0,0,0,1)):
        for ref,tol,single in ((0,0,0),(4,0,0),(8,2,0),(8,2,1),(100,255,0),(255,255,1)):
            o.uc.mem_write(s.TILES&0x1fffff,bytes((0,0,lo,ld,0,0,lf,0)))
            o.uc.mem_write((s.TILES&0x1fffff)+2048,bytes((0,0,hi,hd,0,0,hf,0)))
            o.uc.mem_write(0x192dd8,bytes((1,1,0,0,0)));o.uc.mem_write(0x192dd8+256*5,bytes((1,1,0,0,0)))
            o.call('vertical',ref,tol,single)
            expected=(o.uc.mem_read(0x192dd8,1)[0]!=0)+2*(o.uc.mem_read(0x192dd8+256*5,1)[0]!=0)
            primitive.append([lo,ld,lf,hi,hd,hf,ref,tol,single,expected])
    (out/'fal00.bin').write_bytes(b'FAL1'+struct.pack('>H',len(primitive))+b''.join(bytes(r)for r in primitive))
    report=dict(cases=rows,columns=primitive,functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original unmodified green-panel/vertical/team/presence/list instructions, supplied terrain and13 source slots. Ordinary radial/whole-map footprint, fixed/follow coordinates, uncharmed blue/enemy relations. Real Orbonne/Gariland terrain reused without a disc census. No damage/RNG/reaction/reward/Reflect/Math/directional/random-fire/presentation trace; native mask is unordered and source effect-slot order remains a batch-integration contract.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n');print('Original radial targets:',len(rows),'footprints,',len(primitive),'column boundaries')

if __name__=='__main__':main()
