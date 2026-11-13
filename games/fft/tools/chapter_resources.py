"""Lossless shipping normalization; extraction controls are never rewritten.

One core copy of engine artwork, lean encounter interfaces, chapter-wide scene
blob pooling, and single-hop FRF1 aliases for byte-identical encounter banks.
All resolution happens at load; existing direct archive pointers render frames.
"""
from dataclasses import replace
from pathlib import Path
import re
import struct

TRAILER = b'\0dat\0\xf8'


def word(data, at):
    return struct.unpack_from('>H', data, at)[0]


def changed(v, raw):
    if len(raw) > 65512:
        raise ValueError(f'{v.name}: resource bank exceeds one AMS variable')
    return replace(v, payload=bytes(raw)+TRAILER)


def reference(v, target, offset, size):
    if not re.fullmatch('[a-z][a-z0-9]{0,7}', target) or offset & 1 or not size:
        raise ValueError('Invalid resource reference')
    return changed(v, b'FRF1'+target.encode().ljust(8,b'\0')+struct.pack('>HH',offset,size))


def resolve(source, name, allowed=None):
    raw=source[name].raw
    if raw[:4]!=b'FRF1':return raw
    if len(raw)!=16:raise ValueError(f'{name}: invalid resource reference size')
    target=raw[4:12].rstrip(b'\0').decode('ascii')
    if not re.fullmatch('[a-z][a-z0-9]{0,7}',target) or raw[4:12]!=target.encode().ljust(8,b'\0'):
        raise ValueError(f'{name}: invalid resource reference name')
    if allowed is not None and target not in allowed:
        raise ValueError(f'{name}: reference outside the installed package: {target}')
    data=source[target].raw;off,size=struct.unpack_from('>HH',raw,12)
    if off&1 or not size or off+size>len(data) or data[:4]==b'FRF1':
        raise ValueError(f'{name}: invalid/nested resource reference')
    return data[off:off+size]


def contour_tail(raw):
    if raw[-10:-6]!=b'FC02':return raw,None
    original,offset,size=struct.unpack_from('>3H',raw,len(raw)-6)
    if offset&1 or original>offset or offset+size!=len(raw)-10:
        raise ValueError('Malformed contour tail')
    return raw[:original],raw[offset:offset+size]


def shared_contours(source):
    """Hoist the exact common tail, even when building a continuation-only pack."""
    source=dict(source);base,shared=contour_tail(source['ffcu'].raw)
    views=[n for n in source if re.fullmatch('ff[a-z0-9]v0',n)]
    for name in sorted(views):
        raw=source[name].raw
        if raw[:4]==b'FRF1':continue
        original,tail=contour_tail(raw)
        if tail is None:continue
        if shared is not None and shared!=tail:
            raise ValueError(f'{name}: engine artwork differs from shared ffcu')
        shared=tail;source[name]=changed(source[name],original)
    if shared is not None:
        expected=int(re.search(r'#define CONTOUR_BYTES (\d+)',
                     (Path(__file__).resolve().parents[1]/'contour.h').read_text())[1])
        if len(shared)!=expected:raise ValueError('Shared engine artwork differs from the compiled layout')
        module=source.get('ffbm')
        if module and module.raw[:4]==b'FMB3' and word(module.raw,8)<27:
            raise ValueError('Shared resources require the ABI80 core (27 executable images)')
        offset=(len(base)+1)&~1
        source['ffcu']=changed(source['ffcu'],base+bytes(offset-len(base))+shared+
                                 b'FC02'+struct.pack('>3H',len(base),offset,len(shared)))
    return source


def audit(variables,source):
    """Machine-readable ownership/copy inventory; aliases are not duplicate bytes."""
    rows=[];canonical={};duplicates=[]
    for v in variables:
        if v.kind!=0x1c or not v.payload.endswith(TRAILER):continue
        raw=v.raw;before=source.get(v.name)
        row=dict(name=v.name,bytes=len(v.payload),source_bytes=len(before.payload) if before else 0)
        if raw[:4]==b'FRF1':
            row['shared_with']=raw[4:12].rstrip(b'\0').decode('ascii')
        elif len(raw)>=32:
            if raw in canonical:duplicates.append([canonical[raw],v.name,len(raw)])
            else:canonical[raw]=v.name
        if raw[:4]==b'FSC2':row['scene_pool']=raw[12:20].rstrip(b'\0').decode('ascii')
        rows.append(row)
    if duplicates:
        raise ValueError('Duplicate shipping banks: '+repr(duplicates)+'; move them to one shared owner')
    return dict(banks=rows,duplicate_banks=duplicates)


