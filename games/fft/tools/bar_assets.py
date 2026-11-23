#!/usr/bin/env python3
"""Approved shared Bar artwork (FBR1) and Chapter I pilot rumors (FRU1).

The pilot binds only source event00C, retaining its original three rumors.
Explicit chapter event/flag bindings compile the six Chapter I rumor lifetimes;
later scenes are not invented or marked complete by the Bar.
FBR1: magic, total u16, portrait offset u16, raw room offset u16, reserved u16;
one aligned348-byte portrait, then6000 raw plane bytes (same30-byte stride).
FRU1: magic, total u16, count u8, reserved;14-byte records: source id, start
native flag, stop flag(255:none), page count, source town mask u16, title/text
offsets u16, effect offset u16 (zero:none), page-index offset u16. Page-index
words are byte offsets within the prose, prewrapped with the shipping font.
Effect lists: count u8, then
(profile id,value) byte pairs. NUL text spans/effect lists are pooled.
All archive words are big-endian.
"""
from pathlib import Path
import hashlib
import json
import json
import struct
import sys

import extract
import item_help
import management_data as md
import management_research as mr
import shop_scene
import town_service_assets as assets
import rumor_rules
from review_files import write_changed


def prose(raw):
    end = raw.index(254); out = []; i = 0; variable=0; index=0; effects=[]
    punct = dict(md.PAGED); punct[2064] = '"'
    while i < end:
        c = raw[i]; i += 1
        if c < 10: out.append(chr(c+48))
        elif c < 36: out.append(chr(c+55))
        elif c < 62: out.append(chr(c-36+97))
        elif c in (0xfa,0xf8,0xfd): out.append(' ')
        elif c==0xf5:
            if ''.join(out).strip():raise ValueError('mid-prose effect requires a new consumer')
            variable=raw[i];i+=1
        elif c==0xf6:
            value=raw[i];i+=1
            if variable==0x66:index=value
            else:
                target=variable+index*10;index=0
                if not 0x3c0<=target<0x400 or value>15:
                    raise ValueError('unsupported rumor effect %x' % target)
                effects.append((target-0x3c0,value))
        elif 0xd0 <= c <= 0xdf:
            g = (c&15)*208+raw[i]; i += 1
            out.append(punct[g])
        elif c in md.SIGNS: out.append(md.SIGNS[c])
        else: raise ValueError('unsupported rumor control %02x' % c)
    return ' '.join(''.join(out).split()),effects


def validate(raw, art=False):
    if len(raw) < 8 or struct.unpack_from('>H',raw,4)[0] != len(raw):
        raise ValueError('Bar bank length')
    if art:
        if len(raw)!=6360 or raw[:4]!=b'FBR1' or struct.unpack_from('>3H',raw,6)!=(12,360,0):
            raise ValueError('Bar artwork layout')
        return
    n=raw[6]; start=8+14*n
    if raw[:4]!=b'FRU1' or not 1<=n<=64 or raw[7] or start>=len(raw):
        raise ValueError('Rumor directory')
    ids=set()
    for at in range(8,start,14):
        identity,first,last,pages,mask,title,text,effect,index=struct.unpack_from('>4B5H',raw,at)
        if identity>=64 or identity in ids or not pages or pages>37 or not mask or mask&0x8000:
            raise ValueError('Rumor identity/mask')
        ids.add(identity)
        for off in (title,text):
            if not start<=off<len(raw): raise ValueError('Rumor text offset')
            end=raw.find(b'\0',off)
            if end<off or any(c<32 or c>126 for c in raw[off:end]):
                raise ValueError('Rumor text encoding/terminator')
            if end-off>(40 if off==title else 4096):raise ValueError('Rumor text length')
        if effect:
            if not start<=effect<len(raw) or effect+1+2*raw[effect]>len(raw):raise ValueError('Rumor effects span')
            for e in range(effect+1,effect+1+2*raw[effect],2):
                if raw[e]>=64 or raw[e+1]>15:raise ValueError('Rumor profile effect')
        if not start<=index<len(raw) or index+2*pages>len(raw):raise ValueError('Rumor page-index span')
        positions=struct.unpack_from('>'+str(pages)+'H',raw,index)
        length=raw.index(0,text)-text
        if positions[0] or list(positions)!=sorted(set(positions)) or positions[-1]>=length:
            raise ValueError('Rumor page offsets')


def page_offsets(text,advance):
    """Offline index for item_detail_line's exact142px/63-character word wrap."""
    at=0;lines=[]
    while at<len(text):
        lines.append(at);n=0;last=0;width=0
        while at+n<len(text) and n<63:
            adv=advance[ord(text[at+n])-32]
            if width+adv>142:break
            if text[at+n]==' ':last=n
            width+=adv;n+=1
        if at+n<len(text) and last:n=last
        if not n:raise ValueError('unwrappable rumor')
        at+=n
        while at<len(text) and text[at]==' ':at+=1
    if not lines or len(lines)>255:raise ValueError('rumor line capacity')
    return lines[::7]


