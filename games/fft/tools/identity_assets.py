#!/usr/bin/env python3
"""Compact chapter identity data; all pixels/text remain in shared ffcp FCP2."""
from pathlib import Path
import re,struct,sys
HERE=Path(__file__).resolve().parents[1]
MARK='// Portrait IDs and generic names live in shared ffcp FCP2.'

def write_changed(path,data):
    if not path.exists()or path.read_bytes()!=data:path.write_bytes(data)

def section(bank,k):
    assert bank[:4]==b'FCP2'
    start=struct.unpack_from('>H',bank,4)[0]-10
    return struct.unpack_from('>H',bank,start+2*k)[0]

def compact(chapter):
    folder=HERE/chapter/'data';header=folder/'interface.h';text=header.read_text()
    pool=(folder/'ffcp.bin').read_bytes()
    if MARK in text:
        rebound=re.sub(r'(#define UI_PORTRAITS )\d+UL',lambda m:m[1]+str(section(pool,chapter=='opening'))+'UL',text)
        write_changed(header,rebound.encode());return
    defines={n:int(v)for n,v in re.findall(r'#define UI_(\w+) (\d+)UL',text)}
    cuts=[(defines['PORTRAITS'],defines['MALE'])]
    if chapter=='campaign':cuts.append((defines['MALE'],defines['SLOPES']))
    def moved(at):return at-sum(b-a for a,b in cuts if b<=at)
    path=folder/('ffoi.bin'if chapter=='opening'else'ffgi.bin');bank=bytearray(path.read_bytes())
    for a,b in reversed(cuts):del bank[a:b]
    text=re.sub(r'(#define UI_(\w+) )(\d+)UL',lambda m:m[1]+str(section(pool,chapter=='opening')if m[2]=='PORTRAITS'else 0 if m[2]in('MALE','FEMALE')else moved(int(m[3])))+'UL',text)
    battle=folder/'battle.h';body=battle.read_text()
    names=re.search(r'#define OPENING_NAMES (\d+)UL',body)
    if names:
        old=int(names[1]);at=moved(old);count=(struct.unpack_from('>H',bank,at)[0]-old)//2
        for i in range(count):struct.pack_into('>H',bank,at+2*i,moved(struct.unpack_from('>H',bank,at+2*i)[0]))
    body=re.sub(r'(igfx \+ )(\d+)UL',lambda m:m[1]+str(moved(int(m[2])))+'UL',body)
    body=re.sub(r'(#define OPENING_\w+ )(\d+)UL',lambda m:m[1]+str(moved(int(m[2])))+'UL',body)
    write_changed(path,bank);write_changed(header,(text.rstrip()+'\n'+MARK+'\n').encode());write_changed(battle,body.encode())
    print(chapter,'identity records removed:',sum(b-a for a,b in cuts),'bytes')

if __name__=='__main__':
    for chapter in sys.argv[1:]:compact(chapter)
