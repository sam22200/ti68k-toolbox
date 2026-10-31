#!/usr/bin/env python3
"""Bounded unchanged-MIPS Reflect endpoints and pre-formula suppression."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function


def main():
    out=s.HERE/'campaign/x/core-reflect/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for key,name in dict(reflect='battle_target_apply_reflect',
                         guard='battle_action_check_reflect_reaction').items():
        bind_function(key,name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:])
    o.write(0x800e4e9c,5);o.write(0x800e4ea0,5)
    unit=0x801908cc;rows=[]
    profiles=[(ox,oz,rx,rz,0,4,2,6,0,0)for ox,oz,rx,rz in
              ((0,2,1,2),(4,2,3,2),(2,0,2,1),(2,4,2,3),
               (0,0,1,1),(4,4,3,3),(0,1,1,2),(4,3,3,2),(2,2,2,2),(0,0,4,4))]
    profiles += [(0,2,1,2,layer,h,lo,hi,lb,hb)for layer,h,lo,hi,lb,hb in
        ((0,2,2,8,0,0),(1,8,2,8,0,0),(0,4,2,6,0,0),
         (0,4,6,2,0,0),(0,4,4,4,0,0),(0,4,2,6,1,0),
         (0,4,2,6,0,1),(0,4,2,6,1,1))]
    for p in profiles:
        ox,oz,rx,rz,layer,h,lo,hi,lb,hb=p
        tiles=bytearray(4096)
        for i in range(512):tiles[8*i+2]=2;tiles[8*i+6]=int(i>=256)
        here=rx+5*rz+256*layer;tiles[8*here+2]=h;tiles[8*here+6]=0
        x,z=2*rx-ox,2*rz-oz
        if 0<=x<5 and 0<=z<5:
            for lev,height,blocked in((0,lo,lb),(1,hi,hb)):
                at=8*(x+5*z+256*lev);tiles[at+2]=height;tiles[at+6]=blocked
        o.uc.mem_write(s.TILES&0x1fffff,bytes(tiles));o.uc.mem_write(unit&0x1fffff,bytes(448))
        o.uc.mem_write((unit&0x1fffff)+0x47,bytes((rx,rz,128 if layer else 0)))
        o.write(0x80192dcc,ox);o.write(0x80192dd0,oz)
        result=o.call('reflect',unit)
        dest=struct.unpack('<hhh',o.uc.mem_read((unit&0x1fffff)+0x17a,6))
        rows.append([*p,int(result==0),*dest])
    guards=[]
    for setid,flag in((11,0),(11,128),(21,128),(10,128)):
        o.uc.mem_write(0x1938c0,bytes(0x200));o.uc.mem_write(0x1d7000,bytes(64))
        o.write(0x80192d90,0x801d7000)
        o.uc.mem_write(0x1938c0+0x14,bytes((setid,)))
        o.uc.mem_write(0x1938c0+0x35,bytes((flag,)))
        o.uc.mem_write(0x1938c0+0x16,struct.pack('<H',0x10))
        o.call('guard');data=bytes(o.uc.mem_read(0x1d7000,64))
        guards.append([setid,flag,struct.unpack_from('<H',data,0x0e)[0],data[2],
                       struct.unpack_from('<H',data,0x26)[0]])
    lines=['/* Generated unchanged-MIPS source records, local only. */',
           'static const u16 source_reflect[][14]={']
    lines += [' {'+','.join(str(x&65535)for x in row)+'},'for row in rows]
    lines+=['};','static const u16 source_reflect_guard[][5]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in guards];lines+=['};']
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(endpoints=rows,guards=guards,functions=o.function_hashes,
        battle_sha256=s.core_audit.sha(d.bat),exe_sha256=s.core_audit.sha(d.exe),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Original target_apply_reflect and complete called endpoint instructions; direct reflect guard. No full PS1 rendered spell/reaction dispatcher trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original Reflect:',len(rows),'endpoints,',len(guards),'guard cases')


if __name__=='__main__':main()
