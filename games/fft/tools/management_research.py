#!/usr/bin/env python3
"""Audit referenced US FFT event dialogue pages and original location artwork.

The archive contains unused strings and non-text padding. Only IDs reached by a
linear instruction decode (retail operand-size table) are candidates, not arbitrary
byte patterns resembling DisplayMessage. Generated text/art remain local.
"""
from pathlib import Path
import argparse, collections, hashlib, json, struct
import extract

OUT=Path(__file__).resolve().parents[1]/'x/management'

def expanded(block,at):
    # Mirrors the retail single return cursor and byte-counted dictionary copy.
    out=[];ret=None;remaining=0
    for _ in range(16384):
        if not 0<=at<len(block):raise ValueError('text cursor outside event block')
        c=block[at]
        if 0xf0<=c<=0xf3:
            ret=at+3;remaining=(c&3)*8+(block[at+1]>>5)+4
            at-=(block[at+1]&15)*254+block[at+2];continue
        out.append(c);at+=1
        if remaining:
            remaining-=1
            if not remaining:at=ret;ret=None
        if c>=254 and ret is None:return out
    raise ValueError('unterminated entry')

PUNCT={237:'-',2054:'.',2196:',',2061:':',2197:';',2062:'(',2063:')',2192:'=',2065:"'",2199:'"',2198:'?',2195:' '}
def decode(tokens):
    lines=[''];i=0;unknown=[];dynamic=False
    while i<len(tokens):
        c=tokens[i];i+=1
        if c>=254:break
        if c==248:lines.append('');continue
        if c==244:lines.append('|');continue
        if c in (226,227,230,231,232,236,245,246):i+=1;continue
        if c in (249,251,252,253):continue
        if c==224:lines[-1]+='Ramza';dynamic=True;continue
        if c in (225,228,229,233,234,235):lines[-1]+='{value}';dynamic=True;continue
        if 208<=c<=223:
            g=(c&15)*208+tokens[i];i+=1;ch=PUNCT.get(g,'?')
            if g not in PUNCT:unknown.append(g)
        elif c<10:ch=chr(c+48)
        elif c<36:ch=chr(c-10+65)
        elif c<62:ch=chr(c-36+97)
        elif c==62:ch='!'
        elif c==63:ch='?'
        elif c==250:ch=' '
        elif c in (176,237):ch='-'
        elif c==147:ch=' '
        else:ch='?';unknown.append(c)
        lines[-1]+=ch
    return lines,unknown,dynamic

def audit(cue):
    disc,files=extract.disc_files(cue)
    raw=disc.read(*files['EVENT/TEST.EVT'])
    battle=disc.read(*files['BATTLE.BIN'])
    sizes=battle[0x8014d170-0x80067000:][:256]
    records=[];errors=[];modes=collections.Counter();referenced=0
    for event in range(len(raw)//8192):
        block=raw[event*8192:(event+1)*8192];off=struct.unpack_from('<I',block)[0]
        if not 4<=off<8192:continue
        refs={};at=4;mode=0x10
        while at<off:
            op=block[at]
            if op==0x10:
                mode=block[at+2]&0x70
                tid=struct.unpack_from('<H',block,at+3)[0];refs[tid]=mode
            elif op==0x51:
                tid=struct.unpack_from('<H',block,at+2)[0]
                if tid!=65535:refs.setdefault(tid,mode)
            at+=1+sizes[op]
        # Retail skip_to_entry counts literal FE/FF, including compressed entries.
        starts=[off]
        for pos in range(off,8192):
            if block[pos]>=254:starts.append(pos+1)
        for tid,mode in refs.items():
            referenced+=1;modes[mode]+=1
            try:
                assert 1<=tid<=len(starts)
                tokens=expanded(block,starts[tid-1]);lines,unknown,dynamic=decode(tokens)
            except (AssertionError,IndexError,ValueError) as e:
                errors.append(dict(event=event,text_id=tid,error=str(e)));continue
            # Portrait dialogue displays three lines, including any printed name.
            # Automatic mode 70 is remapped to portrait mode by the original.
            rows=3 if mode in (16,112) else 6
            chunk=[];pages=[]
            for line in lines:
                if line=='|':
                    if chunk:pages.append(chunk);chunk=[]
                else:
                    chunk.append(line)
                    if len(chunk)==rows:pages.append(chunk);chunk=[]
            if chunk:pages.append(chunk)
            for index,page in enumerate(pages):
                records.append(dict(event=event,text_id=tid,page=index,mode=mode,
                                    lines=page,characters=sum(map(len,page)),
                                    unknown=unknown,dynamic=dynamic))
    portrait=[r for r in records if r['mode'] in (16,112)]
    static=[r for r in portrait if not r['unknown'] and not r['dynamic']]
    result=dict(source='EVENT/TEST.EVT',sha256=hashlib.sha256(raw).hexdigest(),
                event_blocks=len(raw)//8192,referenced_entries=referenced,
                parsed_pages=len(records),portrait_pages=len(portrait),modes=dict(modes),
                errors=errors,longest_portrait=max(portrait,key=lambda r:r['characters']),
                longest_static_portrait=max(static,key=lambda r:r['characters']),
                substitutions='Ramza fixed to five letters; other values placeholder, not an absolute runtime maximum',
                scope='Referenced event-script dialogue only; world tutorials, help and player-chosen name lengths are separate',
                top_pages=sorted(portrait,key=lambda r:r['characters'],reverse=True)[:12])
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'dialogue-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('referenced_entries','parsed_pages','errors','longest_portrait','longest_static_portrait')},indent=2))
    return result

if __name__=='__main__':
    ap=argparse.ArgumentParser(__doc__);ap.add_argument('disc');args=ap.parse_args();audit(args.disc)
