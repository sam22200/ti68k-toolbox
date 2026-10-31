#!/usr/bin/env python3
"""Compact source collision columns and bounded original-MIPS geometry oracles.
No PS1 code translation or modified disc; generated commercial bytes stay local.
Unicorn2.1.4 executes the original BATTLE functions with explicit fixture inputs.
"""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
import core_audit,extract,battle
HERE,ROOT=core_audit.HERE,core_audit.ROOT
FUNCTIONS={
    'tile':(0x80183fb4,104,'d20d4eca5e0c109c'),
    'point':(0x801af8b4,644,'fbd97b8e9d34ac60'),
    'ceiling':(0x80176164,200,'a741ccdfa1c4e7c8'),
    'candidate':(0x80177794,584,'700cb4151df1ebef')}
TILES=0x8018f8cc


def terrain(disc,files,number):
    g=disc.read(*files[f'MAP/MAP{number:03}.GNS']);choices=[]
    for off in range(0,len(g)-19,20):
        kind=struct.unpack_from('<H',g,off+4)[0]
        if kind==0x3101:break
        if kind==0x2e01:choices.append((off//20,*struct.unpack_from('<II',g,off+8)))
    if not choices:raise ValueError(f'MAP{number:03}: no primary terrain mesh')
    record,lba,size=choices[-1];mesh=disc.read(lba,size)
    meta=core_audit.terrain(mesh,f'MAP{number:03}');o=meta['terrain_offset']
    raw=mesh[o+2:o+4098]
    return raw,meta,dict(map=number,record=record,mesh_sha256=core_audit.sha(mesh),terrain_sha256=core_audit.sha(raw))


def column(raw,index):
    out=bytearray()
    for layer in range(2):
        b=raw[8*(index+256*layer):8*(index+256*layer+1)]
        out+=bytes((b[2],b[3],b[4],(b[5]&31)|((b[6]&1)<<7)))
    return bytes(out)


def encode(raw,w,h):
    return b'FGS1'+bytes((w,h))+struct.pack('>H',w*h)+b''.join(column(raw,i)for i in range(w*h))


class Original:
    def __init__(self,source):
        import unicorn
        from unicorn import mips_const as reg
        if unicorn.__version__!='2.1.4':raise ValueError('Use unicorn==2.1.4 in tools/pyenv')
        self.version=unicorn.__version__;self.reg=reg
        self.uc=unicorn.Uc(unicorn.UC_ARCH_MIPS,unicorn.UC_MODE_MIPS32|unicorn.UC_MODE_LITTLE_ENDIAN)
        self.uc.mem_map(0,0x200000);self.uc.mem_write(0x67000,source.bat)
        self.function_hashes={}
        for name,(addr,size,sha)in FUNCTIONS.items():
            actual=core_audit.sha(source.bat[addr-0x80067000:addr-0x80067000+size])
            if not actual.startswith(sha):raise ValueError(f'{name}: original function hash mismatch')
            self.function_hashes[name]=actual
        self.write(0x800e4e9c,1);self.write(0x800e4ea0,1)
        self.write(0x8018f4e0,0x801d0000);self.uc.mem_write(0x1d0018,b'\x01\x01')
        self.write(0x8018f4e4,0x801d3000);self.write(0x8018f4ec,0x801d5000)
        self.write(0x801d3000,TILES);self.write(0x801d3014,0x801d4000)
        self.write(0x801d1000,0) # No actor boxes in the solid-column routine fixture.
        if self.call('tile',0,0,0)!=TILES:raise ValueError('MIPS address/ABI sanity failed')
        if self.call('tile',1,0,0)!=0:raise ValueError('MIPS bounds sanity failed')

    def write(self,addr,value):
        self.uc.mem_write(addr&0x1fffff,struct.pack('<I',value))

    def call(self,name,*args):
        for register,value in zip((self.reg.UC_MIPS_REG_A0,self.reg.UC_MIPS_REG_A1,
                                   self.reg.UC_MIPS_REG_A2,self.reg.UC_MIPS_REG_A3),args):
            self.uc.reg_write(register,value&0xffffffff)
        self.uc.reg_write(self.reg.UC_MIPS_REG_RA,0x801ff000)
        self.uc.reg_write(self.reg.UC_MIPS_REG_SP,0x801fe000)
        self.uc.emu_start(FUNCTIONS[name][0],0x801ff000,count=getattr(self,'instruction_limit',10000))
        if self.uc.reg_read(self.reg.UC_MIPS_REG_PC)!=0x801ff000:raise ValueError('MIPS call did not return')
        return self.uc.reg_read(self.reg.UC_MIPS_REG_V0)

    def bind(self,col):
        for layer in range(2):
            h,dh,slope,flags=col[4*layer:4*layer+4]
            self.uc.mem_write((TILES&0x1fffff)+2048*layer,bytes((0,0,h,dh,slope,flags&31,flags>>7,0)))

    def query(self,col,kind,shift,arg):
        self.bind(col)
        if kind==0:
            self.uc.mem_write(0x1d1200,struct.pack('<4i',14,-arg,14,0))
            hit=self.call('point',0x801d1000,0x801d1200,0x801d1220,0x801d1240)
            return 1+struct.unpack('<h',self.uc.mem_read(0x1d1242,2))[0]if hit else 0
        return self.call('ceiling',0,0,shift,arg)

    def room(self,values):
        sc,dc,exit_height,entry,body=values
        self.uc.mem_write(0x1d001b,bytes((body,)));self.uc.mem_write(0x1d0002,b'\xfe')
        self.uc.mem_write(0x1d3046,bytes((sc,dc,exit_height,0,entry)))
        self.uc.mem_write(0x1d3074,bytes((exit_height,)))
        return int(self.call('candidate',0)==0)


def detach_descriptor(data):
    if data[-8:-4]!=b'SGE1':raise ValueError('Missing descriptor surface footer')
    offset,size=struct.unpack('>HH',data[-4:])
    if offset+size!=len(data)-8:raise ValueError('Invalid descriptor surface span')
    return data[:offset],data[offset:-8]


def attach_descriptor(data,surface):
    data=bytes(data)
    if len(data)&1:data+=bytes(1)
    return data+surface+b'SGE1'+struct.pack('>HH',len(data),len(surface))


def oracle(source,columns,out):
    original=Original(source);cases=[]
    for col in sorted(columns):
        heights={-1,0,1}
        exits={0,1,5,6,12,126}
        for layer in range(2):
            h,dh,slope,ceiling=col[4*layer:4*layer+4]
            thickness=(ceiling&31)or(1 if layer else h)
            for edge in (h*12,(h-thickness)*12):heights.update((edge-1,edge,edge+1))
            for shift in (0,2,4,6):
                edge=(2*h+(dh&31)*((slope>>shift)&3))&255
                exits.update((max(0,edge-1),edge,min(255,edge+1)))
        for height in sorted(heights):
            expected=original.query(col,0,0,height)
            cases.append(col+struct.pack('>BBhBB',0,0,height,expected,0))
        for shift in (0,2,4,6):
            for exit_height in sorted(exits):
                expected=original.query(col,1,shift,exit_height)
                cases.append(col+struct.pack('>BBhBB',1,shift,exit_height,expected,0))
    # Exact clearance boundary: equal headroom passes; one half-unit less fails.
    for body in (4,6):
        for exit_height in (0,1,2,5,8,20):
            for entry in (0,1,2,5,8,20):
                for sc in (max(exit_height,entry)+body-1,max(exit_height,entry)+body,127):
                    for dc in (exit_height+body-1,exit_height+body,127):
                        v=(sc,dc,exit_height,entry,body);expected=original.room(v)
                        cases.append(bytes(v)+bytes(3)+struct.pack('>BBhBB',2,0,0,expected,0))
    out.mkdir(parents=True,exist_ok=True)
    for i in range(0,len(cases),4000):
        part=cases[i:i+4000];name=f'fgq{i//4000:02}'
        (out/(name+'.bin')).write_bytes(b'FGO1'+struct.pack('>H',len(part))+b''.join(part))
    return dict(cases=len(cases),unique_columns=len(columns),unicorn=original.version,
                function_hashes=original.function_hashes,case_bytes=14,
                caveat='Original disc MIPS functions, fixture RAM and no actors; no full-frame PS1 movement/render observation.')


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--disc',type=Path,default=core_audit.DEFAULT_DISC)
    ap.add_argument('--oracle',type=Path);args=ap.parse_args()
    disc,files=extract.disc_files(str(args.disc));sources=[];columns=set()
    for number,name in ((22,'ffgt'),(56,'ffot'),(1,None),(29,None),(53,None)):
        raw,meta,provenance=terrain(disc,files,number);w,h=meta['width'],meta['height']
        data=encode(raw,w,h);provenance.update(width=w,height=h,bytes=len(data))
        if name:
            path=HERE/'campaign/data'/(name+'.bin')
            if not path.exists()or path.read_bytes()!=data:path.write_bytes(data)
        sources.append(provenance);columns.update(column(raw,i)for i in range(w*h))
    # Asymmetric bridge, thickness, blocked flags and slope boundary fixtures.
    for lower,upper in ((0,8),(2,8),(4,7),(8,2)):
        for depth in (0,1,2,5):
            for blocked in (0,128):columns.add(bytes((lower,0,0,0,upper,0,0,depth|blocked)))
    if args.oracle:
        report=oracle(battle.Disc(str(args.disc)),columns,args.oracle)
        report['sources']=sources;report['decomp_commit']=subprocess.check_output(['git','-C',ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip()
        (args.oracle/'source.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
        print(json.dumps({k:report[k]for k in ('cases','unique_columns','unicorn')}),flush=True)
    else:print('FGS1 banks: '+', '.join(str(x['bytes'])for x in sources[:2])+' bytes')


if __name__=='__main__':main()