def build(cue,event_bindings=None):
    disc,files=extract.disc_files(cue)
    names=('WORLD/WLDCORE.BIN','EVENT/WORLD.LZW','WORLD/WLDMES.BIN',
           'EVENT/WLDFACE.BIN','WORLD/WLDBK.BIN','EVENT/TEST.EVT','BATTLE.BIN')
    source={n:disc.read(*files[n]) for n in names}
    core,world,messages,faces,back=(source[n] for n in names[:5])
    sizes_at=0x8014d170-0x80067000
    transitions=rumor_rules.read_source(source['EVENT/TEST.EVT'],source['BATTLE.BIN'][sizes_at:sizes_at+256])
    if event_bindings is None:
        spec=Path(__file__).resolve().parents[1]/'campaign/packages/chapter-01.json'
        event_bindings=json.loads(spec.read_text())['rumor_events']
    selected=rumor_rules.gates(event_bindings)
    if not selected:raise ValueError('Chapter has no bound rumor-opening event')
    masks_at=struct.unpack_from('<I',core,0x80096a4c-0x80067000)[0]-0x80067000
    masks=struct.unpack_from('<64I',core,masks_at)
    offsets=struct.unpack_from('<32I',world)
    starts=item_help.packed_entries(world,128+offsets[25],128+offsets[26])
    count=max(i for i,_,_ in selected)+1
    titles=[md.decode(bytes(mr.expanded(world,a))[:-1]) for a in starts[:count]]
    texts=[];at=0
    for _ in range(count):
        texts.append(prose(messages[at:]));at=messages.index(254,at)+1
    face=assets.portrait(faces,117,assets.SELECTED['bar'])
    room=shop_scene.planes(assets.bar_greys(assets.bar_source(back)))
    art=b'FBR1'+struct.pack('>4H',6360,12,360,0)+face+room
    font_bank=(Path(__file__).resolve().parents[1]/'campaign/data/ffmg.bin').read_bytes()
    font=struct.unpack_from('>H',font_bank,18)[0];advance=font_bank[font+103:font+198]
    table=bytearray();pool=bytearray();intern={};base=8+14*len(selected);page_counts=[]
    def put(s):
        if s not in intern:
            intern[s]=base+len(pool);pool.extend(s.encode('ascii')+b'\0')
        return intern[s]
    for i,first,last in selected:
        title=titles[i];text,effects=texts[i]
        if sum(advance[ord(c)-32] for c in title)>124:
            raise ValueError('rumor title needs a fitted shared layout')
        title_at,text_at=put(title),put(text);effect_at=0
        if effects:
            effect_at=base+len(pool);pool.extend(bytes((len(effects),))+bytes(v for pair in effects for v in pair))
        pages=page_offsets(text,advance);page_counts.append(len(pages));index=base+len(pool)
        pool.extend(struct.pack('>'+str(len(pages))+'H',*pages))
        table+=struct.pack('>4B5H',i,first,last,len(pages),masks[i],title_at,text_at,effect_at,index)
    rumors=b'FRU1'+struct.pack('>HBB',base+len(pool),len(selected),0)+table+pool
    validate(art,True);validate(rumors)
    rumor_rules.validate_bank(rumors,event_bindings)
    return art,rumors,dict(source_sha256={n:hashlib.sha256(b).hexdigest() for n,b in source.items()},
        ids=[i for i,_,_ in selected],titles=[titles[i] for i,_,_ in selected],
        masks=[masks[i] for i,_,_ in selected],art_bytes=len(art),rumor_bytes=len(rumors),
        profile_effects=[texts[i][1] for i,_,_ in selected],
        page_counts=page_counts,font_sha256=hashlib.sha256(font_bank).hexdigest(),
        event_bindings=event_bindings,transitions=transitions,
        scope='Only declared native event flags unlock/retire source rumors; no progression on reading.')


if __name__=='__main__':
    import argparse
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('cue');ap.add_argument('out',type=Path)
    ap.add_argument('--chapter-spec',type=Path,help='explicit authored source-event/native-flag bindings')
    args=ap.parse_args()
    rows=json.loads(args.chapter_spec.read_text())['rumor_events'] if args.chapter_spec else None
    art,rumors,info=build(args.cue,rows);out=args.out;out.mkdir(parents=True,exist_ok=True)
    write_changed(out/'ffbr.bin',art);write_changed(out/'ffru.bin',rumors)
    write_changed(out/'bar-source.json',(json.dumps(info,indent=2)+'\n').encode())
    print('Bar:ffbr %d bytes,ffru %d bytes,%d source rumors;full provenance in bar-source.json.'%
          (len(art),len(rumors),len(info['ids'])))