def lean_encounter(source, prefix):
    """Only fields actually consumed from igfx survive in the shipping bank.

    The FCD5 descriptor already owns converted UnitDefs and water positions.
    Legacy abilities/skill sets are consumed offline by chapter_desc.py only.
    """
    dn,inn='ff'+prefix+'d','ff'+prefix+'i'
    desc=bytearray(resolve(source,dn));old=resolve(source,inn)
    # Small metadata-only packaging fixtures have no surface/terrain payload.
    if len(desc)<114 or desc[:4]!=b'FCD5' or desc[-8:-4]!=b'SGE1':return
    import chapter_desc
    fields={f:4+2*i for i,f in enumerate(chapter_desc.FIELDS)}
    def get(f):return word(desc,fields[f])
    def put(f,n):struct.pack_into('>H',desc,fields[f],n)
    count=get('UNIT_COUNT');tiles=get('MAP_W')*get('MAP_H')*12
    at=get('TILES')
    if at+tiles>len(old):raise ValueError(f'{inn}: truncated terrain')
    raw=bytearray(old[at:at+tiles]);put('TILES',0)
    at=get('UNIT_RAW')
    if at:
        if at+20*count>len(old):raise ValueError(f'{inn}: truncated fixed statistics')
        put('UNIT_RAW',len(raw));raw+=old[at:at+20*count]
    at=get('UNIT_NAMES')
    if at:
        if at+2*count>len(old):raise ValueError(f'{inn}: truncated names')
        start=len(raw);raw+=bytes(2*count);strings={}
        for i in range(count):
            p=word(old,at+2*i)
            if p<at+2*count:raise ValueError(f'{inn}: invalid name offset')
            end=old.index(0,p)+1;text=old[p:end]
            if text not in strings:strings[text]=len(raw);raw+=text
            struct.pack_into('>H',raw,start+2*i,strings[text])
        put('UNIT_NAMES',start)
    put('ABILITIES',0);put('SKILL_SETS',0)
    source[inn]=changed(source[inn],raw);source[dn]=changed(source[dn],desc)


class BlobPool:
    """Exact byte interning shared by scene/intro generation and packaging."""
    def __init__(self, start=b''):
        self.data=bytearray(start);self.offsets={}

    def add(self, raw):
        raw=bytes(raw)
        if raw not in self.offsets:
            if len(self.data)&1:self.data.append(0)
            self.offsets[raw]=len(self.data);self.data+=raw
        return self.offsets[raw]


def beat_end(raw,at):
    count=word(raw,at);end=at+2+2*count
    for i in range(count):
        p=at+word(raw,at+2+2*i)
        q=p+4+8*raw[p+1]
        end=max(end,raw.index(0,raw.index(0,q)+1)+1)
    return end


def scene_parts(source, name, shared_serif):
    raw=resolve(source,name)
    if raw[:4] not in (b'FSC1',b'FSC2'):return None
    count=word(raw,10);head=12 if raw[:4]==b'FSC1' else 20
    if count>255 or head+8*count>len(raw):raise ValueError(f'{name}: invalid scene steps')
    assets=raw
    if head==20:
        pool=raw[12:20].rstrip(b'\0').decode('ascii');assets=resolve(source,pool)
    rows=[struct.unpack_from('>BB3H',raw,head+8*i) for i in range(count)]
    # FSP1 records carry sizes so a chapter can be rebuilt from already pooled
    # banks without guessing where a ZX0 stream ends or including another blob.
    sizes={}
    if assets[:4]==b'FSP1':
        for i in range(word(assets,4)):
            at,size=struct.unpack_from('>HH',assets,6+4*i);sizes[at]=size
    starts=set()
    for kind,arg,a,b,c in rows:
        if kind==1 and not shared_serif:starts.add(a)
        elif kind in (2,4,5):starts.update((a,c))
        elif kind==3:starts.add(a)
        elif kind!=1:raise ValueError(f'{name}: unsupported scene operation')
    def picture(at):
        end=at+sizes[at] if at in sizes else min((x for x in starts if x>at),default=len(assets))
        if at>=end or end>len(assets):raise ValueError(f'{name}: invalid scene picture')
        return assets[at:end]
    steps=[]
    for kind,arg,a,b,c in rows:
        if kind==1:values=[0 if shared_serif else assets[a:a+756],b,c]
        elif kind in (2,4):values=[picture(a),b,assets[c:beat_end(assets,c)]]
        elif kind==5:values=[picture(a),b,assets[c:c+1+2*assets[c]]]
        else:
            end=a+1
            for _ in range(assets[a]):end=assets.index(0,end+3)+1
            values=[assets[a:end],b,c]
        steps.append((kind,arg,values))
    return raw[4:10],steps


