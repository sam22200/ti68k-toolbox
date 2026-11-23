#!/usr/bin/env python3
"""Shared Propositions rules/prose and explicit chapter service-event bindings."""
import argparse
import json
from pathlib import Path
import struct

import bar_assets
import extract
import proposition_rules
import item_help
import management_research as mr
from review_files import write_changed


def validate(rules, rewards, prose):
    """Mirror cold native bounds before admitting a matching shipping group."""
    proposition_rules.validate(rules)
    word=lambda p,at:struct.unpack_from('>H',p,at)[0]
    if len(rewards)<28 or rewards[:4]!=b'FPE1' or word(rewards,4)!=len(rewards) or rewards[6:8]!=b'\x0a\x01':
        raise ValueError('Proposition rewards header/length')
    tables=[]
    for k,size in enumerate(proposition_rules.REWARD_SIZES):
        at=word(rewards,8+2*k)
        if at<28 or at+size>len(rewards):raise ValueError('Proposition rewards offset')
        table=rewards[at:at+size];tables.append(table)
        if k<4:
            if any(n>100 for n in table):raise ValueError('Proposition scoring value')
        elif k<6:
            if any(not 1<=word(table,i)<=(200 if k==4 else 10000) for i in range(0,size,2)):
                raise ValueError('Proposition reward base')
        elif k<8:
            if any(a>8 or b>8 or bool(a)!=bool(b) or (k==6 and not a) for a,b in zip(table[::2],table[1::2])):
                raise ValueError('Proposition reward pair')
        elif any(not 100<=word(table,i)<=150 for i in range(0,size,2)):
            raise ValueError('Proposition duration scale')
    for i in range(96):
        r=rules[28+25*i:28+25*i+23]
        if not 1<=r[0]<=8 or not 1<=r[1]<=3 or not 1<=r[9]<=8 or r[10]>2 or r[11]>1 or not 1<=r[12]<=8 or not 1<=r[19]<=4 or r[20]>20:
            raise ValueError('Proposition reward rule')
        if not r[10] and r[11] and not all(tables[7][6*(r[0]-1):6*r[0]]):
            raise ValueError('Proposition random reward pairs')
    if len(prose)<738 or prose[:4]!=b'FPT1' or word(prose,4)!=len(prose) or prose[6:8]!=b'\x60\x4d':
        raise ValueError('Proposition prose header/length')
    for i in range(173):
        at=word(prose,8+6*i if i<96 else 584+2*(i-96))
        if not 738<=at<len(prose):raise ValueError('Proposition text offset')
        end=prose.find(b'\0',at)
        if end<0 or end-at>4096 or any(c<32 or c>126 for c in prose[at:end]):
            raise ValueError('Proposition text bounds/encoding')
        if i<96:
            index=word(prose,10+6*i);pages=prose[12+6*i]
            if not 1<=pages<=37 or prose[13+6*i] or index<738 or index+2*pages>len(prose):
                raise ValueError('Proposition page index')
            offsets=[word(prose,index+2*j) for j in range(pages)]
            if offsets[0] or offsets[-1]>=end-at or any(a>=b for a,b in zip(offsets,offsets[1:])):
                raise ValueError('Proposition page boundaries')


def bindings(spec):
    rows=spec.get('service_events',[]);flags=[255,255];used=set()
    for row in rows:
        event,flag=row.get('event'),row.get('flag')
        if event not in (0x75,0xcb) or type(flag) is not int or not 1<=flag<=255 or flag in used:
            raise ValueError('Service event binding: source075/0CB, distinct one-based flags1..255')
        k=0 if event==0x75 else 1
        if flags[k]!=255:raise ValueError('Duplicate service event')
        used.add(flag);flags[k]=flag-1
    return b'FSV1'+bytes(flags)+b'\0\0'


def texts(cue,font):
    disc,files=extract.disc_files(cue)
    core=disc.read(*files['WORLD/WLDCORE.BIN']);messages=disc.read(*files['WORLD/WLDMES.BIN'])
    blocks=struct.unpack_from('<3I',core,0x8009e470-0x80067000)
    world=disc.read(*files['EVENT/WORLD.LZW']);sections=struct.unpack_from('<32I',world)
    names=item_help.packed_entries(world,128+sections[15],128+sections[16])
    if len(names)!=77:raise ValueError('Proposition reward names')
    at=2048*blocks[1];raw=bytearray(b'FPT1\0\0\x60\x4d'+bytes(96*6+77*2));records=[];pool={}
    for i in range(96):
        end=messages.index(254,at);encoded=messages[at:end+1];at=end+1
        circle=encoded.find(bytes((0xd9,185)))
        if circle>=0:
            footer=encoded.rfind(bytes((0xd9,190)),0,circle)
            if footer<0:raise ValueError('Proposition footer boundary')
            encoded=encoded[:footer]+b'\xfe'
        prose,effects=bar_assets.prose(encoded)
        if effects:raise ValueError('Unexpected proposition description effect')
        text=prose.encode()+b'\0';pages=bar_assets.page_offsets(prose,font)
        if text not in pool:pool[text]=len(raw);raw.extend(text)
        offset=pool[text];index=len(raw);raw.extend(struct.pack('>'+str(len(pages))+'H',*pages))
        struct.pack_into('>HHBB',raw,8+6*i,offset,index,len(pages),0)
        records.append(dict(id=i,text=prose,pages=pages))
    for i,start in enumerate(names):
        prose,effects=bar_assets.prose(bytes(mr.expanded(world,start)))
        if effects:raise ValueError('Reward name effect')
        text=prose.encode()+b'\0'
        if text not in pool:pool[text]=len(raw);raw.extend(text)
        struct.pack_into('>H',raw,8+96*6+2*i,pool[text])
    struct.pack_into('>H',raw,4,len(raw))
    return bytes(raw),records


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('cue');ap.add_argument('out',type=Path)
    ap.add_argument('--chapter-spec',type=Path,required=True);args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    base=args.out/'ffmg.bin'
    if not base.exists():base=Path(__file__).resolve().parents[1]/'campaign/data/ffmg.bin'
    mg=base.read_bytes();off=struct.unpack_from('>H',mg,4+14)[0];font=mg[off+103:off+198]
    rules,ri=proposition_rules.build(args.cue);reward,re=proposition_rules.build_rewards(args.cue);prose,records=texts(args.cue,font)
    validate(rules,reward,prose)
    service=bindings(json.loads(args.chapter_spec.read_text()))
    for name,data in [('ffpr',rules),('ffpe',reward),('ffpt',prose),('ffsv',service)]:write_changed(args.out/(name+'.bin'),data)
    write_changed(args.out/'service-source.json',(json.dumps(dict(rules=ri,rewards=re,descriptions=records),indent=2)+'\n').encode())
    print('Services:',len(rules),len(reward),len(prose),'bytes; source chapter gates',list(service[4:6]))


if __name__=='__main__':main()