def scene_bytes(mapping,steps,pool=None,pool_name='ffsp'):
    shared=pool is not None
    header=(b'FSC2' if shared else b'FSC1')+mapping+struct.pack('>H',len(steps))
    if shared:header+=pool_name.encode().ljust(8,b'\0')
    table=len(header)
    if not shared:pool=BlobPool(header+bytes(8*len(steps)))
    rows=[]
    for kind,arg,values in steps:
        rows.append(struct.pack('>BB3H',kind,arg,*(v if isinstance(v,int) else pool.add(v) for v in values)))
    if shared:return header+b''.join(rows)
    pool.data[table:table+8*len(steps)]=b''.join(rows)
    return bytes(pool.data)


def pool_scenes(source,names):
    serif='ffflow' in source and source['ffflow'].raw[:4]==b'FQF2'
    scenes={n:scene_parts(source,n,serif) for n in names if re.fullmatch('ffs[0-9]',n)}
    scenes={n:s for n,s in scenes.items() if s is not None}
    independent={n:scene_bytes(*s) for n,s in scenes.items()}
    for n,raw in independent.items():source[n]=changed(source[n],raw)
    if not scenes:return set()
    sets={n:{v for _,_,vs in steps for v in vs if not isinstance(v,int)} for n,(_,steps) in scenes.items()}
    def bound(blobs):return 6+4*len(blobs)+sum(len(b)+(len(b)&1) for b in blobs)
    # Best existing overlap first, bounded by one AMS variable. Independent
    # scenes remain inline when a pool would add bytes. No expanded RAM bank.
    groups=[]
    for n in scenes:
        choices=[(sum(map(len,sets[n]&blobs)),i) for i,(_,blobs) in enumerate(groups)
                 if bound(blobs|sets[n])<=65512]
        if choices:
            _,i=max(choices);groups[i][0].append(n);groups[i][1].update(sets[n])
        elif bound(sets[n])<=65512:groups.append(([n],set(sets[n])))
    created=set()
    for k,(members,blobs) in enumerate(groups):
        name='ffsp' if len(groups)==1 else 'ffsp'+str(k)
        pool=BlobPool(b'FSP1'+struct.pack('>H',len(blobs))+bytes(4*len(blobs)))
        combined={n:scene_bytes(*scenes[n],pool,pool_name=name) for n in members}
        for i,(blob,at) in enumerate(pool.offsets.items()):struct.pack_into('>HH',pool.data,6+4*i,at,len(blob))
        # Include the extra variable's transfer envelope and data trailer.
        if sum(map(len,combined.values()))+len(pool.data)+32>=sum(len(independent[n]) for n in members):continue
        template=source[members[0]]
        if name in source and source[name].raw[:4]!=b'FSP1':
            raise ValueError(f'{name}: name reserved for generated scene resources')
        source[name]=changed(replace(template,name=name),pool.data);created.add(name)
        for n,raw in combined.items():source[n]=changed(source[n],raw)
    return created


def encounter_names(spec):
    return {'ff'+r['prefix']+s for r in spec['routes'] for s in ('v0','v1','v2','v3','a','i','d','st')}


def prepare_chapter(source,spec,core_names):
    """Never pool across separately installed chapters or create alias chains."""
    source=dict(source);names=set(spec['variables']);allowed=names|set(core_names)
    # Recover semantic payloads before repacking an already optimized chapter.
    for n in sorted(names):
        if n in source and source[n].raw[:4]==b'FRF1':source[n]=changed(source[n],resolve(source,n,allowed))
    for route in spec['routes']:
        if 'ff'+route['prefix']+'d' in source:lean_encounter(source,route['prefix'])
    pooled=pool_scenes(source,sorted(names))
    names={n for n in names if not re.fullmatch('ffsp[0-9]?',n)}|pooled
    canonical={source[n].raw:n for n in sorted(core_names) if n in source and
               source[n].kind==0x1c and source[n].payload.endswith(TRAILER)}
    for n in sorted(names & (encounter_names(spec)|{n for n in names if re.fullmatch('ffs[0-9]',n)})):
        if n not in source:continue
        raw=source[n].raw
        if len(raw)<=16:continue
        if raw in canonical:source[n]=reference(source[n],canonical[raw],0,len(raw))
        else:canonical[raw]=n
    return source,names
